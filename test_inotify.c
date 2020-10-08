/*
 * A simple inotify test.
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <inttypes.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include <linux/limits.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/mman.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

enum inotify_flags {
	F_VERBOSE = 1 << 0,
};

struct inotify_state {
	unsigned flags;
};

static int inotify_get_options(int argc, char **argv, struct inotify_state *S)
{
	int c;

	memset(S, 0, sizeof(struct inotify_state));

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"cmd",		required_argument,	0, 'c' },
			{"verbose",	no_argument,		0, 'v' },
			{0,		0,			0,  0 }
		};

		c = getopt_long(argc, argv, "c:v",
				long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 0:
			printf("option %s", long_options[option_index].name);
			if (optarg)
				printf(" with arg %s", optarg);
			printf("\n");
			break;

		case 'c':
			printf("cmd\t%li\n", strtol(optarg, 0, 10));
			break;

		case 'v':
			S->flags |= F_VERBOSE;
			printf("set F_VERBOSE\n");
			break;

		case '?':
			break;

		default:
			printf("?? getopt returned character code 0%o ??\n", c);
			break;
		}
	}

	return optind;
}

static int inotify_add(const char *path)
{
	int fd, wd;

	printf("%s '%s'\n", __func__, path);

	fd = inotify_init1(IN_NONBLOCK);
	if (fd == -1) {
		printf("%s inotify_init1: %s\n", __func__, strerror(errno));
		return 0;
	}

	wd = inotify_add_watch(fd, path, IN_CLOSE_WRITE | IN_DELETE_SELF);
	if (wd == -1) {
		printf("%s inotify_add_watch: %i %s\n", __func__,
			errno, strerror(errno));
		return 0;
	}

	printf("OK fd %i, wd %i\n", fd, wd);

	close(wd);
	close(fd);

	return wd;
}

int main(int argc, char **argv)
{
	struct inotify_state S;
	int r;

	r = inotify_get_options(argc, argv, &S);

	while (r < argc) {
		printf("add inotify %s...\n", argv[r]);

		inotify_add(argv[r]);

		r++;
	}

	return 0;
}

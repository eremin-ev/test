/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <getopt.h>

#define PAGE_SIZE 4096

enum ls_flags {
	LS_LONG = 0x1 << 0,
};

static inline size_t l_offset(void *p1, void *p2)
{
	return p2 - p1;
}

int do_ls_flags(const char *path, const char *name, struct stat *st,
	unsigned flags, char *f_str, int f_len)
{
	int r;

	/*
	 * compose full path if a directory name has been passed (not a '.')
	 */
	if ('.' != *path) {
		snprintf(f_str, f_len, "%s/%s", path, name);
	} else {
		f_str = (char *) name;
	}

	r = lstat(f_str, st);
	if (-1 == r) {
		printf("uls: %s: %s\n", f_str, strerror(errno));
		return -1;
	}

	if (flags & LS_LONG) {
		printf("%zu\t", st->st_size);
	}

	printf("%s%s\n", f_str, (S_IFDIR == (st->st_mode & S_IFMT)) ? "/" : "");

	return 0;
}

int do_ls(const char *path, unsigned flags)
{
	int r = 0;
	struct dirent *e = 0;
	/*
	 * A buffer to store 'struct stat' and strings
	 */
	void *b = malloc(PAGE_SIZE);
	if (b) {
		struct stat *st = b;

		/* round up to 16b boundary */
		int round_up = ((sizeof(*st) - 1)|15) + 1;

		/* a memory buffer to store full path name */
		char *f_str = (void *) st + round_up;
		int f_len = PAGE_SIZE - l_offset(b, f_str);

		DIR *d = opendir(path);
		if (d) {
			while (1) {
				errno = 0;
				e = readdir(d);
				if (e) {
					if (-1 == do_ls_flags(path, e->d_name,
							st, flags,
							f_str, f_len)) {
						r = -1;
						break;
					}
				} else {
					if (errno) {
						printf("uls: %s: %s\n", path,
							strerror(errno));
						r = -1;
					}
					break;
				}
			}

			closedir(d);
		} else {
			printf("uls: %s: %s\n", path, strerror(errno));
			r = -1;
		}
		free(b);
	} else {
		printf("uls: %s: %s\n", path, strerror(errno));
		r = -1;
	}

	return r;
}

int main(int argc, char **argv)
{
	struct option o[] = {
		{ "long", no_argument, 0, 'l' }
	};
	int c, i, nr = 0;
	unsigned flags = 0;

	while (-1 != (c = getopt_long(argc, argv, "l", o, &i))) {
		switch (c) {
		case 'l':
			flags |= LS_LONG;
			break;
		default:
			printf("unknown option %c (%x)\n", c, c);
			break;
		}
	}

	while (optind < argc) {
		do_ls(*(argv + optind), flags);
		optind++;
		nr++;
	}

	if (!nr) {
		do_ls(".", flags);
	}

	return 0;
}

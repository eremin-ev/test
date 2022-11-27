/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * This is a micro, non-recursive `ls` implementation.  Some testing:
 *
 *     $ echo $[(1<<32)/4096]
 *     1048576
 *
 *     $ dd if=/dev/zero of=a-big-file.bin bs=4096 count=$[1048576 + 1]
 *     1048577+0 records in
 *     1048577+0 records out
 *     4294971392 bytes (4.3 GB, 4.0 GiB) copied, 1.1342 s, 3.8 GB/s
 *
 *     $ ./test_uls -l
 *     180     ./.
 *     300     ./..
 *     4294971392      ./a-big-file.bin
 *     17360   ./test_uls
 *     2235    ./test_uls.c
 */

#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define PAGE_SIZE 4096

enum ls_flags {
	LS_LONG = 0x1 << 0,
};

static inline size_t l_offset(void *p1, void *p2)
{
	return p2 - p1;
}

static size_t round_up_16b(size_t v)
{
	// 15_{10} = {1111 1111}_{2}
	return ((v - 1) | 15) + 1;
}

static int do_ls_flags(const char *path,
		       const char *name,
		       struct stat *st,
		       unsigned flags,
		       char *f_str,
		       int f_len)
{
	/*
	 * compose full path if a directory name has been passed (not a '.')
	 */
	if ('.' != *path) {
		snprintf(f_str, f_len, "%s/%s", path, name);
	} else {
		f_str = (char *) name;
	}

	int r = lstat(f_str, st);
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

static int do_ls(const char *path, unsigned flags)
{
	/*
	 * A buffer to store 'struct stat' and strings
	 */
	void *b = malloc(PAGE_SIZE);
	if (!b) {
		printf("%s: %s: malloc: %s\n", __func__, path, strerror(errno));
		return -1;
	}

	struct stat *st = b;

	/* round up to 16 bytes boundary */
	int round_up = round_up_16b((sizeof(*st)));

	/* a memory buffer to store the full path name */
	char *f_str = (void *) st + round_up;
	int f_len = PAGE_SIZE - l_offset(b, f_str);

	DIR *d = opendir(path);
	if (!d) {
		printf("%s: %s: opendir: %s\n", __func__, path, strerror(errno));
		free(b);
		return -1;
	}

	int r = 0;
	while (1) {
		errno = 0;
		struct dirent *e = readdir(d);
		if (!e) {
			if (errno) {
				printf("%s: %s: readdir: %s\n", __func__, path, strerror(errno));
				r = -1;
			}
			break;
		}

		r = do_ls_flags(path, e->d_name, st, flags, f_str, f_len);
		if (r == -1) {
			r = -1;
			break;
		}
	}

	closedir(d);

	free(b);

	return r;
}

static unsigned get_flags(int argc, char **argv, int *idx)
{
	struct option o[] = {
		{ "long", no_argument, 0, 'l' }
	};

	int c;
	unsigned flags = 0;
	while (-1 != (c = getopt_long(argc, argv, "l", o, idx))) {
		switch (c) {
		case 'l':
			flags |= LS_LONG;
			break;

		default:
			printf("unknown option %c (%x)\n", c, c);
			break;
		}
	}

	return flags;
}

int main(int argc, char **argv)
{
	int idx;
	unsigned flags = get_flags(argc, argv, &idx);

	int nr = 0;
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

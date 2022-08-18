/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

/*
 * Dump vdso area of a process address space into a file 'vdso.bin'
 *
 *	make test_vdso
 *	./test_vdso
 *	objdump -T vdso.so
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <unistd.h>

struct vm_area {
	void *begin;
	void *end;
};

bool parse_maps(char *buf, __attribute__((unused)) int buf_size, struct vm_area *v)
{
	char *s = buf;
	size_t x1, x2;	/* start, end addresses */
	while (*s) {
		/* beginning of a line */
		char *s_bol = s;
		while (*s && *s != '\n') {
			s++;
		}

		if (*s != '\n') {
			return false;
		}

		// Limit the search by the current line only
		// (use strnstr(...) for avoid modifying supplied buffer)
		*s = 0;
		s++;

		if (!strstr(s_bol, "[vdso]")) {
			continue;
		}

		//printf("line '%s'\n", s_bol);
		char *s_eo_x1;	/* end of x1 -- the start address */
		x1 = strtol(s_bol, &s_eo_x1, 16);
		if (s_bol == s_eo_x1) {
			return false;
		}

		if (*s_eo_x1 != '-') {
			return false;
		}

		s_eo_x1++;

		char *s_eo_x2;
		x2 = strtol(s_eo_x1, &s_eo_x2, 16);
		if (s_eo_x1 == s_eo_x2) {
			return false;
		}

		v->begin = (void *) x1;
		v->end = (void *) x2;

		return true;
	}

	return false;
}

int main(void)
{
	const char *path = "/proc/self/maps";
	int fd_maps = open(path, O_RDONLY);
	if (fd_maps == -1) {
		printf("cannot open %s for reading: %s\n", path, strerror(errno));
		return EXIT_FAILURE;
	}

	const int buf_size = 4096;
	char buf[buf_size];
	int r = read(fd_maps, buf, buf_size);

	close(fd_maps);

	if (r == -1) {
		printf("cannot read %s: %s\n", path, strerror(errno));
		return EXIT_FAILURE;
	}

	struct vm_area v;
	if (!parse_maps(buf, buf_size, &v)) {
		printf("cannot parse %s\n", path);
		return EXIT_FAILURE;
	}

	long size = v.end - v.begin;
	int fd_vdso = open("vdso.so", O_WRONLY | O_CREAT, 0644);
	if (fd_vdso == -1) {
		printf("cannot open 'vdso.so' for writing: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	r = write(fd_vdso, v.begin, size);

	close(fd_vdso);

	if (r == -1) {
		printf("cannot write 'vdso.so': %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	printf("Dump %p, size %li OK\n", v.begin, size);

	return EXIT_SUCCESS;
}

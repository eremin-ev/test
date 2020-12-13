/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int main()
{
	unsigned char buf[1024];
	int fd;
	int r;

	printf("uid %i eud %i\n", getuid(), geteuid());

	fd = open("/etc/shadow", O_RDONLY|O_CLOEXEC);

	if (fd == -1) {
		err(1, "%s", strerror(errno));
	}

	r = read(fd, buf, sizeof(buf));
	if (r == -1) {
		printf("%s: %s\n", __func__, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (buf[0] == 0xab) {
		printf("0xab\n");
	}

	return 0;
}


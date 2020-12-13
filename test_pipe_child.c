/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
	const int buf_size = 256;
	char buf_o[buf_size];
	char buf_i[buf_size];
	int r;

	// FIXME
	setbuf(stdout, NULL);

	r = snprintf(buf_o, buf_size, "Hello from child, uid %i, euid %i", getuid(), geteuid());
	write(STDOUT_FILENO, buf_o, r);

#if 1
	read(STDIN_FILENO, buf_i, buf_size);
	if (r > 0) {
		buf_i[r] = '\0';
	}
#endif

	r = snprintf(buf_o, buf_size, "Child read %i '%s'", r, buf_i);
	write(STDOUT_FILENO, buf_o, r);

	return EXIT_SUCCESS;
}

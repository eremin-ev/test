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

#include "spawn-child.h"

#if 0
static int read_buf(int fd, char *buf, int buf_size)
{
       int b, total = 0;

       while (buf_size > 0) {
	       b = read(fd, &buf[total], buf_size);

	       printf("%s read %i, %i, %i\n", __func__, b, buf_size, total);

	       if (b < 0) {
		       if (errno == EINTR)
			       continue;
		       return b;
	       }
	       if (b == 0)
		       return total;

	       total += b;
	       buf_size -= b;
       }

       return total;
}

int write_buf(int fd, const char *buf, int buf_size)
{
       int b, total = 0;

       while (buf_size > 0) {
               b = write(fd, &buf[total], buf_size);

               if (b < 0) {
                       if (errno == EINTR)
			       continue;
                       return b;
               }
               if (b == 0)
		       return total;

               total += b;
               buf_size -= b;
       }

       return total;
}
#endif

static int talk_to_child(int *fds)
{

	int r;
	const int buf_size = 128;
#if 1
	char buf_i[buf_size];

	printf("%s reading %i ...\n", __func__, fds[PIPE_READ_FD]);
	r = read(fds[PIPE_READ_FD], buf_i, buf_size);
	if (r > 0) {
		buf_i[r] = '\0';
		printf("%s From child: '%s'\n", __func__, buf_i);
	} else {
		printf("%s read error %d: %s\n", __func__,
			r, strerror(errno));
		return r;
	}
#endif

	char buf_o[] = "Hello from parent";
	printf("%s writing from parent to %i...\n", __func__, fds[PIPE_WRITE_FD]);
	r = write(fds[PIPE_WRITE_FD], buf_o, sizeof(buf_o));
	if (r > 0) {
		printf("%s Ok written %i to child\n", __func__, r);
	} else {
		printf("%s write error %d: %s\n", __func__,
			r, strerror(errno));
		return r;
	}

	printf("%s reading %i ...\n", __func__, fds[PIPE_READ_FD]);
	r = read(fds[PIPE_READ_FD], buf_i, buf_size);
	if (r > 0) {
		buf_i[r] = '\0';
		printf("%s From child: '%s'\n", __func__, buf_i);
	} else {
		printf("%s read error %i: %s\n", __func__,
			r, strerror(errno));
		return r;
	}

	return 0;
}

//int main(int argc, char **argv)
int main()
{
	int fds[2];
	int child;
	const char *child_path = "./test_pipe_child";
	const char *child_argv[] = {
		child_path,
		"arg1",
		"arg2",
		NULL
	};
	const char *child_envp[] = {
		"VAR1=val1",
		"VAR2=val2",
		NULL
	};
	struct sigaction oldsa;

	printf("Parent uid %i, euid %i\n", getuid(), geteuid());

	child = child_spawn(child_path, child_argv, child_envp, fds, &oldsa);

	talk_to_child(fds);

	child_wait(child, fds);

	/* restore old signal handler */
	sigaction(SIGCHLD, &oldsa, NULL);

	return child == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

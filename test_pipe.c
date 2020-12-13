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

#define TEST_PIPE_CHILD		"./test_pipe_child"

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

#define READ_FD		0
#define WRITE_FD	1

static int run_pipe_child(const char *child_path)
{
	int retval = 0, child;
	struct sigaction newsa, oldsa;

	/* create a pipe for the messages */
	int fds1[2];
	if (pipe(fds1) != 0) {
		printf("could not create a pipe 1\n");
		return 0;
	}

	int fds2[2];
	if (pipe(fds2) != 0) {
		printf("could not create a pipe 2\n");
		return 0;
	}

	/*
	 * fds1[0] read parent
	 * fds1[1] write child
	 *
	 * fds2[0] read child
	 * fds2[1] write parent
	 *
	 */

	/*
	 * This code arranges that the demise of the child does not cause
	 * the application to receive a signal it is not expecting - which
	 * may kill the application or worse.
	 *
	 * The "noreap" module argument is provided so that the admin can
	 * override this behavior.
	 */
	memset(&newsa, '\0', sizeof(newsa));
	newsa.sa_handler = SIG_DFL;
	sigaction(SIGCHLD, &newsa, &oldsa);

	/* fork */
	child = fork();
	if (child == 0) {
		static char *envp[] = { NULL };
		const char *args[] = {
			child_path,
			"arg1",
			"arg2",
			NULL
		};

		close(fds1[READ_FD]);
		close(fds2[WRITE_FD]);

		/* reopen stdin as pipe */
		if (dup2(fds2[READ_FD], STDIN_FILENO) != STDIN_FILENO) {
			_exit(EXIT_FAILURE);
		}

		/* reopen stdout as pipe */
		if (dup2(fds1[WRITE_FD], STDOUT_FILENO) != STDOUT_FILENO) {
			_exit(EXIT_FAILURE);
		}

		/* reopen stderr as pipe */
		if (dup2(fds1[WRITE_FD], STDERR_FILENO) != STDERR_FILENO) {
			_exit(EXIT_FAILURE);
		}

		close(fds2[READ_FD]);
		close(fds1[WRITE_FD]);

		/* exec binary helper */
		execve(child_path, (char *const *) args, envp);
		//execl("/usr/bin/tee", "tee", (char*) NULL);

		printf("%s helper binary execve failed: %m", __func__);
		_exit(EXIT_FAILURE);
	} else {
		if (child > 0) {
			int rc = 0;
			close(fds2[READ_FD]);
			close(fds1[WRITE_FD]);

#if 1
			char buf_i[128];

			printf("%s reading %i ...\n", __func__, fds1[READ_FD]);
			rc = read(fds1[READ_FD], buf_i, sizeof(buf_i) - 1);
			if (rc > 0) {
				buf_i[rc] = '\0';
				printf("%s From child: '%s'\n", __func__, buf_i);
			} else {
				printf("%s read error %d: %m\n", __func__, rc);
				retval = 1;
			}
#endif

			char buf_o[] = "Hello from parent";
			printf("%s writing from parent to %i...\n", __func__, fds2[WRITE_FD]);
			rc = write(fds2[WRITE_FD], buf_o, sizeof(buf_o));
			if (rc > 0) {
				printf("%s Ok written %i to child\n", __func__, rc);
			} else {
				printf("%s write error %d: %m\n", __func__, rc);
				retval = 1;
			}

			printf("%s reading %i ...\n", __func__, fds1[READ_FD]);
			rc = read(fds1[READ_FD], buf_i, sizeof(buf_i) - 1);
			if (rc > 0) {
				buf_i[rc] = '\0';
				printf("%s From child: '%s'\n", __func__, buf_i);
			} else {
				printf("%s read error %d: %m\n", __func__, rc);
				retval = 1;
			}

			/* wait for helper to complete: */
			printf("%s waiting to complete...\n", __func__);
			while ((rc = waitpid(child, &retval, 0)) < 0 && errno == EINTR);
			if (rc < 0) {
				printf("%s unix_chkpwd waitpid returned %d: %m", __func__, rc);
				retval = 1;
			} else if (!WIFEXITED(retval)) {
				printf("%s unix_chkpwd abnormal exit: %d", __func__, retval);
				retval = 1;
			} else {
				retval = WEXITSTATUS(retval);
			}
		} else {
			printf("%s Fork failed: %m\n", __func__);
			retval = 1;
		}
		close(fds1[READ_FD]);
		close(fds2[WRITE_FD]);
	}

	/* restore old signal handler */
	sigaction(SIGCHLD, &oldsa, NULL);

	printf("%s Returning %d\n", __func__, retval);
	return retval;
}

//int main(int argc, char **argv)
int main()
{
	int r;

	printf("Parent uid %i, euid %i\n", getuid(), geteuid());

	r = run_pipe_child(TEST_PIPE_CHILD);

	return r == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

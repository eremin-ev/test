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

#define PIPE_READ_FD		0
#define PIPE_WRITE_FD		1

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

static void ignore_sigchld(struct sigaction *oldsa)
{
	struct sigaction newsa;

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
	sigaction(SIGCHLD, &newsa, oldsa);
}

/*
 * Reopen stdin, stdout, stderr as pipes in the child process and close
 * unused fds.  (Child cannot use printf after stdout is redirected.)
 */
static void dup_stdio(int *fds1, int *fds2)
{
	close(fds1[PIPE_READ_FD]);
	close(fds2[PIPE_WRITE_FD]);

	if (dup2(fds2[PIPE_READ_FD], STDIN_FILENO) != STDIN_FILENO) {
		_exit(EXIT_FAILURE);
	}

	if (dup2(fds1[PIPE_WRITE_FD], STDOUT_FILENO) != STDOUT_FILENO) {
		_exit(EXIT_FAILURE);
	}

	if (dup2(fds1[PIPE_WRITE_FD], STDERR_FILENO) != STDERR_FILENO) {
		_exit(EXIT_FAILURE);
	}

	close(fds2[PIPE_READ_FD]);
	close(fds1[PIPE_WRITE_FD]);
}

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

int wait_and_fini(int child, int *fds)
{
	int r, status;

	/* wait for helper to complete: */
	printf("%s waiting %i to complete...\n", __func__, child);
	while ((r = waitpid(child, &status, 0)) < 0 && errno == EINTR);
	if (r < 0) {
		printf("%s unix_chkpwd waitpid returned %d: %s", __func__,
			r, strerror(errno));
		r = EXIT_FAILURE;
	} else if (!WIFEXITED(status)) {
		printf("%s unix_chkpwd abnormal exit: %d", __func__, r);
		r = EXIT_FAILURE;
	} else {
		r = WEXITSTATUS(status);
	}

	close(fds[PIPE_READ_FD]);
	close(fds[PIPE_WRITE_FD]);

	return r;
}

static int run_pipe_child(const char *child_path,
			  const char *child_argv[],
			  const char *child_envp[],
			  int *fds,
			  struct sigaction *oldsa)
{
	int child;
	int fds1[2];
	int fds2[2];

	/*
	 * The ends of pipes are used as follows:
	 *
	 * fds1[0] read from parent
	 * fds1[1] write to child
	 *
	 * fds2[0] read from child
	 * fds2[1] write to parent
	 *
	 */

	if (pipe(fds1) != 0) {
		printf("%s could not create a pipe fds1: %s\n",
			__func__, strerror(errno));
		return 0;
	}

	if (pipe(fds2) != 0) {
		printf("%s could not create a pipe fds2: %s\n",
			__func__, strerror(errno));
		return 0;
	}

	ignore_sigchld(oldsa);

	child = fork();
	if (child < 0) {
		printf("%s Fork failed: %s\n", __func__, strerror(errno));
		close(fds1[PIPE_READ_FD]);
		close(fds1[PIPE_WRITE_FD]);
		close(fds2[PIPE_READ_FD]);
		close(fds2[PIPE_WRITE_FD]);
		return 1;
	}

	if (child == 0) {
		dup_stdio(fds1, fds2);

		execve(child_path,
			(char *const *) child_argv,
			(char * const *)child_envp);
		//execl("/usr/bin/tee", "tee", (char*) NULL);

		printf("%s helper binary execve failed: %s", __func__,
			strerror(errno));
		_exit(EXIT_FAILURE);
	} else {
		close(fds2[PIPE_READ_FD]);
		close(fds1[PIPE_WRITE_FD]);

		fds[PIPE_READ_FD] = fds1[PIPE_READ_FD];
		fds[PIPE_WRITE_FD] = fds2[PIPE_WRITE_FD];
	}

	printf("%s Returning child %i\n", __func__, child);
	return child;
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

	child = run_pipe_child(child_path, child_argv, child_envp, fds, &oldsa);

	talk_to_child(fds);

	wait_and_fini(child, fds);

	/* restore old signal handler */
	sigaction(SIGCHLD, &oldsa, NULL);

	return child == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

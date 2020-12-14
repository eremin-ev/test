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
#include <security/pam_appl.h>

#include "spawn-child.h"

#define TEST_PAM_HELPER		"./test_pam_helper"

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

	return 0;
}

int main(int argc, char **argv)
{
	const char *arg1;

	if (argc != 2) {
		printf(	"Usage: %s <auth>|<chtok>\n"
			"(please enter <password>^D afterwards)\n",
			argv[0]);
		return EXIT_FAILURE;
	}

	if (!strcmp(argv[1], "auth")) {
		arg1 = "auth";
	} else if (!strcmp(argv[1], "chtok")) {
		arg1 = "chtok";
	} else {
		printf("Unknown request: %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	int fds[2];
	int child;
	const char *child_path = "./test_pam_helper";
	const char *child_argv[] = {
		child_path,
		arg1,
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

	int r = 0;

	return r == PAM_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;
}

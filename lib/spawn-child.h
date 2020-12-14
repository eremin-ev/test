/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#define PIPE_READ_FD		0
#define PIPE_WRITE_FD		1

int child_wait(int child, int *fds);
int child_spawn(const char *child_path,
		const char *child_argv[],
		const char *child_envp[],
		int *fds,
		struct sigaction *oldsa);

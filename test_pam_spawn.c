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

#include "test_pam_spawn.h"

#define TEST_PAM_HELPER		"./test_pam_helper"

static const char *msg_type_str(int t)
{
	const char *s = "<unknown>";

	switch (t) {
	case PAM_PROMPT_ECHO_OFF: s = "PAM_PROMPT_ECHO_OFF"; break;
	case PAM_PROMPT_ECHO_ON: s = "PAM_PROMPT_ECHO_ON"; break;
	case PAM_ERROR_MSG: s = "PAM_ERROR_MSG"; break;
	case PAM_TEXT_INFO: s = "PAM_TEXT_INFO"; break;
	}

	return s;
}

int read_from_stdin(struct pam_msg_rp *rp)
{
	int r;
	int tok_len = 128;
	char *tok_str = malloc(tok_len);
	r = read(STDIN_FILENO, tok_str, tok_len);
	if (r == -1) {
		printf("%s: %s\n", __func__, strerror(errno));
		return 0;
	}
	tok_str[r] = '\0';

	rp->rep_len = r;
	rp->rep_str = tok_str;

	printf("%s r %i, authtok '%s'\n", __func__, rp->rep_len, rp->rep_str);

	return r;
}

int rq_handle(struct pam_msg_rq *rq)
{
	printf("%s %s '%s:%i'\n", __func__,
		msg_type_str(rq->msg_style), rq->msg, rq->msg_len);
	if (rq->msg_style == PAM_PROMPT_ECHO_OFF &&
			!strcmp(rq->msg, "Password: ")) {
		rq->rq_flags |= REP_PASSWD;
	}

	if (rq->msg_style == PAM_PROMPT_ECHO_OFF &&
			!strcmp(rq->msg, "Current password: ")) {
		rq->rq_flags |= REP_PASSWD_RETYPE;
	}

	if (rq->msg_style == PAM_PROMPT_ECHO_OFF &&
			!strcmp(rq->msg, "New password: ")) {
		rq->rq_flags |= REP_PASSWD_NEW;
	}

	if (rq->msg_style == PAM_PROMPT_ECHO_OFF &&
			!strcmp(rq->msg, "Retype new password: ")) {
		rq->rq_flags |= REP_PASSWD_RETYPE;
	}

	return rq->rq_flags;
}

int msg_read(int fd, struct pam_msg_rq *rq)
{
	int r;

	read(fd, &rq->msg_style, sizeof(rq->msg_style));
	read(fd, &rq->msg_len, sizeof(rq->msg_len));
	rq->msg = malloc(rq->msg_len);
	r = read(fd, rq->msg, rq->msg_len);
	rq->msg[r] = '\0';

	printf("%s msg_style %i, msg '%s:%i'\n", __func__,
		rq->msg_style,
		rq->msg, rq->msg_len);

	return r;
}

static int rq_input_required(unsigned flags)
{
	return flags & REP_PASSWD
			|| flags & REP_PASSWD_CURRENT
			|| flags & REP_PASSWD_NEW
			|| flags & REP_PASSWD_RETYPE;
}

static int handle_ret(int fd_i)
{
	const int ret_len = 256;
	char ret_str[ret_len];
	int ret;
	int r;

	printf("%s handle ret code..\n", __func__);
	read(fd_i, &ret, sizeof(ret));
	r = read(fd_i, ret_str, ret_len);
	ret_str[r] = '\0';
	printf("%s ret: %d '%s:%d', stopping\n", __func__, ret, ret_str, r);

	return ret;
}

static int handle_conv(int fd_i, int fd_o)
{
	int num_msg, input_required = 0;
	struct pam_msg_rq *rq;
	struct pam_msg_rp rp;
	int i, r;

	printf("%s conv... reading num_msg from %d ...\n", __func__, fd_i);
	read(fd_i, &num_msg, sizeof(num_msg));

	rq = malloc(num_msg * sizeof(struct pam_msg_rq));

	for (i = 0; i < num_msg; i++) {
		msg_read(fd_i, &rq[i]);
		if (rq_handle(&rq[i])) {
			++input_required;
		}
	}

	if (input_required == num_msg) {
		r = write(fd_o, &num_msg, sizeof(num_msg));
		printf("%s OK r %d write %d to child %d\n", __func__,
			r, num_msg, fd_o);

		/* obtain input for each request */
		for (i = 0; i < num_msg; i++) {
			if (rq_input_required(rq[i].rq_flags)) {
				if (read_from_stdin(&rp)) {
					r = write(fd_o, &rp.rep_len, sizeof(rp.rep_len));
					r = write(fd_o, rp.rep_str, rp.rep_len);
				} else {
					printf("%s Cannot write to child\n", __func__);
				}
			} else {
				printf("%s Unhandled req\n", __func__);
			}
		}
	} else if (input_required == 0) {
		r = write(fd_o, &input_required, sizeof(input_required));
		printf("%s OK r %d write input_required %d to %d\n", __func__,
			r, input_required, fd_o);
	} else {
		printf("%s bad input_required %d\n", __func__, input_required);
	}

	for (i = 0; i < num_msg; i++) {
		free(rq[i].msg);
	}

	free(rq);

	return 0;
}

static int talk_to_helper(int fd_i, int fd_o)
{
	int ret;
	int cmd;
	int run = 1;

	while (run) {
		printf("%s reading from %d...\n", __func__, fd_i);
		read(fd_i, &cmd, sizeof(cmd));
		printf("%s read cmd %d from %d...\n", __func__, cmd, fd_i);

		switch (cmd) {
		case CMD_RET:
			ret = handle_ret(fd_i);
			run = 0;
			break;

		case CMD_CONV:
			handle_conv(fd_i, fd_o);
			break;
		}
	}

	return ret;
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

	talk_to_helper(fds[PIPE_READ_FD], fds[PIPE_WRITE_FD]);

	child_wait(child, fds);

	/* restore old signal handler */
	sigaction(SIGCHLD, &oldsa, NULL);

	int r = 0;

	return r == PAM_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;
}

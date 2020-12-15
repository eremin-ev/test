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

enum reply_type {
	REP_PASSWD =		1U << 0,
	REP_PASSWD_CURRENT =	1U << 1,
	REP_PASSWD_NEW =	1U << 2,
	REP_PASSWD_RETYPE =	1U << 3,
};

#define TEST_PAM_HELPER		"./test_pam_helper"

struct pam_msg_rq {
	int msg_style;
	int msg_len;
	int rq_flags;
	char *msg;
};

struct pam_msg_rp {
	int in_len;
	char *in_str;
};

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
	r = read(0, tok_str, tok_len);
	if (r == -1) {
		printf("%s: %s\n", __func__, strerror(errno));
		return 0;
	}
	tok_str[r] = '\0';

	rp->in_len = r;
	rp->in_str = tok_str;

	printf("%s r %i, authtok '%s'\n", __func__, rp->in_len, rp->in_str);

	return r;
}

int rq_handle(struct pam_msg_rq *rq)
{
	int rep_flags = 0;
	printf("%s %s '%s:%i'\n", __func__,
		msg_type_str(rq->msg_style), rq->msg, rq->msg_len);
	if (rq->msg_style == PAM_PROMPT_ECHO_OFF &&
			!strcmp(rq->msg, "Password: ")) {
		rep_flags |= REP_PASSWD;
	}

	if (rq->msg_style == PAM_PROMPT_ECHO_OFF &&
			!strcmp(rq->msg, "Current password: ")) {
		rep_flags |= REP_PASSWD_RETYPE;
	}

	if (rq->msg_style == PAM_PROMPT_ECHO_OFF &&
			!strcmp(rq->msg, "New password: ")) {
		rep_flags |= REP_PASSWD_NEW;
	}

	if (rq->msg_style == PAM_PROMPT_ECHO_OFF &&
			!strcmp(rq->msg, "Retype new password: ")) {
		rep_flags |= REP_PASSWD_RETYPE;
	}

	rq->rq_flags = rep_flags;

	return rep_flags;
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

static int talk_to_child(int *fds)
{
	int i, r;
	int num_msg, input_required;
	struct pam_msg_rq *rq;
	struct pam_msg_rp rp;

	do {
		input_required = 0;

		printf("%s reading %i ...\n", __func__, fds[PIPE_READ_FD]);
		read(fds[PIPE_READ_FD], &num_msg, sizeof(num_msg));

		rq = malloc(num_msg * sizeof(struct pam_msg_rq));

		for (i = 0; i < num_msg; i++) {
			msg_read(fds[PIPE_READ_FD], &rq[i]);
			if (rq_handle(&rq[i])) {
				++input_required;
			}
		}

		if (input_required == num_msg) {
			r = write(fds[PIPE_WRITE_FD], &num_msg, sizeof(num_msg));
			printf("%s OK r %d write %d to child %d\n", __func__,
				r, num_msg, fds[PIPE_READ_FD]);

			/* obtain input for each request */
			for (i = 0; i < num_msg; i++) {
				if (rq_input_required(rq[i].rq_flags)) {
					if (read_from_stdin(&rp)) {
						r = write(fds[PIPE_WRITE_FD], &rp.in_len, sizeof(rp.in_len));
						r = write(fds[PIPE_WRITE_FD], rp.in_str, rp.in_len);
					} else {
						printf("%s Cannot write to child\n", __func__);
					}
				} else {
					printf("%s Unhandled req\n", __func__);
				}
			}
		} else if (input_required == 0) {
			r = write(fds[PIPE_WRITE_FD], &input_required, sizeof(input_required));
			printf("%s OK r %d write input_required %d to child %d\n", __func__,
				r, input_required, fds[PIPE_READ_FD]);
		} else {
			printf("%s bad input_required %d\n", __func__, input_required);
		}

		for (i = 0; i < num_msg; i++) {
			free(rq[i].msg);
		}

		free(rq);

	} while (num_msg);

#if 0
	int r;
	const int buf_size = 128;
	char buf_i[buf_size];

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

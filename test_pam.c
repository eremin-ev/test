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

#define TEST_PAM_HELPER		"./test_pam_helper"

enum reply_type {
	REP_PASSWD =		1U << 0,
	REP_PASSWD_CURRENT =	1U << 1,
	REP_PASSWD_NEW =	1U << 2,
	REP_PASSWD_RETYPE =	1U << 3,
};

enum req_type {
	REQ_AUTH =		1U,
	REQ_CHTOK =		2U
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

static char *read_from_stdin()
{
	int r;
	int passwd_len = 128;
	char *passwd_str = malloc(passwd_len);
	r = read(0, passwd_str, passwd_len);
	if (r == -1) {
		printf("%s: %s\n", __func__, strerror(errno));
		return NULL;
	}
	passwd_str[r] = '\0';
	printf("%s r %i, passwd '%s'\n", __func__, r, passwd_str);

	return passwd_str;
}

static int conv_handle_resp(struct pam_response **resp, unsigned rep_flags)
{
	char *passwd_str;

	if (rep_flags & REP_PASSWD
			|| rep_flags & REP_PASSWD_CURRENT
			|| rep_flags & REP_PASSWD_NEW
			|| rep_flags & REP_PASSWD_RETYPE) {
		*resp = malloc(sizeof(struct pam_response));
		passwd_str = read_from_stdin();
		if (passwd_str == NULL) {
			return PAM_BUF_ERR;
		}
		(*resp)->resp_retcode = 0;
		(*resp)->resp = passwd_str;

		/* NOTE: no free(passwd_str) since PAM will free it */
	}

	return PAM_SUCCESS;
}

static int conv_cb(int num_msg, const struct pam_message **msg,
	    struct pam_response **resp, void *user_data_ptr)
{
	unsigned rep_flags = 0;
	int i;

	printf("%s num_msg: %i, user_data_ptr %p\n", __func__,
		num_msg, user_data_ptr);

	/* pam_conv */
	if (num_msg == 1) {
		printf("%s %s '%s'\n", __func__,
			msg_type_str((*msg)->msg_style), (*msg)->msg);
		if ((*msg)->msg_style == PAM_PROMPT_ECHO_OFF &&
				!strcmp((*msg)->msg, "Password: ")) {
			rep_flags |= REP_PASSWD;
		}

		if ((*msg)->msg_style == PAM_PROMPT_ECHO_OFF &&
				!strcmp((*msg)->msg, "Current password: ")) {
			rep_flags |= REP_PASSWD_RETYPE;
		}

		if ((*msg)->msg_style == PAM_PROMPT_ECHO_OFF &&
				!strcmp((*msg)->msg, "New password: ")) {
			rep_flags |= REP_PASSWD_NEW;
		}

		if ((*msg)->msg_style == PAM_PROMPT_ECHO_OFF &&
				!strcmp((*msg)->msg, "Retype new password: ")) {
			rep_flags |= REP_PASSWD_RETYPE;
		}
	} else {
		printf("%s more than one message in reply, skipping...", __func__);
		for (i = 0; i < num_msg; i++) {
			printf("%s %i %s '%s'\n", __func__,
				i, msg_type_str(msg[i]->msg_style), msg[i]->msg);
		}
	}

	return conv_handle_resp(resp, rep_flags);
}

static int do_auth(pam_handle_t *pamh)
{
	int r;

	r = pam_authenticate(pamh, 0);
	printf("%s pam_authenticate() r %i\n", __func__, r);

	if (r != PAM_SUCCESS) {
		printf("%s pam_authenticate() r %i: %s\n", __func__,
			r, pam_strerror(pamh, r));
		return r;
	}

	r = pam_acct_mgmt(pamh, 0);
	printf("%s pam_acct_mgmt() ret %i\n", __func__, r);

	return r;
}

static int do_chtok(pam_handle_t *pamh, unsigned flags)
{
	int r = pam_chauthtok(pamh, flags);
	printf("%s pam_chauthtok() r %i\n", __func__, r);

	if (r != PAM_SUCCESS) {
		printf("%s pam_chauthtok() r %i: %s\n", __func__,	
			r, pam_strerror(pamh, r));
		return r;
	}

	return r;
}

static int do_pam(const char *user, enum req_type req_type)
{
	struct pam_conv pam_conv = { conv_cb, NULL };
	pam_handle_t *pamh;

	int r = pam_start("login", user, &pam_conv, &pamh);
	printf("%s pam_start() r %i\n", __func__, r);

	if (r != PAM_SUCCESS) {
		printf("%s pam_start() r %i: %s\n", __func__,	
			r, pam_strerror(pamh, r));
		return r;
	}

	switch (req_type) {
	case REQ_AUTH:
		r = do_auth(pamh);
		if (r == PAM_NEW_AUTHTOK_REQD) {
			do_chtok(pamh, PAM_CHANGE_EXPIRED_AUTHTOK);
		}
		break;
	case REQ_CHTOK:
	 	r = do_chtok(pamh, 0);
		break;
	}

	pam_end(pamh, r);

	return r;
}

static int pam_modutil_read(int fd, char *buffer, int count)
{
       int block, offset = 0;

       while (count > 0) {
               block = read(fd, &buffer[offset], count);

               if (block < 0) {
                       if (errno == EINTR) continue;
                       return block;
               }
               if (block == 0) return offset;

               offset += block;
               count -= block;
       }

       return offset;
}

int run_helper_binary(const char *user)
{
	int retval=0, child, fds[2];
	struct sigaction newsa, oldsa;

	printf("%s\n", __func__);

	/* create a pipe for the messages */
	if (pipe(fds) != 0) {
		printf("could not create a pipe\n");
		return PAM_AUTH_ERR;
	}

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
		const char *args[] = { NULL, NULL, NULL, NULL };

		/* XXX - should really tidy up PAM here too */

		/* reopen stdout as pipe */
		if (dup2(fds[1], STDOUT_FILENO) != STDOUT_FILENO) {
			printf("%s dup2 of %s failed: %m", __func__, "stdout");
			_exit(PAM_AUTHINFO_UNAVAIL);
		}

		if (geteuid() == 0) {
			/* must set the real uid to 0 so the helper will not error
			   out if pam is called from setuid binary (su, sudo...) */
			if (setuid(0) == -1) {
				printf("%s setuid failed: %m", __func__);
				printf("-1\n");
				fflush(stdout);
				_exit(PAM_AUTHINFO_UNAVAIL);
			}
		}

		/* exec binary helper */
		args[0] = TEST_PAM_HELPER;
		args[1] = user;
		args[2] = "chkexpiry";

		execve(TEST_PAM_HELPER, (char *const *) args, envp);

		printf("%s helper binary execve failed: %m", __func__);
		/* should not get here: exit with error */
		printf("-1\n");
		fflush(stdout);
		_exit(PAM_AUTHINFO_UNAVAIL);
	} else {
		close(fds[1]);
		if (child > 0) {
			char buf[32];
			int rc = 0;
			/* wait for helper to complete: */
			while ((rc=waitpid(child, &retval, 0)) < 0 && errno == EINTR);
			if (rc<0) {
				printf("%s unix_chkpwd waitpid returned %d: %m", __func__, rc);
				retval = PAM_AUTH_ERR;
			} else if (!WIFEXITED(retval)) {
				printf("%s unix_chkpwd abnormal exit: %d", __func__, retval);
				retval = PAM_AUTH_ERR;
			} else {
				retval = WEXITSTATUS(retval);
				rc = pam_modutil_read(fds[0], buf, sizeof(buf) - 1);
				if(rc > 0) {
					buf[rc] = '\0';
					printf("%s From child: '%s'\n", __func__, buf);
				} else {
					printf("%s read unix_chkpwd output error %d: %m", __func__, rc);
					retval = PAM_AUTH_ERR;
				}
			}
		} else {
			printf("%s Fork failed: %m\n", __func__);
			retval = PAM_AUTH_ERR;
		}
		close(fds[0]);
	}

	/* restore old signal handler */
	sigaction(SIGCHLD, &oldsa, NULL);

	printf("%s Returning %d\n ", __func__, retval);
	return retval;
}

int main(int argc, char **argv)
{
	int r;

	if (argc < 3) {
		printf(	"Usage: %s <username> <auth>|<chtok>\n"
			"(please enter <password>^D afterwards)\n",
			argv[0]);
		return EXIT_FAILURE;
	}

	printf("uid %i, euid %i\n", getuid(), geteuid());

	r = run_helper_binary(argv[1]);

	if (0) {
		if (!strcmp(argv[2], "auth")) {
			r = do_pam(argv[1], REQ_AUTH);
			printf("%sAuthenticated.\n", r == PAM_SUCCESS ? "" : "Not ");
		} else if (!strcmp(argv[2], "chtok")) {
			r = do_pam(argv[1], REQ_CHTOK);
			printf("%sChanged auth token.\n", r == PAM_SUCCESS ? "Successfuly" : "Failed ");
		} else {
			r = PAM_AUTH_ERR;
			printf("Unknown request: %s\n", argv[2]);
		}
	}

	return r == PAM_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;
}

/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <pwd.h>
#include <security/pam_appl.h>

#define PAM_TEST_HELPER		"test_pam_helper"

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

__attribute__((__format__ (printf, 2, 3)))
static void helper_log(int prio, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	openlog(PAM_TEST_HELPER, LOG_CONS | LOG_PID, LOG_AUTHPRIV);
	vsyslog(prio, fmt, args);
	va_end(args);
	closelog();
}

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

int main(int argc, char **argv)
{
	int r;

	/*
	 * we establish that this program is running with non-tty stdin.
	 * this is to discourage casual use. It does *NOT* prevent an
	 * intruder from repeatadly running this program to determine the
	 * password of the current user (brute force attack, but one for
	 * which the attacker must already have gained access to the user's
	 * account).
	 */

	if (isatty(STDIN_FILENO) || argc != 2 ) {
		helper_log(LOG_NOTICE,
			"inappropriate use of %s [UID=%d]",
			PAM_TEST_HELPER,
			getuid());
		fprintf(stderr,
		"This binary is not designed for running in this way\n"
		      "-- the system administrator has been informed\n");
		sleep(10);	/* this should discourage/annoy the user */
		return PAM_SYSTEM_ERR;
	}

	struct passwd *pw = getpwuid(getuid());
	if (!pw) {
		helper_log(LOG_NOTICE, "Cannot retrieve info for uid %d",
			getuid());
		return PAM_AUTHTOK_ERR;
	}

	helper_log(LOG_NOTICE, "uid %i, euid %i, user '%s'\n",
		getuid(), geteuid(), pw->pw_name);

	if (!strcmp(argv[1], "auth")) {
		r = do_pam(pw->pw_name, REQ_AUTH);
		helper_log(LOG_NOTICE, "%sAuthenticated.\n",
			r == PAM_SUCCESS ? "" : "Not ");
	} else if (!strcmp(argv[1], "chtok")) {
		r = do_pam(pw->pw_name, REQ_CHTOK);
		helper_log(LOG_NOTICE, "%sChanged auth token.\n",
			r == PAM_SUCCESS ? "Successfuly" : "Failed ");
	} else {
		r = PAM_AUTH_ERR;
		printf("Unknown request: %s\n", argv[2]);
	}

	return r == PAM_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;
}

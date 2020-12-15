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

struct pam_msg_rp {
	int rep_len;
	char *rep_str;
};

static int read_from_parent(struct pam_msg_rp *rp)
{
	int r;

	r = read(STDIN_FILENO, &rp->rep_len, sizeof(rp->rep_len));
	helper_log(LOG_NOTICE, "%s OK r %d rep_len %d", __func__, r, rp->rep_len);

	rp->rep_str = malloc(rp->rep_len);
	r = read(STDIN_FILENO, rp->rep_str, rp->rep_len);
	if (r == -1) {
		helper_log(LOG_NOTICE, "%s: %s", __func__, strerror(errno));
		return 0;
	}
	rp->rep_str[r] = '\0';
	helper_log(LOG_NOTICE, "%s r %i, rep_str '%s'", __func__, r, rp->rep_str);

	return r;
}

static int conv_handle_resp(struct pam_response **resp)
{
	int r;
	unsigned i;
	struct pam_msg_rp rp;
	unsigned rep_nr;

	r = read(STDIN_FILENO, &rep_nr, sizeof(rep_nr));
	helper_log(LOG_NOTICE, "%s OK r %d rep_nr %d", __func__, r, rep_nr);

	if (rep_nr) {
		*resp = malloc(rep_nr * sizeof(struct pam_response));

		for (i = 0; i < rep_nr; i++) {
			r = read_from_parent(&rp);
			if (!r) {
				return PAM_BUF_ERR;
			}
			resp[i]->resp_retcode = 0;
			resp[i]->resp = rp.rep_str;

			helper_log(LOG_NOTICE, "%s OK rep_str '%s:%d'",
				__func__, rp.rep_str, rp.rep_len);

			/* NOTE: no free(rep_str) since PAM will free it */
		}
	}

	return PAM_SUCCESS;
}

int write_msg(int msg_len, const struct pam_message *msg)
{
	write(STDOUT_FILENO, &msg->msg_style, sizeof(msg->msg_style));
	write(STDOUT_FILENO, &msg_len, sizeof(msg_len));
	write(STDOUT_FILENO, msg->msg, msg_len);
	return 0;
}

static int conv_cb(int num_msg, const struct pam_message **msg,
	    struct pam_response **resp, void *user_data_ptr)
{
	unsigned msg_len;
	int i;

	helper_log(LOG_NOTICE, "%s num_msg: %i, user_data_ptr %p\n", __func__,
		num_msg, user_data_ptr);

	write(STDOUT_FILENO, &num_msg, sizeof(num_msg));
	for (i = 0; i < num_msg; i++) {
		msg_len = strlen(msg[i]->msg);
		helper_log(LOG_NOTICE, "%s %i %s '%s:%d'\n", __func__,
			i, msg_type_str(msg[i]->msg_style),
			msg[i]->msg, msg_len);
		write_msg(msg_len, msg[i]);
	}

	return conv_handle_resp(resp);
}

static int do_auth(pam_handle_t *pamh)
{
	int r;

	r = pam_authenticate(pamh, 0);

	if (r != PAM_SUCCESS) {
		helper_log(LOG_NOTICE, "%s pam_authenticate() r %i: %s",
			__func__, r, pam_strerror(pamh, r));
		return r;
	}

	r = pam_acct_mgmt(pamh, 0);
	helper_log(LOG_NOTICE, "%s pam_acct_mgmt() ret %i", __func__, r);

	return r;
}

static int do_chtok(pam_handle_t *pamh, unsigned flags)
{
	int r = pam_chauthtok(pamh, flags);
	helper_log(LOG_NOTICE, "%s pam_chauthtok() r %i", __func__, r);

	if (r != PAM_SUCCESS) {
		helper_log(LOG_NOTICE, "%s pam_chauthtok() r %i: %s",
			__func__, r, pam_strerror(pamh, r));
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
		helper_log(LOG_NOTICE, "%s pam_start() r %i: %s",
			__func__, r, pam_strerror(pamh, r));
		return r;
	}

	switch (req_type) {
	case REQ_AUTH:
		r = do_auth(pamh);
		if (r == PAM_NEW_AUTHTOK_REQD) {
			r = do_chtok(pamh, PAM_CHANGE_EXPIRED_AUTHTOK);
		}
		break;
	case REQ_CHTOK:
	 	r = do_chtok(pamh, 0);
		break;
	}

	pam_end(pamh, r);

	/* send a message to parent that conversation is over */
	int t = 0;
	write(STDOUT_FILENO, &t, sizeof(t));

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

	helper_log(LOG_NOTICE, "uid %i, euid %i, user '%s'",
		getuid(), geteuid(), pw->pw_name);

	if (!strcmp(argv[1], "auth")) {
		r = do_pam(pw->pw_name, REQ_AUTH);
		helper_log(LOG_NOTICE, "%sAuthenticated.",
			r == PAM_SUCCESS ? "" : "Not ");
	} else if (!strcmp(argv[1], "chtok")) {
		r = do_pam(pw->pw_name, REQ_CHTOK);
		helper_log(LOG_NOTICE, "%sChanged auth token.",
			r == PAM_SUCCESS ? "Successfully " : "Not ");
	} else {
		r = PAM_AUTH_ERR;
		helper_log(LOG_NOTICE, "Unknown request: %s", argv[2]);
	}

	return r == PAM_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;
}

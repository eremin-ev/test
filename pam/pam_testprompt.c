/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * See https://github.com/lnussel/pam_testprompt
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

#include <security/pam_modules.h>

static int pam_converse(pam_handle_t *pamh, char **pincode)
{
	struct pam_message msg[1];
	const struct pam_message *pmsg[1];
	struct pam_response *resp;
	const struct pam_conv *conv;
	int ret;
	char *tmp;

	syslog(LOG_WARNING, "%s uid %d, gid %d, euid %d", __func__, getuid(), getgid(), geteuid());

	ret = pam_get_item(pamh, PAM_USER, (const void**)(char*)&tmp);
	if (ret == PAM_SUCCESS) {
		syslog(LOG_WARNING, "%s user %s", __func__, tmp);
	}

	ret = pam_get_item(pamh, PAM_TTY, (const void**)(char*)&tmp);
	if (ret == PAM_SUCCESS) {
		syslog(LOG_WARNING, "%s tty %s", __func__, tmp);
	}

	ret = pam_get_item(pamh, PAM_CONV, (const void**)(char*)&conv);
	if (ret != PAM_SUCCESS) {
		syslog(LOG_WARNING, "%s %d no conversation function: %s",
				__func__, getuid(), pam_strerror(pamh, ret));
		return ret;
	}

	/* set up conversation call */

	pmsg[0] = &msg[0];
	msg[0].msg_style = PAM_PROMPT_ECHO_OFF;
	msg[0].msg = "TestPIN: ";
	resp = NULL;

	ret = conv->conv(1, pmsg, &resp, conv->appdata_ptr);

	if (ret != PAM_SUCCESS) {
		return ret;
	}

	if (resp) {
		if (resp[0].resp == NULL) {
			syslog(LOG_WARNING, "%s pam_sm_authenticate: NULL authtok given", __func__);
		}

		tmp = resp[0].resp;          /* remember this! */

		resp[0].resp = NULL;
	} else {
		syslog(LOG_WARNING, "pam_sm_authenticate: no error reported but NULL returned");

		return PAM_CONV_ERR;
	}
	free(resp);

	*pincode = tmp;

	return ret;
}

static int parse_args(const char *type, int argc, const char **argv)
{
	int i;
	int ret = PAM_SUCCESS;

	for (i = 0; i < argc; ++i) {
		syslog(LOG_WARNING, "%s option %s", __func__, argv[i]);
	}

	return ret;
}

int pam_sm_authenticate(pam_handle_t * pamh, int flags,int argc, const char **argv)
{
	int ret;
	char *pincode;

	parse_args("auth", argc, argv);

	ret = pam_converse(pamh, &pincode);

	syslog(LOG_WARNING, "%s uid:%d euid:%d, pincode '%s'",
		__func__, getuid(), geteuid(), pincode);

	return ret;
}

int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
	syslog(LOG_WARNING, "%s uid:%d euid:%d", __func__, getuid(), geteuid());

	int ret = parse_args(NULL, argc, argv);

	return ret;
}

int pam_sm_open_session(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
	int ret;
	char *pincode;

	syslog(LOG_WARNING, "%s uid:%d euid:%d", __func__, getuid(), geteuid());

	parse_args("session", argc, argv);

	ret = pam_converse(pamh, &pincode);

	return ret;
}

int pam_sm_close_session(pam_handle_t * pamh, int flags, int argc, const char **argv)
{
	syslog(LOG_WARNING, "%s uid:%d euid:%d", __func__, getuid(), geteuid());

	int ret = parse_args("session", argc, argv);

	return ret;
}

int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
	syslog(LOG_WARNING, "%s", __func__);

	int ret = parse_args("account", argc, argv);

	return ret;
}

int pam_sm_chauthtok(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
	syslog(LOG_WARNING, "%s uid:%d euid%d", __func__, getuid(), geteuid());

	parse_args("chauthtok", argc, argv);

	return PAM_SUCCESS;
}

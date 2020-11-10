/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <security/pam_appl.h>

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

int conv_cb(int num_msg, const struct pam_message **msg,
	    struct pam_response **resp, void *user_data_ptr)
{
	const int passwd_len = 128;
	char *passwd_str;
	int r;

	printf("%s num_msg: %i, user_data_ptr %p\n", __func__,
		num_msg, user_data_ptr);

	/* pam_conv */
	if (num_msg == 1) {
		printf("%s %s '%s'\n", __func__,
			msg_type_str((*msg)->msg_style), (*msg)->msg);
		/* Handle password passing */
		if (!strcmp((*msg)->msg, "Password: ")) {
			*resp = malloc(num_msg * sizeof(struct pam_response));
			passwd_str = malloc(passwd_len);
			r = read(0, passwd_str, passwd_len);
			if (r == -1) {
				printf("%s: %s\n", __func__, strerror(errno));
				exit(EXIT_FAILURE);
			}
			passwd_str[r] = '\0';
			printf("%s r %i, passwd '%s'\n", __func__, r, passwd_str);
			(*resp)->resp_retcode = 0;
			(*resp)->resp = passwd_str;

			/* NOTE: no free(passwd_str) since PAM will free it */
		}
	}

	return PAM_SUCCESS;
}

int do_auth(const char *user)
{
	struct pam_conv pam_conv = { conv_cb, NULL };
	pam_handle_t *pamh;

	int ret = pam_start("login", user, &pam_conv, &pamh);

	if (ret == PAM_SUCCESS) {
		ret = pam_authenticate(pamh, 0);
	}

	if (ret == PAM_SUCCESS) {
		ret = pam_acct_mgmt(pamh, 0);
	}

	pam_end(pamh, ret);

	return ret;
}

int main(int argc, char **argv)
{
	int ret;

	if (argc < 2) {
		printf(	"Usage: %s <username>\n"
			"(please enter <password>^D afterwards)\n",
			argv[0]);
		return EXIT_FAILURE;
	}

	ret = do_auth(argv[1]);

	printf("%sAuthenticated.\n", ret == PAM_SUCCESS ? "" : "Not ");

	return ret == PAM_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;
}

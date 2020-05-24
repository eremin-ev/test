/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

//
// To build:
//
//      gcc pam_password.c -lpam
//
// To get PAM development library:
//
//      sudo apt-get install libpam-dev
//
//

#include <security/pam_appl.h>
//#include <security/pam_misc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int conv_cb(int num_msg, const struct pam_message **msg,
	    struct pam_response **resp, void *user_data_ptr)
{
	const char *passwd = user_data_ptr;

	printf("%s num_msg: %i\n", __func__, num_msg);

	/* pam_conv */
	if (num_msg == 1) {
		printf("%s %s\n", __func__, (*msg)->msg);
		*resp = malloc(1 * sizeof(struct pam_response));
		(*resp)->resp_retcode = 0;
		(*resp)->resp = strdup(passwd);
	}

	return PAM_SUCCESS;
}

int main(int argc, char** argv)
{
	if (argc < 3) {
		fprintf(stderr,
			"Usage: %s <username> <password>\n"
			"(use a space before command to avoid storing "
				"<password> to the Bash history)\n",
			argv[0]);
		return 1;
	}

	const char *user = argv[1];
	const char *passwd = argv[2];
	struct pam_conv pam_conv = { conv_cb, (void *)passwd };
	//struct pam_conv pam_conv = { misc_conv, NULL };
	pam_handle_t *pamh;

	int res = pam_start("login", user, &pam_conv, &pamh);

	if (res == PAM_SUCCESS) {
		res = pam_authenticate(pamh, 0);
	}

	if (res == PAM_SUCCESS) {
		res = pam_acct_mgmt(pamh, 0);
	}

	if (res == PAM_SUCCESS) {
		fprintf(stdout, "Authenticated.\n");
	}
	else {
		fprintf(stdout, "Not Authenticated.\n");
	}

	pam_end(pamh, res);

	return res == PAM_SUCCESS ? 0 : 1;
}

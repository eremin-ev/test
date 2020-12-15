
#pragma once

enum cmd_type {
	CMD_RET = 0,
	CMD_CONV,
};

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

struct pam_msg_rq {
	int msg_style;
	int msg_len;
	int rq_flags;
	char *msg;
};

struct pam_msg_rp {
	int rep_len;
	char *rep_str;
};


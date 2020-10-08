/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "keyval.h"

#define ARRAY_SIZE(a)	((long) sizeof(a)/sizeof(a[0]))

struct event_syscall {
	int pid;
	int ppid;
	int uid;
	int gid;
	int euid;
	int egid;
	int exit;
	int syscall;
	char *a0_str;
	int a0_len;
	char *exe_str;
	int exe_len;
	char *comm_str;
	int comm_len;
	char *key_str;
	int key_len;
};

struct event_path {
	int inode;
	char *name_str;
	int name_len;
};

struct event_cwd {
	char *cwd_str;
	int cwd_len;
};

struct event_arb1 {
	char *key1_str;
	int key1_len;
	char *key2_str;
	int key2_len;
	char *key3_str;
	int key3_len;
};

struct event_arb2 {
	char *key4_str;
	int key4_len;
	char *key5_str;
	int key5_len;
	char *key6_str;
	int key6_len;
	char *key7_str;
	int key7_len;
};

struct event_arb3 {
	char *key9_str;
	int key9_len;
};

struct event_arb4 {
	char *key10_str;
	int key10_len;
	char *key11_str;
	int key11_len;
	char *key12_str;
	int key12_len;
};

struct event {
	struct event_syscall syscall;
	struct event_cwd cwd;
	struct event_path path;
	struct event_arb1 arb1;
	struct event_arb2 arb2;
	struct event_arb3 arb3;
	struct event_arb4 arb4;
};

const struct keyval input[] = {
	{
		/* SYSCALL */
		.k_str = "arch=abcd0012 syscall=11 per=800000 success=yes exit=0 "
			 "a0=1f69518 a1=1f85280 a2=1f81330 a3=9aaaf500 "
			 "items=2 ppid=7003 pid=7985 auid=100000 uid=0 "
			 "gid=0 euid=0 suid=0 fsuid=0 egid=0 sgid=0 fsgid=0 "
			 "ses=4 tty=pts2 comm=\"cal\" exe=\"/usr/bin/cal\" "
			 "key=(null)",
		.k_len = 250,
	}, {
		/* PATH */
		.k_str = "item=0 name=\"/usr/bin/cal\" inode=16458 dev=fe:00 "
			 "mode=0100755 ouid=0 ogid=0 rdev=00:00 "
			 "nametype=NORMAL",
		.k_len = 102,
	}, {
		/* CWD */
		.k_str = "cwd=\"/home/user/Downloads\"xxxx",
		.k_len = 26,	/* actual length is 30 */
	}, {
		/* CWD length k_len exceeds actual k_str length, rely on '\0' */
		.k_str = "cwd=\"/home/user/Downloads\"xxxx",
		.k_len = 31,	/* actual length is 30 */
	}, {
		/* Arbitrary */
		.k_str = "  key1 = 33 key2 = \"333\" key3 = \"\" xxxx",
		.k_len = 39,
	}, {
		/* A message ends with key2 value */
		.k_str = "  key4 = 33 key5 = \"333 key6 = yyyy\" "
			 "key7 = \t\"55555 555 55555\" key8 = some_value5",
		.k_len = 81,
	}, {
		/* No closing quote '"' character, key10 will be dropped */
		.k_str = "  key9 = 33 key10 = \"333",
		.k_len = 24,
	}, {
		/* Closing quote '"' character after max length */
		.k_str = "   key10 = 0112358  key11 = 33 key12 = \"3333 333\"xxxx",
		.k_len = 35,
	},
};

const struct keyval expect[] = {
	/* SYSCALL */
	{ .k_str = "arch", .k_len = 4, .v_str = "abcd0012", .v_len = 8, },
	{ .k_str = "syscall", .k_len = 7, .v_str = "11", .v_len = 2, },
	{ .k_str = "per", .k_len = 3, .v_str = "800000", .v_len = 6, },
	{ .k_str = "success", .k_len = 7, .v_str = "yes", .v_len = 3, },
	{ .k_str = "exit", .k_len = 4, .v_str = "0", .v_len = 1, },
	{ .k_str = "a0", .k_len = 2, .v_str = "1f69518", .v_len = 7, },
	{ .k_str = "a1", .k_len = 2, .v_str = "1f85280", .v_len = 7, },
	{ .k_str = "a2", .k_len = 2, .v_str = "1f81330", .v_len = 7, },
	{ .k_str = "a3", .k_len = 2, .v_str = "9aaaf500", .v_len = 8, },
	{ .k_str = "items", .k_len = 5, .v_str = "2", .v_len = 1, },
	{ .k_str = "ppid", .k_len = 4, .v_str = "7003", .v_len = 4, },
	{ .k_str = "pid", .k_len = 3, .v_str = "7985", .v_len = 4, },
	{ .k_str = "auid", .k_len = 4, .v_str = "100000", .v_len = 6, },
	{ .k_str = "uid", .k_len = 3, .v_str = "0", .v_len = 1, },
	{ .k_str = "gid", .k_len = 3, .v_str = "0", .v_len = 1, },
	{ .k_str = "euid", .k_len = 4, .v_str = "0", .v_len = 1, },
	{ .k_str = "suid", .k_len = 4, .v_str = "0", .v_len = 1, },
	{ .k_str = "fsuid", .k_len = 5, .v_str = "0", .v_len = 1, },
	{ .k_str = "egid", .k_len = 4, .v_str = "0", .v_len = 1, },
	{ .k_str = "sgid", .k_len = 4, .v_str = "0", .v_len = 1, },
	{ .k_str = "fsgid", .k_len = 5, .v_str = "0", .v_len = 1, },
	{ .k_str = "ses", .k_len = 3, .v_str = "4", .v_len = 1, },
	{ .k_str = "tty", .k_len = 3, .v_str = "pts2", .v_len = 4, },
	{ .k_str = "comm", .k_len = 4, .v_str = "cal", .v_len = 3, },
	{ .k_str = "exe", .k_len = 3, .v_str = "/usr/bin/cal", .v_len = 12, },
	{ .k_str = "key", .k_len = 3, .v_str = "(null)", .v_len = 6, },

	/* PATH */
	{ .k_str = "item", .k_len = 4, .v_str = "0", .v_len = 1, },
	{ .k_str = "name", .k_len = 4, .v_str = "/usr/bin/cal", .v_len = 12, },
	{ .k_str = "inode", .k_len = 5, .v_str = "16458", .v_len = 5, },
	{ .k_str = "dev", .k_len = 3, .v_str = "fe:00", .v_len = 5, },
	{ .k_str = "mode", .k_len = 4, .v_str = "0100755", .v_len = 7, },
	{ .k_str = "ouid", .k_len = 4, .v_str = "0", .v_len = 1, },
	{ .k_str = "ogid", .k_len = 4, .v_str = "0", .v_len = 1, },
	{ .k_str = "rdev", .k_len = 4, .v_str = "00:00", .v_len = 5, },
	{ .k_str = "nametype", .k_len = 8, .v_str = "NORMAL", .v_len = 6, },

	/* CWD */
	{ .k_str = "cwd", .k_len = 3, .v_str = "/home/user/Downloads", .v_len = 20, },

	/* CWD length k_len exceeds actual k_str length, rely on '\0' */
	{ .k_str = "cwd", .k_len = 3, .v_str = "/home/user/Downloads", .v_len = 20, },

	/* Arbitrary arb1 */
	{ .k_str = "key1", .k_len = 4, .v_str = "33", .v_len = 2, },
	{ .k_str = "key2", .k_len = 4, .v_str = "333", .v_len = 3, },
	{ .k_str = "key3", .k_len = 4, .v_str = "", .v_len = 0, },

	/* A message ends with key2 value arb2 */
	{ .k_str = "key4", .k_len = 4, .v_str = "33", .v_len = 2, },
	{ .k_str = "key5", .k_len = 4, .v_str = "333 key6 = yyyy", .v_len = 15, },
	{ .k_str = "key7", .k_len = 4, .v_str = "55555 555 55555", .v_len = 15, },
	{ .k_str = "key8", .k_len = 4, .v_str = "some_value5", .v_len = 11, },

	/* No closing quote '"' character arb3 */
	{ .k_str = "key9", .k_len = 4, .v_str = "33", .v_len = 2, },

	/* Closing quote '"' character after max length arb4 */
	{ .k_str = "key10", .k_len = 5, .v_str = "0112358", .v_len = 7, },
	{ .k_str = "key11", .k_len = 5, .v_str = "33", .v_len = 2, },
	{ .k_str = "key12", .k_len = 5, .v_str = "3333 333", .v_len = 8, },
};

static int kv_valid(const struct keyval *kv)
{
	return kv->k_str && kv->k_len && kv->v_str && kv->v_len;
}

static int kv_cmp(const struct keyval *kv1, const struct keyval *kv2)
{
	return !(kv1->k_len == kv2->k_len) +
		!(kv1->v_len == kv2->v_len) +
		abs(strncmp(kv1->k_str, kv2->k_str, kv2->k_len)) +
		abs(strncmp(kv1->v_str, kv2->v_str, kv2->v_len));
}

static int
kv_set_str(const struct keyval *kv, const char *key, char **p_str, int *p_len)
{
	if (!strncmp(kv->k_str, key, kv->k_len)) {
		*p_str = kv->v_str;
		*p_len = kv->v_len;
		return 1;
	}
	return 0;
}

static int
kv_set_int(const struct keyval *kv, const char *key, int *p_val)
{
	char *ss;
	if (!strncmp(kv->k_str, key, kv->k_len)) {
		*p_val = strtol(kv->v_str, &ss, 10);
		if (kv->v_str != ss)
			return 1;
	}
	return 0;
}

static int upd_key(const struct keyval *kv, struct event *e)
{
	if (kv_valid(kv)) {
		switch (*kv->k_str) {
		case 'a':
			if (kv_set_str(kv, "a0", &e->syscall.a0_str,
					         &e->syscall.a0_len))
				return 1;
			break;
		case 'c':
			if (kv_set_str(kv, "comm", &e->syscall.comm_str,
						   &e->syscall.comm_len))
				return 1;
			else if (kv_set_str(kv, "cwd", &e->cwd.cwd_str,
						       &e->cwd.cwd_len))
				return 1;
			break;
		case 'e':
			if (kv_set_int(kv, "egid", &e->syscall.egid))
				return 1;
			else if (kv_set_int(kv, "euid", &e->syscall.euid))
				return 1;
			else if (kv_set_str(kv, "exe", &e->syscall.exe_str,
						       &e->syscall.exe_len))
				return 1;
			else if (kv_set_int(kv, "exit", &e->syscall.exit))
				return 1;
			break;
		case 'g':
			if (kv_set_int(kv, "gid", &e->syscall.gid))
				return 1;
			break;
		case 'i':
			if (kv_set_int(kv, "inode", &e->path.inode))
				return 1;
			break;
		case 'k':
			if (kv_set_str(kv, "key10", &e->arb4.key10_str,
						    &e->arb4.key10_len))
				return 1;
			else if (kv_set_str(kv, "key11", &e->arb4.key11_str,
							 &e->arb4.key11_len))
				return 1;
			else if (kv_set_str(kv, "key12", &e->arb4.key12_str,
							 &e->arb4.key12_len))
				return 1;
			else if (kv_set_str(kv, "key1", &e->arb1.key1_str,
							&e->arb1.key1_len))
				return 1;
			else if (kv_set_str(kv, "key2", &e->arb1.key2_str,
							&e->arb1.key2_len))
				return 1;
			else if (kv_set_str(kv, "key3", &e->arb1.key3_str,
							&e->arb1.key3_len))
				return 1;
			else if (kv_set_str(kv, "key4", &e->arb2.key4_str,
							&e->arb2.key4_len))
				return 1;
			else if (kv_set_str(kv, "key5", &e->arb2.key5_str,
							&e->arb2.key5_len))
				return 1;
			else if (kv_set_str(kv, "key6", &e->arb2.key6_str,
							&e->arb2.key6_len))
				return 1;
			else if (kv_set_str(kv, "key7", &e->arb2.key7_str,
							&e->arb2.key7_len))
				return 1;
			else if (kv_set_str(kv, "key9", &e->arb3.key9_str,
							&e->arb3.key9_len))
				return 1;
			else if (kv_set_str(kv, "key", &e->syscall.key_str,
						       &e->syscall.key_len))
				return 1;
			break;
		case 'n':
			if (kv_set_str(kv, "name", &e->path.name_str,
						   &e->path.name_len))
				return 1;
			break;
		case 'p':
			if (kv_set_int(kv, "pid", &e->syscall.pid))
				return 1;
			else if (kv_set_int(kv, "ppid", &e->syscall.ppid))
				return 1;
			break;
		case 's':
			if (kv_set_int(kv, "syscall", &e->syscall.syscall))
				return 1;
			break;
		case 'u':
			if (kv_set_int(kv, "uid", &e->syscall.uid))
				return 1;
			break;
		}
	}

	return 0;
}

static int key_check(const struct keyval *kv1, const struct keyval *kv2)
{
	int c;
	int r = 0;

	c = kv_cmp(kv1, kv2);

	printf("\t%s\t'%.*s':%i = '%.*s':%i",
		c == 0 ? "OK" : "--",
		kv1->k_len, kv1->k_str, kv1->k_len,
		kv1->v_len, kv1->v_str, kv1->v_len);

	if (c) {
		printf(	"\n"
			"\t\t\tkey '%.*s':%i '%s':%i\n"
			"\t\t\tvalue '%.*s':%i '%s':%i\n",
			kv1->k_len, kv1->k_str, kv1->k_len,
			kv2->k_str, kv2->k_len,
			kv1->v_len, kv1->v_str, kv1->v_len,
			kv2->v_str, kv2->v_len);
	} else {
		r = 1;
		printf("\n");
	}

	return r;
}

static void e_dump(struct event *e)
{
	printf("%s\n"
	       "\tpid %i, ppid %i, uid %i, gid %i, euid %i, egid %i\n"
	       "\texit %i (%s), syscall %i, key '%s:%i'\n"
	       "\texe '%s:%i', comm '%s:%i', a0 '%s:%i', cwd '%s:%i'\n",
		__func__,
		e->syscall.pid,
		e->syscall.ppid,
		e->syscall.uid,
		e->syscall.gid,
		e->syscall.euid,
		e->syscall.egid,
		e->syscall.exit,
		strerror(-e->syscall.exit),
		e->syscall.syscall,
		e->syscall.key_str,
		e->syscall.key_len,
		e->syscall.exe_str,
		e->syscall.exe_len,
		e->syscall.comm_str,
		e->syscall.comm_len,
		e->syscall.a0_str,
		e->syscall.a0_len,
		e->cwd.cwd_str,
		e->cwd.cwd_len);
}

int main(void)
{
	struct keyval kv;
	struct event e;
	int s0_len;
	const char *s;
	const char *s0;
	unsigned i, j;
	int test_nr = 0;
	int test_ok = 0;

	for (i = 0, j = 0; i < ARRAY_SIZE(input); i++) {
		s0 = input[i].k_str;
		s0_len = input[i].k_len;

		memset(&e, 0, sizeof(struct event));

		printf("input[%i] '%s':%i\n", i, s0, s0_len);

		for (s = kv_next(&kv, s0, s0_len); kv_ok(&kv);
				s = kv_next(&kv, s, s0_len - (s - s0))) {

			if (!upd_key(&kv, &e)) {
				printf(	"Not found:\n"
					"\tkey '%.*s':%i\n"
					"\tvalue '%.*s':%i\n",
					kv.k_len, kv.k_str, kv.k_len,
					kv.v_len, kv.v_str, kv.v_len);
			}

			if (j < ARRAY_SIZE(expect)) {
				test_ok += key_check(&kv, &expect[j]);
				++j;
			} else {
				printf(	"}, {\n"
					"\t.k_str = \"%.*s\",\n"
					"\t.k_len = %i,\n"
					"\t.v_str = \"%.*s\",\n"
					"\t.v_len = %i,\n",
					kv.k_len, kv.k_str, kv.k_len,
					kv.v_len, kv.v_str, kv.v_len);
			}

			test_nr++;
		}

		e_dump(&e);
	}

	printf("Total: %i, ok %i\n", test_nr, test_ok);

	return 0;
}

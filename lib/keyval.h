/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#ifndef __KEYVAL_H
#define __KEYVAL_H

#include <stdlib.h>
#include <string.h>

enum keyval_flags {
	KV_OK		= 1 << 0,	/* kv_next() has next key-value */
	KV_STRING	= 1 << 1,
};

struct keyval {
	char *k_str;
	char *v_str;
	int k_len;
	int v_len;
	unsigned flags;
};

const char *kv_next(struct keyval *kv, const char *s_str, int s_len);
int kv_ok(const struct keyval *kv);

#endif	/* __KEYVAL_H */

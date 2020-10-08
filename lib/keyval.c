/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <stddef.h>

#include "keyval.h"

static void kv_init(struct keyval *kv)
{
	kv->flags = 0;
	kv->k_str = NULL;
	kv->k_len = 0;
	kv->v_str = NULL;
	kv->v_len = 0;
}

int kv_ok(const struct keyval *kv)
{
	return kv->flags & KV_OK;
}

static const char *kv_skip_space(const char *s, const char *s0, int s_len)
{
	while (s - s0 < s_len) {
		switch (*s) {
		case '\t':
		case ' ':
			s++;
			break;
		default:
			return s;
		}
	}

	return s;
}

static const char *kv_eat_to_eq(const char *s, const char *s0, int s_len)
{
	while (s - s0 < s_len) {
		switch (*s) {
		case '=':
		case ' ':
		case '\0':
			return s;
		default:
			s++;
			break;
		}
	}

	return s;
}

static const char *kv_eat_to_space(const char *s, const char *s0, int s_len)
{
	while (s - s0 < s_len) {
		switch (*s) {
		case '\t':
		case ' ':
			return s;;
		default:
			s++;
			break;
		}
	}

	return s;
}

static const char *kv_eat_to_quote(const char *s, const char *s0, int s_len)
{
	while (s - s0 < s_len) {
		switch (*s) {
		case '"':
		case '\0':
			return s;
		default:
			s++;
			break;
		}
	}

	return s;
}

//static const char *kv_left(const char *s, const char *s_str, int s_len)
//{
//}

/*
 * Get a next key-value pair which separated with a whitespace (' ')
 *
 * TODO: this should be rewritten using higher-level functions
 *       like strtok(), strchr(), sscanf() etc.
 */
const char *kv_next(struct keyval *kv, const char *s_str, int s_len)
{
	const char *s = s_str;

	kv_init(kv);

	//printf("%s\ts_str '%.*s':%i\n", __func__, s_len, s_str, s_len);

	s = kv_skip_space(s, s_str, s_len);

	kv->k_str = (char *) s;

	s = kv_eat_to_eq(s, s_str, s_len);

	kv->k_len = s - kv->k_str;

	s = kv_skip_space(s, s_str, s_len);

	if (*s && kv->k_len && *s == '=' && s - s_str < s_len) {
		s++;

		s = kv_skip_space(s, s_str, s_len);

		if (*s && *s == '"') {
			s++;
			kv->flags |= KV_STRING;
			kv->v_str = (char *) s;
			s = kv_eat_to_quote(s, s_str, s_len);
			kv->v_len = s - kv->v_str;

			if (*s == '"' && s - s_str < s_len) {
				kv->flags |= KV_OK;
				s++;
			}
		} else {
			kv->flags &= ~KV_STRING;
			kv->v_str = (char *) s;
			s = kv_eat_to_space(s, s_str, s_len);
			kv->v_len = s - kv->v_str;
			if (kv->v_len) {
				kv->flags |= KV_OK;
			}
		}
	}

	//printf("%s\t'%.*s':%i = '%.*s':%i flags 0x%x\n", __func__,
	//	kv->k_len, kv->k_str, kv->k_len,
	//	kv->v_len, kv->v_str, kv->v_len, kv->flags);

	return s;
}

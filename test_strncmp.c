/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_SIZE(a)	(sizeof(a) / sizeof(a[0]))

#define def_str(x)	{ .s_str = x, .s_len = sizeof(x) - 1, }

struct str {
	char *s_str;
	unsigned s_len;
};

int some_long_check(const char *a, const char *b, unsigned len)
{
	printf("Ok, let's check for '%s:%d'\n", b, len);

	return strncmp(a, b, len) == 0;
}

void test_condition_before(const struct str *names, unsigned names_nr,
			   const struct str *targets, unsigned targets_nr,
			   const char *special_str, unsigned special_len)
{
	int r;
	unsigned i;

	for (i = 0; i < names_nr && i < targets_nr; i++) {
		r = !!strncmp(names[i].s_str, targets[i].s_str, targets[i].s_len);

		printf("names[%d] '%s', target_str '%s:%d' r %d\n",
			i, names[i].s_str, targets[i].s_str, targets[i].s_len, r);

		// r == 0: match
		// r != 0: mismatch
		if (r && some_long_check(names[i].s_str, special_str, special_len)) {
			printf("Ok, let's check suffix\n");

			r &= !!strncmp("three_a", targets[i].s_str, targets[i].s_len);
			r &= !!strncmp("three_b", targets[i].s_str, targets[i].s_len);
		}

		printf("names[%d] '%s' r %d: %s\n",
			i, names[i].s_str, r, r == 0 ? "OK" : "Failed");
	}
}

/*
 * The only difference against the previous is
 *
 *   if (some_long_check() && r) ...
 *
 * condition
 */
void test_condition_after(const struct str *names, unsigned names_nr,
			  const struct str *targets, unsigned targets_nr,
			  const char *special_str, unsigned special_len)
{
	int r;
	unsigned i;

	for (i = 0; i < names_nr && i < targets_nr; i++) {
		r = !!strncmp(names[i].s_str, targets[i].s_str, targets[i].s_len);

		printf("names[%d] '%s', target_str '%s:%d' r %d\n",
			i, names[i].s_str, targets[i].s_str, targets[i].s_len, r);

		// r == 0: match
		// r != 0: mismatch
		if (some_long_check(names[i].s_str, special_str, special_len) && r) {
			printf("Ok, let's check suffix\n");

			r &= !!strncmp("three_a", targets[i].s_str, targets[i].s_len);
			r &= !!strncmp("three_b", targets[i].s_str, targets[i].s_len);
		}

		printf("names[%d] '%s' r %d: %s\n",
			i, names[i].s_str, r, r == 0 ? "OK" : "Failed");
	}
}

void test_strncmp_return_value()
{
	int r;
	unsigned i;

	char buf1[64];
	char buf2[64] = "a";

	for (i = 0; i < 26; i++) {
		buf1[0] = i + 'a';
		buf1[1] = '\0';
		r = strncmp(buf1, buf2, 2);
		printf("%s %s %x\n", buf1, buf2, r);
	}

	printf("test %x\n", strncmp("b", "a", 1));
	printf("test %x\n", strncmp("c", "a", 1));
	printf("test %x\n", strncmp("f", "a", 1));

	r = strncmp("b", "a", 1);
	printf("r %x\n", r);

	r &= strncmp("e", "a", 1);
	printf("r %x\n", r);
}

int main()
{
	const struct str names[] = {
		def_str("one"),
		def_str("two"),
		def_str("three"),
		def_str("four"),
	};

	/* search targets, match names */
	const struct str targets[] = {
		def_str("one"),
		def_str("two"),
		def_str("three_e"),
		def_str("four"),
	};

	/* special case */
	const char *special_str = "three";
	const unsigned special_len = sizeof("three") - 1;

	printf("Test Condition Before...\n");
	test_condition_before(names, ARRAY_SIZE(names),
			      targets, ARRAY_SIZE(targets),
			      special_str, special_len);

	printf("Test Condition After...\n");
	test_condition_after(names, ARRAY_SIZE(names),
			     targets, ARRAY_SIZE(targets),
			     special_str, special_len);

	test_strncmp_return_value();

	return EXIT_SUCCESS;
}

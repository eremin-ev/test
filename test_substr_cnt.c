/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <stdio.h>

const unsigned C1 = 0xb6b6b6b6;
const unsigned C2 = 0xb7b7b7b7;

static int srch(const char *s, const char *h)
{
	const char *h0 = h;
	int nr = 0;

	while (*s) {
		if (*s == *h) {
			h++;
		}
		if (*h == '\0') {
			h = h0;
			nr++;
		}
		s++;
	}

	return nr;
}

static int test_substr_cnt(unsigned c1, unsigned c2)
{
	char buf1[65] = {0};
	const int c_len = 2*sizeof(unsigned) + 1;
	char c_buf[c_len];
	int r;
	char *o;
	/* number of unsigned elements in 'buf1' buffer */
	int nr = (sizeof(buf1) - 1) / (2 * sizeof(unsigned));
	char *ok;

	o = buf1;

	while (o - buf1 < sizeof(buf1)) {
		o += snprintf(o, sizeof(buf1) - (o - buf1), "%08x", c1);
	}

	snprintf(c_buf, c_len, "%08x", c2);

	r = srch(buf1, c_buf);

	if (c1 == c2)
		if (nr == r)
			ok = "OK";
		else
			ok = "Failed";
	else
		if (nr != r)
			ok = "OK";
		else
			ok = "Failed";

	printf("%s string full of %x, substring '%x', found %i, %s\n",
		__func__, c1, c2, r, ok);

	return r;
}

int main(void)
{
	test_substr_cnt(C1, C2);

	test_substr_cnt(C1, C1);

	test_substr_cnt(C2, C1);

	test_substr_cnt(C2, C2);

	return 0;
}

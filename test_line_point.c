/*
 * A test to determine which side of a line AB a point P falls on.  See [1]
 * for the details.
 *
 * [1] https://math.stackexchange.com/questions/274712/calculate-on-which-side-of-a-straight-line-is-a-given-point-located
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <stdio.h>
#include <math.h>

#if 0
typedef float real_t;
#define REAL_T GL_FLOAT
#else
typedef double real_t;
#define REAL_T GL_DOUBLE
#endif

#define ARRAY_SIZE(a)	(sizeof(a)/sizeof(a[0]))

struct v2 {
	real_t x;
	real_t y;
};

static real_t vec_len(struct v2 *A, struct v2 *B)
{
	real_t dx = B->x - A->x;
	real_t dy = B->y - A->y;
	return sqrt(dx*dx + dy*dy);
}

static real_t test_line_point(struct v2 *A, struct v2 *B,
		struct v2 *P)
{
	return (P->x - A->x)*(B->y - A->y) - (P->y - A->y)*(B->x - A->x);
}

int main(void)
{
	unsigned i, j;
	real_t len, sign;

	struct v2 a[] = {
		{ .x = 1, .y = 1 },
	};

	struct v2 b[] = {
		{ .x = 5, .y = 3 },
	};

	struct v2 p[] = {
		{ .x = 1, .y = 2 },
		{ .x = 2, .y = 5 },
		{ .x = 4, .y = 5 },
		{ .x = 6, .y = 7 },
		{ .x = 1, .y = 1 },
		{ .x = 2, .y = 1 },
		{ .x = 3, .y = 1 },
		{ .x = 7, .y = 1 },
	};

	printf("size a, b %zi, size p %zi\n", ARRAY_SIZE(a), ARRAY_SIZE(p));

	if (ARRAY_SIZE(a) == ARRAY_SIZE(b)) {
		for (i = 0; i < ARRAY_SIZE(a); i++) {
			for (j = 0; j < ARRAY_SIZE(p); j++) {
				len = vec_len(&a[i], &b[i]);
				sign = test_line_point(&a[i], &b[i], &p[j]);
				printf("|(%g,%g)--(%g,%g)|=%g -> (%g,%g): %g\n",
					a[i].x, a[i].y,
					b[i].x, b[i].y,
					len,
					p[j].x, p[j].y,
					sign/len);
			}
		}
	}

	return 0;
}

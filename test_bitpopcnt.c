/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 */

/*
 * Bit population count subprograms.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static uint64_t rdtsc(void)
{
	unsigned int low, high;

	asm volatile("rdtsc" : "=a" (low), "=d" (high));

	return low | ((uint64_t) high) << 32;
}

static int popcnt_1(unsigned v)
{
	int cnt = 0;
	while (v) {
		cnt += v & 1;
		v >>= 1;
	}
	return cnt;
}

static int popcnt_2(unsigned v)
{
	int cnt = 0;
	while (v) {
		v = v & (v - 1);
		cnt++;
	}
	return cnt;
}

static int popcnt_3_x(int v)
{
	switch (v & 0xf) {
	case 0x0: return 0;
	case 0x1: return 1;
	case 0x2: return 1;
	case 0x3: return 2;
	case 0x4: return 1;
	case 0x5: return 2;
	case 0x6: return 2;
	case 0x7: return 3;
	case 0x8: return 1;
	case 0x9: return 2;
	case 0xa: return 2;
	case 0xb: return 3;
	case 0xc: return 2;
	case 0xd: return 3;
	case 0xe: return 3;
	case 0xf: return 4;
	}
	return 0;
}

static int popcnt_3(unsigned v)
{
	int cnt = 0;

	while (v) {
		cnt += popcnt_3_x(v & 0xf);
		v >>= 4;
	}

	return cnt;
}

/*
 * 0000 0x0
 * 0001 0x1
 * 0010 0x2
 * 0011 0x3
 * 0100 0x4
 * 0101 0x5
 * 0110 0x6
 * 0111 0x7
 * 1000 0x8
 * 1001 0x9
 * 1010 0xa
 * 1011 0xb
 * 1100 0xc
 * 1101 0xd
 * 1110 0xe
 * 1111 0xf
 *
 * The key idea for two-bit integer value v = xy is the following:
 *
 *	v & 2				// x0
 *	v & 1				// 0y
 *
 *	(v & 2) >> 1			// 0x
 *
 *	((v & 2) >> 1) + (v & 1)	// 0x + 0y
 *
 */
static int popcnt_4(unsigned v)
{
	v = ((v & 0xaaaaaaaa) >> 1) + (v & 0x55555555);  // 10101010 + 01010101
	v = ((v & 0xcccccccc) >> 2) + (v & 0x33333333);  // 11001100 + 00110011
	v = ((v & 0xf0f0f0f0) >> 4) + (v & 0x0f0f0f0f);  // 11110000 + 0000ffff
	v = ((v & 0xff00ff00) >> 8) + (v & 0x00ff00ff);  // ...
	v = ((v & 0xffff0000) >> 16) + (v & 0x0000ffff);

	return v;
}

static int popcnt_5(unsigned v)
{
	return __builtin_popcount(v);
}

int main()
{
	int i, n, N = 4000;
	int c_1, c_2, c_3, c_4, c_5;
	long t_1, t_2, t_3, t_4, t_5;

	/* warm-up */
	c_1 = c_2 = c_3 = c_4 = 0;

	srand(314);
	for (i = 0; i < N; i++) {
		c_1 += popcnt_2(rand());
	}
	srand(314);
	for (i = 0; i < N; i++) {
		c_2 += popcnt_2(rand());
	}
	srand(314);
	for (i = 0; i < N; i++) {
		c_3 += popcnt_3(rand());
	}
	srand(314);
	for (i = 0; i < N; i++) {
		c_4 += popcnt_4(rand());
	}
	srand(314);
	for (i = 0; i < N; i++) {
		c_5 += popcnt_5(rand());
	}

	printf("t_1\tt_2\tt_3\tt_4\tt_5\n");

	/* real timing */
	for (n = 1; n < N; n++) {
		srand(314);
		c_1 = 0;
		t_1 = rdtsc();
		for (i = 0; i < n; i++) {
			c_1 += popcnt_1(rand());
		}
		t_1 = rdtsc() - t_1;

		srand(314);
		c_2 = 0;
		t_2 = rdtsc();
		for (i = 0; i < n; i++) {
			c_2 += popcnt_2(rand());
		}
		t_2 = rdtsc() - t_2;

		srand(314);
		c_3 = 0;
		t_3 = rdtsc();
		for (i = 0; i < n; i++) {
			c_3 += popcnt_3(rand());
		}
		t_3 = rdtsc() - t_3;

		srand(314);
		c_4 = 0;
		t_4 = rdtsc();
		for (i = 0; i < n; i++) {
			c_4 += popcnt_4(rand());
		}
		t_4 = rdtsc() - t_4;

		srand(314);
		c_5 = 0;
		t_5 = rdtsc();
		for (i = 0; i < n; i++) {
			c_5 += popcnt_5(rand());
		}
		t_5 = rdtsc() - t_5;

		if (c_1 == c_2 && c_2 == c_3 && c_3 == c_4 && c_4 == c_5) {
			printf("%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
				(double) t_1/t_1,
				(double) t_2/t_1,
				(double) t_3/t_1,
				(double) t_4/t_1,
				(double) t_5/t_1);
		} else {
			printf("c_1 %i t_1 %lu\n", c_1, t_1);
			printf("c_2 %i t_2 %lu\n", c_2, t_2);
			printf("c_3 %i t_3 %lu\n", c_3, t_3);
			printf("c_4 %i t_4 %lu\n", c_4, t_4);
			printf("c_5 %i t_5 %lu\n", c_5, t_5);
			break;
		}
	}

	return 0;
}

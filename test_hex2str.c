/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

static double l_time(struct timespec *ts)
{
	clock_gettime(CLOCK_MONOTONIC, ts);
	return ts->tv_sec + 1e-9*ts->tv_nsec;
}

void gen_hash(char *h_bin, int h_size)
{
	int i, r;
	for (i = 0; i < h_size; i++) {
		do {
			r = rand()%255;
		} while (r == 0);
		h_bin[i] = 0xff & r;
	}
}

void hash_to_ascii(const char *h_bin, int h_size, char *h_ascii)
{
	int i;
	for (i = 0; i < h_size; i++) {
		sprintf(&h_ascii[2*i], "%02x", 0xff & h_bin[i]);
	}
}

char *hash_to_str(const char *h_bin, int h_size)
{
	static char hexd[] = "0123456789abcdef";
	char *h_str = malloc(2*h_size + 1);
	int i, b;
	for (i = 0; i < h_size; i++) {
		b = h_bin[i];
		h_str[2*i] =     hexd[0xf & (b >> 4)];
		h_str[2*i + 1] = hexd[0xf & b];
	}
	return h_str;
}

void hash_to_str_r(const char *h_bin, int h_size, char *h_str, int h_len)
{
	static char hexd[] = "0123456789abcdef";
	int i, b;
	char *o = h_str;

#if 1
	if (2*h_size < h_len) {
		for (i = 0; i < h_size; i++) {
			b = h_bin[i];
			*o++ = hexd[0xf & (b >> 4)];
			*o++ = hexd[0xf & b];
		}
	}
#else
	for (i = 0; i < h_size && o - h_str < h_len - 1; i++) {
		b = h_bin[i];
		*o++ = hexd[0xf & (b >> 4)];
		*o++ = hexd[0xf & b];
	}
#endif
}

enum {
	T_ASCII = 0,
	T_STR,
	T_STR_P,
	T_NR
};

int main(int argc, char **argv)
{
	int i, seed = 777, n = 0;

	const int h_size = 32;
	char h_bin[h_size];
	const int h_len = 2*h_size + 1;
	char h_ascii[h_len];
	char *h_ascii_p;

	struct timespec ts;
	double t[T_NR];

	if (argc > 1) {
		n = strtol(argv[1], 0, 10);
	}

	if (n < 1) {
		printf("Bad arg, usage: %s <n>\n", argv[0]);
		return 0;
	}

	gen_hash(h_bin, h_size);

	/* warm up */
	srand(seed);
	t[T_STR_P] = l_time(&ts);
	for (i = 0; i < n; i++) {
		hash_to_str_r(h_bin, h_size, h_ascii, h_len);
		if (n < 10) {
			printf("to_str_r %s\n", h_ascii);
		}
	}
	t[T_STR_P] = l_time(&ts) - t[T_STR_P];

	/* measurement */
	srand(seed);
	t[T_ASCII] = l_time(&ts);
	for (i = 0; i < n; i++) {
		hash_to_ascii(h_bin, h_size, h_ascii);
		if (n < 10) {
			printf("sprintf %s\n", h_ascii);
		}
	}
	t[T_ASCII] = l_time(&ts) - t[T_ASCII];

	srand(seed);
	t[T_STR] = l_time(&ts);
	for (i = 0; i < n; i++) {
		h_ascii_p = hash_to_str(h_bin, h_size);
		if (n < 10) {
			printf("to_str   %s\n", h_ascii_p);
		}
		free(h_ascii_p);
	}
	t[T_STR] = l_time(&ts) - t[T_STR];

	srand(seed);
	t[T_STR_P] = l_time(&ts);
	for (i = 0; i < n; i++) {
		hash_to_str_r(h_bin, h_size, h_ascii, h_len);
		if (n < 10) {
			printf("to_str_r %s\n", h_ascii);
		}
	}
	t[T_STR_P] = l_time(&ts) - t[T_STR_P];

	printf("t: %g\t%g\t%g\n", t[T_ASCII], t[T_STR], t[T_STR_P]);

	return 0;
}

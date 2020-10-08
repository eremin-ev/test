/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <stdio.h>

#define TEST_STRING_1	"A test string"
#define TEST_STRING_2	"abcd"
#define TEST_STRING_3	"123"

/*
 * The following
 *
 *	#define TEST_CHAR	'c'
 *
 * produces an error:
 *
 *	error: expected ‘)’ before 'c'
 *
 */

int foo_int(int a, int b)
{
	return a + b;
}

size_t foo_size_t(size_t a, size_t b)
{
	return a + b;
}

int main(void)
{
	printf("sizeof(\"" TEST_STRING_1 "\"): %zi\n", sizeof(TEST_STRING_1));

	printf("sizeof(\"" TEST_STRING_2 "\"): %zi\n", sizeof(TEST_STRING_2));

	printf("sizeof(\"" TEST_STRING_3 "\"): %zi\n", sizeof(TEST_STRING_3));

	//printf("sizeof('" TEST_CHAR "'): %zi\n", sizeof(TEST_CHAR));


	printf("sizeof(int)\t%zi\n", sizeof(int));
	printf("sizeof(long)\t%zi\n", sizeof(long));
	printf("sizeof(size_t)\t%zi\n", sizeof(size_t));

	/*
	 * Add the following line to inspect code generated for different
	 * machines for foo_int() and foo_size_t()
	 *
	 *	test_sizeof: CFLAGS += -save-temps
	 *
	 */

	printf("int\t%i\n", foo_int(337, 773));
	printf("size_t\t%zi\n", foo_size_t(337, 773));

	return 0;
}

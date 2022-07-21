/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

/*
 * Add the following line to inspect code generated for different
 * machines for foo_int() and foo_size_t()
 *
 *	test_sizeof: CFLAGS += -save-temps
 *
 */

#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdint.h>
#include <stdio.h>

#define TEST_STRING_1	"A test string"
#define TEST_STRING_2	"abcd"
#define TEST_STRING_3	"123"

const char string4[] = "12345";

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

void test_str(uint32_t value)
{
	//
	// The following:
	//
	//char b_str[sizeof(value)];
	//
	// gives a compile-time error:
	// test_sizeof.c: In function ‘test_str’:
	// test_sizeof.c:50:25: error: ‘%08x’ directive writing 8 bytes into a
	//                             region of size 4 [-Werror=format-overflow=]
        //   50 |         sprintf(b_str, "%08x", value);
        //      |                         ^~~~
        // test_sizeof.c:50:9: note: ‘sprintf’ output 9 bytes into a destination of size 4
	//   50 |         sprintf(b_str, "%08x", value);
	//      |         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//
	char b_str[2*sizeof(value) + 1];
	sprintf(b_str, "%08x", value);
	printf("%s '%s'\n", __func__, b_str);
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
	printf("sizeof(off_t)\t%zi\n", sizeof(off_t));
	printf("sizeof(off64_t)\t%zi\n", sizeof(off64_t));

	printf("int\t%i\n", foo_int(337, 773));
	printf("size_t\t%zi\n", foo_size_t(337, 773));

	printf("sizeof(%s) %zu\n", string4, sizeof(string4) - 1);

	test_str(737);

	return 0;
}

/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * See "C Operator Precedence" [1] and GeeksforGeeks article on
 * Difference between ++*p, *p++ and *++p
 *
 * [1] cppreference.com: C Operator Precedence
 *     https://en.cppreference.com/w/c/language/operator_precedence
 *
 * [2] GeeksforGeeks: Difference between ++*p, *p++ and *++p
 *     https://www.geeksforgeeks.org/difference-between-p-p-and-p/
 */

#include <stdio.h>

void dump(const int *A, const int *p, int v)
{
	printf("A[0] %d, A[1] %d, *p %d, v %d\n", A[0], A[1], *p, v);
}

void test1(void)
{
	int A[] = {10, 20};
	int *p = A;
	int v = ++*p;
	dump(A, p, v);
}

void test2(void)
{
	int A[] = {10, 20};
	int *p = A;
	int v = *p++;
	dump(A, p, v);
}

void test3(void)
{
	int A[] = {10, 20};
	int *p = A;
	int v = *++p;
	dump(A, p, v);
}

int main(void)
{
	test1();
	test2();
	test3();

	return 0;
}

/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

/*
 * See	https://lkml.org/lkml/2018/3/20/805
 *
 *	From: "Uecker, Martin" <>
 *	Subject: detecting integer constant expressions in macros
 *	Date: Tue, 20 Mar 2018 22:13:35 +0000
 *
 * #define ICE_P(x) (sizeof(int) == sizeof(*(1 ? ((void*)((x) * 0l)) : (int*)1)))
 *
 * The ICE_P macro turned into __is_constexpr() when patch was merged,
 * see include/linux/kernel.h for min(x,y)/max(x,y)/etc. implementation
 * which use it.
 *
 */

#include <stdio.h>

#define is_constexpr(x) (sizeof(int) == sizeof(*(1 ? ((void*)((x) * 0l)) : (int*)1)))

#define __is_constexpr(x) \
	(sizeof(int) == sizeof(*(8 ? ((void *)((long)(x) * 0l)) : (int *)8)))

int main(void)
{
	int x = 373;

	printf("is_constexpr\t%i\t%i\n", 337, is_constexpr(337));

	printf("is_constexpr\t%i\t%i\n", 773, is_constexpr(773));

	printf("is_constexpr\tsizeof(1)\t%i\n", is_constexpr(sizeof(1)));

	printf("is_constexpr\tsizeof(int)\t%i\n", is_constexpr(sizeof(int)));

	printf("is_constexpr\tsizeof(x)\t%i\n", is_constexpr(sizeof(x)));

	printf("is_constexpr\tx\t%i\n", is_constexpr(x));

	//printf("is_constexpr\tsizeof((void*)((1) * 0l))\t%zi\n", sizeof((void*)((1) * 0l)));
	//printf("is_constexpr\tsizeof((void*)((x) * 0l))\t%zi\n", sizeof((void*)((x) * 0l)));
	//printf("is_constexpr\tsizeof((void*)((x) * 0l))\t%zi\n", sizeof(*(1 ? NULL : (int*)1)));
	printf("is_constexpr\tsizeof(*(1 ? ((void*)((1) * 0l)) : (int*)1))\t%zi\n",
		sizeof(*(1 ? ((void*)((1) * 0l)) : (int*)1)));
	printf("is_constexpr\tsizeof(*(char*)1)\t%zi\n",
		sizeof(*(char *)1));
	printf("is_constexpr\tsizeof(*(void*)1)\t%zi\n",
		sizeof(*(void *)1));

	printf("is_constexpr\tsizeof(*(1 ? NULL : (int*)1))\t%zi\n",
		sizeof(*(1 ? NULL : (int*)1)));
	printf("is_constexpr\tsizeof(*(1 ? ((void*)((x) * 0l)) : (int*)1))\t%zi\n",
		sizeof(*(1 ? ((void*)((x) * 0l)) : (int*)1)));

	return 0;
}

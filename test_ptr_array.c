/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * From pam_conv(3):
 *
 *   In passing, it is worth noting that there is a discrepancy between the
 *   way Linux-PAM handles the const struct pam_message **msg conversation
 *   function argument and the way that Solaris' PAM (and derivatives, known
 *   to include HP/UX, are there others?) does. Linux-PAM interprets the msg
 *   argument as entirely equivalent to the following prototype const struct
 *   pam_message *msg[] (which, in spirit, is consistent with the commonly
 *   used prototypes for argv argument to the familiar main() function: char
 *   **argv; and char *argv[]). Said another way Linux-PAM interprets the
 *   msg argument as a pointer to an array of num_msg read only 'struct
 *   pam_message' pointers. Solaris' PAM implementation interprets this
 *   argument as a pointer to a pointer to an array of num_msg pam_message
 *   structures. Fortunately, perhaps, for most module/application
 *   developers when num_msg has a value of one these two definitions are
 *   entirely equivalent. Unfortunately, casually raising this number to two
 *   has led to unanticipated compatibility problems.
 *
 *   For what its worth the two known module writer work-arounds for trying
 *   to maintain source level compatibility with both PAM implementations
 *   are:
 *
 *   - never call the conversation function with num_msg greater than one.
 *
 *   - set up msg as doubly referenced so both types of conversation
 *     function can find the messages. That is, make
 *
 *             msg[n] = & (( *msg )[n])
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE	64
#define ARRAY_SIZE(a)	((long)sizeof(a)/sizeof(a[0]))

/* This should be large enough to be easily distinguished from a pointer */
struct A {
	int i;
	int j;
	int k;
	int z;
};

void do_fill_Linux_PAM(struct A **a, int N)
{
	int i;

	a[0] = malloc(N*sizeof(struct A));

	printf("%s a %p, *a %p\n", __func__, a, *a);

	for (i = 0; i < N; ++i) {
		a[i] = &(*a)[i];
		/*
		 * Or somewhat simpler:
		 *
		 *	a[i] = (void *)*a + i*sizeof(struct A);
		 */
	}

	for (i = 0; i < N; ++i) {
		(*a[i]).i = i;
		(*a[i]).j = N - i;
		(*a[i]).k = 7;
		(*a[i]).z = -7;
	}
}

void do_fill_Solaris(struct A **a, int N)
{
	int i;

	*a = malloc(N*(sizeof(struct A *) + sizeof(struct A)));

	printf("%s a %p, *a %p\n", __func__, a, *a);

	for (i = 0; i < N; ++i) {
		(*a)[i].i = i;
		(*a)[i].j = N - i;
		(*a)[i].k = 7;
		(*a)[i].z = -7;
	}
}

int main(void)
{
	int i;
	const int N = 4;
	struct A *a1 = NULL;
	struct A *a2[N];

	printf("%s sizeof(struct A) %zi\n", __func__, sizeof(struct A));

	do_fill_Solaris(&a1, N);
	for (i = 0; i < N; ++i) {
		printf("%i {%i,%i,%i,%i}\n", i, a1[i].i, a1[i].j, a1[i].k, a1[i].z);
	}
	free(a1);

	do_fill_Linux_PAM(a2, N);
	for (i = 0; i < N; ++i) {
		printf("%i {%i,%i,%i,%i}\n", i, a2[i]->i, a2[i]->j, a2[i]->k, a2[i]->z);
	}
	free(a2[0]);

	printf("%s a1 %p, a2 %p\n", __func__, a1, a2);

	return 0;
}

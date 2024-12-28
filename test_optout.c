/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * Some variables may be optimized out by the compiler.  In case of
 *
 *	$ gcc -dumpmachine
 *	x86_64-pc-linux-gnu
 *	$ gcc -dumpversion
 *	14.1.1
 *
 * (results are the same), and compiling with -O0
 *
 *	cc -Wall -Wextra  -O0 -g   test_optout.c  -lm -o test_optout
 *
 * the output is quasi-expected:
 *
 *	$ ./test_optout
 *	dump_controllers:
 *	dump_controllers '0x7ffd780da790'
 *	strv_dump 'cpu'
 *	strv_dump 'cpuacct'
 *	dump_controllers '0x7ffd780da7b0'
 *	strv_dump 'net_cls'
 *	strv_dump 'net_prio'
 *
 * In case of compiling with -O2:
 *
 *	cc -Wall -Wextra  -O2 -g   test_optout.c  -lm -o test_optout
 *
 * the output is empty:
 *
 *	$ ./test_optout
 *	dump_controllers:
 *
 * and the binary doesn't contain strings in question:
 *
 *	$ strings ./test_optout | grep net_prio | wc -l
 *	0
 *
 * they are completely optimized out.   In case of aarch64 target:
 *
 *	$ gcc -dumpmachine
 *	aarch64-meego-linux-gnu
 *	$ gcc -dumpversion
 *	12.3.1
 *
 * the result for -O0 is the same as for x86_64, but
 *
 *	$ ./test_optout
 *	dump_controllers:
 *	dump_controllers '0x4000008004d0'
 *	strv_dump ''
 *	strv_dump ':
 *		    '
 *	dump_controllers '0x40000093b4c0'
 *	Segmentation fault (core dumped)
 *
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//static inline void *malloc_multiply(size_t need, size_t size);

const char *split(const char **state, size_t *l, const char *separator, bool quoted);

#define unlikely(x) (__builtin_expect(!!(x), 0))

//#define new(n, t) ((t*) malloc_multiply((n), sizeof(t)))

#define streq(a, b) (strcmp((a), (b)) == 0)

#define WHITESPACE        " \t\n\r"

#define FOREACH_WORD(word, length, s, state)                            \
        _FOREACH_WORD(word, length, s, WHITESPACE, false, state)

#define FOREACH_WORD_SEPARATOR(word, length, s, separator, state)       \
        _FOREACH_WORD(word, length, s, separator, false, state)

#define _FOREACH_WORD(word, length, s, separator, quoted, state)        \
        for ((state) = (s), (word) = split(&(state), &(length), (separator), (quoted)); \
	     (word); \
	     (word) = split(&(state), &(length), (separator), (quoted)))

#define STRV_FOREACH(s, l)                      \
        for ((s) = (l); (s) && *(s); (s)++)

#define STRV_MAKE(...) ((char**) ((const char*[]) { __VA_ARGS__, NULL }))

static inline bool size_multiply_overflow(size_t need, size_t size)
{
	return unlikely(need != 0 && size > (SIZE_MAX / need));
}

__attribute__ ((malloc))
__attribute__ ((alloc_size (1, 2)))
static inline void *malloc_multiply(size_t need, size_t size)
{
        if (size_multiply_overflow(size, need))
                return NULL;

        return malloc(need * size);
}

static size_t strcspn_escaped(const char *s, const char *reject)
{
	bool escaped = false;
	int n;

	for (n=0; s[n]; n++) {
		if (escaped) {
			escaped = false;
		} else if (s[n] == '\\') {
			escaped = true;
		} else if (strchr(reject, s[n])) {
			break;
		}
	}

	/* if s ends in \, return index of previous char */
	return n - escaped;
}

/* Split a string into words. */
const char *split(const char **state, size_t *l, const char *separator, bool quoted)
{
        const char *current;

        current = *state;

        if (!*current) {
                assert(**state == '\0');
                return NULL;
        }

        current += strspn(current, separator);
        if (!*current) {
                *state = current;
                return NULL;
        }

        if (quoted && strchr("\'\"", *current)) {
                char quotechars[2] = {*current, '\0'};

                *l = strcspn_escaped(current + 1, quotechars);
                if (current[*l + 1] == '\0' || current[*l + 1] != quotechars[0] ||
                    (current[*l + 2] && !strchr(separator, current[*l + 2]))) {
                        /* right quote missing or garbage at the end */
                        *state = current;
                        return NULL;
                }
                *state = current++ + *l + 2;
        } else if (quoted) {
                *l = strcspn_escaped(current, separator);
                if (current[*l] && !strchr(separator, current[*l])) {
                        /* unfinished escape */
                        *state = current;
                        return NULL;
                }
                *state = current + *l;
        } else {
                *l = strcspn(current, separator);
                *state = current + *l;
        }

        return current;
}

#if 0
static char **strv_new(const char *x, ...)
{
        char **r;
        va_list ap;

        va_start(ap, x);
        r = strv_new_ap(x, ap);
        va_end(ap);

        return r;
}
#endif

static inline void *mfree(void *memory)
{
        free(memory);

        return NULL;
}

void strv_clear(char **l)
{
	char **k;

	if (!l) {
		return;
	}

	for (k = l; *k; k++) {
		free(*k);
	}

	*l = NULL;
}

char **strv_free(char **l)
{
	strv_clear(l);

	return mfree(l);
}

char **strv_split(const char *s, const char *separator)
{
        const char *word, *state;
        size_t l;
        unsigned n, i;
        char **r;

        assert(s);

        n = 0;
        FOREACH_WORD_SEPARATOR(word, l, s, separator, state) {
                n++;
	}

        r = malloc_multiply(n + 1, sizeof(char *));
        if (!r)
                return NULL;

        i = 0;
        FOREACH_WORD_SEPARATOR(word, l, s, separator, state) {
                r[i] = strndup(word, l);
                if (!r[i]) {
                        strv_free(r);
                        return NULL;
                }

                i++;
        }

        r[i] = NULL;
        return r;
}

char **strv_remove(char **l, const char *s)
{
        char **f, **t;

        if (!l) {
                return NULL;
	}

        assert(s);

        /* Drops every occurrence of s in the string list, edits
         * in-place. */

        for (f = t = l; *f; f++) {
                if (streq(*f, s)) {
                        free(*f);
                } else {
                        *(t++) = *f;
		}
	}

        *t = NULL;

        return l;
}

/*
 * Drops duplicate entries. The first identical string will be
 * kept, the others dropped
 */
char **strv_uniq(char **l)
{
        char **i;

        STRV_FOREACH(i, l) {
		printf("%s '%s'\n", __func__, *i);
                strv_remove(i + 1, *i);
	}

        return l;
}

unsigned strv_length(char * const *l)
{
        unsigned n = 0;

        if (!l) {
                return 0;
	}

        for (; *l; l++) {
                n++;
	}

        return n;
}

void strv_dump(char **l)
{
        char **i;

        STRV_FOREACH(i, l) {
		printf("%s '%s'\n", __func__, *i);
	}
}

static char ***parse_controllers()
{
	// JoinControllers="cpu,cpuacct net_cls,netprio";
	const char *word1 = "cpu,cpuacct";
	char **l1 = strv_split(word1, ",");
	if (!l1) {
		printf("%s bad word '%s'\n", __func__, word1);
		return NULL;
	}

	if (strv_length(l1) <= 1) {
		printf("%s bad length of '%s'\n", __func__, word1);
		strv_free(l1);
		return NULL;
	}

	const char *word2 = "net_cls,netprio";
	char **l2 = strv_split(word2, ",");
	if (!l2) {
		printf("%s bad word '%s'\n", __func__, word2);
		return NULL;
	}

	if (strv_length(l2) <= 1) {
		printf("%s bad length of '%s'\n", __func__, word2);
		strv_free(l2);
		return NULL;
	}

	char ***controllers = malloc_multiply(3, sizeof(char **));
	if (!controllers) {
		printf("%s bad controllers\n", __func__);
		strv_free(l1);
		strv_free(l2);
		return NULL;
	}

	controllers[0] = l1;
	controllers[1] = l2;
	controllers[2] = NULL;

	//strv_uniq(l);
	//strv_free(l);

	return controllers;
}

void dump_controllers(char ***controllers)
{
	printf("%s:\n", __func__);
	for (char ***k = controllers; k && *k; k++) {
		printf("%s '%p'\n", __func__, *k);
		strv_dump(*k);
		//if (strv_find(*k, controller))
		//	break;
	}
}

#if 0
int test_optout_v1(char ***join_controllers)
{
	bool has_argument = !!join_controllers;
	char **default_join_controllers[] ={
		STRV_MAKE("cpu", "cpuacct"),
		STRV_MAKE("net_cls", "net_prio"),
		NULL,
	};
	int r;

	/* Mount all available cgroup controllers that are built into the kernel. */

	if (!has_argument) {
		/* The defaults:
		 * mount "cpu" + "cpuacct" together, and "net_cls" + "net_prio".
		 *
		 * We'd like to add "cpuset" to the mix, but "cpuset" doesn't really
		 * work for groups with no initialized attributes.
		 */
		join_controllers = default_join_controllers;
	}

	return 0;
}
#endif

int test_optout_v2(char ***join_controllers)
{
	bool has_argument = !!join_controllers;
	/*char **default_join_controllers[] ={
	  STRV_MAKE("cpu", "cpuacct"),
	  STRV_MAKE("net_cls", "net_prio"),
	  NULL,
	  };*/
	//int r;

	/* Mount all available cgroup controllers that are built into the kernel. */

	if (!has_argument) {
		/* The defaults:
		 * mount "cpu" + "cpuacct" together, and "net_cls" + "net_prio".
		 *
		 * We'd like to add "cpuset" to the mix, but "cpuset" doesn't really
		 * work for groups with no initialized attributes.
		 */
		join_controllers = (char**[]) {
			STRV_MAKE("cpu", "cpuacct"),
			STRV_MAKE("net_cls", "net_prio"),
			NULL,
		};
		//join_controllers = default_join_controllers;
	}

	dump_controllers(join_controllers);

	return 0;
}

int main(int argc, char *argv[])
{
	//char ***arg_join_controllers = parse_controllers();
	char ***arg_join_controllers = NULL;

	//strv_new();

	//test_optout_v1();
	//int r2 = test_optout_v2(arg_join_controllers);
	int r2 = test_optout_v2(arg_join_controllers);

	return r2;
}

/*
 * A list-vs-tree test.
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "list.h"
#include "rbtree.h"

#define DEBUG
#undef DEBUG

enum my_flags {
	MY_F_NEW = 0x1 << 0,
};

struct my_tree {
	struct rb_root rb_root;
	int my_nr;
};

struct my_struct_l {
	struct list_head node;
	int val;
};

struct my_struct_t {
	struct rb_node rb_node;
	int val;
};

static double l_time(struct timespec *ts)
{
	clock_gettime(CLOCK_MONOTONIC, ts);
	return ts->tv_sec + 1e-9*ts->tv_nsec;
}

/*
 * Tree
 */
static struct my_struct_t *my_new_t(int val)
{
	struct my_struct_t *my = malloc(sizeof(struct my_struct_t));

	my->val = val;

	return my;
}

static struct my_struct_t *my_findnew_t(struct my_tree *my_tree,
				        int val, unsigned flags)
{
	int r;
	struct my_struct_t *my;
	struct rb_node **p = &my_tree->rb_root.rb_node;
	struct rb_node *parent = NULL;

	while (*p != NULL) {
		parent = *p;

		my = rb_entry(parent, struct my_struct_t, rb_node);

		r = my->val - val;

		if (r > 0)
			p = &(*p)->rb_left;
		else if (r < 0)
			p = &(*p)->rb_right;
		else
			return my;
	}

	if (flags & MY_F_NEW) {
		my = my_new_t(val);
		if (my) {
			my_tree->my_nr++;
			rb_link_node(&my->rb_node, parent, p);
			rb_insert_color(&my->rb_node, &my_tree->rb_root);
		}

		return my;
	}

	return NULL;
}

static void my_free_t(struct my_tree *my_tree)
{
	struct my_struct_t *my;
	struct rb_node *n, *p;
	n = rb_first(&my_tree->rb_root);
	while (n) {
		p = n;
		n = rb_next(n);
		rb_erase(p, &my_tree->rb_root);
		my = rb_entry(p, struct my_struct_t, rb_node);
		free(my);
	}
}

static int test_tree_build(struct my_tree *my_tree,
			   const int *val, const int val_nr)
{
	int i, r;
	struct rb_node *n;

	/* build a tree */
	for (i = 0; i < val_nr; i++) {
		my_findnew_t(my_tree, val[i], MY_F_NEW);
	}

#ifdef DEBUG
	struct my_struct_t *my1 = rb_entry(my_tree->rb_root.rb_node,
					 struct my_struct_t, rb_node);
	struct my_struct_t *my2 = ((void *) my_tree->rb_root.rb_node
				- sizeof(my_tree->rb_root.rb_node));
	struct my_struct_t *my3 = ((void *) my_tree->rb_root.rb_node);

	printf("root %p, my1 %p, my2 %p, my3 %p, val %i\n",
		my_tree->rb_root.rb_node, my1, my2, my3, my1->val);
#endif

	/*
	 * determine max depth (FIXME this has n*log(n) compexity, maybe
	 * we can do better)
	 */
	int max_depth = 0;
	for (n = rb_first(&my_tree->rb_root); n; n = rb_next(n)) {
		r = rb_depth(n);
		if (max_depth < r) {
			max_depth = r;
		}
	}

#ifdef DEBUG
	printf("val_nr %i, tree_nr %i, max_depth %i, log2(%i)=%i, msb(%i)=%i\n",
		val_nr, my_tree->my_nr, max_depth,
		my_tree->my_nr, (int) (logl(my_tree->my_nr)/logl(2)),
		my_tree->my_nr, (int) (8*sizeof(my_tree->my_nr)
				       - __builtin_clz(my_tree->my_nr)));
#endif

	return max_depth;
}

static int test_tree_search(struct my_tree *my_tree,
			    const int *key, const int key_nr)
{
	int i, hit_nr = 0;
	struct my_struct_t *my;

	/* search each key trough the tree */
	for (i = 0; i < key_nr; i++) {
		my = my_findnew_t(my_tree, key[i], 0);
		if (my)
			hit_nr++;
	}

	return hit_nr;
}

#ifdef DEBUG
static int test_tree_iter(const char *h, struct my_tree *my_tree,
			  const int *key, const int key_nr)
{
	struct my_struct_t *my;
	struct rb_node *n;
	int r = 0;

	printf("%s", h);
	for (n = rb_first(&my_tree->rb_root); n; n = rb_next(n)) {
		my = rb_entry(n, struct my_struct_t, rb_node);
		printf("%i ", my->val);
		r++;
	}
	printf("\n");

	return r;
}
#endif

/*
 * List
 */
static struct my_struct_l *my_new_l(int val)
{
	struct my_struct_l *my = malloc(sizeof(struct my_struct_l));

	my->val = val;

	return my;
}

static int test_list_build(struct list_head *my_list,
			   const int *val, const int val_nr)
{
	int i;
	struct my_struct_l *my;

	for (i = 0; i < val_nr; i++) {
		my = my_new_l(val[i]);
		list_add_tail(&my->node, my_list);
	}

	return val_nr;
}

static void my_free_l(struct list_head *my_list)
{
	struct my_struct_l *my;
	while (!list_empty(my_list)) {
		my = list_entry(my_list->next, struct my_struct_l, node);
		list_del_init(my_list->next);
		free(my);
	}
}

static int test_list_search(struct list_head *my_list,
			    int *key, const int key_nr)
{
	struct my_struct_l *my;
	struct list_head *n;
	int i, hit_nr = 0;

	for (i = 0; i < key_nr; i++) {
		list_for_each(n, my_list) {
			my = list_entry(n, struct my_struct_l, node);
			if (my->val == key[i]) {
				hit_nr++;
				break;
			}
		}
	}

	return hit_nr;
}

#ifdef DEBUG
static int test_list_iter(const char *h, struct list_head *my_list,
			  int *val, const int val_nr)
{
	struct my_struct_l *my;
	struct list_head *n;
	int r = 0;

	printf("%s", h);
	list_for_each(n, my_list) {
		my = list_entry(n, struct my_struct_l, node);
		printf("%i ", my->val);
		r++;
	}
	printf("\n");

	return r;
}
#endif

/*
 * Array
 */

static void test_arr_init(int *a, const int n)
{
	int i;

	for (i = 0; i < n; i++) {
		a[i] = rand()%1000;
	}
}

#ifdef DEBUG
static void test_arr_dump(const char *h, int *a, const int n)
{
	int i;

	printf("%s", h);
	for (i = 0; i < n; i++) {
		printf("%i ", a[i]);
	}
	printf("\n");
}
#endif

static int get_options(int argc, char **argv, int *val_nr, int *key_nr)
{
	int c, r = 0;

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"val_nr",  required_argument, 0, 'v' },
			{"key_nr",  required_argument, 0, 'k' },
			{0,         0,                 0,  0 }
		};

		c = getopt_long(argc, argv, "k:v:",
				long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
			case 0:
				printf("option %s", long_options[option_index].name);
				if (optarg)
					printf(" with arg %s", optarg);
				printf("\n");
				break;

			case 'k':
				*key_nr = strtol(optarg, 0, 10);
				r = 1;
				break;

			case 'v':
				*val_nr = strtol(optarg, 0, 10);
				r = 1;
				break;

			case '?':
				break;

			default:
				printf("?? getopt returned character code 0%o ??\n", c);
		}
	}

	if (optind < argc) {
		printf("non-option ARGV-elements: ");
		while (optind < argc)
			printf("%s ", argv[optind++]);
		printf("\n");
	}

	return r;
}

int main(int argc, char **argv)
{
	struct my_tree my_tree = {
		.rb_root = {
			.rb_node = 0,
		},
		.my_nr = 0,
	};
	struct list_head my_list = LIST_HEAD_INIT(my_list);

	double t_l_1, t_l_2, t_t_1, t_t_2;
	struct timespec ts;
	int hit_tree = 0;
	int hit_list = 0;
	int seed = time(0);
	int i, N = 1000;

	int val_nr = 99;
	int key_nr = 10;

	get_options(argc, argv, &val_nr, &key_nr);

	int *val = malloc(val_nr*sizeof(int));
	int *key = malloc(key_nr*sizeof(int));

	srand(seed);
	test_arr_init(val, val_nr);
	test_arr_init(key, key_nr);

#ifdef DEBUG
	printf("key_nr '%i'\n", key_nr);
	printf("val_nr '%i'\n", val_nr);
	test_arr_dump("Nodes:\t", val, val_nr);
	test_arr_dump("Keys:\t", key, key_nr);
#endif

	test_tree_build(&my_tree, val, val_nr);
	test_list_build(&my_list, val, val_nr);

#ifdef DEBUG
	test_tree_iter("tree:\t", &my_tree, val, val_nr);
	test_list_iter("list:\t", &my_list, val, val_nr);
#endif

	/*
	 * warm up CPU cache
	 */
	t_t_1 = l_time(&ts);
	for (i = 0; i < N; i++)
		test_tree_search(&my_tree, key, key_nr);
	t_t_1 = l_time(&ts) - t_t_1;

	t_l_1 = l_time(&ts);
	for (i = 0; i < N; i++)
		test_list_search(&my_list, key, key_nr);
	t_l_1 = l_time(&ts) - t_l_1;

	/*
	 * the real sampling
	 */
	t_t_2 = l_time(&ts);
	for (i = 0; i < N; i++)
		hit_tree += test_tree_search(&my_tree, key, key_nr);
	t_t_2 = l_time(&ts) - t_t_2;

	t_l_2 = l_time(&ts);
	for (i = 0; i < N; i++)
		hit_list += test_list_search(&my_list, key, key_nr);
	t_l_2 = l_time(&ts) - t_l_2;

	my_free_t(&my_tree);
	my_free_l(&my_list);
	free(key);
	free(val);

	/*
	 *   1      2       3        4          5          6        7
	 * match  val_nr  key_nr  hit_tree  hit_list  time_tree  time_list
	 */
	hit_tree /= N;
	hit_list /= N;
	t_t_2 /= N;
	t_l_2 /= N;
	printf("%s\t%i\t%i\t%i\t%i\t%g\t%g\n",
		hit_tree == hit_list ? "OK" : "mismatch",
		val_nr, key_nr, hit_tree, hit_list,
		//(t_t_1 + t_t_2) / 2,
		//(t_l_1 + t_l_2) / 2);
		t_t_2,
		t_l_2);

	return 0;
}

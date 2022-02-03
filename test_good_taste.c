/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * See:
 *
 *   Linus Torvalds: The mind behind Linux
 *   - https://www.ted.com/talks/linus_torvalds_the_mind_behind_linux
 *
 *   Applying the Linus Torvalds “Good Taste” Coding Requirement
 *   - https://medium.com/@bartobri/applying-the-linus-tarvolds-good-taste-coding-requirement-99749f37684a
 *
 *   santisbon/Good taste.md
 *   - https://gist.github.com/santisbon/42580049705ba3d8fbef7168e4668e3c
 */

#include <stdio.h>
#include <stdlib.h>

/*
 * Example:
 *
 *   - 8bit byte
 *   - 32 bit pointer,
 *   - 32 bit integer,
 *   - decimal addresses starting from 1000
 *
 *                      |        |  1000, 1001, 1002, 1003
 *                      +--------+
 *                      |        |  1004
 *                      +--------+
 *                      |        |  1008
 *                      +--------+
 *                      |        |  1012
 *                      +--------+
 * struct node *head;   |  1028  |  1016
 *                      +--------+
 *                      |        |  1020
 *                      +--------+
 *                      |        |  1024
 *                      +--------+
 *              .next   |  1040  |  1028
 *                      +--------+
 *              .value  |   777  |  1032
 *                      +--------+
 *                      |        |  1036
 *                      +--------+
 *              .next   |  1048  |  1040
 *                      +--------+
 *              .value  |   123  |  1044
 *                      +--------+
 *              .next   |  1064  |  1048
 *                      +--------+
 *              .value  |   313  |  1052
 *                      +--------+
 *                      |        |  1056
 *                      +--------+
 *                      |        |  1060
 *                      +--------+
 *              .next   |  NULL  |  1064
 *                      +--------+
 *              .value  |   271  |  1068
 *                      +--------+
 *                      |        |  1072
 *
 */

/*
 * Step 1.  Initialize head
 *
 * struct node *head = NULL;
 *
 *                      |        |  1008
 *                      +--------+
 *                      |        |  1012
 *                      +--------+
 * struct node *head;   |  NULL  |  1016
 *                      +--------+
 *                      |        |  1020
 *                      +--------+
 *                      |        |  1024
 *
 */

/*
 * Step 2.  Allocate new
 *
 *    new = malloc(sizeof(struct node));
 *
 *                      |        |  1008
 *                      +--------+
 *                      |        |  1012
 *                      +--------+
 * struct node *head;   |  NULL  |  1016
 *                      +--------+
 *                      |        |  1020
 *                      +--------+
 * struct node *new;    |  1028  |  1024
 *                      +--------+
 *          node.next   |  ????  |  1028
 *                      +--------+
 *          node.value  |  ????  |  1032
 *                      +--------+
 *                      |        |  1036
 *
 *
 */


/*
 * Step 3.  Set head to new
 *
 *   head = new;
 *
 *                      |        |  1008
 *                      +--------+
 *                      |        |  1012
 *                      +--------+
 * struct node *head;   |  1024  |  1016
 *                      +--------+
 *                      |        |  1020
 *                      +--------+
 * struct node *new;    |  1028  |  1024
 *                      +--------+
 *          node.next   |  ????  |  1028
 *                      +--------+
 *          node.value  |  ????  |  1032
 *                      +--------+
 *                      |        |  1036
 *
 *
 */

struct node {
	struct node *next;
	int value;
};

struct node *list_ins_tail_poor(struct node **head, int value)
{
	struct node *new = malloc(sizeof(struct node));

	new->value = value;
	new->next = NULL;

	if (*head == NULL) {
		*head = new;
	} else {
		struct node *walk = *head;
		while (walk->next != NULL) {
			walk = walk->next;
		}
		walk->next = new;
	}

	return new;
}

struct node *list_find(struct node *head, int value)
{
	struct node *walk= head;

	while (walk && walk->value != value) {
		walk = walk->next;
	}

	return walk;
}

/*
 * Remove a B entry:      before         after
 *
 *                      |        |    |        |  1012
 *                      +--------+    +--------+
 * struct node *head;   |  1028  |    |  1028  |  1016
 *                      +--------+    +--------+
 *                      |        |    |        |  1020
 *                      +--------+    +--------+
 *                      |        |    |        |  1024
 *                      +--------+    +--------+
 *             A.next   |  1040  |    |  1040  |  1028
 *                      +--------+    +--------+
 *             A.value  |   777  |    |   777  |  1032
 *                      +--------+    +--------+
 *                      |        |    |        |  1036
 *                      +--------+    +--------+
 *             B.next   |  1048  |    |  1064  |  1040
 *                      +--------+    +--------+
 *             B.value  |   123  |    |   313  |  1044
 *                      +--------+    +--------+
 *             C.next   |  1064  |    |  1064  |  1048	free(1048), size is
 *                      +--------+    +--------+        sizeof(struct node)
 *             C.value  |   313  |    |   ???  |  1052
 *                      +--------+    +--------+
 *                      |        |    |        |  1056
 *                      +--------+    +--------+
 *                      |        |    |        |  1060
 *                      +--------+    +--------+
 *             D.next   |  NULL  |    |  NULL  |  1064
 *                      +--------+    +--------+
 *             D.value  |   271  |    |   271  |  1068
 *                      +--------+    +--------+
 *                      |        |    |        |  1072
 *
 * A -> B -> C -> D
 *
 * A -> C  free -> D
 *       \_____/
 *
 *
 */

struct node *list_remove_poor(struct node *entry)
{
	struct node *next = entry->next;

	if (next) {
		entry->next = next->next;
		entry->value = next->value;
	}

	free(next);

	printf("%s entry %p, next %p\n", __func__, entry, next);
	return NULL;
}

void node_free(struct node *node)
{
	printf("node %p, next %p, value %d\n", node, node->next, node->value);
	node->next = NULL;
	node->value = 0;
	//printf("node %p, next %p, value %d\n", node, node->next, node->value);
	free(node);
}

void list_free(struct node **head)
{
	struct node *walk = *head;

	struct node *prev = walk;
	while (walk) {
		prev = walk;
		walk = walk->next;
		node_free(prev);
	}

	//printf("head %p, next %p, value %d\n", *head, (*head)->next, (*head)->value);
	//printf("prev %p, next %p, value %d\n", prev, prev->next, prev->value);

	*head = NULL;
}

void list_dump(struct node *head)
{
	struct node *walk = head;

	while (walk) {
		//printf(" %d", walk->value);
		printf("walk %p, next %p, value %d\n", walk, walk->next, walk->value);
		walk = walk->next;
	}

	printf("\n");
}

int main(void)
{
	struct node *head = NULL;

	list_ins_tail_poor(&head, 0);
	list_ins_tail_poor(&head, 1);
	list_ins_tail_poor(&head, 1);
	list_ins_tail_poor(&head, 2);
	list_ins_tail_poor(&head, 3);
	list_ins_tail_poor(&head, 5);
	list_ins_tail_poor(&head, 8);
	list_ins_tail_poor(&head, 777);

	list_dump(head);

	struct node *entry = list_find(head, 3);

	if (entry) {
		printf("Found %p %d\n", entry, entry->value);
	}

	list_remove_poor(entry);

	list_free(&head);

	printf("head %p\n", head);

	return 0;
}

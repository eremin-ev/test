/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * Merge two sorted linked lists [1]
 *
 * Medium   Accuracy: 62.91%    Submissions: 127K+  Points: 4
 *
 * Given two sorted linked lists consisting of N and M nodes respectively.
 * The task is to merge both of the list (in-place) and return head of the
 * merged list.
 *
 * Example 1:
 *
 *  Input:  N = 4, M = 3, valueN[] = { 5, 10, 15, 40 }, valueM[] = { 2, 3, 20 }
 *
 *  Output: 2 3 5 10 15 20 40
 *
 *  Explanation: After merging the two linked lists, we have merged
 *               list as 2, 3, 5, 10, 15, 20, 40.
 *
 * Example 2:
 *
 *  Input:  N = 2, M = 2, valueN[] = { 1, 1 }, valueM[] = { 2, 4 }
 *
 *  Output: 1 1 2 4
 *
 *  Explanation: After merging the given two linked list, we have
 *               1, 1, 2, 4 as output.
 *
 * Your Task:
 *
 *      The task is to complete the function sortedMerge() which takes
 *      references to the heads of two linked lists as the arguments and
 *      returns the head of merged linked list.
 *
 * Expected Time Complexity : O(n + m)
 * Expected Auxilliary Space : O(1)
 *
 * Constraints:
 *
 *      1 <= N, M <= 10^4
 *      0 <= Node's data <= 10^5
 *
 * [1] Merge two sorted linked lists
 *     https://practice.geeksforgeeks.org/problems/merge-two-sorted-linked-lists/1
 *
 */

#include <iostream>

namespace {

/* Link list Node */
struct Node {
    int data;
    Node *next;
    Node(int x)
    {
        data = x;
        next = nullptr;
        //std::cout << __func__ << " " << data << '\n';
    }
};

/* Function to print Nodes in a given linked list */
static void printList(const Node *n)
{
    while (n != nullptr) {
        std::cout << n->data << ' ';
        n = n->next;
    }
    std::cout << std::endl;
}

static Node *readLine(int n)
{
    int data;
    std::cin >> data;
    Node *head = new Node(data);
    Node *tail = head;
    for (int i = 1; i < n; ++i) {
        std::cin >> data;
        tail->next = new Node(data);
        tail = tail->next;
    }

    return head;
}

/*static Node *getMin(Node *h1, Node *h2)
{
    if (h1->data < h2->data) {
        return h1;
    }

    return h2;
}

static Node *appendTail(Node *head, Node *rest)
{
    Node *tail = head;
    while (rest) {
        tail->next = rest;
        rest = rest->next;
        tail = tail->next;
    }

    return tail;
}*/

/*
 * Function to merge two sorted linked list.
 *
 * Problem Solved Successfully
 *
 *      Test Cases Passed:   303 / 303
 *	    Total Points Scored: 4 /4
 *	    Your Total Score:    28
 *	    Total Time Taken:    0.36
 *	    Your Accuracy:       100%
 *	    Attempts No.:        1
 *
 */
static Node *sortedMerge(Node *h1, Node *h2)
{
    Node head(0);
    Node *tail = &head;
    while (h1 && h2) {
        if (h1->data < h2->data) {
            tail->next = h1;
            h1 = h1->next;
        } else {
            tail->next = h2;
            h2 = h2->next;
        }

        tail = tail->next;
    }

    // appent the rest from the longest list
    if (h1 && !h2) {
        tail->next = h1;
    } else if (!h1 && h2) {
        tail->next = h2;
    } else {
        std::cout << __func__ << " bug\n";
        exit(1);
    }

    h1 = head.next;

    return h1;
}

} // anonymous namespace

/* Driver program to test above function*/
int main()
{
    int t;
    std::cin >> t;
    while (t--) {
        int n, m;
        std::cin >> n >> m;

        //std::cout << __func__ << " n " << n <<  " m " << m << '\n';

        Node *head1 = readLine(n);
        Node *head2 = readLine(m);
        Node *head = sortedMerge(head1, head2);

        printList(head);

        // FIXME free head
    }
    return 0;
}

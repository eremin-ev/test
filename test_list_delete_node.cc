/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * Delete without head pointer
 *
 *  Easy    Accuracy: 78.57%    Submissions: 159K+  Points: 2
 *
 * You are given a pointer/reference to the node which is to be deleted
 * from the linked list of N nodes. The task is to delete the node.
 * Pointer/reference to head node is not given.
 *
 * Note: No head reference is given to you. It is guaranteed that the
 * node to be deleted is not a tail node in the linked list.
 *
 * Example 1:
 *
 *  Input:  N = 2, value[] = { 1, 2 }, node = 1
 *
 *  Output: 2
 *
 *  Explanation: After deleting 1 from the linked list, we have
 *               remaining nodes as 2.
 *
 * Example 2:
 *
 *  Input:   N = 4, value[] = { 10, 20, 4, 30 }, node = 20
 *
 *  Output: 10 4 30
 *
 *  Explanation: After deleting 20 from the linked list, we have
 *               remaining nodes as 10, 4 and 30.
 *
 * Your Task:
 *
 *      You only need to complete the function deleteNode that takes
 *      reference to the node that needs to be deleted. The printing is
 *      done automatically by the driver code.
 *
 * Expected Time Complexity : O(1)
 * Expected Auxilliary Space : O(1)
 *
 * Constraints:
 *
 *      2 <= N <= 10^3
 *
 * [1] Delete without head pointer
 *     https://practice.geeksforgeeks.org/problems/delete-without-head-pointer/1
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

static Node *findNode(Node *head, int search_for)
{
    Node *current = head;
    while (current) {
        if (current->data == search_for)
            break;

        current = current->next;
    }

    return current;
}

} // anonymous namespace

class Solution {
public:

    /*
     * Problem Solved Successfully
     *
     *  Test Cases Passed: 130/130
     *  Total Points Scored: 2/2
     *  Your Total Score: 30
     *  Total Time Taken: 0.06
     *  Your Accuracy: 100%
     *  Attempts No.: 1
     */

    // Function to delete a node without any reference to head pointer.
    void deleteNode(Node *del)
    {
        // Your code here
        if (!del) {
            return;
        }

        //Node *tmp_del = del;
        Node *tmp_next = del->next;
        if (!tmp_next) {
            return;
        }

        del->next = tmp_next->next;
        del->data = tmp_next->data;
        delete tmp_next;
    }
};

int main()
{
    int t;
    std::cin >> t;
    while (t--) {
        int n;
        std::cin >> n;

        //std::cout << __func__ << " n " << n << '\n';

        Node *head = readLine(n);

        int k;
        std::cin >> k;
        Node *del = findNode(head, k);
        Solution ob;
        if (del != nullptr && del->next != nullptr) {
            ob.deleteNode(del);
        }

        printList(head);

        // FIXME free head
    }

    return 0;
}

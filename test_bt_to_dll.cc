/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * Binary Tree to Doubly Linked List
 *
 *  Hard    Accuracy: 53.36%    Submissions: 117K+      Points: 8
 *
 * Given a Binary Tree (BT), convert it to a Doubly Linked List (DLL)
 * in-place. The left and right pointers in nodes are to be used as
 * previous and next pointers respectively in converted DLL. The order
 * of nodes in DLL must be same as Inorder of the given Binary Tree.
 * The first node of Inorder traversal (leftmost node in BT) must be
 * the head node of the DLL.
 *
 * Note: H is the height of the tree and this space is used implicitly
 * for the recursion stack.
 *
 * Eremin note: Seems the tree is not ordered by any kind of '<'.
 *
 *             10
 *
 *           /    \
 *
 *        12        15
 *       /  \      /
 *      25   30  36
 *
 * The above tree should be in-place converted to following Doubly
 * Linked List:
 *
 * 25 <-> 12 <-> 30 <-> 10 <-> 36 <-> 15
 *
 * Example 1:
 *
 *  Input:
 *
 *          1
 *        /  \
 *       3    2
 *
 *  Output:
 *
 *      3 1 2
 *      2 1 3
 *
 *  Explanation: DLL would be 3<=>1<=>2
 *
 * Example 2:
 *
 *  Input:
 *
 *            10
 *          /   \
 *         20   30
 *       /   \
 *      40   60
 *
 *  Output:
 *
 *      40 20 60 10 30
 *      30 10 60 20 40
 *
 *  Explanation:  DLL would be 40<=>20<=>60<=>10<=>30.
 *
 * Your Task:
 *
 *      You don't have to take input. Complete the function bToDLL()
 *      that takes root node of the tree as a parameter and returns the
 *      head of DLL . The driver code prints the DLL both ways.
 *
 * Expected Time Complexity: O(N).
 * Expected Auxiliary Space: O(H).
 *
 * Constraints:
 *
 *      1 <= Number of nodes <= 10^5
 *      0 <= Data of a node <= 10^5
 *
 * [1] Binary Tree to Doubly Linked List
 *     https://practice.geeksforgeeks.org/problems/binary-tree-to-dll/1
 *
 */

#include <iostream>
#include <queue>
#include <sstream>
#include <stack>
#include <string>

namespace {

/* Structure for tree and linked list */

// Tree Node
struct Node {
    int data;
    Node *left;
    Node *right;
    Node()
    {
	    left = right = nullptr;
	}
    Node(int x)
        : Node()
    {
        data = x;
    }
};

// Utility function to create a new Tree Node
static Node *newNode(int val)
{
    return new Node(val);
}

// Function to Build Tree
static Node *buildTree(std::string str)
{
    // Corner Case
    if (str.length() == 0 || str[0] == 'N') {
        return nullptr;
    }

    // Creating vector of strings from input
    // string after spliting by space
    std::vector<std::string> ip;

    std::istringstream iss(str);
    for(std::string str; iss >> str; )
        ip.push_back(str);

    // Create the root of the tree
    Node *root = newNode(stoi(ip[0]));

    // Push the root to the queue
    std::queue<Node *> queue;
    queue.push(root);

    // Starting from the second element
    unsigned i = 1;
    while (!queue.empty() && i < ip.size()) {

        // Get and remove the front of the queue
        Node *currNode = queue.front();
        queue.pop();

        // Get the current node's value from the string
        std::string currVal = ip[i];

        // If the left child is not null
        if (currVal != "N") {

            // Create the left child for the current node
            currNode->left = newNode(stoi(currVal));

            // Push it to the queue
            queue.push(currNode->left);
        }

        // For the right child
        i++;
        if (i >= ip.size())
            break;

        currVal = ip[i];

        // If the right child is not null
        if (currVal != "N") {

            // Create the right child for the current node
            currNode->right = newNode(stoi(currVal));

            // Push it to the queue
            queue.push(currNode->right);
        }
        i++;
    }

    return root;
}

/* Function to print nodes in a given doubly linked list */
static void printList(Node *node)
{
    std::cout << __func__ << '\n';

    Node *prev = nullptr;
    while (node) {
        std::cout << node->data << " ";
        prev = node;
        node = node->right;
    }
    std::cout << std::endl;

    while (prev) {
        std::cout << prev->data << " ";
        prev = prev->left;
    }
    std::cout << std::endl;
}

static void inorder(Node *root)
{
    if (!root) {
        return;
    }

    inorder(root->left);
    std::cout << root->data << ' ';
    inorder(root->right);
}

// Find leftmost child node on the desired level
static const Node *first_on_level(const Node *root,
                                  int level,
                                  std::stack<const Node *> &parent)
{
    //std::cout << __func__ << " " << level << " root: " << root->data << '\n';
    int cur_level = 0;
    const Node *p = root;
    while (p && cur_level < level) {
        if (p->left) {
            parent.push(p);
            p = p->left;
            ++cur_level;
        } else if (p->right) {
            parent.push(p);
            p = p->right;
            ++cur_level;
        } else {
            // Cannot continue descending into levels
            return nullptr;
        }
    }

    return p;
}

static const Node *next_on_level(const Node *node, int level, std::stack<const Node *> &parent)
{
    if (parent.empty()) {
        return nullptr;
    }

    int cur_level = level;
    const Node *p = nullptr;
    const Node *p_prev = node;

    // Go up to the next predecessor which has right child different from us
    while (true) {
        p = parent.top();
        parent.pop();
        --cur_level;
        if (p->right && p->right != p_prev) {
            break;
        }

        // If walked up to the root from the right subtree, no nodes left on
        // the desired level, give up
        if (parent.empty()) {
            //std::cout << __func__ << "walked up\n";
            return nullptr;
        }

        p_prev = p;
    }

    // Descent to the right subtree
    if (p->right) {
        parent.push(p);
        p = p->right;
        ++cur_level;
    }

    // Returned back to the source node -- no more left on this level
    if (p && p->right == node) {
        return nullptr;
    }

    // Go through the right subtree to find leftmost child node on the
    // desired level
    while (p && cur_level < level) {
        if (p->left) {
            parent.push(p);
            p = p->left;
            ++cur_level;
        } else if (p->right) {
            parent.push(p);
            p = p->right;
            ++cur_level;
        } else {
            // Cannot continue descending into levels
            // FIXME need to go up
            return nullptr;
        }
    }

    return p;
}

static void printBinaryTree(const Node *root)
{
    if (!root) {
        return;
    }

    std::stack<const Node *> parent;
    const Node *p = root;

    int level_current = 0;

    // root
    std::cout << p->data << '\n';

    while (true) {
        ++level_current;
        p = first_on_level(root, level_current, parent);
        if (!p) {
            break;
        }
        //const Node *leftmost = p;
        //parent = std::stack<const Node *>();
        std::cout << p->data << ' ';

        //std::cout << "parent " << (parent.top())->data << '\n';

        while (true) {
            p = next_on_level(p, level_current, parent);
            if (!p) {
                break;
            }
            std::cout << p->data << ' ';
        };
        std::cout << '\n';
        //p = leftmost;
        //std::cout << "leftmost " << p->data
        //          << " parent " << (parent.empty() ? -1 : (parent.top())->data) << '\n';
    }
}

} // anonymous namespace

// This function should return head to the DLL
class Solution
{
public:
    // Function to convert binary tree to doubly linked list and return it.
    Node *bToDLL(Node *root)
    {
        // your code here
        return root;
    }
};

int main()
{
    int t;
    std::cin >> t;
    getchar();

    while (t--) {
        std::string inp;
        getline(std::cin, inp);
        Node *root = buildTree(inp);

        std::cout << "inorder: ";
        inorder(root);
        std::cout << std::endl;

        std::cout << "printBinaryTree:\n";
        printBinaryTree(root);
        std::cout << std::endl;

        Solution ob;
        Node *head = ob.bToDLL(root);
        printList(head);

    }

    return 0;
}

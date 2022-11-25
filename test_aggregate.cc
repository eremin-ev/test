/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * C++ supports designated initializers only for aggregate types.  See
 * the following StackOverflow question:
 *
 * - Why I can not use designated initalizers with structs that are not aggregates?
 *   https://stackoverflow.com/questions/64770166/why-i-can-not-use-designated-initalizers-with-structs-that-are-not-aggregates
 *
 */

#include <map>
#include <iostream>

class A {
    static std::map<A *, int> &Indices()
    {
        static std::map<A *, int> ret;
        return ret;
    }

public:
    int dummy = 0;

    A(int index)
    {
        Indices().emplace(this, index);
    }

    A(const A &) = delete;
    A &operator=(const A &) = delete;

    ~A()
    {
        auto it = Indices().find(this);
        std::cout << "Deleting #" << it->second << '\n';
        Indices().erase(it);
    }
};

int main()
{
    A a(3);

    A b(17);

    //
    // Attempting to do
    //
    //   A b{.dummy = 42};
    //
    // gives the
    // error: designated initializers cannot be used with a non-aggregate type ‘A’
    //
    // If the compiler enabled this, such initialization would bypass A(int)
    // constructor causing undefined behaviour in the destructor.
    //

    return 0;
}

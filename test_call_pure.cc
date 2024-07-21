/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

/*
 * To quote the following StackOverflow question:
 *
 *   - call to pure virtual function from base class constructor
 *     https://stackoverflow.com/questions/8630160/call-to-pure-virtual-function-from-base-class-constructor
 *
 * a1ex07 answered Dec 25, 2011 at 15:23
 *
 *    "In short, objects are constructed from the base up to the derived.
 *    So when you try to call a virtual function from the base class
 *    constructor, overriding from derived classes hasn't yet happened
 *    because the derived constructors haven't been called yet."
 *
 * Test:
 *
 *    $ ./test_call_pure
 *    pure virtual method called
 *    terminate called without an active exception
 *    Aborted (core dumped)
 *
 */

#include <iostream>

struct Base {
    Base();
    void init(int a);
    virtual bool foo(int a) = 0;
};

struct Derived: public Base {
    bool foo(int a) override;
};

Base::Base()
{
    init(0);
}

void Base::init(int a)
{
    foo(a);
}

bool Derived::foo(int a)
{
    return a > 0;
}

int main()
{
    Derived d;
    return 0;
}

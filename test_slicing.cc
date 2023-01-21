/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 */

#include <iostream>
#include <string>

struct A
{
    virtual std::string GetName() const {return "A";}
};

struct B: public A
{
    virtual std::string GetName() const {return "B";}
};

static void func1(A a)
{
    std::string name = a.GetName();
    std::cout << __func__ << ' ' << name << '\n';
}

int main()
{
    B b;
    func1(b);
}

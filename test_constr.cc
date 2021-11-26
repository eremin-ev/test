/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <fmt/core.h>
#include <fmt/format.h>

struct A {
    A(int x_, int y_) : x(x_), y(y_)
    {
        fmt::print("A::A() {}\n"
                   "\tx {}\n"
                   "\ty {}\n",
                   fmt::ptr(this), x, y);
    }

    A() : A(1, 2) {}

    ~A()
    {
        fmt::print("A::~A() {}\n"
                   "\tx {}\n"
                   "\ty {}\n",
                   fmt::ptr(this), x, y);
    }

    int get_x();
    int get_y();
    void set_x(int x_);
    void set_y(int y_);

private:
    int x;
    int y;
};

int A::get_x()
{
    fmt::print("{}: {}\n", __PRETTY_FUNCTION__, x);
    return x;
}

int A::get_y()
{
    fmt::print("{}: {}\n", __PRETTY_FUNCTION__, y);
    return y;
}

void A::set_x(int x_)
{
    fmt::print("{}: {}\n", __PRETTY_FUNCTION__, x_);
    this->x = x;
}

void A::set_y(int y_)
{
    fmt::print("{}: {}\n", __PRETTY_FUNCTION__, y_);
    this->y = y;
}

int bar()
{
    A().set_x(4);

    return A().get_x();
}

int foo(int arg)
{
    A a;
    int r;

    a.set_x(3);

    if (arg) {
        r = 0;
    } else {
        r = 1;
    }

    fmt::print("{}: before foo()\n", __PRETTY_FUNCTION__);
    r = bar();
    fmt::print("{}: after foo() r {}\n", __PRETTY_FUNCTION__, r);

    return r;
}

const A &baz()
{
    static A a(3, 7);

    return a;
}

int main(int argc, char **argv)
{
    int r;

    fmt::print("before A()\n");
    A();
    fmt::print("after A()\n");

    if (1 < argc) {
        int val = strtol(argv[argc - 1], NULL, 10);
        r = foo(val);
    } else {
        r = foo(0);
    }

    printf("---\n");

    const A &ref1 = baz();
    const A &ref2 = baz();

    fmt::print("ref1 {}, ref2 {}\n", fmt::ptr(std::addressof(ref1)),
                                     fmt::ptr(std::addressof(ref2)));

    return r;
}

/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * This is a subset of test_map.cc to make up a Minimal Working
 * Example for StackOverflow question.
 */

#include <map>
#include <iostream>

#define DEBUG
//#define MOVE_CONSTRUCTOR

struct A {
    explicit A(int nr_, const std::string &str_)
#ifdef DEBUG
    ;
#else
        : nr{nr_}, str{str_}
    {};
#endif

    explicit A(const A &other)
#ifdef DEBUG
    ;
#else
        : nr{other.nr}, str{other.str}
    {}
#endif

#ifdef MOVE_CONSTRUCTOR
    explicit A(A &&other);
#endif

    ~A()
#ifdef DEBUG
    ;
#else
    {}
#endif

    int nr;
    std::string str;
};

static std::ostream &operator<<(std::ostream &o, const A &a)
{
    return (o << " {" << a.nr << ", '" << a.str << "'}");
}

#ifdef DEBUG
A::A(int nr_, const std::string &str_) : nr{nr_}, str{str_}
{
    std::cout << __PRETTY_FUNCTION__ << " this " << this << *this << '\n';
}

A::A(const A &other) : nr{other.nr}, str{other.str}
{
    std::cout << __PRETTY_FUNCTION__ << " this " << this << *this << " other " << &other << other << '\n';
}
#endif

#ifdef MOVE_CONSTRUCTOR
A::A(A &&other) : nr{other.nr}, str{other.str}
{
    std::cout << __PRETTY_FUNCTION__ << " this " << this << *this << " other " << &other << other << '\n';
    other.nr = 0;
    other.str.clear();
}
#endif

#ifdef DEBUG
A::~A()
{
    std::cout << __PRETTY_FUNCTION__ << " this " << this << *this << '\n';
}
#endif

int main()
{
    std::map<std::string, A> amap;
    amap.insert(std::make_pair(std::string{"key1"}, A{1, std::string("one")}));

    //std::pair p2 = std::make_pair(std::string{"key2"}, A{2, std::string("two")});
    //std::cout << "ok hit\n";
    //amap.insert(std::move(p2));

    //A a3{3, "three"};
    //std::pair p3 = std::make_pair(std::string{"key3"}, std::move(a3));
    //std::cout<<"ok here\n";
    //amap.insert(std::move(p3));

    for (const auto &m : amap) {
        std::cout << m.first << m.second << '\n';
    }

	return 0;
}

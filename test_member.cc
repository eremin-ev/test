/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 */

#include <iostream>
#include <map>
#include <string>

struct A {
    A();

    void set1(int param);
    void set2(int param);
    void factor1(int factor);
    void factor2(int factor);

private:
    int m_data1;
    int m_data2;
};

using ptr_to_member = void (A::*)(int param);

const std::map <std::string, ptr_to_member> setter_map = {
    { "set1", &A::set1 },
    { "set2", &A::set2 },
    { "factor1", &A::factor1 },
    { "factor2", &A::factor2 },
};

A::A()
    : m_data1{0}
    , m_data2{0}
{}

void A::set1(int param)
{
    std::cout << __func__ << " data1 " << m_data1 << " -> " << param << '\n';

    m_data1 = param;
}

void A::set2(int param)
{
    std::cout << __func__ << " data2 " << m_data2 << " -> " << param << '\n';

    m_data2 = param;
}

void A::factor1(int factor)
{
    std::cout << __func__ << " data1 " << m_data1 << " -> " << m_data1*factor << '\n';

    m_data1 *= factor;
}

void A::factor2(int factor)
{
    std::cout << __func__ << " data2 " << m_data2 << " -> " << m_data2*factor << '\n';

    m_data2 *= factor;
}

static void test_ptr(A *a, const std::string &name, int param)
{
    const auto i = setter_map.find(name);
    if (i == setter_map.cend()) {
        std::cout << "method '" << name << "' not found\n";
        return;
    }

    const ptr_to_member p1 = i->second;
    const ptr_to_member p2 = nullptr;

    std::cout << __func__ << " p1 " << p1 << ", p2 " << p2 << '\n';

    std::cout << "key " << i->first << ", typeid(p) " << typeid(p1).name() << '\t';

    (a->*p1)(param);
}

int main()
{
    A a;

    a.set2(3);

    test_ptr(&a, "set1", 5);
    test_ptr(&a, "factor2", 7);
    test_ptr(&a, "factor5", 11);

    return 0;
}

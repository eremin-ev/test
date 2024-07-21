/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * - Placement new
 *   https://en.cppreference.com/w/cpp/language/new#Placement_new
 *
 * - What uses are there for "placement new"?
 *   https://stackoverflow.com/questions/222557/what-uses-are-there-for-placement-new
 */

#include <iostream>
#include <string>

struct Base final {
    Base();
    void init();
    void check() const;
    std::string put(const int a[], int nr) const;

private:
    static constexpr int m_nr = 10;
    int m_static_data[m_nr];
    int *m_data;
};

Base::Base()
    : m_static_data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
{
    std::cout << __func__ << " nr " << m_nr << '\n';
    std::cout << __func__ << " m_static_data " << m_static_data << ' ' << put(m_static_data, m_nr) << '\n';
}

void Base::init()
{
    m_data = new (m_static_data) int[m_nr];
    std::cout << __func__ << " m_data " << m_data << ' ' << put(m_data, m_nr) << '\n';
}

void Base::check() const
{
    std::cout << __func__ << ' ' << (m_data == m_static_data ? "ok" : "failed") << '\n';
}

std::string Base::put(const int a[], int nr) const
{
    std::string o;
    for (int i = 0; i < nr; ++i) {
        o += ' ' + std::to_string(a[i]);
    }
    return o;
}

int main()
{
    Base b;
    b.init();
    b.check();
    return 0;
}

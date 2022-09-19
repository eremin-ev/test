/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <iostream>
#include <list>
#include <string>

struct Inner {
    std::string m_one;
    bool m_two;
    //std::string m_three;
};

struct Outer {
    Inner m_inner;
    std::list<std::string> m_list;
};

int main()
{
    Outer outer {
        .m_inner {
            .m_one{"m_one"},
            .m_two{true},
            //.m_three{"m_three"},
        },
        .m_list {
            "theFirst",
            "theSecond",
        },
    };

    std::cout << outer.m_inner.m_one << ' ' << outer.m_inner.m_two << '\n';

    for (const auto &i : outer.m_list) {
        std::cout << i << '\n';
    }

    return 0;
}

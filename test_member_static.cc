/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <string>

#include <stdio.h>
#include <stdlib.h>

namespace {

static bool callStatic(const char *m1_str, int m1_len,
                       const char *m2_str, int m2_len)
{
    printf("%s '%s:%i' vs '%s:%i'\n", __func__,
           m1_str, m1_len, m2_str, m2_len);

    return m1_str == m2_str;
}

}

struct A {
    enum A_States {
        StateInit = 1,
        StateSet1,
        StateSet2,
        StateCall,
        StateFini,
    };

    A();
    void nextState(const std::string &s);

    A_States m_state;
    std::string m_member1;
    std::string m_member2;
};

A::A()
    : m_state{StateInit}
    , m_member1{}
    , m_member2{}
{ }

void A::nextState(const std::string &s)
{
    printf("%s %d\n", __func__, m_state);
    switch (m_state) {
    case StateInit:
        m_state = StateSet1;
        break;

    case StateSet1:
        m_member1 = s;
        m_state = StateSet2;
        break;

    case StateSet2:
        m_member2 = s;
        m_state = StateCall;
        break;

    case StateCall:
        {
            bool success = callStatic(m_member1.c_str(),
                                      // FIXME: size of <CharT>, need bytes
                                      m_member1.size(),
                                      m_member2.c_str(),
                                      // FIXME: size of <CharT>, need bytes
                                      m_member2.size());
            printf("%s success %d\n", __func__, success);
        }
        break;

    case StateFini:
        printf("%s error, should never happen\n", __func__);
        break;
    }
}

int main()
{
    std::string ss[] = {
        {"The first state"},
        {"The second state"},
        {"The third state"},
        {"The fourth state"},
        {"The fifth state"},
        {},
    };

    A a;
    int i = 0;
    while (a.m_state != A::StateFini && !ss[i].empty()) {
        a.nextState(ss[i]);
        ++i;
    }

    return EXIT_SUCCESS;
}

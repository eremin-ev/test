/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <map>
#include <iostream>

namespace Test2 {

struct A {
    A();
    explicit A(int nr, const std::string &value);
    explicit A(const A &other) = delete;
    explicit A(A &&other);
    ~A();
    A &operator=(const A &other);
    A &operator=(A &&other);

    // Or add nr() and str() getters to avoid 'operator<<' being a friend of A
    friend std::ostream &operator<<(std::ostream &o, const A &a);

private:
    void debug(const char *func, const char *hint, const A *a);

    int m_nr;
    std::string m_str;
};

A::A(int nr, const std::string &value)
    : m_nr{nr}
    , m_str{value}
{
    debug(__PRETTY_FUNCTION__, "this", this);
}

#if 0
A::A(const A &other)
    : m_nr{other.m_nr}
    , m_str{other.m_str}
{
    debug(__PRETTY_FUNCTION__, this);
}
#endif

A::A(A &&other)
    : m_nr{other.m_nr}
    , m_str{other.m_str}
{
    debug(__PRETTY_FUNCTION__, "this", this);
    debug(__PRETTY_FUNCTION__, "other", &other);
    other.m_nr = 0;
    other.m_str.clear();
}

A::A()
    : A(0, "")
{
    debug(__PRETTY_FUNCTION__, "this", this);
}

A::~A()
{
    debug(__PRETTY_FUNCTION__, "this", this);
}

A &A::operator=(const A &other)
{
	if (m_nr != other.m_nr) {
        m_nr = other.m_nr;
	}

	if (m_str != other.m_str) {
        m_str = other.m_str;
	}

	return *this;
}

A &A::operator=(A &&other)
{
    std::swap(m_nr, other.m_nr);
    std::swap(m_str, other.m_str);

    return *this;
}

void A::debug(const char *func, const char *hint, const A *a)
{
    std::cout << func << hint << ' ' << a << ' ' << *a << '\n';
}

std::ostream &operator<<(std::ostream &o, const A &a)
{
    o << " {nr " << a.m_nr << ", str '" << a.m_str << "'}";

    return o;
}

using AMap = std::map<std::string, A>;

#if 0
void test2_1(AMap &amap, const std::string &key, int nr, const std::string &str)
{
    A a{nr, str};

    amap.insert(std::make_pair(key, a));
}
#endif

#if 0
void test2_2(AMap &amap, const std::string &key, int nr, const std::string &str)
{
    amap.insert({key, {nr, str}});
    // This produces:
    //
    // error: converting to ‘const A’ from initializer list would use
    //        explicit constructor ‘A::A(int, const std::string&)’
    //
}
#endif

void test2_3(AMap &amap, const std::string &key, int nr, const std::string &str)
{
    amap.insert(std::make_pair(key, A{nr, str}));
}

void test2_4(AMap &amap, const std::string &key, int nr, const std::string &str)
{
    A a{nr, str};

    std::cout << "ok inserting " << key << a << '\n';

    amap.insert(std::make_pair(key, std::move(a)));
}

#if 0
void test2_5(AMap &amap, const std::string &key, int nr, const std::string &str)
{
    A a{nr, str};

    std::cout << "ok inserting" << key << a;

    amap.emplace(std::make_pair(key, std::move(a)));
}
#endif

void dump(AMap &amap)
{
    for (const auto &m : amap) {
        std::cout << m.first << m.second << '\n';
    }
}

} // Test2

void test1()
{
    std::map<const char *, int> m1 = {
        { "two",    2 },
        { "one",    1 },
        { "null",   0 },
        { "three",  3 },
        { "seven",  7 },
    };

    //std::map<std::string,int> m2 = {
    //  { "one",    1 },
    //  { "two",    2 }
    //};

    for (const auto &v : m1) {
        //std::cout << v.first << v.second << std::endl;
        printf("%s %i\n", v.first, v.second);
    }

    // Existing keys
    printf("value for 'three': %i\n", m1["three"]);
    printf("value for 'null': %i\n", m1["null"]);

    // Nonexistent key
    //printf("value for 'four': %i\n", m1["four"]);
    printf("value for 'four': %i\n", m1.find("four") == m1.end());
}

void test2()
{
    Test2::AMap amap;
    //Test2::test2_1(amap, "key1", 1, "one");
    //Test2::test2_2(amap, "key2", 2, "two");
    //Test2::test2_3(amap, "key3", 3, "three");
    Test2::test2_4(amap, "key4", 4, "four");
    //Test2::test2_5(amap, "key5", 5, "five");
    //amap.insert(std::make_pair("key6", Test2::A{6, "six"}));
    Test2::dump(amap);
}

int main()
{
    //test1();
    test2();

	return 0;
}

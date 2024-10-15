/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <algorithm>
#include <string>
#include <vector>

#include <stdio.h>

static std::string vec_dump(const std::vector<int> &v)
{
    std::string output{"{"};
    for (const auto &el : v) {
        output += std::to_string(el);
        output += ',';
    }
    output += "}";
    return output;
}

struct Inc {
private:
    int m_addend;
public:
    Inc(int a)
    : m_addend(a)
    {}

    int operator()(int value) const
    {
        return value + m_addend;
    }
};

static void test_inc()
{
    int a[] = { 1, 2, 3, 4, 5 };
    int n = sizeof(a) / sizeof(a[0]);
    int to_add = 7;

    std::transform(a, a + n, a, Inc{to_add});

    for (int i = 0; i < n; ++i) {
        printf("%s a[%d] %d\n", __func__, i, a[i]);
    }
}

static bool custom_cmp(int a, int b)
{
    return a < b;
}

static void test_cmp1()
{
    std::vector<int> vec = { 503, 87, 512, 61, 908, 170, 897, 275, 653, 426, 154, 509, 612, 677, 765, 703 };
    printf("%s input: %s\n", __func__, vec_dump(vec).c_str());
    std::sort(vec.begin(), vec.end(), custom_cmp);
    printf("%s output: %s\n", __func__, vec_dump(vec).c_str());
}

static void test_cmp2()
{
    std::vector<int> vec = { 503, 87, 512, 61, 908, 170, 897, 275, 653, 426, 154, 509, 612, 677, 765, 703 };
    printf("%s input: %s\n", __func__, vec_dump(vec).c_str());
    sort(vec.begin(), vec.end(), [](int a, int b) {
        return a < b;
    });
    printf("%s output: %s\n", __func__, vec_dump(vec).c_str());
}

struct Cmp {
    bool operator()(int a, int b)
    {
        return a < b;
    }

    int operator()(int a)
    {
        return -a;
    }
};

static void test_cmp3()
{
    std::vector<int> vec = { 503, 87, 512, 61, 908, 170, 897, 275, 653, 426, 154, 509, 612, 677, 765, 703 };
    printf("%s input: %s\n", __func__, vec_dump(vec).c_str());
    Cmp cmp;
    int n = 37;
    printf("%s cmp(%d) %d\n", __func__, n, cmp(n));
    sort(vec.begin(), vec.end(), cmp);
    printf("%s output: %s\n", __func__, vec_dump(vec).c_str());
}

int main()
{
    printf("--- test_inc ---\n");
    test_inc();

    printf("--- test_cmp1 ---\n");
    test_cmp1();

    printf("--- test_cmp2 ---\n");
    test_cmp2();

    printf("--- test_cmp3 ---\n");
    test_cmp3();
}

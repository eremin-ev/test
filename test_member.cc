/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 */

#include <iostream>
#include <map>
#include <string>
#include <variant>

struct A {
    A();

    void set1(int param);
    void set2(int param);
    void factor1(int factor);
    void factor2(int factor);

    using ptr_to_member = void (A::*)(int param);

private:
    int m_data1;
    int m_data2;
};

template <typename T, typename V>
struct Option {
    using SetOption = bool (T::*)(const V &value);
    SetOption setOption;
    V value;
};

template <typename T, typename V>
using Options = std::map<std::string, Option<T, V>>;

struct SomeData {
    using ValueType = std::variant<std::monostate, int, std::string>;

    const static std::string Value1;
    const static std::string Value2;

    ValueType getOption(const std::string &key);
    bool setOption(const std::string &key, const ValueType &value);
    void dumpState();

    bool setValue1(const ValueType &value);
    bool setValue2(const ValueType &value);

    Options<SomeData, ValueType> &cache;

    int value1;
    std::string value2;
};

const std::map <std::string, A::ptr_to_member> setter_map = {
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

const std::string SomeData::Value1 = "value1";
const std::string SomeData::Value2 = "value2";

bool SomeData::setValue1(const ValueType &value)
{
    value1 = std::get<int>(value);

    return true;
}

bool SomeData::setValue2(const ValueType &value)
{
    value2 = std::get<std::string>(value);

    return true;
}

SomeData::ValueType SomeData::getOption(const std::string &key)
{
    auto option = cache.find(key);
    if (option == cache.end()) {
        return std::monostate{};
    }

    return option->second.value;
}

bool SomeData::setOption(const std::string &key,
                         const ValueType &value)
{
    auto option = cache.find(key);
    if (option == cache.end()) {
        return false;
    }

    if (option->second.value == value) {
        return true;
    }

    Option<SomeData, ValueType>::SetOption setFunc = option->second.setOption;
    if (setFunc && !(this->*setFunc)(value)) {
        return false;
    }

    option->second.value = value;

    return true;
}

void SomeData::dumpState()
{
    std::cout << __func__ << " " << SomeData::Value1 << " " << value1 << '\n';
    std::cout << __func__ << " " << SomeData::Value2 << " '" << value2 << "'\n";
}

static void test_ptr(A *a, const std::string &name, int param)
{
    const auto i = setter_map.find(name);
    if (i == setter_map.cend()) {
        std::cout << "method '" << name << "' not found\n";
        return;
    }

    const A::ptr_to_member p1 = i->second;
    const A::ptr_to_member p2 = nullptr;

    std::cout << __func__ << " p1 " << p1 << ", p2 " << p2 << '\n';

    std::cout << "key " << i->first << ", typeid(p) " << typeid(p1).name() << '\t';

    (a->*p1)(param);
}

static void test1()
{
    A a;

    a.set2(3);

    test_ptr(&a, "set1", 5);
    test_ptr(&a, "factor2", 7);
    test_ptr(&a, "factor5", 11);
}

static void test2()
{
    Options<SomeData, SomeData::ValueType> someDataOptions = {
        {
            SomeData::Value1, {
                .setOption = &SomeData::setValue1,
                .value = -1,
            },
        }, {
            SomeData::Value2, {
                .setOption = &SomeData::setValue2,
                .value = std::string{"null"},
            },
        }
    };

    SomeData d {
        .cache = someDataOptions,
        .value1 = std::get<int>(someDataOptions[SomeData::Value1].value),
        .value2 = std::get<std::string>(someDataOptions[SomeData::Value2].value),
    };

    d.dumpState();
    d.setOption(SomeData::Value1, 123);
    d.setOption(SomeData::Value2, "abc");
    d.dumpState();

    int value1_1 = std::get<int>(someDataOptions[SomeData::Value1].value);
    std::cout << __func__ << " " << SomeData::Value1 << " v1 " << value1_1 << '\n';

    int value1 = std::get<int>(d.getOption(SomeData::Value1));
    std::cout << __func__ << " " << SomeData::Value1 << " v2 " << value1 << '\n';
}

int main()
{
    //test1();
    test2();
    return 0;
}

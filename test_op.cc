/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <iostream>

#include <fmt/core.h>
#include <fmt/format.h>

struct vec {
public:
    vec() = delete;

    vec(const std::string name = "", int x = 0) :
        m_name{name},
        m_x{x}
    {
        fmt::print("Constr {} from members {}\n", fmt::ptr(this), *this);
    };

    /*vec(const std::string name, const vec &v) :
        m_name(name),
        m_x(v.m_x),
    {
        std::cout << "Constr " << this <<
            " from const ref " << v << std::endl;
    };*/

    vec(const vec &v) :
        vec(v.m_name, v.m_x)
    {
        std::cout << "Constr " << this <<
            " from const ref " << v << std::endl;
    };

    const vec &operator=(const vec &rhs)
    {
        std::cout << "Assign " << this <<
            " return reference " << *this << " <- " << rhs << std::endl;
        m_name = m_name + "-copyof-" + rhs.m_name;
        m_x = rhs.m_x;
        return *this;
    };

    ~vec()
    {
        std::cout << "Destr " << this << " " << *this << std::endl;
    };

    const vec &operator += (const vec &v)
    {
        m_x += v.m_x;
        return *this;
    };

    const vec &operator += (int rhs)
    {
        m_x += rhs;
        return *this;
    };

    // prefix
    const vec &operator++()
    {
        std::cout << "Prefix ++" << this << " " << *this << std::endl;
        ++m_x;
        return *this;
    };

    // postfix
    const vec operator++(int)
    {
        std::cout << "Postfix " << this << " " << *this << "++" << std::endl;
        vec tmp{*this};
        ++(*this);
        return tmp;
    };

    std::ostream &print(std::ostream &out) const
    {
        out << m_name << "{" << m_x << "}";
        return out;
    };

    friend std::ostream &operator<<(std::ostream &out, const vec &v);

//private:
public:
    std::string m_name;
    int m_x;
};

const vec operator+(const vec &lhs, const vec &rhs)
{
    std::cout << "operator+(" << lhs << ", " << rhs << ")" << std::endl;
    vec tmp{lhs};
    tmp += rhs;
    return tmp;
}

/*const vec &operator+(vec &lhs, int rhs)
{
    std::cout << "operator+(" << lhs << ", " << rhs << ")" << std::endl;
    //vec tmp{lhs};
    //tmp += rhs;
    lhs += rhs;
    return lhs;
}*/

std::ostream &operator<<(std::ostream &out, const vec &v)
{
    return v.print(out);
}

// Deal with formatting
template<>
struct fmt::formatter<vec> {
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const vec &v, FormatContext &ctx)
    {
        return format_to(ctx.out(), "'{}'{{{}}}", v.m_name, v.m_x);
    }
};

void test_init_ref()
{
    vec k{"k"};
    vec l{k};
    std::cout << k << l << std::endl;
}

void test_op_sum1()
{
    vec a{"a", 1};
    vec c{"c"};
    c += a;
    std::cout << c << std::endl;
}

void test_op_sum2()
{
    vec a{"a", 1};
    vec b{"b", 3};
    vec c = a + b;
    //vec c = a + 7;
    //vec c = a + b + 7;
    //vec c{"c"};
    //c = a + b;
    //c += a;
    //c += b;
    //vec c{"c"};
    //c = a + b;
    //c += 2;
    std::cout << c << std::endl;
}

void test_assign()
{
    vec e{"e"};
    fmt::print("e {} {}\n", fmt::ptr(&e), e);

    vec f = vec("f");
    fmt::print("f {} {}\n", fmt::ptr(&f), f);

    vec g{"g"};

    g = f;
}

void test_inc()
{
    vec e{"e"};
    vec f = e++;

    fmt::print("f {} {}\n", fmt::ptr(&f), f);
    fmt::print("e {} {}\n", fmt::ptr(&e), e);

    ++e;
    std::cout << e << std::endl;
}

void test_assignment_pre_inc()
{
    vec f{"f"};
    vec g{"g"};
    f = ++g;
    std::cout << f << std::endl;
}

void test_assignment_post_inc()
{
    vec i{"i", 5};
    vec j{"j", 11};
    i = j++;
    std::cout << i << j << std::endl;
}

int main()
{
    test_assign();
    //test_op_sum1();
    //test_op_sum2();
    //test_inc();
    //test_pre_inc_assignment();
    //test_post_inc_assignment();
    //test_init_ref();

    return 0;
}

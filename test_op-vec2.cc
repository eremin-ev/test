/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <iostream>

struct vec {
public:
	vec() = delete;

	vec(const std::string name = "", int x = 0, int y = 0) :
		m_name{name},
		m_x{x},
		m_y{y}
	{
		std::cout << "Constr " << this <<
			" from members " << *this << std::endl;
	};

	/*vec(const std::string name, const vec &v) :
		m_name(name),
		m_x(v.m_x),
		m_y(v.m_y)
	{
		std::cout << "Constr " << this <<
			" from const ref " << v << std::endl;
	};*/

	vec(const vec &v) :
		vec(v.m_name, v.m_x, v.m_y)
	{
		std::cout << "Constr " << this <<
			" from const ref " << v << std::endl;
	};

	const vec &operator = (const vec &rhs)
	{
		std::cout << "Assign " << this <<
			" return reference " << *this << " <- " << rhs << std::endl;
		m_name = m_name + "-copyof-" + rhs.m_name;
		m_x = rhs.m_x;
		m_y = rhs.m_y;
		return *this;
	};

	~vec()
	{
		std::cout << "Destr " << this << " " << *this << std::endl;
	};

	vec &operator += (const vec &v)
	{
		m_x += v.m_x;
		m_y += v.m_y;
		return *this;
	};

	vec &operator += (const int a)
	{
		m_x += a;
		m_y += a;
		return *this;
	};

	vec &operator++()
	{
		++m_x;
		++m_y;
		std::cout << "Prefix ++" << *this << std::endl;
		return *this;
	};

	vec &operator++(int)
	{
		std::cout << "Postfix ++" << *this << std::endl;
		return ++(*this);
	};

	std::ostream &print(std::ostream &out) const
	{
		out << m_name << ": {" << m_x << "," << m_y << "}";
		return out;
	};

	friend std::ostream &operator<<(std::ostream &out, const vec &v);

private:
	std::string m_name;
	int m_x;
	int m_y;
};

const vec operator+(const vec &lhs, const vec &rhs)
{
	std::cout << "operator+(" << lhs << ", " << rhs << ")" << std::endl;
	vec tmp{lhs};
	//vec tmp{"tmp"};
	//tmp = lhs;
	tmp += rhs;
	return tmp;
}

std::ostream &operator<<(std::ostream &out, const vec &v)
{
	return v.print(out);
}

void test_init_ref()
{
	vec k{"k"};
	vec l{k};
	std::cout << k << l << std::endl;
}

void test_op_sum1()
{
	vec a{"a", 1, 2};
	vec c{"c"};
	c += a;
	std::cout << c << std::endl;
}

void test_op_sum2()
{
	vec a{"a", 1, 2};
	vec b{"b", 3, 4};
	vec c = a + b;
	//vec c{"c"};
	//c = a + b;
	//c += a;
	//c += b;
	//vec c{"c"};
	//c = a + b;
	//c += 2;
	std::cout << c << std::endl;
}

void test_inc()
{
	vec e{"e"};
	e++;
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
	vec i{"i", 5, 7};
	vec j{"j", 11, 13};
	i = j++;
	std::cout << i << j << std::endl;
}

int main()
{
	//test_op_sum1();
	test_op_sum2();
	//test_inc();
	//test_pre_inc_assignment();
	//test_post_inc_assignment();
	//test_init_ref();

	return 0;
}

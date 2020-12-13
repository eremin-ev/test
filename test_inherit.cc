/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <iostream>

template <typename T>
struct A {
	A<T>(T arg) : value{arg} {
		std::cout << "Construct A " << this << ", arg " << arg << std::endl;
	}

	T getA() { return value; }

private:
	T value;
};

struct B: A<int> {
	// Initializer list must be used
	B(int arg) : A(arg) {
		std::cout << "Construct B " << this << ", arg " << arg << std::endl;
	}

	int getB() { return getA(); };
};

struct C: B {
	// Initializer list must be used
	C(int arg) : B{arg} {
		std::cout << "Construct C " << this << ", arg " << arg << std::endl;
	}

	int getC() { return getB(); };
};

int main()
{
	A<int> a(10);
	std::cout << "--- a.getA(): " << a.getA() << std::endl;

	B b(20);
	std::cout << "--- b.getB(): " << b.getB() << std::endl;

	C c(30);
	std::cout << "--- c.getC(): " << c.getC() << std::endl;

	return 0;
}

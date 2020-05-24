/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <iostream>
using namespace std;

class A {
private:
	int i;
public:
	A(int);
	auto getI() {return i;}
};

A::A(int arg)
{
	i = arg;
	cout << "A's Constructor called, i: " << i << endl;
}

class B: A {
public:
	B(int);
	auto getII() {
		return getI();
	};
};

// Initializer list must be used
B::B(int x) : A(x)
{
	cout << "B's Constructor called, x: " << x << endl;
}

class C: B {
public:
	C(int);
	auto getIII() {
		return getII();
	};
};

// Initializer list must be used
C::C(int x) : B(x)
{
	cout << "C's Constructor called, x: " << x << endl;
}

int main()
{
	A a(10);
	B b(20);
	C c(30);
	cout << "a.getI(): " << a.getI() << endl;
	cout << "b.getII(): " << b.getII() << endl;
	cout << "c.getIII(): " << c.getIII() << endl;
	return 0;
}

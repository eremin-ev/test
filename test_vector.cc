/*
 * A simple C++ vector container test.
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <vector>
#include <iostream>
#include <algorithm>

void test_v1()
{
	std::vector<int> v1 = {
		2, 1, 0, 3, 7,
	};

	printf("sizeof(v1) %zi, v1.capacity %zu\n",
		sizeof(v1), v1.capacity());

	for (auto &v : v1) {
		std::cout << v << std::endl;
	}
}

void test_v2()
{
	std::vector<int> v2 = {
		26, 11, 31, 23, 29, 17,
	};

	printf("sizeof(v2) %zi, v2.capacity %zu\n",
		sizeof(v2), v2.capacity());

	printf("--- loop index ---\n");
	for (unsigned i = 0; i < v2.size(); i++) {
		printf("%p: %i\n", &v2[i], v2[i]);
	}

	printf("--- loop iter ---\n");
	for (std::vector<int>::iterator v = v2.begin(); v != v2.end(); ++v) {
		printf("%p: %i\n", (void *)&(*v), *v);
		//std::cout << *v << std::endl;
	}

	std::vector<int>::iterator c = std::find(v2.begin(), v2.end(), 31);

	printf("before sort c %p: %i\n", (void *)&(*c), *c);
	std::sort(v2.begin(), v2.end());
	printf("after sort c %p: %i\n", (void *)&(*c), *c);

	printf("--- std::sort ---\n");
	for (std::vector<int>::iterator v = v2.begin(); v != v2.end(); ++v) {
		printf("%p: %i\n", (void *)&(*v), *v);
		//std::cout << *v << std::endl;
	}
}

void test_v3()
{
	std::vector<double> v3 = {
		2.2, 1.1, 0.0, 3.3, 7.7,
	};

	printf("sizeof(v3) %zi, nr %zi, v3.capacity %zu\n",
		sizeof(v3), sizeof(v3)/sizeof(double), v3.capacity());
}

int main()
{
	test_v1();
	test_v2();
	test_v3();

	return 0;
}

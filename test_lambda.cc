/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <iostream>
#include <string>

int main()
{
	std::cout << []() ->std::string {
		return "Hello World 1!";
	}() << std::endl;

	[]{
		std::cout << "Hello World 2!" << std::endl;
	}();

	std::string result = [](const std::string &s) ->std::string {
		return "Hello World " + s;
	}("2!");
	std::cout << "Result: " << result << std::endl;

	result = [](const std::string &s) {
		return "Hello World " + s;
	}("3!");
	std::cout << "Result: " << result << std::endl;

	auto lambda = [](const std::string &s) {
		return "Hello World " + s;
	};

	std::cout << "typeid(lambda): " << typeid(lambda).name() << std::endl;

	std::cout << "Result " << lambda("7!") << std::endl;

	return 0;
}

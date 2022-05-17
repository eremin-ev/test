/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <list>
#include <iostream>
#include <fmt/core.h>
//#include <fmt/format.h>

template<typename T>
T adder(T v)
{
	return v;
}

template<typename T, typename... Args>
T adder(T first, Args... args)
{
	return first + adder(args...);
}

template<typename T>
void adderAppend(std::list<T> &list, T last)
{
    list.push_back(last);
}

template<typename T, typename... Args>
void adderAppend(std::list<T> &list, T first, Args... args)
{
    list.push_back(first);
	adderAppend(list, args...);
}

template<typename T, typename... Args>
std::list<T> adderList(T first, Args... args)
{
    std::list<T> list;
    list.push_back(first);
	adderAppend(list, args...);
    return list;
}

void printList(std::list<std::string> const &list)
{
    for (auto const &i: list) {
        fmt::print("{}, ", i);
    }
    fmt::print("\n");
}

int main()
{
	long sum1 = adder(1, 2, 3, 8, 7);

	std::string s1 = "x", s2 = "aa", s3 = "bb", s4 = "yy";
	std::string sum2 = adder(s1, s2, s3, s4);

    fmt::print("sum1 {}, sum2 {}\n", sum1, sum2);

    std::list<std::string> list = adderList(s1, s2, s3, s4);

    printList(list);

    return 0;
}

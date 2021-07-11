/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <iostream>
#include <functional>
#include <iomanip>
#include <string>

int main()
{
    std::string s = "Hello, world!";
    //std::size_t str_hash = std::hash<std::string>{}(s);
    std::hash<std::string> hash_obj;
    std::size_t str_hash = hash_obj(s);
    //std::hash<std::string> str_hash;
    std::cout << "hash(" << std::quoted(s) << ") = " << str_hash << '\n';
}

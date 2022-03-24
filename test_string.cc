/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <cstdlib>
#include <iostream>
#include <string>
#include <regex>

int main()
{
    const std::string s[] = {
        "i'm starting with something",
        "i'm not starting with something"
    };

    const std::regex re = std::regex("i'm starting.*$");

    for (const auto &ss : s) {
        bool r = std::regex_match(ss, re);
        std::cout << r << " " << ss << '\n';
    }

    return EXIT_SUCCESS;
}

/*
 * A simple C++ map container test.
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <map>
#include <iostream>

int main()
{
    std::map<const char *,int> m1 = {
        { "two",    2 },
        { "one",    1 },
        { "null",   0 },
        { "three",  3 },
        { "seven",  7 },
    };

    //std::map<std::string,int> m2 = {
    //  { "one",    1 },
    //  { "two",    2 }
    //};

    for (const auto &v : m1) {
        //std::cout << v.first << v.second << std::endl;
        printf("%s %i\n", v.first, v.second);
    }

    // Existing keys
    printf("value for 'three': %i\n", m1["three"]);
    printf("value for 'null': %i\n", m1["null"]);

    // Nonexistent key
    //printf("value for 'four': %i\n", m1["four"]);
    printf("value for 'four': %i\n", m1.find("four") == m1.end());

	return 0;
}

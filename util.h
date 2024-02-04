/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 */

#ifndef _UTIL_H
#define _UTIL_H

#include <string>
#include <vector>

template <typename T>
std::string dump_list(const std::vector<T> &input, char sep = ',')
{
    std::string output;
    bool first = true;
    for (const auto &i : input) {
        if (first) {
            output += '(';
        } else {
            output += sep;
        }
        output += i;
        first = false;
    }

    if (!first) {
        output += ')';
    }

    return output;
}

#endif /* _UTIL_H */

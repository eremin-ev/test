/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 */

#ifndef _LIB_UTIL_H
#define _LIB_UTIL_H

#include <iostream>
#include <iterator>
#include <vector>

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &v)
{
    std::ostream_iterator<T> oit(std::cout, ", ");
    //*oit = '{';
    std::copy(v.begin(), v.end(), oit);
    //*oit = '}';
    return os;
}

#endif  /* _LIB_UTIL_H */

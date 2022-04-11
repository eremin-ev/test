/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <iostream>

#include "ston.h"

int main()
{
    Ston *ston1 = Ston::instance();
    std::cout << ston1 << '\t' << ston1->count() << '\n';

    Ston *ston2 = Ston::instance();
    std::cout << ston2 << '\t' << ston2->count() << '\n';

    delete ston1;

    return 0;
}

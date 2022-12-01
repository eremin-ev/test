/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <iostream>

int main()
{
    int8_t a = 0b01011101;
    int8_t b = 0b10100011;  // two-complement

    // check 1: substract n-bit number from a 2^n
    int8_t c1 = 0b100000000 - a;

    // check 2: a simpler/faster equivalent of the subtraction:
    //          invert all bits and add 1
    int8_t c2 = ~a + 1;

    // check 3
    int8_t c3 = -a;

    int ia = a;
    int ib = b;

    std::cout << ia << ", negative is " << ib
              << ", check 1: " << (b == c1 ? "Ok" : "failed")
              << ", check 2: " << (b == c2 ? "Ok" : "failed")
              << ", check 3: " << (b == c3 ? "Ok" : "failed") << '\n';

    return 0;
}

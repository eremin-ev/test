/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <cstdint>
#include <iostream>

int main()
{
    int8_t a = 0b01011101;
    int8_t b = 0b10100011;  // two-complement

    // check 1: substract n-bit number from a 2^n
    int8_t c1 = static_cast<int8_t>((1U<<8*sizeof(c1)) - a);

    std::cout << sizeof(c1) << '\n';

    // check 2: 2^N-complement or just two's complement --
    //          a simpler/faster equivalent of the subtraction:
    //          invert all bits and add 1 so that the sum of
    //          these numbers is always equal to 2^N
    int8_t c2 = ~a + 1;

    // check 3
    int8_t c3 = -a;

    unsigned ia = static_cast<uint8_t>(a);
    unsigned ib = static_cast<uint8_t>(c2);

    std::cout << ia << ", negative is " << ib
              << ", check 1: " << (b == c1 ? "Ok" : "failed")
              << ", check 2: " << (b == c2 ? "Ok" : "failed")
              << ", check 3: " << (b == c3 ? "Ok" : "failed")
              << ", check 4: " << ((ia + ib) == (1U<<8*sizeof(c2)) ? "Ok" : "failed")
              << '\n';

    return 0;
}

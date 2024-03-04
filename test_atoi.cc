/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * 8. String to Integer (atoi) [1]
 *
 *  Medium
 *
 *  Topics: String
 *
 *  Companies
 *
 * Implement the myAtoi(string s) function, which converts a string to a
 * 32-bit signed integer (similar to C/C++'s atoi function).
 *
 * The algorithm for myAtoi(string s) is as follows:
 *
 * 1. Read in and ignore any leading whitespace.
 *
 * 2. Check if the next character (if not already at the end of the string)
 *    is '-' or '+'. Read this character in if it is either. This determines if
 *    the final result is negative or positive respectively. Assume the result
 *    is positive if neither is present.
 *
 * 3. Read in next the characters until the next non-digit character or the
 *    end of the input is reached. The rest of the string is ignored.
 *
 * 4. Convert these digits into an integer (i.e. "123" -> 123, "0032" -> 32).
 *    If no digits were read, then the integer is 0. Change the sign as
 *    necessary (from step 2).
 *
 * 5. If the integer is out of the 32-bit signed integer range
 *    [-2^31, 2^31 - 1], then clamp the integer so that it remains in the
 *    range. Specifically, integers less than -2^31 should be clamped to
 *    -2^31, and integers greater than 2^31 - 1 should be clamped to
 *    2^31 - 1.
 *
 * Note:
 *
 *   - Only the space character ' ' is considered a whitespace character.
 *
 *   - Do not ignore any characters other than the leading whitespace or
 *     the rest of the string after the digits.
 *
 * Constraints:
 *
 *    0 <= s.length <= 200
 *
 *    s consists of English letters (lower-case and upper-case),
 *    digits (0-9), ' ', '+', '-', and '.'.
 *
 *
 * Accepted 1.5M    Submissions 8.5M       Acceptance Rate 17.1%
 *
 * Topics   String
 *
 * Result:  Accepted
 *
 *    Evgeny Eremin submitted at Mar 02, 2024 02:09
 *
 *    Runtime      0ms      Beats 100.00% of users with C++
 *    Memory    7.84MB      Beats  71.18% of users with C++
 *
 * [1] https://leetcode.com/problems/string-to-integer-atoi/
 *
 */

#include <iostream>
#include <string>

#include <limits.h>

namespace {

} // anonymous namespace

class Solution {
public:
    int atoi(const std::string &str) const
    {
        int sign = 1;
        const char *s = str.c_str();

        while (*s && *s == ' ') {
            ++s;
        }

        if (*s) {
            switch (*s) {
            case '-':
                sign = -1;
                ++s;
                break;

            case '+':
                ++s;
                break;

            default:
                break;
            }
        }

        bool overflow = false;
        bool run = true;
        int n = 0;
        while (*s && run) {
            switch (*s) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                // n *= 10;
                if (__builtin_mul_overflow(n, 10, &n)) {
                    overflow = true;
                    run = false;
                    break;
                }
                //n += *s - '0';
                if (__builtin_add_overflow(n, *s - '0', &n)) {
                    overflow = true;
                    run = false;
                    break;
                }
                break;
            }

            default:
                run = false;
                break;
            }
            ++s;
        }

        if (overflow) {
            if (sign > 0) {
                return INT_MAX;
            } else {
                return INT_MIN;
            }
        }

        return sign*n;
    }
};

int main()
{
    const struct {
        std::string str;
        int num;
    } cases[] = {
        { .str = "123", .num = 123 },
        { .str = "  -123", .num = -123 },
        { .str = "  00", .num = 0 },
        { .str = "  -00", .num = 0 },
        { .str = "  -0032", .num = -32 },
        { .str = "-0032", .num = -32 },
        { .str = "-0", .num = 0 },
        { .str = "-  134", .num = 0 },
        { .str = "-13004", .num = -13004 },
        { .str = "  +13004", .num = 13004 },
        { .str = "-  13sdfg3244sd", .num = 0 },
        { .str = "  -013sdfg3244sd", .num = -13 },
        { .str = "- dfsdf  13sdfg3244sd", .num = 0 },
        { .str = "-239239829389285", .num = INT_MIN },
        { .str = "+239239829389285", .num = INT_MAX },
        { .str = "  +239239829389285", .num = INT_MAX },
        { .str = "  239239829389285", .num = INT_MAX },
        { .str = "-91283472332", .num = INT_MIN },
        { .str = "-90283472332", .num = INT_MIN },
        { .str = "-92283472332", .num = INT_MIN },
        { .str = "21474836460", .num = INT_MAX },
        { .str = "-6147483648", .num = INT_MIN },
        { .str = "2147483648", .num = INT_MAX },
        { .str = "- + - +23", .num = 0 },
    };

    Solution s;

    for (const auto &e : cases) {
        int n = s.atoi(e.str);
        std::cout << (e.num == n ? "ok" : "failed") << " "
                  << e.str << " " << e.num << " " << n
                  << '\n';
    }

    return 0;
}

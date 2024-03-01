/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * See
 *
 * [1] Detecting signed overflow in C/C++
 *     https://stackoverflow.com/questions/3944505/detecting-signed-overflow-in-c-c
 *
 * [2] Catch and compute overflow during multiplication of two large integers
 *     https://stackoverflow.com/questions/1815367/catch-and-compute-overflow-during-multiplication-of-two-large-integers
 * [3] Multiplication of very long integers
 *     https://stackoverflow.com/questions/193398/multiplication-of-very-long-integers
 *
 */

#include <climits>
#include <cstdint>
#include <iostream>

namespace {

static uint32_t hi(uint32_t x)
{
    return x >> 16;
}

static uint32_t lo(uint32_t x)
{
    return ((1U << 16) - 1) & x;
}

static void uint_sum(uint32_t a, uint32_t b, uint32_t *result, uint32_t *carry)
{
    uint32_t x0 = lo(a) + lo(b);
    uint32_t s0 = lo(x0);
    uint32_t cr = hi(x0);

    uint32_t x1 = hi(a) + hi(b) + cr;
    uint32_t s1 = lo(x1);

    cr = hi(x1);

    *result = (s1 << 16) | s0;
    *carry = cr;
}

/*
 * Example of long multiplication:
 *
 *      79
 *    x 86
 *      --   Step 1: calculate products
 *      54     x0 = 6*9 = 54
 *     42      x1 = 6*7 = 42
 *     72      x2 = 8*9 = 72
 *    56       x3 = 8*7 = 56
 *    ----   Step 2: Add hi part of the x0 to the x1
 *      04     s0 = lo(x0) = 04   : we've got final value of s0
 *     47      x1 = 6*7 + hi(x0) = 42 + 5 = 47
 *     72
 *    56
 *    ----   Step 3: Add lo part of x1 to x2
 *      04     s0 = 04
 *     40      x1 = 47
 *     79      x2 = 8*9 + lo(x1) = 72 + 7 = 79
 *    56
 *    ----   Step 4: get final value of s1
 *      04     s0 = 04
 *     40
 *     79      s1 = lo(x2) = 09
 *    56
 *    ----   Step 5: Add hi values of x1 and x2 to x3
 *      04     s0 = 04
 *     00
 *     09      s1 = lo(x2) = 09
 *    67       x3 = 8*7 + hi(x1) + hi(x2)
 *    ----   Step 6: Get final values of s2 and s3
 *      04     s0 = 04
 *     00
 *     09      s1 = 09
 *    67       s2 = lo(x3) = 07
 *             s3 = hi(x3) = 6
 *
 *    lo part of result = s1 s0 = 94
 *    hi part of result = s3 s2 = 67
 *
 *    full result = 6794.
 *
 */
static void uint_mul(uint32_t a, uint32_t b, uint32_t *result, uint32_t *carry)
{
    if (a == 0 || b == 0) {
        *result = 0;
        *carry = 0;
        return;
    }

    uint32_t a_lo = lo(a);
    uint32_t a_hi = hi(a);
    uint32_t b_lo = lo(b);
    uint32_t b_hi = hi(b);

    uint32_t x0 = a_lo * b_lo;

    uint32_t s0 = lo(x0);

    uint32_t x1 = a_lo * b_hi + hi(x0);
    uint32_t x2 = a_hi * b_lo + lo(x1);

    uint32_t s1 = lo(x2);

    uint32_t x3 = a_hi * b_hi + hi(x1) + hi(x2);

    uint32_t s2 = lo(x3);
    uint32_t s3 = hi(x3);

    *result = (s1 << 16) | s0;
    *carry = (s3 << 16) | s2;
}

static int test_sum_uint()
{
    struct {
        uint32_t a;
        uint32_t b;
    } cases[] = {
        { .a = 0, .b = 0, },
        { .a = 2, .b = 2, },
        { .a = 79, .b = 86, },
        { .a = UINT_MAX, .b = 4, },
        { .a = UINT_MAX, .b = 3, },
        { .a = UINT_MAX, .b = 2, },
        { .a = 7, .b = UINT_MAX, },
        { .a = UINT_MAX, .b = UINT_MAX, },
    };

    int errors = 0;

    for (const auto &c : cases) {
        uint32_t result;
        uint32_t carry;
        uint_sum(c.a, c.b, &result, &carry);
        uint64_t r1 = (static_cast<uint64_t>(carry) << 32) | result;
        uint64_t r2 = static_cast<uint64_t>(c.a) + c.b;
        bool r = r1 == r2;
        std::cout << (r ? "ok" : "failed") << " "
                  << c.a << " + " << c.b
                  << " = " << carry << " | " << result
                  << " = " << (r1)
                  << " = " << (r2)
                  << '\n';
        errors += !r;
    }

    return errors;
}

static int test_mul_uint()
{
    struct {
        uint32_t a;
        uint32_t b;
    } cases[] = {
        { .a = 0, .b = 0, },
        { .a = 2, .b = 2, },
        { .a = 79, .b = 86, },
        { .a = UINT_MAX, .b = 4, },
        { .a = UINT_MAX, .b = 3, },
        { .a = UINT_MAX, .b = 2, },
        { .a = 7, .b = UINT_MAX, },
        { .a = UINT_MAX, .b = UINT_MAX, },
    };

    int errors = 0;

    for (const auto &c : cases) {
        uint32_t result;
        uint32_t carry;
        uint_mul(c.a, c.b, &result, &carry);
        uint64_t r1 = (static_cast<uint64_t>(carry) << 32) | result;
        uint64_t r2 = static_cast<uint64_t>(c.a) * c.b;
        bool r = r1 == r2;
        std::cout << (r ? "ok" : "failed") << " "
                  << c.a << " x " << c.b
                  << " = " << carry << " | " << result
                  << " = " << (r1)
                  << " = " << (r2)
                  << '\n';
        errors += !r;
    }

    return errors;
}

static int test_mul_int()
{
    struct {
        int a;
        int b;
    } cases[] = {
        //{ .a = -6147483648, .b = 10, },
        { .a = -614748364, .b = 10, },
        { .a = -61474836, .b = 10, },
    };

    for (const auto &c : cases) {
        int r;
        if (__builtin_mul_overflow(c.a, c.b, &r)) {
            std::cout << __func__ <<  " overflow " << c.a << " x " << c.b << '\n';
        } else {
            std::cout << __func__ <<  " result " << c.a << " x " << c.b << " = " << r << '\n';
        }
    }

    return 0;
}

} // anonymous namespace

int main()
{
    int errors = 0;
    errors += test_sum_uint();
    errors += test_mul_uint();
    errors += test_mul_int();

    std::cout << "errors " << errors << '\n';

    return 0;
}

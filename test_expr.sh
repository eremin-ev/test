#!/bin/sh

case1()
{
    echo 'def f(x, y) y + x + 1;'
    echo 'f(43); f(1,3); f(4.55, 5.667)'
}

case2()
{
    echo 'def f(x, y) x - y;'
    echo 'f(23, 76);'
    echo 'f(4.55, 5.667)'
}

case3()
{
    echo 'def f(x, y) y - x - 1;'
    echo 'f(a, 767);'
}

case4()
{
    echo 'def f(x, y) y - x - 1;'
    echo 'a = 123'
    echo 'f(a, 767);'
}

case5()
{
    echo 'a;a'
}

make test_expr &&
case1 | ./test_expr
case2 | ./test_expr
case3 | ./test_expr
case4 | ./test_expr
case5 | ./test_expr

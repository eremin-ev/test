/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <iostream>
#include <stdio.h>

//bool bar(int a)
//{
//    return a > 0;
//}

bool foo(int a)
{
    //if (bar(a)) {
    //if (a > 0) {
    //    return true;
    //}

    printf("a %d\n", a);
}

int main(void)
{
    srand(time(nullptr));
    int a = rand()%10 - 5;

    if (!foo(a)) {
        printf("foo() returned false\n");
    }

    return 0;
}

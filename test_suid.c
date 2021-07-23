/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main (void)
{
    printf("Real UID:\t%d\n", getuid());
    printf("Effective UID:\t%d\n", geteuid());
    //seteuid(getuid());
    return 0;
}


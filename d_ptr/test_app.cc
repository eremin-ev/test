/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <iostream>

#include "label.h"

int main()
{
    Label *label = new Label("widget-geometry", "label-text");

    printf("%s label %p\n", __PRETTY_FUNCTION__, label);

    std::cout << label->geometry() << std::endl;
    //std::cout << label->stylesheet() << std::endl;
    std::cout << label->text() << std::endl;

    delete label;

    return 0;
}

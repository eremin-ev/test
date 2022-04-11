/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <cassert>
#include <iostream>

#include "ston.h"

Ston *Ston::sharedInstance = nullptr;

Ston::Ston()
    : m_count(1)
{
    assert(!sharedInstance);
    sharedInstance = this;
    std::cout << "Ston::Ston " << sharedInstance << '\n';
}

Ston *Ston::instance()
{
    std::cout << "Ston::instance " << sharedInstance << '\n';
    return sharedInstance ? sharedInstance : new Ston;
}

Ston::~Ston()
{
    std::cout << "Ston::~Ston " << sharedInstance << '\n';
    sharedInstance = nullptr;
}

int Ston::count()
{

    return m_count;
}

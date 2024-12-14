/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 */

//#include <algorithm>
#include <iostream>
#include <string>
#include <set>

namespace {


} // anonymous namespace

struct Element {
    //bool operator<(const Element &rhs) const;
    std::string name;
    std::string color;
    int priority;
};

using ElementPtr = const Element *;

struct ComparePrio {
    using is_transparent = void;

    bool operator()(const Element &e1, const Element &e2) const { return e1.priority < e2.priority; }
    bool operator()(int prio, const Element &e2) const { return prio < e2.priority; }
    bool operator()(const Element &e1, int prio) const { return e1.priority < prio; }
};

struct CompareColor {
    using is_transparent = void;

    bool operator()(const ElementPtr &e1, const ElementPtr &e2) const { return e1->color < e2->color; }
    bool operator()(const std::string &color, const ElementPtr &e2) const { return color < e2->color; }
    bool operator()(const ElementPtr &e1, const std::string &color) const { return e1->color < color; }
};

struct CompareName {
    using is_transparent = void;

    bool operator()(const ElementPtr &e1, const ElementPtr &e2 ) const { return e1->name < e2->name; }
    bool operator()(const std::string &name, const ElementPtr &e2 ) const { return name < e2->name; }
    bool operator()(const ElementPtr &e1, const std::string &name ) const { return e1->name < name; }
};

/*bool Element::operator<(const Element &rhs) const
{
    return priority < rhs.priority;
}*/

int main()
{
    std::set<Element, ComparePrio> list {
        Element{"orange", "orange", 50},
        Element{"banana", "yellow", 30},
        Element{"apple", "red", 40},
        Element{"topmost", "white", 10},
    };

    std::set<ElementPtr, CompareColor> by_color;
    std::set<ElementPtr, CompareName> by_name;

    for (const auto &e : list) {
        by_color.emplace(&e);
        by_name.emplace(&e);
    }

    constexpr int prio = 30;
    const auto i = list.find(prio);
    if (i != list.end()) {
        std::cout << "found " << i->priority << " " << i->color << " " << i->name << '\n';
    } else {
        std::cout << "not found " << prio << '\n';
    }

    const std::string color = "yellow";
    const auto j = by_color.find(color);
    if (j != by_color.end()) {
        std::cout << "found " << (*j)->priority << " " << (*j)->color << " " << (*j)->name << '\n';
    } else {
        std::cout << "not found " << color << '\n';
    }

    const std::string name = "banana";
    const auto k = by_name.find(name);
    if (k != by_color.end()) {
        std::cout << "found " << (*k)->priority << " " << (*k)->color << " " << (*k)->name << '\n';
    } else {
        std::cout << "not found " << name << '\n';
    }

    return 0;
}

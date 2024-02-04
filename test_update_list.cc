/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 */

#include "util.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace {

} // anonymous namespace

struct Element {
    std::string name;
    int priority;
};

class List {
public:
    explicit List(const std::vector<Element> &list);
    int mark(const std::vector<std::string> &new_value);
    std::vector<std::string> get_marked() const;
    bool set_current_as_default();
    bool set_current_as_disabled();
private:
    std::vector<Element> m_elements;
    unsigned m_marked;
    unsigned m_default_marked;
    unsigned m_disabled;
    unsigned m_max_size;
};

List::List(const std::vector<Element> &list)
    : m_elements{list}
    , m_marked{0}
    , m_default_marked{0}
    , m_disabled{0}
    , m_max_size{8*sizeof(m_marked)}
{
    std::cout << __func__ << " max size " << m_max_size << '\n';

    std::sort(m_elements.begin(), m_elements.end(), [](const Element &a, const Element &b) {
        if (a.priority < b.priority) {
            return true;
        }

        return a.name < b.name;
    });

    /*for (const auto &e : m_elements) {
        std::cout << __func__ << " element " << e.name << " prio " << e.priority << '\n';
    }*/
}

int List::mark(const std::vector<std::string> &new_value)
{
    int insert_count = 0;

    m_marked = 0;

    for (const auto &e1 : new_value) {
        const auto i = std::find_if(m_elements.begin(), m_elements.end(), [&e1](const Element &e2){
            return e1 == e2.name;
        });

        if (i == m_elements.end()) {
            std::cout << "\t..." << __func__ << " skip unavailable " << e1 << '\n';
            continue;
        }

        ptrdiff_t idx = i - m_elements.begin();
        if (idx >= m_max_size) {
            std::cout << __func__ << "max size reached\n";
            return insert_count;
        }

        //std::cout << __func__ << " marking element idx " << idx
        //          << " name " << i->name << " prio " << i->priority << '\n';

        m_marked |= 1U << idx;

        ++insert_count;
    }

    return insert_count;
}

std::vector<std::string> List::get_marked() const
{
    std::vector<std::string> output;
    unsigned marked = (m_marked | m_default_marked) & ~m_disabled;
    for (unsigned i = 0; i < m_max_size; ++i) {
        if (marked & (1U << i)) {
            const Element &e = m_elements.at(i);
            output.push_back(e.name);
        }
    }
    return output;
}

bool List::set_current_as_default()
{
    m_default_marked = m_marked;

    return !!m_default_marked;
}

bool List::set_current_as_disabled()
{
    m_disabled = m_marked;

    //std::cout << __func__ << " " << m_disabled << '\n';

    return !!m_disabled;
}

int main()
{
    List list{ {
            Element{"orange", 50},
            Element{"banana", 50},
            Element{"grape", 50},
            Element{"apple", 50},
            Element{"melon", 40},
            Element{"apricot", 40},
            Element{"secondmost", 2},
            Element{"topmost", 1},
        }
    };

    list.mark({"topmost", "secondmost"});
    list.set_current_as_default();

    list.mark({"grape"});
    list.set_current_as_disabled();

    const struct Input {
        const std::vector<std::string> input;
        const std::vector<std::string> expected;
    } inputs[] = {
        { {"apple", "topmost", "melon"}, {"topmost", "secondmost", "melon", "apple"} },
        { {"grape", "apple", "orange", "secondmost" }, {"topmost", "secondmost", "apple", "orange"} },
        { {"grape", "cucumber", "topmost"}, {"topmost", "secondmost"} },
        { {"grape", "apple", "tomato"}, {"topmost", "secondmost", "apple"} },
        { {"apple", "apricot"}, {"topmost", "secondmost", "apricot", "apple"} },
    };

    for (const auto &i : inputs) {
        list.mark(i.input);
        const auto marked = list.get_marked();
        std::cout << __func__
                  << " " << (marked == i.expected ? "ok" : "failed")
                  << " in " << dump_list(i.input)
                  << " out " << dump_list(marked)
                  << '\n';
    }

    return 0;
}

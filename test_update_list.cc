/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 */

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
    void show_marked();
private:
    std::vector<Element> m_elements;
    unsigned m_marked;
    unsigned m_max_size;
};

List::List(const std::vector<Element> &list)
    : m_elements{list}
    , m_marked{0}
    , m_max_size{8*sizeof(m_marked)}
{
    for (const auto &e : m_elements) {
        std::cout << __func__ << " element " << e.name << " prio " << e.priority << '\n';
    }
}

int List::mark(const std::vector<std::string> &new_value)
{
    int insert_count = 0;
    m_marked = 0;
    for (const auto &e1 : new_value) {
        const auto i = std::find_if(m_elements.begin(), m_elements.end(), [&e1](const Element &e2){
            return e1 == e2.name;
        });

        if (i != m_elements.end()) {
            ptrdiff_t idx = i - m_elements.begin();

            if (idx >= m_max_size) {
                std::cout << __func__ << "max size reached\n";
                return insert_count;
            }

            std::cout << __func__ << " marking element idx " << idx
                      << " name " << i->name << " prio " << i->priority << '\n';

            m_marked |= 1U << idx;

            ++insert_count;
        }
    }

    return insert_count;
}

void List::show_marked()
{
    for (unsigned i = 0; i < 8*sizeof(m_marked); ++i) {
        if (m_marked & (1U << i)) {
            const Element &e = m_elements.at(i);
            std::cout << __func__ << " " << i << " " << e.name << " " << e.priority << '\n';
        }
    }
}

int main()
{
    std::vector<Element> initial_list = {
        Element{"orange", 50},
        Element{"banana", 50},
        Element{"apple", 50},
        Element{"topmost", 1},
    };

    std::sort(initial_list.begin(), initial_list.end(), [](const Element &a, const Element &b){
        if (a.priority < b.priority) {
            return true;
        }

        return a.name < b.name;
    });

    List l{initial_list};

    const struct Input {
        std::vector<std::string> list;
    } inputs[] = {
        { {"orange", "topmost"} },
        { {"apple", "topmost"} },
    };

    for (const auto &i : inputs) {
        std::cout << "---\n";
        l.mark(i.list);
        l.show_marked();
    }

    return 0;
}

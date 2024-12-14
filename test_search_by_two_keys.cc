/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * See C++ Predefined macros [1] for `__cplusplus` constant values
 * (201402L for c++14, 201103L for c++11, and so on).
 *
 * See also Make object searchable with two different keys [2], What are
 * transparent comparators? [3] and an excellent (but not that upvoted)
 * answer to How can I make find() work with a set of structs? [4]
 * StackOverflow questions and answers.
 *
 * [1] https://en.cppreference.com/w/cpp/preprocessor/replace#Predefined_macros
 *
 * [2] Make object searchable with two different keys
 *     https://stackoverflow.com/questions/73868751/make-object-searchable-with-two-different-keys
 *
 * [3] What are transparent comparators?
 *     https://stackoverflow.com/questions/20317413/what-are-transparent-comparators/20318064
 *
 * [4] How can I make find() work with a set of structs?
 *     (and "I want find() to look only at one of the strings in the struct.")
 *     https://stackoverflow.com/questions/5865303/how-can-i-make-find-work-with-a-set-of-structs/58609219#58609219
 */

//#include <algorithm>
#include <iostream>
#include <string>
#include <set>

namespace {


} // anonymous namespace

struct CompareColor;
struct Element {
public:
    Element(const std::string &name, const std::string &color, int priority);
    std::string dump() const;

#if __cplusplus < 201402L
    // If only c++11 is available, go with constructing a temporary
    bool operator<(const Element &rhs) const { return m_name < rhs.m_name; }
#else
    // Transparent comparison requires c++14 or newer
    friend bool operator<(const Element &lhs, const std::string &name);
    friend bool operator<(const std::string &name, const Element &lhs);
    friend bool operator<(const Element &lhs, const Element &rhs);
#endif

private:
    friend CompareColor;

    std::string m_name;
    std::string m_color;
    int m_priority;
};

#if __cplusplus >= 201402L
// Transparent comparison requires c++14 or newer
bool operator<(const Element &lhs, const std::string &name) { return lhs.m_name < name; }
bool operator<(const std::string &name, const Element &rhs) { return name < rhs.m_name; }
bool operator<(const Element &lhs, const Element &rhs) { return lhs.m_name < rhs.m_name; }
#endif

Element::Element(const std::string &name, const std::string &color, int priority)
    : m_name{name}
    , m_color{color}
    , m_priority{priority}
{
    std::cout << __func__ << " p " << this << dump() << '\n';
}

std::string Element::dump() const
{
    return '{' + m_name + ',' + m_color + ',' + std::to_string(m_priority) + '}';
}

#if 0
struct CompareName {
    using is_transparent = void;

    bool operator()(const ElementPtr &e1, const ElementPtr &e2 ) const { return e1->name < e2->name; }
    bool operator()(const std::string &name, const ElementPtr &e2 ) const { return name < e2->name; }
    bool operator()(const ElementPtr &e1, const std::string &name ) const { return e1->name < name; }
};
#endif

#if __cplusplus >= 201402L
using ElementPtr = const Element *;

struct CompareColor {
    using is_transparent = void;

    bool operator()(const ElementPtr &lhs, const ElementPtr &rhs) const {
        //std::cout << __func__ << lhs->m_color << " " << rhs->m_color << '\n';
        return lhs->m_color < rhs->m_color; }
    bool operator()(const std::string &color, const ElementPtr &rhs) const {
        //std::cout << __func__ << color << " " << rhs->m_color << '\n';
        return color < rhs->m_color; }
    bool operator()(const ElementPtr &lhs, const std::string &color) const {
        //std::cout << __func__ << lhs->m_color << " " << color << '\n';
        return lhs->m_color < color; }
};
#endif

#if 0
struct ComparePriority {
    using is_transparent = void;

    bool operator()(const Element &e1, const Element &e2) const { return e1.priority < e2.priority; }
    bool operator()(int prio, const Element &e2) const { return prio < e2.priority; }
    bool operator()(const Element &e1, int prio) const { return e1.priority < prio; }
};
#endif

struct SearchSet {
public:
#if __cplusplus < 201402L
    // If only c++11 is available, go with constructing a temporary
    using ElementSet = std::set<Element>;
#else
    // Transparent comparison requires c++14 or newer
    using ElementSet = std::set<Element, std::less<>>;
    using ElementByColor = std::set<ElementPtr, CompareColor>;
#endif

    explicit SearchSet(const ElementSet &list);
    const Element *find(const std::string &name) const;
    const Element *find_by_color(const std::string &color) const;
    bool insert(const Element &element);

    //std::set<Element, ComparePriority> list;
    //std::set<ElementPtr, CompareName> by_name;
    //std::set<ElementPtr, ComparePriority> by_priority;

private:
    ElementSet m_list;
    ElementByColor m_by_color;
};

SearchSet::SearchSet(const ElementSet &list)
    : m_list{list}
{
    for (const auto &e : m_list) {
        m_by_color.emplace(&e);
    }
}

const Element *SearchSet::find(const std::string &name) const
{
#if __cplusplus < 201402L
    // If only c++11 is available, go with constructing a temporary
    const auto i = m_list.find(Element{name, "", 0});
#else
    // Transparent comparison requires c++14 or newer
    const auto i = m_list.find(name);
#endif
    if (i == m_list.end()) {
        return nullptr;
    }

    return &(*i);
}

const Element *SearchSet::find_by_color(const std::string &color) const
{
#if __cplusplus < 201402L
    return nullptr;
#else
    // Transparent comparison requires c++14 or newer
    const auto i = m_by_color.find(color);
    if (i == m_by_color.end()) {
        return nullptr;
    }

    return *i;
#endif
}

bool SearchSet::insert(const Element &element)
{
    const std::pair<ElementSet::iterator, bool> r = m_list.emplace(element);
    if (!r.second) {
        std::cout << "refuse duplicating " << element.dump() << '\n';
        return false;
    }

    const std::pair<ElementByColor::iterator, bool> r2 = m_by_color.emplace(&element);

    return r2.second;
}

#if 0
void SearchSet::init_by_priority()
{
    for (const auto &e : m_list) {
        by_priority.emplace(&e);
    }
}

bool SearchSet::test_search_by_prio(int prio)
{
    constexpr int prio = 30;
    const auto i = list.find(prio);
    if (i == list.end()) {
        std::cout << "not found " << prio << '\n';
        return false;
    }

        std::cout << "found " << i->priority << " " << i->color << " " << i->name << '\n';
}
#endif

static bool test_search_by_name(const SearchSet &e, const std::string &name)
{
    const auto k = e.find(name);
    if (!k) {
        std::cout << "not found " << name << '\n';
        return false;
    }

    std::cout << "found " << k->dump() << '\n';

    return true;
}

static bool test_search_by_color(const SearchSet &e, const std::string &color)
{
    const auto i = e.find_by_color(color);
    if (!i) {
        std::cout << "not found " << color << '\n';
        return false;
    }

    std::cout << "found " << i->dump() << '\n';

    return true;
}

int main(int argc, char *argv[])
{
    SearchSet e{{
        Element{"orange", "orange", 50},
        Element{"banana", "yellow", 30},
        Element{"apple", "red", 40},
        Element{"topmost", "white", 10},
    }};

    e.insert(Element{"grape", "green", 60});
    e.insert(Element{"grape", "black", 70});

    if (argc > 1) {
        test_search_by_name(e, argv[1]);
        test_search_by_color(e, argv[1]);
    }


    //e.init_by_prio();
    //e.test_search_by_prio(30);

    return 0;
}

/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <algorithm>
#include <iostream>
#include <list>
#include <string>

struct Item {
    std::string name;
    std::string descr;
};

template <typename T>
struct ItemList {
public:
    ItemList();
    explicit ItemList(const std::list<T> &list);

    const ItemList &operator=(const ItemList &other);
    const ItemList &operator=(const std::list<T> &list);

    void reset();
    void reset(const std::list<T> &list);
    bool at_end() const;
    const T *current() const;
    const typename std::list<T>::const_iterator &iter() const;
    bool next();
    bool set_to(const std::list<T> &list, const std::string &name);

private:
    const std::list<T> *m_list;
    typename std::list<T>::const_iterator m_iter;
};

template <typename T>
ItemList<T>::ItemList()
    : m_list{nullptr}
    , m_iter{}
{}

template <typename T>
ItemList<T>::ItemList(const std::list<T> &list)
    : m_list{&list}
    , m_iter{m_list->cbegin()}
{}

// const ref protects against (x=y).nonConstMember()
template <typename T>
const ItemList<T> &ItemList<T>::operator=(const ItemList &other)
{
    m_list = other.m_list;
    if (m_list) {
        m_iter = m_list->cbegin();
    }

    return *this;
}

template <typename T>
const ItemList<T> &ItemList<T>::operator=(const std::list<T> &list)
{
    m_list = &list;
    m_iter = m_list->cbegin();

    return *this;
}

template <typename T>
void ItemList<T>::reset()
{
    if (!m_list) {
        return;
    }

    m_iter = m_list->cbegin();
}

template <typename T>
void ItemList<T>::reset(const std::list<T> &list)
{
    m_list = &list;
    m_iter = m_list->cbegin();
}

template <typename T>
bool ItemList<T>::at_end() const
{
    if (!m_list) {
        return true;
    }

    return m_iter == m_list->cend();
}

template <typename T>
const T *ItemList<T>::current() const
{
    if (at_end()) {
        return nullptr;
    }

    return &(*m_iter);
}

template <typename T>
const typename std::list<T>::const_iterator &ItemList<T>::iter() const
{
    return m_iter;
}

template <typename T>
bool ItemList<T>::next()
{
    if (!m_list) {
        return false;
    }

    if (m_iter == m_list->cend()) {
        return false;
    }

    ++m_iter;

    return m_iter != m_list->cend();
}

template <typename T>
bool ItemList<T>::set_to(const std::list<T> &list, const std::string &name)
{
    auto i = std::find_if(list.cbegin(), list.cend(), [name](const T &el){
        // FIXME how do this generic?
        return el.name == name;
    });

    m_list = &list;

    if (i == list.cend()) {
        m_iter = list.cbegin();
        return false;
    }

    m_iter = i;

    return true;
}

const Item *getCurrent(const std::list<Item> &list, std::list<Item>::const_iterator i)
{
    if (i == list.cend()) {
        return nullptr;
    }

    return &(*i);
}

std::string dump_el(const Item *i, bool at_end)
{
    return '(' + i->name + ',' + i->descr + "," + (at_end ? "true" : "false") + ')';
}

bool check(const std::string &info, ItemList<Item> &f, const std::list<Item> &reference)
{
    bool ok = true;

    std::cout << __func__ << ' ' << info << '\t';

    auto it = reference.cbegin();

    while (1) {
        const Item *i = f.current();
        const Item *r = getCurrent(reference, it);

        if (i != r) {
            std::cout << "\tItems don't match (1)\n";
            ok = false;
        }

        if (!i) {
            std::cout << "(noCurrent)\n";
            break;
        }

        if (!r) {
            std::cout << "(no next reference item)\n";
            break;
        }

        std::cout << dump_el(i, f.at_end());

        if (i != r) {
            std::cout << "\tItems don't match (2)\n";
            ok = false;
        }

        ++it;

        if (!f.next()) {
            std::cout << "(Eol)\n";
            break;
        }
    };

    std::cout << "result: " << (ok ? "true" : "false") << '\n' << '\n';

    return true;
}

int main()
{
    std::list<Item> available1 {
        { "first", "First Description", },
        { "second", "Second Summary", },
        { "third", "Third Outline", },
    };

    std::list<Item> available2 {
        { "apple", "Description of the apple", },
        { "orange", "Details on the orange", },
        { "grape", "About the grape", },
    };

    ItemList<Item> items1(available1);
    check("items1", items1, available1);

    ItemList<Item> items2{available2};
    check("items2", items2, available2);

    items1 = available2;
    check("items1", items1, available2);

    items2.reset(available1);
    check("items2", items2, available1);

    std::list<Item> empty{};
    ItemList<Item> items3{empty};
    check("items3", items3, empty);

    ItemList<Item> items4{};
    check("items4", items4, empty);

    items4 = items3 = available1;
    check("items3", items3, available1);
    check("items4", items4, available1);

    items3 = items2 = items1;
    check("items2", items2, available2);
    check("items3", items3, available2);

    items4.reset();
    check("items4", items4, available1);

    items2 = items2;
    check("items2", items2, available2);

    bool success = items2.set_to(available1, "second");
    if (success) {
        std::cout << "Ok, found and set to " << dump_el(items2.current(), items2.at_end()) << '\n';
        items2.next();
        std::cout << "next, is " << dump_el(items2.current(), items2.at_end()) << '\n';
    }

    return 0;
}

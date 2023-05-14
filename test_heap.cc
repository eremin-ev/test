/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * Priority queue impl using a heap, see Sedgewick p. 344
 */

#include <vector>
#include <iostream>

template <typename Item>
struct PQ final {
private:
    Item *m_pq;
    int m_Nmax;
    int m_Ncur;

public:
    explicit PQ(int N);
    explicit PQ(const std::vector<Item> &a);
    ~PQ();
    bool empty() const;
    bool insert(const Item &item);
    Item getMax();
    Item peekMax() const;
};

template <typename Item>
void exch(Item &a, Item &b)
{
    Item t = a;
    a = b;
    b = t;
}

template <typename Item>
void fixUp(Item a[], int k)
{
    while (k > 1 && a[k/2] < a[k]) {
        exch(a[k/2], a[k]);
        k = k/2;
    }
}

template <typename Item>
void fixDown(Item a[], int k, int N)
{
    while (2*k <= N) {
        int j = 2*k;
        if (a[j] < a[j + 1] && j < N) {
            ++j;
        }
        if (a[k] < a[j]) {
            exch(a[k], a[j]);
        }
        k = j;
    }
}

template <typename Item>
PQ<Item>::PQ(int N)
    : m_pq{new Item[N + 1]}
    , m_Nmax{N}
    , m_Ncur{0}
{
}

template <typename Item>
PQ<Item>::PQ(const std::vector<Item> &A)
    : PQ{static_cast<int>(A.size())}
{
    for (const auto &a : A) {
        insert(a);
    }
}

template <typename Item>
PQ<Item>::~PQ()
{
    if (m_pq) {
        delete[] m_pq;
    }
}

template <typename Item>
bool PQ<Item>::empty() const
{
    return m_Ncur == 0;
}

template <typename Item>
bool PQ<Item>::insert(const Item &item)
{
    if (m_Ncur >= m_Nmax) {
        std::cout << __func__ << " cannot add " << item << '\n';
        return false;
    }

    m_pq[++m_Ncur] = item;
    fixUp(m_pq, m_Ncur);
    return true;
}

template <typename Item>
Item PQ<Item>::getMax()
{
    exch(m_pq[1], m_pq[m_Ncur]);
    fixDown(m_pq, 1, m_Ncur);
    return m_pq[m_Ncur--];
}

template <typename Item>
Item PQ<Item>::peekMax() const
{
    return m_pq[1];
}

int main()
{
    PQ<int> pq(10);

    pq.insert(3);
    pq.insert(1);
    pq.insert(5);
    pq.insert(23);
    pq.insert(33);
    pq.insert(73);
    pq.insert(17);
    pq.insert(34);
    pq.insert(29);
    pq.insert(45);
    pq.insert(21);
    pq.insert(53);
    pq.insert(45);

    std::cout << "max: " << pq.peekMax() << '\n';

    PQ<int> pq2({3, 4, 45, 34, 35, 83, 13});

    std::cout << "max2: " << pq2.peekMax() << '\n';

    return 0;
}

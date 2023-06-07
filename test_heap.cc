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
void fixDown0(Item a[], int k, int N)
{
    if (k == 0 && N > 1) {
        int j = 1;
        if (a[j + 1] < a[j] && j < N - 1) {
            ++j;
        }
        if (a[j] < a[0]) {
            exch(a[j], a[0]);
        }
        k = j;
    }

    while (2*k < N) {
        int j = 2*k;
        if (a[j + 1] < a[j] && j < N - 1) {
            ++j;
        }
        if (a[j] < a[k]) {
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

namespace {

static void test_pq()
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
}

static void merge(long long a1[], long long a2[], int n, int m)
{
    for (int i = 0; i < n; ++i) {
        if (a2[0] < a1[i]) {
            exch(a1[i], a2[0]);
            if (m > 1) {
                fixDown0(a2, 0, m);
            }
        }
    }
}

static void ins_sort(long long a[], int n)
{
    for (int i = 1; i < n; ++i) {
        const auto tmp = a[i];
        int j = i - 1;
        while (j >= 0 && tmp < a[j]) {
            a[j + 1] = a[j];
            --j;
        }
        a[j + 1] = tmp;
    }
}

static void dump(long long *a, int n)
{
    for (int i = 0; i < n; ++i) {
        std::cout << a[i] << ' ';
    }
    std::cout << '\n';
}

static void test_merge()
{
    int n = 4, m = 5;
    long long a1[] = {3, 22, 55, 777};
    long long a2[] = {0, 2, 8, 102, 113};
    merge(a1, a2, n, m);
    ins_sort(a2 + 1, m - 1);
    dump(a1, n);
    dump(a2, m);
}

} // anonymous namespace

int main()
{
    test_pq();
    test_merge();

    return 0;
}

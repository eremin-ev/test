/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <QDebug>
#include <QVector>
#include <QString>

#define USER_DEFINED_DESTR  1

struct VectorData {
    VectorData();
    VectorData(int x, int y);
    ~VectorData();
    int m_x;
    int m_y;
};

struct VectContainer {
    VectContainer();
    void getState();
#ifdef USER_DEFINED_DESTR
    ~VectContainer();
#else
    ~VectContainer() = default;
#endif

private:
    QVector<VectorData> m_items;
};

VectContainer::VectContainer()
    : m_items{ {111,222}, {333,444}, {555,666} }
{
    qDebug() << __PRETTY_FUNCTION__ << this;
}

#ifdef USER_DEFINED_DESTR
VectContainer::~VectContainer()
{
    qDebug() << __PRETTY_FUNCTION__;
    //qDeleteAll(m_items);
    //m_items.clear();
}
#endif

void VectContainer::getState()
{
    for (const auto &item : m_items) {
        qDebug() << __PRETTY_FUNCTION__ << item.m_x << item.m_y;
    }
}

VectorData::VectorData()
{
    qDebug() << __PRETTY_FUNCTION__ << this << this->m_x << this->m_y;
}

VectorData::VectorData(int x, int y)
    : m_x(x)
    , m_y(y)
{
    qDebug() << __PRETTY_FUNCTION__ << this << this->m_x << this->m_y;
}

VectorData::~VectorData()
{
    qDebug() << __PRETTY_FUNCTION__ << this;
}

void test_1()
{
    VectContainer v;

    v.getState();
}

int main()
{
    test_1();

    return 0;
}

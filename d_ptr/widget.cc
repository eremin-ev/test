#include "widget.h"
#include "widget_p.h"

WidgetPrivate::WidgetPrivate(Widget *q)
    : q_ptr(q)
{
    qDebug("%s this %p, q_ptr %p", __PRETTY_FUNCTION__, this, q_ptr);
}

WidgetPrivate::~WidgetPrivate()
{
    qDebug("%s this %p, q_ptr %p", __PRETTY_FUNCTION__, this, q_ptr);
}

// Create private data.
// Pass the 'this' pointer to initialize the q-ptr
Widget::Widget()
    : d_ptr(new WidgetPrivate(this))
{
    qDebug("%s this %p (%zi), new d_ptr %p", __PRETTY_FUNCTION__, this, sizeof(*this), d_ptr);
}

Widget::~Widget()
{
    qDebug("%s this %p, delete d_ptr %p", __PRETTY_FUNCTION__, this, d_ptr);
    delete d_ptr;
}

QRect Widget::geometry() const
{
    // the d-ptr is only accessed in the library code
    return d_ptr->geometry;
}

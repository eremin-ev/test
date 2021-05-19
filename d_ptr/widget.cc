/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include "widget.h"
#include "widget_p.h"

WidgetPrivate::WidgetPrivate(Widget *q, const std::string &geometry)
    : q_ptr(q)
    , m_geometry(geometry)
    //, stylesheet("base-stylesheet")
{
    printf("%s this %p, sizeof %zi, q_ptr %p, m_geometry %s\n", __PRETTY_FUNCTION__,
           this, sizeof(*this), q_ptr, m_geometry.c_str());
}

WidgetPrivate::~WidgetPrivate()
{
    printf("%s this %p, q_ptr %p\n", __PRETTY_FUNCTION__, this, q_ptr);
}

#if 0
Widget::Widget()
    : d_ptr(new WidgetPrivate(this))
{
    printf("%s this %p, sizeof %zi, new d_ptr %p\n", __PRETTY_FUNCTION__, this, sizeof(*this), d_ptr);
}
#endif

Widget::Widget(WidgetPrivate &d)
    : d_ptr(&d)
{
    printf("%s this %p, sizeof %zi, new d_ptr %p\n", __PRETTY_FUNCTION__, this, sizeof(*this), d_ptr);
}

#if 1
Widget::~Widget()
{
    printf("%s this %p, delete d_ptr %p\n", __PRETTY_FUNCTION__, this, d_ptr);
    delete d_ptr;
}
#endif

std::string Widget::geometry() const
{
    return d_ptr->m_geometry;
}

/*std::string Widget::stylesheet() const
{
    return d_ptr->m_stylesheet;
}*/

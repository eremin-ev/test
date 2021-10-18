/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <iostream>

#include "widget_p.h"
#include "label.h"

struct LabelPrivate : public WidgetPrivate {
    explicit LabelPrivate(Label *q, const std::string &geometry, const std::string &text);
    ~LabelPrivate();
    std::string m_text;
};

LabelPrivate::LabelPrivate(Label *q, const std::string &geometry, const std::string &text)
    : WidgetPrivate(q, geometry)
    , m_text(text)
{
    printf("%s this %p (sizeof %zu), q_ptr %p, m_text %s\n", __PRETTY_FUNCTION__,
           this, sizeof(*this), q_ptr, m_text.c_str());
}

LabelPrivate::~LabelPrivate()
{
    printf("%s this %p, q_ptr %p\n", __PRETTY_FUNCTION__, this, q_ptr);
}

Label::Label(const std::string &geometry, const std::string &text)
    : Widget(*new LabelPrivate(this, geometry, text))
{
    printf("%s this %p (sizeof %zi), new d_ptr %p\n", __PRETTY_FUNCTION__,
           this, sizeof(*this), d_ptr);
}

#if 0
Label::Label(const std::string &text)
    : d_ptr(*new LabelPrivate(this, text))
{
    printf("%s this %p (sizeof %zi), new d_ptr %p\n", __PRETTY_FUNCTION__,
           this, sizeof(*this), d_ptr);
    std::cout << geometry();
    std::cout << stylesheet();
    std::cout << text();
}
#endif

Label::~Label()
{
    printf("%s this %p, delete d_ptr %p\n", __PRETTY_FUNCTION__, this, d_ptr);
    //delete d_ptr;
}

void Label::setText(const std::string &text)
{
    T_D(Label);
    printf("%s d_ptr %p\n", __PRETTY_FUNCTION__, d);
    if (d->m_text != text) {
        d->m_text = text;
    }
}

std::string Label::text() const
{
    T_D(const Label);
    printf("%s d_ptr %p\n", __PRETTY_FUNCTION__, d);
    return d->m_text;
}

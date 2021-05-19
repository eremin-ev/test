#include "label.h"

// Unlike WidgetPrivate, the author decided LabelPrivate
// to be defined in the source file itself
struct LabelPrivate {
    LabelPrivate(Label *q);
    ~LabelPrivate();
    Label *q_ptr;
    QString text;
};

LabelPrivate::LabelPrivate(Label *q)
    : q_ptr(q)
{
    qDebug("%s this %p, q_ptr %p", __PRETTY_FUNCTION__, this, q_ptr);
}

LabelPrivate::~LabelPrivate()
{
    qDebug("%s this %p, q_ptr %p", __PRETTY_FUNCTION__, this, q_ptr);
}

Label::Label()
    : d_ptr(new LabelPrivate(this))
{
    qDebug("%s this %p (%zi), new d_ptr %p", __PRETTY_FUNCTION__, this, sizeof(*this), d_ptr);
}

Label::~Label()
{
    qDebug("%s this %p, delete d_ptr %p", __PRETTY_FUNCTION__, this, d_ptr);
    delete d_ptr;
}

QString Label::text() const
{
    return d_ptr->text;
}

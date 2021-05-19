#ifndef WIDGET_P_H
#define WIDGET_P_H

#include "widget.h"

struct WidgetPrivate {
    // Constructor that initializes the q-ptr
    WidgetPrivate(Widget *q);
    ~WidgetPrivate();

    // q-ptr points to the API class
    Widget *q_ptr;

    QRect geometry;
    QString stylesheet;
};

#endif  /* WIDGET_P_H */

#ifndef WIDGET_H
#define WIDGET_H

#include <QDebug>
#include <QRect>
#include <QString>

class WidgetPrivate;
class Widget {
public:
    Widget();
    ~Widget();
    QRect geometry() const;

private:
    WidgetPrivate *d_ptr;
};

#endif  /* WIDGET_H */

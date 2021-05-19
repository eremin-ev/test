/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#ifndef WIDGET_H
#define WIDGET_H

#include <string>

class WidgetPrivate;
class Widget {
public:
    //Widget();
    ~Widget();

    std::string geometry() const;
    //std::string stylesheet() const;

protected:
    Widget(WidgetPrivate &d);
    WidgetPrivate *d_ptr;
};

#endif  /* WIDGET_H */

/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#ifndef WIDGET_P_H
#define WIDGET_P_H

#include "widget.h"

struct WidgetPrivate {
    WidgetPrivate(Widget *q, const std::string &geometry);
    ~WidgetPrivate();

    Widget *q_ptr;

    std::string m_geometry;
    //std::string m_stylesheet;
};

#endif  /* WIDGET_P_H */

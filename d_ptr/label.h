/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#ifndef LABEL_H
#define LABEL_H

#include "widget.h"

class Label : public Widget {
public:
    explicit Label(const std::string &geometry, const std::string &text);
    ~Label();

    std::string text() const;
};

#endif  /* LABEL_H */

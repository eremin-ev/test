#ifndef LABEL_H
#define LABEL_H

#include "widget.h"

class LabelPrivate;
class Label : public Widget {
public:
    Label();
    ~Label();
    QString text() const;

private:
    LabelPrivate *d_ptr;
};

#endif  /* LABEL_P_H */

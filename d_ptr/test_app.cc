#include <QString>

#include "label.h"

int main()
{
    Label *label = new Label;

    qDebug("%s label %p", __PRETTY_FUNCTION__, label);

    delete label;

    return 0;
}

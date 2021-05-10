#ifndef CALC_H
#define CALC_H

#include <QObject>
#include <QDBusAbstractAdaptor>

#include "addr.h"

class Calc;
class CalcAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", CALC_DBUS_INTERFACE)

public:
    explicit CalcAdaptor(Calc *calc);

public slots:
    double multiply(double factor0, double factor2);
    double divide(double divident, double divisor);

signals:
	void newProduct(double product);
	void newQuotient(double quotient);

private:
    Calc *m_calc;
};

class Calc : public QObject {
    Q_OBJECT

public:
	Calc(QObject *parent);
	virtual ~Calc();

private:
    CalcAdaptor *m_adaptor;
};

#endif  /* CALC_H */

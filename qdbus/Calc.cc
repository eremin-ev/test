#include <QDebug>
#include <QDBusConnection>

#include "Calc.h"
#include "addr.h"

CalcAdaptor::CalcAdaptor(Calc *calc)
    : QDBusAbstractAdaptor(calc)
    , m_calc(calc)
{
    qDebug() << __PRETTY_FUNCTION__;
}

Calc::Calc(QObject *parent)
	: QObject(parent),
	m_adaptor(new CalcAdaptor(this))
{
	qDebug() << __PRETTY_FUNCTION__;
	QDBusConnection dbus = QDBusConnection::sessionBus();
	dbus.registerService(QStringLiteral(CALC_DBUS_SERVICE));
	dbus.registerObject(QStringLiteral(CALC_DBUS_OBJECT_PATH), this);
}

Calc::~Calc()
{
	delete m_adaptor;
}

double CalcAdaptor::multiply(double factor0, double factor1)
{
	qDebug() << __PRETTY_FUNCTION__ << factor0 << factor1;
	double product = factor0 * factor1;
	emit newProduct(product);
	return product;
}

double CalcAdaptor::divide(double dividend, double divisor)
{
	qDebug() << __PRETTY_FUNCTION__ << dividend << divisor;
	double quotient = dividend / divisor;
	emit newQuotient(quotient);
	return quotient;
}

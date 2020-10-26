#include <QDebug>

#include "Calc.h"
#include "calculatoradaptor.h"

Calc::Calc(QObject *parent)
	: QObject(parent),
	m_adaptor(new ICalculatorAdaptor(this))
{
	qDebug() << __PRETTY_FUNCTION__;
	QDBusConnection dbus = QDBusConnection::sessionBus();
	dbus.registerObject("/my/test/OCalculator", this);
	dbus.registerService("my.test.SCalculator");
}

Calc::~Calc()
{
	delete m_adaptor;
}

double Calc::multiply(double factor0, double factor1)
{
	qDebug() << __PRETTY_FUNCTION__ << factor0 << factor1;
	double product = factor0 * factor1;
	emit newProduct(product);
	return product;
}

double Calc::divide(double dividend, double divisor)
{
	qDebug() << __PRETTY_FUNCTION__ << dividend << divisor;
	double quotient = dividend / divisor;
	emit newQuotient(quotient);
	return quotient;
}

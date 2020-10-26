#ifndef CALC_HH
#define CALC_HH

#include <QObject>

class ICalculatorAdaptor;

class Calc : public QObject
{
	Q_OBJECT;
	Q_CLASSINFO("D-Bus Interface", "my.test.ICalculator");

public:
	Calc(QObject *parent);
	virtual ~Calc();

public slots:
	double multiply(double factor0, double factor2);
	double divide(double divident, double divisor);

signals:
	void newProduct(double product);
	void newQuotient(double quotient);

private:
	ICalculatorAdaptor *m_adaptor;
};

#endif

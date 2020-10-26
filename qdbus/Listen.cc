#include "Listen.h"

Listen::Listen(QObject *parent)
	: QObject(parent)
{
	qDebug() << __PRETTY_FUNCTION__;
}

void Listen::reportNewProduct(double product)
{
	qDebug() << "Received a new product " << product;
}

void Listen::reportNewQuotient(double quotient)
{
	qDebug() << "Received a new quotient " << quotient;
}

#include "Listener.h"

Listener::Listener(QObject *parent)
	: QObject(parent)
{
	qDebug() << __PRETTY_FUNCTION__;
}

void Listener::reportNewProduct(double product)
{
	qDebug() << "Received a new product " << product;
}

void Listener::reportNewQuotient(double quotient)
{
	qDebug() << "Received a new quotient " << quotient;
}

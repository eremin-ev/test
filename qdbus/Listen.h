#ifndef LISTEN_HH
#define LISTEN_HH

#include <QtCore>

class Listen : public QObject
{
	Q_OBJECT;

public:
	Listen(QObject *parent=nullptr);

public slots:
	void reportNewProduct(double product);
	void reportNewQuotient(double quotient);
};

#endif

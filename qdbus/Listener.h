#ifndef LISTEN_HH
#define LISTEN_HH

#include <QtCore>

class Listener : public QObject
{
    Q_OBJECT

public:
    Listener(QObject *parent=nullptr);

public slots:
	void reportNewProduct(double product);
	void reportNewQuotient(double quotient);
};

#endif

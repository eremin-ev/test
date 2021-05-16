#include <QCoreApplication>

#include "Serv.h"

int main(int ac, char **av)
{
	QCoreApplication a(ac, av);

	Serv serv(&a);

	return a.exec();
}

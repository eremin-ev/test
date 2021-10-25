/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <QCoreApplication>

#include "authd.h"

int main(int ac, char **av)
{
	QCoreApplication a(ac, av);

	Authd authd(&a);

	return a.exec();
}

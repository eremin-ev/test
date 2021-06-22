/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTimer>

#include "UDisks2.h"

static void parseOptions(const QCoreApplication &a, QCommandLineParser &parser)
{
    parser.setApplicationDescription("UDisks2 set label test");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOptions({
        {{"u", "user"},
            QCoreApplication::translate("main", "Drop privileges to a user <user>."),
            QCoreApplication::translate("main", "user")},
        {{"g", "group"},
            QCoreApplication::translate("main", "Drop privileges to a group <group>."),
            QCoreApplication::translate("main", "group")},
    });
    parser.process(a);
}

int main(int argc, char **argv)
{
    QCoreApplication a(argc, argv);
    QCommandLineParser options;

    parseOptions(a, options);

    UDisks2 ud2(&a);
    ud2.m_user = options.value("user");
    ud2.m_group = options.value("group");
    if (ud2.m_user.isNull() || ud2.m_group.isNull()) {
        options.showHelp(0);
    }

    // FIXME: give UDisks2::getProperties a chance to obtain all async replies
    QTimer::singleShot(1000, [&ud2](){
        ud2.startTestSetLabel(QStringLiteral("/dev/mmcblk1p1"),
                              QStringLiteral("SDNEW3"));
    });

    return a.exec();
}

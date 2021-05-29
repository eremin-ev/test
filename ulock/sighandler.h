/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#ifndef SIGHANDLER_H
#define SIGHANDLER_H

#include <QObject>
#include <QSocketNotifier>

class QCoreApplication;
class SigHandler : public QObject {
    Q_OBJECT

public:
    SigHandler(QObject *parent = nullptr, QCoreApplication *app = nullptr);
    ~SigHandler();

    // Unix signal handlers.
    static void unixHupHandler(int unused);
    static void unixTermHandler(int unused);
    static void unixIntHandler(int unused);

    int setupUnixSignalHandlers();

public slots:
    // Qt signal handlers.
    void handleUnixHup();
    void handleUnixTerm();
    void handleUnixInt();

signals:
    void unixHupArrived();
    void unixTermArrived();
    void unixIntArrived();

private:
    static int sighupFd[2];
    static int sigtermFd[2];
    static int sigintFd[2];

    QCoreApplication *m_app;

    QSocketNotifier *snHup;
    QSocketNotifier *snTerm;
    QSocketNotifier *snInt;
};

int setup_unix_signal_handlers();

#endif  // SIGHANDLER_H

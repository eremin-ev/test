/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>

#include <QDebug>

#include "sighandler.h"

int SigHandler::sighupFd[2];
int SigHandler::sigtermFd[2];
int SigHandler::sigintFd[2];

static int setUnixSignalHandler(int signum, void (*handler)(int))
{
    struct sigaction sa;

    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handler;
    sa.sa_flags = 0;
    sa.sa_flags |= SA_RESTART;

    if (sigaction(signum, &sa, 0)) {
        printf("Cannot setup handler for %d\n", signum);
        return 1;
    }

    return 0;
}

int SigHandler::setupUnixSignalHandlers()
{
    if (setUnixSignalHandler(SIGHUP, SigHandler::unixHupHandler)) {
        printf("Cannot setup handler for SIGHUP");
        return 1;
    }

    if (setUnixSignalHandler(SIGTERM, SigHandler::unixTermHandler)) {
        printf("Cannot setup handler for SIGTERM");
        return 2;
    }

    if (setUnixSignalHandler(SIGINT, SigHandler::unixIntHandler)) {
        printf("Cannot setup handler for SIGINT");
        return 3;
    }

    return 0;
}

SigHandler::SigHandler(QObject *parent, QCoreApplication *app)
    : QObject(parent)
    , m_app(app)
{
    qDebug() << __PRETTY_FUNCTION__ << this << "parent" << parent;

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sighupFd))
       qFatal("Couldn't create HUP socketpair");

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermFd))
       qFatal("Couldn't create TERM socketpair");

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigintFd))
       qFatal("Couldn't create INT socketpair");

    snHup = new QSocketNotifier(sighupFd[1], QSocketNotifier::Read, this);
    connect(snHup, &QSocketNotifier::activated, this, &SigHandler::handleUnixHup);

    snTerm = new QSocketNotifier(sigtermFd[1], QSocketNotifier::Read, this);
    connect(snTerm, &QSocketNotifier::activated, this, &SigHandler::handleUnixTerm);

    snInt = new QSocketNotifier(sigintFd[1], QSocketNotifier::Read, this);
    connect(snInt, &QSocketNotifier::activated, this, &SigHandler::handleUnixInt);
}

SigHandler::~SigHandler()
{
    qDebug() << __PRETTY_FUNCTION__;
}

void SigHandler::unixHupHandler(int)
{
    char a = 1;
    ::write(sighupFd[0], &a, sizeof(a));
}

void SigHandler::unixTermHandler(int)
{
    char a = 1;
    ::write(sigtermFd[0], &a, sizeof(a));
}

void SigHandler::unixIntHandler(int)
{
    char a = 1;
    ::write(sigintFd[0], &a, sizeof(a));
}

void SigHandler::handleUnixHup()
{
    snHup->setEnabled(false);
    char tmp;
    ::read(sighupFd[1], &tmp, sizeof(tmp));

    // do Qt stuff
    qDebug() << __PRETTY_FUNCTION__;
    emit unixHupArrived();

    snHup->setEnabled(true);
}

void SigHandler::handleUnixTerm()
{
    snTerm->setEnabled(false);
    char tmp;
    ::read(sigtermFd[1], &tmp, sizeof(tmp));

    // do Qt stuff
    qDebug() << __PRETTY_FUNCTION__;
    emit unixTermArrived();

    snTerm->setEnabled(true);
}

void SigHandler::handleUnixInt()
{
    snHup->setEnabled(false);
    char tmp;
    ::read(sigintFd[1], &tmp, sizeof(tmp));

    // do Qt stuff
    qDebug() << __PRETTY_FUNCTION__;
    emit unixIntArrived();

    snHup->setEnabled(true);
}

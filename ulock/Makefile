CFLAGS = -Wall -Werror -O0 -g
CXXFLAGS = $(CFLAGS) -fPIC
QT = Qt5Core Qt5Gui Qt5Quick Qt5Qml

all: test_ulock

test_ulock: CXXFLAGS += $(shell pkg-config --cflags $(QT))
test_ulock: LDLIBS += $(shell pkg-config --libs $(QT))

test_ulock: auth-type.o \
	    sighandler.o \
	    ulock.o \
	    moc_auth-type.o \
	    moc_sighandler.o \
	    moc_ulock.o \
	    qrc_resources.o

moc_auth-type.cc: auth-type.h
	moc $< -o $@

moc_sighandler.cc: sighandler.h
	moc $< -o $@

moc_ulock.cc: ulock.h
	moc $< -o $@

qrc_resources.cc: resources.qrc \
		  qml/ulock.qml \
		  qml/ULockMain.qml \
		  qml/ULockPassword.qml
	rcc -name resources $< -o $@

clean:
	rm -vf test_ulock moc_*.cc *.o qrc_resources.cc

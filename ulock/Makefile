CFLAGS = -Wall -Werror -O0 -g
CXXFLAGS = $(CFLAGS) -fPIC

GCC_VER_MAJOR := $(shell gcc -dumpversion | cut -f1 -d'.')

ifeq ($(shell expr $(GCC_VER_MAJOR) '>=' 10), 1)
  CXXFLAGS +=  -Wno-error=mismatched-new-delete
endif

all: test_authd test_ulock

test_ulock: CXXFLAGS += $(shell pkg-config --cflags Qt5Core Qt5Gui Qt5Quick Qt5Qml Qt5DBus)
test_ulock: LDLIBS += $(shell pkg-config --libs Qt5Core Qt5Gui Qt5Quick Qt5Qml Qt5DBus)

test_authd: CXXFLAGS += $(shell pkg-config --cflags Qt5Core Qt5DBus)
test_authd: LDLIBS += $(shell pkg-config --libs Qt5Core Qt5DBus dbus-1)

authd.o: CXXFLAGS += $(shell pkg-config --cflags Qt5Core Qt5DBus dbus-1)

test_ulock: auth-type.o \
	    sighandler.o \
	    ulock.o \
	    moc_auth-type.o \
	    moc_sighandler.o \
	    moc_ulock.o \
	    qrc_resources.o

test_authd: LDLIBS += -lstdc++

test_authd: authd.o \
	    moc_authd.o

moc_%.cc: %.h
	moc $< -o $@

qrc_resources.cc: resources.qrc \
		  qml/ulock.qml \
		  qml/ULockMain.qml \
		  qml/ULockPassword.qml
	rcc -name resources $< -o $@

clean:
	rm -vf test_authd test_ulock moc_*.cc *.o qrc_resources.cc

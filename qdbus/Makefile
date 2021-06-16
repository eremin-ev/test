src ?= .
CXXFLAGS = -Wall -Wextra -Werror -O0 -g -std=c++11 -fPIC -I$(src) -I$(PWD)

CXXFLAGS += $(shell pkg-config --cflags Qt5Core Qt5DBus)
LDLIBS = $(shell pkg-config --libs Qt5Core Qt5DBus)

all: client server listener tags

server: Serv.o moc_Serv.o moc_PeerWatcher.o

server: CXXFLAGS += $(shell pkg-config --cflags dbus-1)
server: LDLIBS += $(shell pkg-config --libs dbus-1)

listener: Listener.o moc_Listener.o

# Note: Serv.xml is not built by default
Serv.xml: Serv.h
	qdbuscpp2xml -A $< -o $@

moc_Serv.cc: Serv.h
	moc -o $@ $<

moc_Listener.cc: Listener.h
	moc -o $@ $<

moc_PeerWatcher.cc: PeerWatcher.h
	moc -o $@ $<

tags:
	ctags -R .

.PHONY: clean distclean

clean:
	rm -vf *.o client server listener Serv.xml *.moc moc_*.cc

distclean: clean
	rm -vf tags

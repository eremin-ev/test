src ?= .
CXXFLAGS = $(WALL) -O0 -g -std=c++11 -fPIC -I$(src) -I$(PWD)
CXXFLAGS += $(shell pkg-config --cflags Qt5Core Qt5DBus)
LDLIBS = $(shell pkg-config --libs Qt5Core Qt5DBus)

all: client client-p2p server listener tags

server: Serv.o moc_Serv.o

listener: Listener.o moc_Listener.o

# Note: Serv.xml is not built by default
Serv.xml: Serv.h
	qdbuscpp2xml -A $< -o $@

moc_Serv.cc: Serv.h
	moc -o $@ $<

moc_Listener.cc: Listener.h
	moc -o $@ $<

tags:
	ctags -R .

clean:
	rm -vf *.o tags client server listener Serv.xml *.moc moc_*.cc

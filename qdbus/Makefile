src ?= .
CXXFLAGS = $(WALL) -O0 -g -std=c++11 -fPIC -I$(src) -I$(PWD)
CXXFLAGS += $(shell pkg-config --cflags Qt5Core Qt5DBus)
LDLIBS = $(shell pkg-config --libs Qt5Core Qt5DBus)

all: client server listener tags

server: Calc.o moc_Calc.o

listener: Listener.o moc_Listener.o

# Note: Calc.xml is not built by default
Calc.xml: Calc.h
	qdbuscpp2xml -A $< -o $@

moc_Calc.cc: Calc.h
	moc -o $@ $<

moc_Listener.cc: Listener.h
	moc -o $@ $<

tags:
	ctags -R .

clean:
	rm -vf *.o tags client server listener Calc.xml *.moc moc_*.cpp

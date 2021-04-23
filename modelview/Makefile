WALL = -Wall
CFLAGS = $(WALL) -O2 -g -Ilib $(DBG)
LDFLAGS = -Wl,--build-id=none
CXXFLAGS = $(WALL) -O2 -g -std=c++11 -fPIC
ARCH = $(shell gcc -dumpmachine)

bin := modelview

all: $(bin)

modelview: mymodel.o moc_mymodel.o
modelview: CXXFLAGS += $(shell pkg-config --cflags Qt5Core Qt5Widgets)
modelview: LDLIBS += $(shell pkg-config --libs Qt5Core Qt5Widgets)

moc_mymodel.cc: mymodel.h
	moc -o $@ $<

#
# Rely on the implicit rules to build
#

.PHONY: clean
clean:
	rm -vf $(bin) *.i *.o *.s moc_*.cc

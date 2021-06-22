CXXFLAGS = -Wall -Werror -O0 -g -fPIC -Wno-psabi
QT = Qt5Core Qt5DBus

all: test_udisks2

test_udisks2: CXXFLAGS += $(shell pkg-config --cflags $(QT))
test_udisks2: LDLIBS += $(shell pkg-config --libs $(QT))

test_udisks2: UDisks2.o moc_UDisks2.o

UDisks2.o: UDisks2.h

moc_UDisks2.cc: UDisks2.h
	moc $< -o $@

.PHONY: clean
clean:
	rm -vf test_udisks2 moc_*.cc *.o

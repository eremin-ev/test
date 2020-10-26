src ?= .
CXXFLAGS = $(WALL) -O0 -g -std=c++11 -fPIC -I$(src) -I$(PWD)
CXXFLAGS += $(shell pkg-config --cflags Qt5Core Qt5DBus)
LDLIBS = $(shell pkg-config --libs Qt5Core Qt5DBus)

all: client server listener

server: Calc.o moc_Calc.o calculatoradaptor.o

listener: Listen.o moc_Listen.o

Calc.o: calculatoradaptor.cpp calculatoradaptor.h

Calc.xml: Calc.h
	qdbuscpp2xml -A $< -o $@

calculatoradaptor.o: calculatoradaptor.moc

calculatoradaptor.moc: calculatoradaptor.h
	moc -o $@ $<

calculatoradaptor.cpp calculatoradaptor.h: Calc.xml
	qdbusxml2cpp -m -a calculatoradaptor -i Calc.h -l Calc $<

moc_Calc.cpp: Calc.h
	moc -o $@ $<

moc_Listen.cpp: Listen.h
	moc -o $@ $<

clean:
	rm -vf *.o tags client server Calc.xml *.moc moc_*.cpp
	rm -vf calculatoradaptor.h calculatoradaptor.cpp

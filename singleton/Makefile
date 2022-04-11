DBG ?= -O2 -g
WALL = -Wall -Wextra
CXXFLAGS = $(WALL) $(DBG) -std=c++17 -fPIC

bin = test_ston

all: $(bin)

$(bin): ston.o

.PHONY: clean
clean:
	rm -vf $(bin) *.i *.o *.s

# Simple UI shared object

CXXFLAGS = -g -Wall -Wextra -Werror -std=c++11 -fPIC
LDFLAGS = -g

.PHONY: all clean

all: libsui.so test_app

qt := libsui.so test_app

$(qt): CXXFLAGS += $(shell pkg-config --cflags Qt5Core)
$(qt): LDLIBS = $(shell pkg-config --libs Qt5Core)

libsui.so: widget.o label.o
	$(CC) -shared $(LDFLAGS) -o $@ $^

test_app: LDFLAGS += -L.
test_app: LDLIBS += -lsui

clean:
	rm -vf libsui.so test_app *.o

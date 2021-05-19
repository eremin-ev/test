CXXFLAGS = -g -Wall -Wextra -Werror -std=c++11 -fPIC
LDFLAGS = -g

.PHONY: all clean

all: test_app

libsimple.so: widget.o label.o
	$(CC) -shared $(LDFLAGS) -o $@ $^

test_app: libsimple.so
test_app: LDFLAGS += -L.
test_app: LDLIBS += -lsimple

widget.o: widget.h widget_p.h

label.o: label.h

clean:
	rm -vf libsimple.so test_app *.o *.so

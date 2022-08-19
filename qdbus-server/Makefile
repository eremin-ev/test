CFLAGS = -Wall -Werror -O0 -g -I../
CXXFLAGS = $(CFLAGS) -fPIC

bin := sampled client

all: $(bin)

$(bin): CXXFLAGS += $(shell pkg-config --cflags dbus-1 Qt5Core Qt5DBus)
$(bin): LDLIBS += $(shell pkg-config --libs dbus-1 Qt5Core Qt5DBus)
$(bin): LDLIBS += -lstdc++

sampled: sampled.o \
	 sample-service.o	moc_sample-service.o

client: client.o \
	sample-client.o		moc_sample-client.o

sample-service.o: sample-service.moc

moc_%.cc: %.h
	moc $< -o $@

%.moc: %.cc
	moc $(MOCFLAGS) $< -o $@

clean:
	rm -vf $(bin) moc_*.cc *.o *.moc

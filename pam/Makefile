CFLAGS = $(WALL) -O0 -g -fPIC

bin = pam_testprompt.so

all: $(bin)

pam_testprompt.so: LDLIBS += -lpam
pam_testprompt.so: LDFLAGS += -shared

pam_testprompt.so: pam_testprompt.o
	$(LD) $(LDFLAGS) $(LDLIBS) -o $@ $<

.PHONY: clean
clean:
	rm -vf $(bin) *.i *.o *.s

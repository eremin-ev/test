src ?= .
VPATH := $(VPATH) $(src)/lib
WALL = -Wall -Wextra
WERROR = -Werror
#DBG = -O0 -g
DBG ?= -O2 -g
CFLAGS = $(WALL) $(WERROR) $(DBG) -I$(src)/lib
# -I$(PWD)
CXXFLAGS = $(WALL) $(DBG) -std=c++17 -fPIC -I$(src)
LDLIBS = -lm

bin =	test_dayofweek \
	test_destr \
	test_division_by_zero \
	test_fanotify \
	test_fizzbuzz \
	test_func \
	test_get_seats \
	test_getpwnam \
	test_idxupd \
	test_inherit \
	test_inotify \
	test_keyval \
	test_lambda \
	test_line_point \
	test_list_vs_tree \
	test_map \
	test_memmove \
	test_mmap_chunks \
	test_op \
	test_page \
	test_pam \
	test_pam_spawn \
	test_pam_helper \
	test_pipe \
	test_pipe_child \
	test_ptr_array \
	test_qvariant \
	test_ref \
	test_sizeof \
	test_setter \
	test_substr_cnt \
	test_suid \
	test_vector \
	test_virtual \
	test_read

all: $(bin) tags
	$(MAKE) -C d_ptr
	$(MAKE) -C modelview
	$(MAKE) -C pam
	$(MAKE) -C qdbus
	$(MAKE) -C udisks2
	$(MAKE) -C ulock

test_dayofweek: moc_dayofweek.o

test_get_seats: LDLIBS += -lsystemd

test_keyval: CFLAGS += -I$(src)/lib
test_keyval: keyval.o

test_list_vs_tree: rbtree.o

test_op: LDLIBS += -lfmt

test_pam_spawn: spawn-child.o

test_pam: LDLIBS += -lpam
test_pam_helper: LDLIBS += -lpam

test_pipe: spawn-child.o

test_mmap_chunks: LDLIBS += $(shell pkg-config --libs libcrypto)

qt :=	test_dayofweek \
	test_destr \
	test_getpwnam \
	test_qvariant \
	test_setter

$(qt): CXXFLAGS += $(shell pkg-config --cflags Qt5Core)
$(qt): LDLIBS += $(shell pkg-config --libs Qt5Core)

moc_dayofweek.cc: test_dayofweek.h
	moc -o $@ $<

tags:
	ctags -R .

#
# Rely on the implicit rules to build
#

.PHONY: clean
clean:
	rm -vf $(bin) *.i *.o *.s
	$(MAKE) -C d_ptr clean
	$(MAKE) -C modelview clean
	$(MAKE) -C pam clean
	$(MAKE) -C qdbus clean
	$(MAKE) -C udisks2 clean
	$(MAKE) -C ulock clean

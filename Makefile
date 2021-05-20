src ?= .
VPATH := $(VPATH) $(src)/lib
WALL = -Wall -Wextra
WERROR = -Werror
#DBG = -O0 -g
DBG ?= -O2 -g
CFLAGS = $(WALL) $(WERROR) $(DBG) -I$(src)/lib
# -I$(PWD)
CXXFLAGS = $(WALL) $(DBG) -std=c++11 -fPIC -I$(src)
LDLIBS = -lm

bin =	test_dayofweek \
	test_division_by_zero \
	test_fanotify \
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
	test_ref \
	test_sizeof \
	test_setter \
	test_substr_cnt \
	test_vector \
	test_read

all: $(bin) tags
	$(MAKE) -C qdbus

test_list_vs_tree: rbtree.o

test_pam: LDLIBS += -lpam
test_pam_helper: LDLIBS += -lpam

test_pipe: spawn-child.o

test_pam_spawn: spawn-child.o

test_keyval: CFLAGS += -I$(src)/lib
test_keyval: keyval.o

qt := test_dayofweek test_setter

$(qt): CXXFLAGS += $(shell pkg-config --cflags Qt5Core)
$(qt): LDLIBS = $(shell pkg-config --libs Qt5Core)

test_mmap_chunks: LDLIBS += $(shell pkg-config --libs libcrypto)

test_dayofweek: moc_dayofweek.o

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

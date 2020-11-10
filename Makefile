src ?= .
VPATH := $(VPATH) $(src)/lib
WALL = -Wall
#CFLAGS = -Wall -Werror
CFLAGS = $(WALL) -O2 -g -I$(src)/lib $(DBG)
CXXFLAGS = $(WALL) -O2 -g
LDLIBS = -lm

bin =	test_class_derivation \
	test_division_by_zero \
	test_fanotify \
	test_inotify \
	test_keyval \
	test_line_point \
	test_list_vs_tree \
	test_map \
	test_memmove \
	test_page \
	test_pam \
	test_ptr_array \
	test_ref \
	test_sizeof \
	test_substr_cnt \
	test_vector

all: $(bin) tags

test_list_vs_tree: rbtree.o

test_pam: LDLIBS += -lpam

test_keyval: CFLAGS += -I$(src)/lib
test_keyval: keyval.o

tags:
	ctags -R .

#
# Rely on the implicit rules to build
#

.PHONY: clean
clean:
	rm -vf $(bin) *.i *.o *.s

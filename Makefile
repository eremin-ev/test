VPATH = lib
WALL = -Wall
#CFLAGS = -Wall -Werror
CFLAGS = $(WALL) -O2 -g -Ilib $(DBG)
CXXFLAGS = $(WALL) -O2 -g
LDLIBS = -lm

bin =	test_class_derivation \
	test_division_by_zero \
	test_line_point \
	test_list_vs_tree \
	test_map \
	test_pam \
	test_ref

all: $(bin) tags

test_list_vs_tree: rbtree.o

test_pam: LDLIBS += -lpam -lpam_misc

tags:
	ctags -R .

#
# Rely on the implicit rules to build
#

.PHONY: clean
clean:
	rm -vf $(bin) *.i *.o *.s

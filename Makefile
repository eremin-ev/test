VPATH = lib
WALL = -Wall
CFLAGS = $(WALL) -O2 -g -Ilib $(DBG)
LDLIBS = -lm

bin =	test_division_by_zero \
	test_line_point \
	test_list_vs_tree \
	test_ref

all: $(bin) tags

test_list_vs_tree: rbtree.o

tags:
	ctags -R .

#
# Rely on the implicit rules to build
#

.PHONY: clean
clean:
	rm -vf $(bin) *.i *.o *.s

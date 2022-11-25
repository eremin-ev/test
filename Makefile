src ?= .
VPATH := $(VPATH) $(src)/lib
WALL = -Wall -Wextra
#WERROR = -Werror
DBG = -O0 -g
#DBG ?= -O2 -g
override CFLAGS += $(WALL) $(WERROR) $(DBG) -I$(src)/lib
# -I$(PWD)
override CXXFLAGS += $(WALL) $(WERROR) $(DBG) -std=c++17 -fPIC -I$(src)
LDLIBS = -lm

bin =	test_actions \
	test_aggregate \
	test_align \
	test_array \
	test_cast \
	test_comm \
	test_constr \
	test_crypto_sock \
	test_dayofweek \
	test_destr \
	test_division_by_zero \
	test_dlopen \
	test_fanotify \
	test_fizzbuzz \
	test_ftempl \
	test_func \
	test_get_seats \
	test_getpwnam \
	test_good_taste \
	test_hex2str \
	test_idxupd \
	test_inherit \
	test_inotify \
	test_is_constexpr \
	test_iter \
	test_keyval \
	test_lambda \
	test_line_point \
	test_list_vs_tree \
	test_luks \
	test_map \
	test_map_insert \
	test_member_static \
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
	test_qfile \
	test_qsettings \
	test_qstring \
	test_qvariant \
	test_read \
	test_ref \
	test_ret_local \
	test_ret_no_value \
	test_sizeof \
	test_setter \
	test_str_diff \
	test_string \
	test_strncmp \
	test_substr_cnt \
	test_suid \
	test_triv \
	test_uninitialized \
	test_variadic \
	test_vdso \
	test_vector \
	test_virtual \
	test_yaml

all: $(bin) tags
	$(MAKE) -C d_ptr
	$(MAKE) -C modelview
	$(MAKE) -C pam
	$(MAKE) -C qdbus
	$(MAKE) -C udisks2
	$(MAKE) -C ulock

test_comm: LDLIBS += -lpthread

test_constr test_op test_variadic: LDLIBS += -lfmt

test_dayofweek: moc_dayofweek.o

test_get_seats: LDLIBS += -lsystemd

test_keyval: CFLAGS += -I$(src)/lib
test_keyval: keyval.o

test_list_vs_tree: rbtree.o

test_pam_spawn: spawn-child.o

test_pam: LDLIBS += -lpam
test_pam_helper: LDLIBS += -lpam

test_pipe: spawn-child.o

test_mmap_chunks: LDLIBS += $(shell pkg-config --libs libcrypto)

qt :=	test_actions \
	test_dayofweek \
	test_destr \
	test_getpwnam \
	test_qfile \
	test_qsettings \
	test_qstring \
	test_qvariant \
	test_setter \
	test_triv

$(qt): CXXFLAGS += $(shell pkg-config --cflags Qt5Core)
$(qt): LDLIBS += $(shell pkg-config --libs Qt5Core)

test_yaml: CXXFLAGS += $(shell pkg-config --cflags yaml-cpp)
test_yaml: LDLIBS += $(shell pkg-config --libs yaml-cpp)

test_dlopen: LDLIBS += -ldl

moc_dayofweek.cc: test_dayofweek.h
	moc -o $@ $<

test_luks: CXXFLAGS += $(shell pkg-config --cflags libcryptsetup)
test_luks: LDLIBS += $(shell pkg-config --libs libcryptsetup)

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

# sim2d - 2D bouncing-ball simulator in plain C, using xcb + EGL + OpenGL ES.
#
# Dependencies (pkg-config names): egl glesv2 xcb
# Debian/Ubuntu: apt install libegl-dev libgles2-mesa-dev libxcb1-dev \
#                    libxcb-ewmh-dev mesa-utils   (last one optional, for glxinfo)
# Arch:          pacman -S mesa libx11 xcb-proto  (pkgconf provides the rest)

PKG_CONFIG ?= pkg-config
CC         ?= cc

PKGS      := egl glesv2 xcb
CFLAGS    ?= -O2 -g
CFLAGS    += -std=gnu11 -Wall -Wextra -Wpedantic -Wshadow -Wmissing-prototypes
CPPFLAGS  += $(shell $(PKG_CONFIG) --cflags $(PKGS))
LDLIBS    := $(shell $(PKG_CONFIG) --libs $(PKGS)) -lm

SRC := main.c sim.c platform_x11.c gl_draw.c
OBJ := $(SRC:.c=.o)
BIN := sim2d

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

main.o:         sim.h platform.h gl_draw.h
sim.o:          sim.h
platform_x11.o: platform.h
gl_draw.o:      gl_draw.h sim.h

# headless run: physics self-test + PPM screenshot
test: $(BIN)
	./$(BIN) --selftest --seed 7

demo-ppm: $(BIN)
	./$(BIN) --mode off --size 800x600 --balls 30 --frames 240 \
	          --dump-ppm frame.ppm --quiet
	@echo "wrote frame.ppm"

clean:
	rm -f $(OBJ) $(BIN) frame.ppm selftest.ppm

.PHONY: all test demo-ppm clean

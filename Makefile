# Makefile to build Soundflow
#
# Inspired by: https://github.com/edubart/sokol_gp/blob/master/Makefile
#

TARGET = soundflow

CFLAGS=-std=c99
CFLAGS+=-Wall -Wextra -Wshadow -Wno-unused-function -Wno-missing-field-initializers
CC=clang
INCS=-Iinclude
BUILDDIR=build
OUTEXT=

# platform
ifndef platform
	platform=macos
endif
ifeq ($(platform), windows)
	CC=x86_64-w64-mingw32-gcc
	LIBS+=-lkernel32 -luser32 -lshell32 -lgdi32 -ld3d11 -ldxgi
	OUTEXT=.exe
else ifeq ($(platform), linux)
	CC=gcc
	DEFS+=-D_GNU_SOURCE
	CFLAGS+=-pthread
	LIBS+=-lX11 -lXi -lXcursor -lGL -ldl -lm
	ifeq ($(backend), gles3)
		LIBS+=-lEGL
	endif
else ifeq ($(platform), macos)
	CC=clang
	LIBS+=-framework Cocoa -framework QuartzCore -framework Metal -framework MetalKit
	CFLAGS+=-ObjC
else ifeq ($(platform), web)
	CC=/usr/local/emsdk/upstream/emscripten/emcc
	LIBS+=-sFULL_ES3
	OUTEXT=.html
	CFLAGS+=--shell-file=web/shell.html --embed-file assets
endif

# build type
ifndef build
	build=debug
endif
ifeq ($(platform), web)
	ifeq ($(build), debug)
		CFLAGS+=-Og -g -sASSERTIONS -sALLOW_MEMORY_GROWTH
	else
		CFLAGS+=-O3 -g0 -s -sALLOW_MEMORY_GROWTH
	endif
else ifeq ($(build), debug)
	CFLAGS+=-Og -ffast-math -g
else ifeq ($(build), release)
	CFLAGS+=-Ofast -fno-plt -g
	DEFS+=-DNDEBUG
endif

ifeq ($(build), debug)
	DEFS+=-DSOKOL_DEBUG
endif

# backend
ifndef backend
	ifeq ($(platform), windows)
		backend=d3d11
	else ifeq ($(platform), macos)
		backend=metal
	else ifeq ($(platform), web)
		backend=gles3
	else
		backend=glcore
	endif
endif
ifeq ($(backend), glcore)
	DEFS+=-DSOKOL_GLCORE
else ifeq ($(backend), gles3)
	DEFS+=-DSOKOL_GLES3
else ifeq ($(backend), d3d11)
	DEFS+=-DSOKOL_D3D11
else ifeq ($(backend), metal)
	DEFS+=-DSOKOL_METAL
else ifeq ($(backend), dummy)
	DEFS+=-DSOKOL_DUMMY_BACKEND
endif
ifeq ($(platform), macos)
ifeq ($(backend), glcore)
	LIBS+=-framework OpenGL
endif
endif

.PHONY: all run clean update-deps shaders

all: $(TARGET)$(OUTEXT)

$(TARGET)$(OUTEXT): src/main.c sokol.o miniaudio.o src/shader_glsl.h src/node_editor.c
	$(CC) -o $@ $< sokol.o miniaudio.o $(INCS) $(DEFS) $(CFLAGS) $(LIBS)

sokol.o: src/sokol.c
	$(CC) -c $< $(INCS) $(DEFS) $(CFLAGS)

stb.o: src/stb.c
	$(CC) -c $< $(INCS) $(DEFS) $(CFLAGS)

miniaudio.o: src/miniaudio.c
	$(CC) -c $< $(INCS) $(DEFS) $(CFLAGS)

run: $(TARGET)$(OUTEXT)
	./$(TARGET)$(OUTEXT)

clean:
	rm -f $(TARGET)
	rm -f *.o

update-deps:
	mkdir -p include
	wget -O include/sokol_app.h https://raw.githubusercontent.com/floooh/sokol/master/sokol_app.h
	wget -O include/sokol_audio.h https://raw.githubusercontent.com/floooh/sokol/master/sokol_audio.h
	wget -O include/sokol_gfx.h https://raw.githubusercontent.com/floooh/sokol/master/sokol_gfx.h
	wget -O include/sokol_glue.h https://raw.githubusercontent.com/floooh/sokol/master/sokol_glue.h
	wget -O include/sokol_time.h https://raw.githubusercontent.com/floooh/sokol/master/sokol_time.h
	wget -O include/sokol_log.h https://raw.githubusercontent.com/floooh/sokol/master/sokol_log.h
	wget -O include/stb_image.h https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
	wget -O include/sokol_nuklear.h https://github.com/floooh/sokol/raw/refs/heads/master/util/sokol_nuklear.h
	wget -O include/nuklear.h https://github.com/Immediate-Mode-UI/Nuklear/raw/refs/heads/master/nuklear.h
	wget -O include/miniaudio.h https://raw.githubusercontent.com/mackron/miniaudio/refs/heads/master/miniaudio.h


debug:
	@echo $(SHADERS_H)

# shaders
shaders: src/shader_glsl.h

src/shader_glsl.h: src/shader.glsl
	sokol-shdc --input $< --output $@ --slang glsl430:glsl300es:hlsl4:metal_macos:metal_ios:wgsl

.PHONY: web

web:
	$(MAKE) -B platform=web


FORCE:


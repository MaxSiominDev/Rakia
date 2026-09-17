CC      ?= cc
CFLAGS  += -std=c99 -Wall -Wextra -O2 -MMD -MP -I$(SRCDIR) -I$(THIRD) -DGL_SILENCE_DEPRECATION $(EXTRA_CFLAGS)
LDFLAGS += -framework GLUT -framework OpenGL -framework Cocoa
LDLIBS  += -lm

SRCDIR  := src
THIRD   := third_party
BUILD   := build
SRCS    := $(wildcard $(SRCDIR)/engine/*.c) $(wildcard $(SRCDIR)/game/*.c)
OBJS    := $(patsubst $(SRCDIR)/%.c,$(BUILD)/%.o,$(SRCS))
TARGET  := rakia

MINGW     := x86_64-w64-mingw32-gcc
FREEGLUT  := $(THIRD)/freeglut-win32
WINBUILD  := build-win
WINTARGET := rakia.exe
WINOBJS   := $(patsubst $(SRCDIR)/%.c,$(WINBUILD)/%.o,$(SRCS))
WINCFLAGS := -std=c99 -Wall -Wextra -O2 -MMD -MP -I$(SRCDIR) -I$(THIRD) -DFREEGLUT_STATIC -I$(FREEGLUT)/include $(EXTRA_CFLAGS)
WINLIBS   := -L$(FREEGLUT)/lib -lfreeglut_static -lopengl32 -lglu32 -lgdi32 -lwinmm -lm

# MSYS2 on the CI runner ships the tool under its plain name, a cross toolchain under the prefixed one
WINDRES   ?= $(shell command -v x86_64-w64-mingw32-windres >/dev/null 2>&1 && echo x86_64-w64-mingw32-windres || echo windres)

RELEASE_BUILD    := build/release/macos
RELEASE_CFLAGS   := -std=c99 -Wall -Wextra -O2 -MMD -MP -I$(SRCDIR) -I$(THIRD) -DGL_SILENCE_DEPRECATION $(EXTRA_CFLAGS) \
                     -arch arm64 -arch x86_64 -mmacosx-version-min=11.0
RELEASE_LDFLAGS  := -framework GLUT -framework OpenGL -framework Cocoa -arch arm64 -arch x86_64 -mmacosx-version-min=11.0
RELEASE_OBJS     := $(patsubst $(SRCDIR)/%.c,$(RELEASE_BUILD)/%.o,$(SRCS))
RELEASE_PACK     := build/release/pack.bin
RELEASE_TARGET   := build/release/rakia-macos

WINRELEASE_BUILD  := build/release/windows
WINRELEASE_CFLAGS := -std=c99 -Wall -Wextra -O2 -MMD -MP -I$(SRCDIR) -I$(THIRD) -DFREEGLUT_STATIC -I$(FREEGLUT)/include $(EXTRA_CFLAGS)
WINRELEASE_LIBS   := -L$(FREEGLUT)/lib -lfreeglut_static -lopengl32 -lglu32 -lgdi32 -lwinmm -lm -static
WINRELEASE_OBJS   := $(patsubst $(SRCDIR)/%.c,$(WINRELEASE_BUILD)/%.o,$(SRCS))
WINRELEASE_PACK   := $(WINRELEASE_BUILD)/pack.bin
WINRELEASE_RES    := build/release/pack.res
WINRELEASE_TARGET := build/release/rakia-windows.exe

TESTSRCS := $(filter-out $(SRCDIR)/game/main.c,$(SRCS)) $(wildcard tests/*.c)
TESTBIN  := $(BUILD)/run-tests

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

$(BUILD)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

windows: $(WINTARGET)

$(WINTARGET): $(WINOBJS)
	$(MINGW) $(WINOBJS) -o $@ -mwindows $(WINLIBS)

$(WINBUILD)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(MINGW) $(WINCFLAGS) -c $< -o $@

test: $(TESTBIN)
	@$(TESTBIN)

$(TESTBIN): $(TESTSRCS) $(wildcard $(SRCDIR)/engine/*.h $(SRCDIR)/game/*.h tests/*.h)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(TESTSRCS) -o $@ $(LDFLAGS) $(LDLIBS)

# the generated files are committed, so a normal build needs no Python
font:
	python3 tools/gen-font-atlas.py

markings:
	python3 tools/gen-markings.py

paint:
	python3 tools/gen-jet-paint.py

release: $(RELEASE_TARGET)

$(RELEASE_PACK): tools/pack-assets.py tools/pack-manifest.txt
	@mkdir -p build/release
	python3 tools/pack-assets.py tools/pack-manifest.txt $(RELEASE_PACK)

# the linker only ad-hoc signs the native slice; codesign covers both, or Rosetta refuses the x86_64 one
$(RELEASE_TARGET): $(RELEASE_OBJS) $(RELEASE_PACK)
	$(CC) $(RELEASE_OBJS) -o $@ $(RELEASE_LDFLAGS) $(LDLIBS) -Wl,-sectcreate,__RAKIA,__pack,$(RELEASE_PACK)
	codesign -s - --force $@

$(RELEASE_BUILD)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(RELEASE_CFLAGS) -c $< -o $@

windows-release: $(WINRELEASE_TARGET)

$(WINRELEASE_PACK): tools/pack-assets.py tools/pack-manifest.txt
	@mkdir -p $(WINRELEASE_BUILD)
	python3 tools/pack-assets.py tools/pack-manifest.txt $(WINRELEASE_PACK)

$(WINRELEASE_RES): tools/pack.rc $(WINRELEASE_PACK)
	@mkdir -p build/release
	$(WINDRES) tools/pack.rc -O coff -o $(WINRELEASE_RES)

$(WINRELEASE_TARGET): $(WINRELEASE_OBJS) $(WINRELEASE_RES)
	$(MINGW) $(WINRELEASE_OBJS) $(WINRELEASE_RES) -o $@ -mwindows $(WINRELEASE_LIBS)

$(WINRELEASE_BUILD)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(MINGW) $(WINRELEASE_CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD) $(WINBUILD) $(TARGET) $(WINTARGET) build/release

-include $(OBJS:.o=.d) $(WINOBJS:.o=.d) $(RELEASE_OBJS:.o=.d) $(WINRELEASE_OBJS:.o=.d)

.PHONY: all windows test font markings paint release windows-release clean

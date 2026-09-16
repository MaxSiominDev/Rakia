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

clean:
	rm -rf $(BUILD) $(WINBUILD) $(TARGET) $(WINTARGET)

-include $(OBJS:.o=.d) $(WINOBJS:.o=.d)

.PHONY: all windows test font markings clean

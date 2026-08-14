CC      ?= cc
CFLAGS  ?= -O2
LDFLAGS ?=

# `override` so that `make CFLAGS="-O1 -g -fsanitize=address"` keeps the
# include paths and dependency generation instead of dropping them.
WARNINGS         := -Wall -Wextra
override CFLAGS  += $(WARNINGS) -I./include -MMD -MP

SRCS := src/main.c src/util.c src/parser.c src/render.c src/build.c src/tree.c \
        src/serve.c src/init.c src/deploy.c src/rss.c toml.c

# ---------------------------------------------------------------------------
# cmark
#
# By default cmark is compiled from the copy in vendor/cmark and linked
# statically, so a plain `make` needs no preinstalled library and the resulting
# binary has no runtime dependency on one.
#
# Build against a system-installed cmark instead with:  make USE_SYSTEM_CMARK=1
# ---------------------------------------------------------------------------
USE_SYSTEM_CMARK ?= 0

ifeq ($(USE_SYSTEM_CMARK),1)
  PKG_CONFIG   ?= pkg-config
  CMARK_CFLAGS := $(shell $(PKG_CONFIG) --cflags libcmark 2>/dev/null)
  CMARK_LIBS   := $(shell $(PKG_CONFIG) --libs libcmark 2>/dev/null)

  ifeq ($(strip $(CMARK_LIBS)),)
    CMARK_LIBS := -lcmark
    ifeq ($(shell uname -s 2>/dev/null),Darwin)
      BREW_PREFIX := $(shell brew --prefix 2>/dev/null)
      ifneq ($(strip $(BREW_PREFIX)),)
        CMARK_CFLAGS := -I$(BREW_PREFIX)/include
        CMARK_LIBS   := -L$(BREW_PREFIX)/lib -lcmark
      endif
    endif
  endif

  override CFLAGS += $(CMARK_CFLAGS)
  LIBS   += $(CMARK_LIBS)
else
  CMARK_DIR  := vendor/cmark
  CMARK_SRCS := $(wildcard $(CMARK_DIR)/*.c)
  CMARK_OBJS := $(CMARK_SRCS:.c=.o)
  override CFLAGS += -I$(CMARK_DIR)
endif

ifeq ($(OS),Windows_NT)
  BIN  := atomik-ssg.exe
  LIBS += -lws2_32
else
  BIN  := atomik-ssg
endif

OBJS := $(SRCS:.c=.o)
DEPS := $(OBJS:.o=.d) $(CMARK_OBJS:.o=.d)

all: $(BIN)

$(BIN): $(OBJS) $(CMARK_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(CMARK_OBJS) $(LIBS)

# Third-party code is not held to this project's warning settings.
$(CMARK_OBJS): CFLAGS := $(filter-out $(WARNINGS),$(CFLAGS)) -w

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(BIN)
	./tests/run.sh

clean:
	rm -f $(OBJS) $(CMARK_OBJS) $(DEPS) atomik-ssg atomik-ssg.exe

-include $(DEPS)

.PHONY: all clean test

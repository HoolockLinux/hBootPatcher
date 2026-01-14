ROOT != pwd
PLATFORM_SRC = $(wildcard patches/*)
SRC = $(wildcard src/*)
OBJDIR = $(ROOT)/obj
PLATFORM_OBJS = $(patsubst patches/%,$(OBJDIR)/patches/%,$(PLATFORM_SRC:.c=.o))
OBJS = $(patsubst src/%,$(OBJDIR)/%,$(SRC:.c=.o)) $(PLATFORM_OBJS)
PLOOSHFINDER = plooshfinder/libplooshfinder.a
INCLDIRS = -I./include -I./plooshfinder/include

LDFLAGS ?=
LDFLAGS += -L./plooshfinder
CFLAGS ?= -O2 -g -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
CC_FOR_BUILD = cc
CC := clang
LIBS = -lplooshfinder

export OBJDIR CC CFLAGS

.PHONY: $(PLOOSHFINDER) all

all: submodules dirs $(PLOOSHFINDER) $(OBJS) hBootPatcher

submodules:
	@git submodule update --init --remote --recursive || true

dirs:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(OBJDIR)/patches

clean:
	@rm -rf hBootPatcher obj
	@$(MAKE) -C plooshfinder clean

format:
	clang-format -i patches/**.c include/**.h src/**.c

hBootPatcher: $(OBJS) $(PLOOSHFINDER)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) $(INCLDIRS) -o $@

$(OBJDIR)/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLDIRS) -c -o $@ $<

$(OBJDIR)/patches/%.o: patches/%.c
	$(CC) $(CFLAGS) $(INCLDIRS) -c -o $@ $<


$(PLOOSHFINDER):
	$(MAKE) -C plooshfinder all

.PHONY: all dirs clean

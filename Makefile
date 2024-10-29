# config
CC = gcc
EXECUTABLE = server

INCDIR := include
SRCDIR := src
LIBDIR := lib
DEPDIR := dep
OBJDIR := obj
BINDIR := .

# preprocessor flags
PPFLAGS =
# include flags
IFLAGS = -I$(INCDIR)
# compile flags
CFLAGS = -march=sandybridge -mtune=sandybridge -masm=intel -std=gnu11 -D_GNU_SOURCE
# linker flags
LFLAGS = -lpthread -lrt -lm -Wl,-rpath,lib

# build

C_FILES := $(shell find $(SRCDIR) -type f -name "*.c")

ifneq (,$(findstring build,$(MAKECMDGOALS)))
	DEPDIR := $(DEPDIR)/build
	OBJDIR := $(OBJDIR)/build
else ifneq (,$(findstring clean,$(MAKECMDGOALS)))
	OBJDIR := $(OBJDIR)
else ifneq (,$(findstring purge,$(MAKECMDGOALS)))
	DEPDIR := $(DEPDIR)
	OBJDIR := $(OBJDIR)
else
# (,$(findstring build,$(MAKECMDGOALS)))
	DEPDIR := $(DEPDIR)/release
	OBJDIR := $(OBJDIR)/release
endif

D_FILES := $(patsubst $(SRCDIR)/%.c, $(DEPDIR)/%.d, $(C_FILES))
O_FILES := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(C_FILES))

ifneq (,$(findstring burn,$(MAKECMDGOALS)))
	D_FILES := 
else ifneq (,$(findstring clean,$(MAKECMDGOALS)))
	D_FILES :=
else ifneq (,$(findstring run,$(MAKECMDGOALS)))
	D_FILES :=
else ifneq (,$(findstring lines,$(MAKECMDGOALS)))
	D_FILES :=
else
endif

ifeq (,$(findstring avx,$(shell lscpu)))
$(error CPU does not support AVX, clearly someone lied)
endif

all: release

.PHONY: build release clean run lines burn

include $(D_FILES)

build: CFLAGS += -O1 -Wall -Wextra -ggdb -fsanitize=leak,address,undefined -fPIE -pie -fno-omit-frame-pointer
build: LFLAGS += -ggdb  -fsanitize=leak,address,undefined -fPIE -pie -fno-omit-frame-pointer
build: PPFLAGS += -DDEBUG
build: link

release: CFLAGS += -O3 -Wall -Wextra -flto=auto
release: LFLAGS += -s -flto=auto
release: PPFLAGS += -DRELEASE
release: link

burn:
	@rm -f $(BINDIR)/$(EXECUTABLE)
	@rm -rf $(OBJDIR)

clean: burn
	@rm -rf $(DEPDIR)

run:
	./$(BINDIR)/$(EXECUTABLE)

lines:
	@find $(SRCDIR) -type f -name "*.[chiS]" | xargs wc -l | sort -n

# link
link: $(O_FILES) $(D_FILES)
	$(CC) $(O_FILES) -o $(BINDIR)/$(EXECUTABLE) $(LFLAGS)

# build dependency files
$(DEPDIR)/%.d: $(SRCDIR)/%.c
	@mkdir -p $(@D)
	@echo -n "$@ " > $@
	$(CC) $(IFLAGS) -MM -MT $(patsubst $(DEPDIR)/%.d, $(OBJDIR)/%.o, $@) $(patsubst $(DEPDIR)/%.d, $(SRCDIR)/%.c, $@) >> $@ || rm $@

# compile code
$(OBJDIR)/%.o:
	@mkdir -p $(@D)
	$(CC) $(IFLAGS) $(CFLAGS) $(PPFLAGS) -c $(patsubst $(OBJDIR)/%.o, $(SRCDIR)/%.c, $@) -o $@

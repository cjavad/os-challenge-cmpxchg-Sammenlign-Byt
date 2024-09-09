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
CFLAGS = -march=sandybridge -mtune=sandybridge -masm=intel -std=c11
# linker flags
LFLAGS = -lssl -lcrypto -lpthread -lrt -lm -Wl,-rpath,lib

# build

C_FILES := $(shell find $(SRCDIR) -type f -name "*.c")

ifneq (,$(findstring release,$(MAKECMDGOALS)))
	DEPDIR := $(DEPDIR)/release
	OBJDIR := $(OBJDIR)/release
else
# (,$(findstring build,$(MAKECMDGOALS)))
	DEPDIR := $(DEPDIR)/build
	OBJDIR := $(OBJDIR)/build
endif

D_FILES := $(patsubst $(SRCDIR)/%.c, $(DEPDIR)/%.d, $(C_FILES))
O_FILES := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(C_FILES))

all: build

.PHONY: build release clean run lines

include $(D_FILES)

build: CFLAGS += -O2 -Wall -Wextra -ggdb -march=native
build: LFLAGS += -ggdb
build: PPFLAGS += -DDEBUG
build: link

release: CFLAGS += -O3 -Wall -Wextra -march=native
release: PPFLAGS += -DRELEASE
release: link

clean:
	@rm $(BINDIR)/$(EXECUTABLE)
	@rm -rf $(OBJDIR)
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

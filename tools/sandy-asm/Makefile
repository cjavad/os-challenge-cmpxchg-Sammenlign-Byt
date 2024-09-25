CC = gcc
EXECUTABLE = sandy-asm

C_FLAGS = -std=gnu11

C_FILES := $(shell find src -type f -name "*.c")

all:
	$(CC) $(C_FLAGS) -o $(EXECUTABLE) $(C_FILES)

run:
	./$(EXECUTABLE)

lines:
	@find src -type f -name "*.[ch]" | xargs wc -l | sort -n
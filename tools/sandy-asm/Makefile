CC = gcc
EXECUTABLE = sandy-asm

C_FLAGS = -std=gnu11
L_FLAGS = -z execstack

C_FILES := $(shell find src -type f -name "*.c")

all:
	$(CC) $(C_FLAGS) $(L_FLAGS) -o $(EXECUTABLE) $(C_FILES)

run:
	./$(EXECUTABLE)

lines:
	@find src -type f -name "*.[ch]" | xargs wc -l | sort -n
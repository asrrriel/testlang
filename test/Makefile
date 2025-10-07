CC := clang

CFLAGS := -std=c23 -g -O1 -I. -Wall -Wextra -pedantic -Werror -fsanitize=address,undefined,leak
LDFLAGS := -fsanitize=address,undefined,leak

TARGET := x86_64-pc-linux-gnu

SRC = $(shell find . -name "*.c")
OBJ = $(patsubst ./%.c,obj/%.o,$(SRC))

.PHONY: all

all: dirs $(OBJ) 
	$(CC) $(LDFLAGS) -o bin/test $(OBJ)

dirs:
	mkdir -p obj
	mkdir -p bin

obj/%.o: ./%.c
	mkdir -p obj/$(shell dirname $<)
	$(CC) -target $(TARGET) $(CFLAGS) -c $< -o $@

bear:
	bear -- make all -B
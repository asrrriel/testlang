CC := clang

CFLAGS := -g
LDFLAGS := 

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
	$(CC) -target $(TARGET) $(CFLAGS) -c $< -o $@

bear:
	bear -- make all -B
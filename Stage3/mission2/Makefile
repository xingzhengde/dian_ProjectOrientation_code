CC=gcc
CFLAGS=-Wall -Wextra -std=c11 -g -O0
TARGET=mission2
SRC=main.c ../common/parser.c ../common/graph.c ../common/build.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean

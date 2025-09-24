.PHONY: clean run all

CC=gcc
EXE=mission1
CFLAGS=-Iinclude -Wall -Wextra -std=c11
SRCS=main.c src/help.c src/argumentRecognize.c
OBJS=$(SRCS:.c=.o)
RM=rm -f

$(EXE):$(OBJS)
	$(CC) -o $@ $^
%.o:%.c
	$(CC) $(CFLAGS) -o $@ -c $<

run: $(EXE)
	./$(EXE) --help
clean:
	$(RM) $(EXE) $(OBJS)
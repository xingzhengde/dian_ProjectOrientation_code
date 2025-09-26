# 这个Makefile只是为了方便mission2的编译
.PHONY:clean run
CC=gcc
EXE=mission2
RM=rm

$(EXE):main.o
	$(CC) -o $@ $^
main.o:main.c
	$(CC) -g -O0 -o $@ -c $<
run: $(EXE)
	./$(EXE) --mk=../mission1/Makefile app
clean:
	$(RM) $(EXE) main.o
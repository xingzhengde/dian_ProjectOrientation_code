all: app test

app: main.o utils.o
    gcc -o app main.o utils.o

main.o: main.c common.h
    gcc -c main.c

utils.o: utils.c common.h
    gcc -c utils.c

test: test.o utils.o
    gcc -o test test.o utils.o

test.o: test.c
    gcc -c test.c

clean:
    rm -f app test *.o
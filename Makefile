CC=gcc
LIBS= -lm
CFLAGS=-Wall -Wextra -std=c99 -pedantic -ggdb

main: main.o time.o kr_time.h
	$(CC) $(CFLAGS) -g -o main main.o time.o kr_time.h $(LIBS)

main.o: main.c kr_time.h
	$(CC) $(CFLAGS) -g -c main.c -o main.o $(LIBS)

time.o: time.c kr_time.h
	$(CC) $(CFLAGS) -g -c time.c -o time.o $(LIBS)

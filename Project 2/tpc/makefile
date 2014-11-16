CFLAGS = -Wall
cleanup: tpc
	rm *.o
tpc: install tpc.o
	cc tpc.o -o bin/tpc -lrt -pthread
tpc.o: tpc.c
	cc -c tpc.c
install:
	mkdir bin

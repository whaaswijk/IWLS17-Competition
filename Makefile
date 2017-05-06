CC=g++
CFLAGS=-std=c++11

main: main.o
	$(CC) $(CFLAGS) main.o -o main libabc.a -ldl -lpthread

main.o: main.cpp
	$(CC) $(CFLAGS) -c main.cpp -I/home/haaswijk/abc/src

clean:
	$(RM) *.o main

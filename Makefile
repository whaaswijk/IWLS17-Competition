CC=g++
CFLAGS=-std=c++11 -fpermissive

main: main.cpp mlpreader.o mlpcircuit2mig.o 
	$(CC) $(CFLAGS) -o main main.cpp mlpreader.o mlpcircuit2mig.o -I/home/haaswijk/abc/src -I/home/haaswijk/libmajesty/include -I/home/haaswijk/hiredis -L/home/haaswijk/hiredis -L/home/haaswijk/abc -L/home/haaswijk/libmajesty/build/lib -L/home/haaswijk/libmajesty/build/lib/minisat -BStatic -lmajesty -labc -ldl -lpthread -lboost_filesystem -lboost_system -lMiniSat -lhiredis

mlpreader.o: mlpreader.c
	gcc -c mlpreader.c -I/home/haaswijk/libmajesty/include

mlpcircuit2mig.o: mlpcircuit2mig.c
	gcc -c mlpcircuit2mig.c -I/home/haaswijk/libmajesty/include

clean:
	$(RM) *.o main

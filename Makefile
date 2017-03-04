# really simple makefile for really simple compilation
# it's really simple

all: solution

solution: solution.c processor.o
	gcc solution.c processor.o -o run_sim

processor.o: processor.c
	gcc -c processor.c

clean:
	rm -f *.o
	rm -f ..?* .[!.]*

CFLAGS=-c -Wall -O3

all: isbic_demo

isbic_demo: main.o
	$(CC) main.o -o isbic_demo

isbic_demo.o: main.c
	$(CC) $(CFLAGS) main.c

clean:
	rm -rf *.o isbic_demo


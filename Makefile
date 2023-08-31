CFLAGS=-c -Wall -Os

all: isbic_demo

isbic_demo: main.o isbic.o isbic.h
	$(CC) main.o isbic.o -o isbic_demo

isbic_demo.o: main.c isbic.h
	$(CC) $(CFLAGS) main.c

isbic.o: isbic.c isbic.h
	$(CC) $(CFLAGS) isbic.c

clean:
	rm -rf *.o isbic_demo


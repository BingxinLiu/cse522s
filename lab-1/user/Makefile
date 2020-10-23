CC = arm-linux-gnueabihf-gcc
INCLUDE_DIR=$(PWD)/../include
CFLAGS =-I$(INCLUDE_DIR) -Wall

all: dense_mm floyd_warshall

clean:
	rm dense_mm floyd_warshall

dense_mm: dense_mm.c
	$(CC) $(CFLAGS) dense_mm.c -o dense_mm

floyd_warshall: floyd_warshall.c
	$(CC) $(CFLAGS) floyd_warshall.c -o floyd_warshall

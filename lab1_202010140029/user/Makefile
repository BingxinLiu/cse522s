CC = arm-linux-gnueabihf-gcc
INCLUDE_DIR=$(PWD)/../include
CFLAGS =-I$(INCLUDE_DIR) -Wall

all: dense_mm rand_assign_submatrix

clean:
	rm dense_mm rand_assign_submatrix

dense_mm: dense_mm.c
	$(CC) $(CFLAGS) dense_mm.c -o dense_mm

rand_assign_submatrix: rand_assign_submatrix.c
	$(CC) $(CFLAGS) rand_assign_submatrix.c -o rand_assign_submatrix

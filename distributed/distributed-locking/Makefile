
CC = gcc

compile: 
	dd if=/dev/zero of=mm count=1 bs=100
	$(CC) sub.c -o sub -pthread
	$(CC) rsum.c -o rsum

clean:
	rm -rf sub rsum mm


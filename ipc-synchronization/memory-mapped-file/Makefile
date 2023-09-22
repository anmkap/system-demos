
CC = gcc

compile:
	dd if=/dev/zero of=mm count=1 bs=1M
	$(CC) -o sub sub.c -pthread

clean:
	rm -rf sub
	rm -rf mm

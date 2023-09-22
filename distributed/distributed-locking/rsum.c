#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>

int main()
{
	int fd = open("./mm",O_RDWR);
	long *data = (long*)mmap(0,sizeof(long),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	printf("Sum = %ld\n",(*data));
}

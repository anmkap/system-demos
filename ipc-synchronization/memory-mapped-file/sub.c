#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h>

#define maxVal 2000000

void main(int argc, char *argv[])
{
	int fd = open("./mm",O_RDWR);
	long *data = mmap(0,sizeof(long),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	long *tmp = data;

	sem_t *mysem;
	mysem = sem_open("/my_sem",O_CREAT,0644,1);
	
	int ix = 0;
	if (argc==1) {
		while (ix < maxVal)
		{
			sem_wait(mysem);
			(*tmp)++;
			sem_post(mysem);
			printf("sum is %ld\n",(*tmp));
			ix++;
		}
	}
	else 
	{
		while (ix < maxVal) {
			sem_wait(mysem);
			(*tmp)--;
			sem_post(mysem);
			printf("sum is %ld\n",(*tmp));
			ix++;
		}
	}
}

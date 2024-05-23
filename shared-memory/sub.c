#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

#define KEY 1001

void main(int argc, char* argv[])
{
	size_t size = 1024;
	int shid = shmget(KEY,size,IPC_CREAT|0666);
	char* data = (char*)shmat(shid,NULL,SHM_RND);

	if (argc == 2)
	{
		bzero(data,size);
		char* msg = argv[1];
		char* tmp = msg;

		for(int i=0; i<strlen(argv[1]); i++)
		{
			*(data+i)=*(tmp+i);
		}
	}
	else if (argc == 1)
	{
  		printf("%s\n",data);

	}

  	shmdt(data);
}
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
# include <string.h>

int main (int argc, char **argv)
{
	int mk = mkfifo("./named_pipe.txt", 0666);
	int fid = open( "./named_pipe.txt", 0666);
 
	printf("mkfifo return = %d\n",mk);
	printf("fid = %d\n",fid);

	char *msg = "Hello World\n";
	int size = strlen(msg);

	if(argc == 1) 
	{
		while (1)
		{
    			printf("writing msg = %s",msg);
        		int i =  write(fid,msg,size);
        		printf("writing exit code = %d\n\n",i);
			sleep(1);
		}	
	}
	else
	{
		while (1) 
		{
			char read_msg[size];    
			int i= read(fid,read_msg,size	);
			printf("read_msg = %s",read_msg);
			printf("reading exit code = %d\n",i);
			sleep(1);
		}
	}
	close(fid);
	return 0;
}
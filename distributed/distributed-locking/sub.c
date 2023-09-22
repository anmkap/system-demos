#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <netdb.h>

#define numRequests 1000
#define queSize 1000
#define numListeners 1
#define numWorkers 1

pthread_mutex_t mut_que = PTHREAD_MUTEX_INITIALIZER;

struct trans {
int sfd;
unsigned int req[sizeof(unsigned int)*2];
};

struct pkt {
int portNum;
unsigned int pkt_seq;
unsigned int pkt_dat;
};


struct trans que[queSize];
struct trans *tmpP = que, *tmpC = que;
sem_t *semFull, *semEmpty, *semFile;
int proc_id, num_proc, port_num, port_numS, *port_nums, cntP = 0, cntC = 0;;
unsigned int my_time, *time_vals;
char file_path[25];

void *workerProc(void* ptr)
{
	struct sockaddr_in server_addr;
	struct hostent *server;
	server = gethostbyname("localhost");
	if(server == NULL)
	{
		fprintf(stderr, "No such host exists\n");
		exit(0);
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_numS);
	bcopy((char*)server->h_addr,(char*)&server_addr.sin_addr.s_addr,server->h_length);
	
	unsigned int buff[sizeof(unsigned int)*2];
	int itr = 0;
	while(itr<1000)
	{
		printf("Iteration = %d\n",itr);
		srand(time(NULL));
		buff[0] = proc_id;
		buff[1] = (rand()/proc_id)+proc_id;

		int socketfd = socket(AF_INET, SOCK_STREAM, 0);
		if(socketfd < 0)
		{
			fprintf(stderr, "Error creating socket\n");
			exit(0);
		}
	
		if(connect(socketfd,(struct sockaddr*) &server_addr,sizeof(server_addr)) < 0)
		{
			fprintf(stderr, "Error connecting\n");
		}		

		int n = write(socketfd, buff, sizeof(buff));
		if(n < 0)
		{
			fprintf(stderr, "Error writing to socket\n");
			exit(1);	
		}
		printf("Sent request to server\n");

		n = read(socketfd, buff, sizeof(buff));
		if(n < 0)
		{
			fprintf(stderr, "Error reading from socket\n");
			exit(1);	
		}
		printf("Received grant from server\n");

		sem_wait(semFile);
		int fd = open("./mm",O_RDWR);
		long *data = (long*)mmap(0,sizeof(long),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
		long *tmp = data;
		(*tmp)++;
		printf("sum is %ld\n",(*tmp));
		sem_post(semFile);

		n = write(socketfd, buff, sizeof(buff));
		if(n < 0)
		{
			fprintf(stderr, "Error writing to socket\n");
			exit(1);	
		}
		printf("Sent release to server - closing socket\n");
		
		close(socketfd);
		itr++;
	}
}

void *workerCoor(void* ptr)
{
	while(1)
	{
		printf("Worker loop started\n");
		sem_wait(semFull);
		pthread_mutex_lock(&mut_que);
		printf("Worker locked service queue\n");

		if (cntC>=queSize) {tmpC=que; cntC=0;}

		unsigned int buff[sizeof(unsigned int)*2];
		
		int socketfd = tmpC->sfd;
		buff[0] = (tmpC->req)[0];
		buff[1] = (tmpC->req)[1];

		tmpC++;
		cntC++;
	
		printf("Worker got request %u %u, unlocking service queue\n",buff[0],buff[1]);
		pthread_mutex_unlock(&mut_que);
		sem_post(semEmpty);		

		int n = write(socketfd, buff, sizeof(buff));
		if(n < 0)
		{
			fprintf(stderr, "Error writing to socket\n");
			exit(1);	
		}
		printf("Sending grant to client\n");

		bzero(buff,sizeof(buff));
		if (n = read(socketfd, buff, sizeof(buff)))
		{
			//printf("reply for i= %d -> %u %u %u %u\n",i,buffx[0],buffx[1],buffx[2],buffx[3]);
		}
		printf("Recieved release from client\n");

		close(socketfd);
	}
}

void *listenerCoor(void* ptr)
{
	struct sockaddr_in serveradd, clientaddr;
	socklen_t clientlen = sizeof(clientaddr);
	
	int socketfd = socket(AF_INET,SOCK_STREAM,0);
	if(socketfd < 0) fprintf(stderr, "Error creating socket\n");

	bzero((char*)&serveradd,sizeof(serveradd));
	serveradd.sin_family = AF_INET;
	serveradd.sin_addr.s_addr = INADDR_ANY;
	serveradd.sin_port = htons(port_num);

	if(bind(socketfd,(struct sockaddr*)&serveradd,sizeof(serveradd)) < 0) fprintf(stderr, "Error binding socket\n");
	listen(socketfd, numRequests);

	while(1)
	{
		printf("Coor Listening\n");
		int newsockfd = accept(socketfd,(struct sockaddr*)&clientaddr,&clientlen);
		if(newsockfd < 0) fprintf(stderr, "Error accepting client request\n");

		unsigned int buff[sizeof(unsigned int)*2];
		bzero(buff,sizeof(buff));

		int n = read(newsockfd, buff, sizeof(buff));

		sem_wait(semEmpty);
		pthread_mutex_lock(&mut_que);
		printf("Listener locked service queue\n");

		if (cntP>=queSize) {tmpP=que; cntP=0;}

		tmpP->sfd = newsockfd;
		(tmpP->req)[0] = buff[0];
		(tmpP->req)[1] = buff[1];

		tmpP++;
		cntP++;

		printf("Listener unlocking service queue\n");
		pthread_mutex_unlock(&mut_que);
		sem_post(semFull);
	}
}

int main (int argc, char* argv[])
{
	if (argc < 3)
	{
		fprintf(stderr, "Server Usage: ./sub 1` <port_number>\n");
		fprintf(stderr, "Client Usage: ./sub <proc_id> <port_number>\n");
		exit(0);
	}

	proc_id = atoi(argv[1]);
	printf("Process ID = %d\n",proc_id);

	// Process
	if (proc_id != 1)
	{

		port_numS = atoi(argv[2]);
		semFile = sem_open("/semFile",O_CREAT,0644,1);
		*(int*)semFile = 1;
		sleep(1);
		pthread_t wrkr;
		if(pthread_create(&wrkr,NULL,&workerProc,NULL)) fprintf(stderr, "Error creating worker thread\n");

		sleep(5);
	}

	// Coordinator
	else if (proc_id == 1)
	{	
		port_num = atoi(argv[2]);
		semEmpty = sem_open("/semEmpty",O_CREAT,0644,queSize);
		semFull = sem_open("/semFull",O_CREAT,0644,queSize);
		*(int*)semEmpty = queSize;
		*(int*)semFull = 0;
	
		pthread_t* lstnr;
		lstnr = (pthread_t*)malloc(sizeof(pthread_t)*numListeners);
		for (int i=0; i<numListeners; i++)
		{
			if(pthread_create((lstnr+i),NULL,&listenerCoor,NULL))
   			{
   				fprintf(stderr, "Error creating coordinator listener thread\n");
    			}
		}

		pthread_t* wrkr;
		wrkr = (pthread_t*)malloc(sizeof(pthread_t)*numWorkers);
		for (int i=0; i<numWorkers; i++)
		{
			if(pthread_create((wrkr+i),NULL,&workerCoor,NULL))
	   		{
	   			fprintf(stderr, "Error creating coordinator worker thread\n");
	    		}
		}
		sleep(120);
	}

	return 0;
}

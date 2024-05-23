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

#define numRequests 100
#define queSize 50
#define numListeners 1
#define numWorkers 1

struct trans {
int vld;
unsigned int msg_seq;
unsigned int msg_dat;
};

struct pkt {
int portNum;
unsigned int pkt_seq;
unsigned int pkt_dat;
};

pthread_mutex_t mut_que = PTHREAD_MUTEX_INITIALIZER;
struct trans que[queSize];
int proc_id, num_proc, port_num, port_num2, *port_nums;
unsigned int my_time, *time_vals;

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
	server_addr.sin_port = htons(port_num2);
	bcopy((char*)server->h_addr,(char*)&server_addr.sin_addr.s_addr,server->h_length);
	
	unsigned int buff[sizeof(unsigned int)*2];
	buff[0] = 0;

	while(1)
	{
		sleep(1);
		srand(time(NULL));
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

		printf("Sent msg %u to server\n",buff[1]);

		close(socketfd);
	}
}

void *listenerProc(void* ptr)
{
	printf("Listener Proc started listening on port %d\n",port_num);
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

	unsigned int seq = 1;
	bzero(que,sizeof(que));

	while(1)
	{
		int newsockfd = accept(socketfd,(struct sockaddr*)&clientaddr,&clientlen);
		unsigned int buff[sizeof(unsigned int)*2];
		int n = read(newsockfd, buff, sizeof(buff));
		unsigned int temp_time;
		if (buff[0] == seq)
		{
			pthread_mutex_lock(&mut_que);
			seq++;
			printf("--> Delivered seq=%u dat=%u\n",buff[0],buff[1]);

			int flg0 = 1;
			while (flg0)
			{
				flg0 = 0;
				for (int i=0;i<queSize;i++)
				{
					if ((que+i)->vld == 1 && (que+i)->msg_seq == seq)
					{
						(que+i)->vld = 0;
						printf("--> Delivered seq=%u dat=%u\n",(que+i)->msg_seq,(que+i)->msg_dat);
						seq++;
						flg0 = 1;
					}
				}
			}
			pthread_mutex_unlock(&mut_que);
		}
		else
		{
			pthread_mutex_lock(&mut_que);
			for (int i=0;i<queSize;i++)
			{
				if ((que+i)->vld == 0)
				{
					(que+i)->vld = 1;
					(que+i)->msg_seq = buff[0];
					(que+i)->msg_dat = buff[1];
					printf("<-- Queued seq=%u dat=%u\n",buff[0],buff[1]);
					break;
				}
			}
			pthread_mutex_unlock(&mut_que);
		}
		close(newsockfd);
	}
}

void *workerCoor(void* ptr)
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
	server_addr.sin_port = htons(((struct pkt*)ptr)->portNum);
	bcopy((char*)server->h_addr,(char*)&server_addr.sin_addr.s_addr,server->h_length);
	
	unsigned int buff[sizeof(unsigned int)*2];
	buff[0] = ((struct pkt*)ptr)->pkt_seq;
	buff[1] = ((struct pkt*)ptr)->pkt_dat;

	srand(time(NULL));
	unsigned int rnd = rand()%5;
	sleep(rnd);
	printf("Forwarding seq=%u dat=%u to portnum=%d\n",buff[0],buff[1],((struct pkt*)ptr)->portNum);

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

	close(socketfd);
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

	unsigned int seq = 0;
	bzero(que,sizeof(que));
	struct pkt *pyld;
	pyld = (struct pkt*)malloc(sizeof(struct pkt)*num_proc);

	while(1)
	{
		int newsockfd = accept(socketfd,(struct sockaddr*)&clientaddr,&clientlen);
		if(newsockfd < 0) fprintf(stderr, "Error accepting client request\n");

		unsigned int buff[sizeof(unsigned int)*2];
		bzero(buff,sizeof(buff));

		int n = read(newsockfd, buff, sizeof(buff));
		seq++;
		
		pthread_t wrkr;
		for (int i=0;i<num_proc;i++)
		{
			(pyld+i)->pkt_seq = seq;
			(pyld+i)->pkt_dat = buff[1];
			(pyld+i)->portNum = *(port_nums+i);
			if(pthread_create(&wrkr,NULL,&workerCoor,pyld+i)) fprintf(stderr, "Error creating worker thread\n");
		}

		close(newsockfd);
	}
}

int main (int argc, char* argv[])
{
	if (argc < 4 || argc != atoi(argv[3])+4)
	{
		fprintf(stderr, "Usage: ./sub <proc_id> <port_number> <num_proc> <proc_1_port> <proc_2_port> ...\n");
		exit(0);
	}
	
	proc_id = atoi(argv[1]);
	port_num = atoi(argv[2]);
	num_proc = atoi(argv[3]);
	port_num2 = atoi(argv[4]);
	printf("Process ID = %d\n",proc_id);

	// Process
	if (proc_id != 1)
	{
		sleep(1);
		pthread_t lsnr;
		if(pthread_create(&lsnr,NULL,&listenerProc,NULL)) fprintf(stderr, "Error creating listener thread\n");
		pthread_t wrkr;
		if(pthread_create(&wrkr,NULL,&workerProc,NULL)) fprintf(stderr, "Error creating worker thread\n");

		sleep(15);
	}
	// Manager
	else if (proc_id == 1)
	{	
		port_nums = (int*)malloc(sizeof(int)*num_proc);
		for (int i=0;i<num_proc;i++)
		{
			*(port_nums+i) = atoi(argv[4+i]);
			printf("Member port number %d\n",*(port_nums+i));
		}

		pthread_t lsnr;
		if(pthread_create(&lsnr,NULL,&listenerCoor,NULL)) fprintf(stderr, "Error creating prober thread\n");
		
		sleep(18);
	}

	return 0;
}
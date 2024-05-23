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
#define numListeners 1
#define numWorkers 1

pthread_mutex_t mut_flg = PTHREAD_MUTEX_INITIALIZER, mut_myt = PTHREAD_MUTEX_INITIALIZER;

int proc_id, num_proc, flg = 0;
unsigned int my_time;
unsigned int* time_vals;

void *prober(void* port)
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
	server_addr.sin_port = htons(*(int*)port);
	bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr,server->h_length);

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

	unsigned int buff[sizeof(unsigned int)*2];
	buff[0] = 1;
	buff[1] = 0;

	int n = write(socketfd, buff, sizeof(buff));
	if(n < 0)
	{
		fprintf(stderr, "Error writing to socket\n");
		exit(1);	
	}

	printf("Sent Synch Request\n");
	
	if (n = read(socketfd, buff, sizeof(buff)) > 0)
	{
		pthread_mutex_lock(&mut_flg);
		memcpy(time_vals+flg,buff+1,sizeof(unsigned int));
		flg++;
		pthread_mutex_unlock(&mut_flg);
	}

	while (flg<num_proc)
	{
		usleep(200000);
	}

	unsigned int avg_time, diff_time, sum_time = 0;
	for (int z=0;z<num_proc;z++) sum_time += *(time_vals+z);
	avg_time = sum_time/num_proc;
	
	if (avg_time > buff[1]) {diff_time = avg_time - buff[1]; buff[0] = 2;}
	else {diff_time = buff[1] - avg_time; buff[0] = 3;}
	buff[1] = diff_time;
	n = write(socketfd, buff, sizeof(buff));

	if (*(int*)port == 33332)
	{
		printf("Time values received: ");
		for (int z=0;z<num_proc;z++)
		{
			printf("%u ",*(time_vals+z));
		}	
		printf("\n");
		printf("Average %u sent\n",avg_time);
	}

	close(socketfd);
}

void *listener(void* fd)
{
	int sockfd = *(int*)fd;	
	struct sockaddr_in clientaddr;
	socklen_t clientlen = sizeof(clientaddr);

	printf("Listener Started\n");

	while(1)
	{
		int newsockfd = accept(sockfd,(struct sockaddr*)&clientaddr,&clientlen);
		if(newsockfd < 0) fprintf(stderr, "Error accepting client request\n");

		unsigned int buff[sizeof(unsigned int)*2];
		bzero(buff,sizeof(buff));
		int n = read(newsockfd, buff, sizeof(buff));

		printf("Received synch request\n");

		unsigned int temp_time;
		if (buff[0] == 1)
		{
			pthread_mutex_lock(&mut_myt);
			buff[1] = my_time;
			printf("Sending time %u\n",my_time);
			n = write(newsockfd, buff, sizeof(buff));
			n = read(newsockfd, buff, sizeof(buff));
		
			if (buff[0] == 2)
			{
				temp_time = my_time + buff[1];
				printf("Time before: %u\n",my_time);
				my_time = temp_time;
				printf("Time after %u\n",my_time);
			}
			else if (buff[0] == 3)
			{
				temp_time = my_time - buff[1];
				printf("Time before: %u\n",my_time);
				my_time = temp_time;
				printf("Time after: %u\n",my_time);
			}
			pthread_mutex_unlock(&mut_myt);
		}
	}
}

void *worker(void* fd)
{
	while(1)
	{
		usleep(500000);
		srand(time(NULL));
		unsigned int inc = rand()%100;
		pthread_mutex_lock(&mut_myt);
		my_time += inc;
		pthread_mutex_unlock(&mut_myt);
	}
}

int main (int argc, char* argv[])
{
	if (argc < 4 || argc != atoi(argv[3])+4)
	{
		fprintf(stderr, "Usage: ./sub <proc_id> <port_number> <num_proc> <proc_1_port> <proc_2_port> ...\n");
		exit(0);
	}
	
	num_proc = atoi(argv[3]);
	proc_id = atoi(argv[1]);
	printf("Process ID = %d\n",proc_id);

	// Process
	if (proc_id != 1)
	{
		srand(time(NULL));
		my_time = rand()%10000;
		printf("Starting Time: %u\n",my_time);

		int socketfd, portno;
		struct sockaddr_in serveradd, clientaddr;
		struct client_request;
	
		socketfd = socket(AF_INET,SOCK_STREAM,0);
		if(socketfd < 0) fprintf(stderr, "Error creating socket\n");

		bzero((char*)&serveradd,sizeof(serveradd));
		portno = atoi(argv[2]);
		serveradd.sin_family = AF_INET;
		serveradd.sin_addr.s_addr = INADDR_ANY;
		serveradd.sin_port = htons(portno);
	
		if(bind(socketfd,(struct sockaddr*)&serveradd,sizeof(serveradd)) < 0)
		{
			fprintf(stderr, "Error binding socket\n");
		}

		listen(socketfd, numRequests);

		pthread_t lsnr;
		if(pthread_create(&lsnr,NULL,&listener,&socketfd)) fprintf(stderr, "Error creating listener thread\n");

		pthread_t wrkr;
		if(pthread_create(&wrkr,NULL,&worker,&socketfd)) fprintf(stderr, "Error creating worker thread\n");
		
	}
	// Manager
	else if (proc_id == 1)
	{	
		time_vals = (unsigned int*)malloc(sizeof(unsigned int)*num_proc);
		int* portno;
		portno = (int*)malloc(sizeof(int)*num_proc);
		pthread_t* prbr;
		prbr = (pthread_t*)malloc(sizeof(pthread_t)*num_proc);

		for (int i=0;i<num_proc;i++)
		{
			*(portno+i) = atoi(argv[4+i]);
		}

		while(1)
		{
			sleep(3);

			for (int i=0;i<num_proc;i++)
			{
				if(pthread_create((prbr+i),NULL,&prober,portno+i)) fprintf(stderr, "Error creating prober thread\n");
			}

			for (int i=0;i<num_proc;i++) pthread_join(*(prbr+i),NULL);

			pthread_mutex_lock(&mut_flg);
			flg=0;
			pthread_mutex_unlock(&mut_flg);
		}
	}

	while(1)
	{
		usleep(750000);
		printf("Time: %u\n",my_time);
	}

	return 0;
}
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
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

using namespace std;

#define numRequests 1000
#define queueSize 100
#define numListeners 20
#define timeOut 20

struct trans {
int sfd;
int op;
double tar1;
double tar2;
};

struct toProc {
int port;
int sfd;
int op;
double tar1;
double tar2;
};

struct beSrvr {
int prt;
int lve;
int cmt;
double res;
};

pthread_mutex_t mut_que = PTHREAD_MUTEX_INITIALIZER, mut_cmt = PTHREAD_MUTEX_INITIALIZER, mut_comt = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cvFull = PTHREAD_COND_INITIALIZER, cvEmpty = PTHREAD_COND_INITIALIZER;


struct trans queue[queueSize];
struct trans* tmpP = queue;
struct trans* tmpC = queue;
struct beSrvr* be_info;
int num_proc, *port_num, fll = 0, abrt = 0, comt=0;

/* PROCESSOR */
void *processor(void* pkg)
{
	struct toProc *info = (struct toProc*)pkg;	
	struct sockaddr_in server_addr;
	struct hostent *server;
	server = gethostbyname("localhost");
	if(server == NULL)
	{
		fprintf(stderr, "No such host exists\n");
		exit(0);
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(info->port);
	bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr,server->h_length);

	int socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if(socketfd < 0)
	{
		fprintf(stderr, "Error creating socket\n");
		for(int i=0; i<num_proc; i++)
		{
			if ((be_info+i)->prt == info->port) (be_info+i)->lve = 0;
			pthread_exit(NULL);
		}
	}

	if(connect(socketfd,(struct sockaddr*) &server_addr,sizeof(server_addr)) < 0)
	{
		fprintf(stderr, "Error connecting to backend server\n");
		for(int i=0; i<num_proc; i++)
		{
			if ((be_info+i)->prt == info->port)
			{
				(be_info+i)->lve = 0; 
				pthread_exit(NULL);
			}
		}
	}

	double buff[sizeof(double)*3];
	buff[0] = info->op;
	buff[1] = info->tar1;
	buff[2] = info->tar2;

	//cout << "Processor sending to port " << info->port << " the request " << buff[0] << " " << buff[1] << " " << buff[2] << endl;

	int n = write(socketfd, buff, sizeof(buff));
	if(n < 0)
	{
		fprintf(stderr, "Error writing to socket\n");
		for(int i=0; i<num_proc; i++)
		{
			if ((be_info+i)->prt == info->port) (be_info+i)->lve = 0;
			pthread_exit(NULL);
		}
	}

	n = read(socketfd, buff, sizeof(buff));
	if (buff[0] == 100)
	{
		pthread_mutex_lock(&mut_cmt);
		for(int i=0; i<num_proc; i++)
		{
			if ((be_info+i)->prt == info->port) (be_info+i)->cmt = 1; 
		}
		pthread_mutex_unlock(&mut_cmt);
	}
	else if (buff[0] == 101)
	{
		pthread_mutex_lock(&mut_cmt);
		abrt = 1;
		pthread_mutex_unlock(&mut_cmt);
	}

	int comtemp = 0;
	while (comtemp == 0 and abrt == 0)
	{
		comtemp = 1;
		pthread_mutex_lock(&mut_comt);
		for(int i=0; i<num_proc; i++)
		{
			if ((be_info+i)->cmt == 0 and (be_info+i)->lve == 1) comtemp = 0; 
		}
		pthread_mutex_unlock(&mut_comt);
		usleep(100000);
	}
	comt = comtemp;

	if (comt == 1)
	{
		bzero(buff,sizeof(buff));
		buff[0] = 200;
		n = write(socketfd, buff, sizeof(buff));
		n = read(socketfd, buff, sizeof(buff));
		for(int i=0; i<num_proc; i++)
		{
			if ((be_info+i)->prt == info->port) (be_info+i)->res = buff[1]; 
		}
	}
	else if (abrt == 1)
	{
		bzero(buff,sizeof(buff));
		buff[0] = 201;
		n = write(socketfd, buff, sizeof(buff));
	}
}

/* TIMER */
void *timer(void* ptr)
{
	double tar1, tar2;
	int newsockfd, n, op;

	pthread_mutex_lock(&mut_que);
	while (fll == 0) pthread_cond_wait(&cvEmpty,&mut_que);
	if (tmpC-queue>=queueSize) tmpC=queue;
	newsockfd = tmpC->sfd;
	op = tmpC->op;
	tar1 = tmpC->tar1;
	tar2 = tmpC->tar2;
	fll--;
	tmpC++;	
	pthread_cond_signal(&cvFull);
	pthread_mutex_unlock(&mut_que);

	struct toProc *pass;
	pass = (struct toProc*)malloc(num_proc*sizeof(struct toProc));
	for (int i=0;i<num_proc;i++)
	{
		(pass+i)->port = (be_info+i)->prt;
		(pass+i)->sfd = newsockfd;
		(pass+i)->op = op;
		(pass+i)->tar1 = tar1;
		(pass+i)->tar2 = tar2;
		cout << "Timer popped/passing request to processor " << (pass+i)->port << " " << (pass+i)->sfd << " " << (pass+i)->op << " " << (pass+i)->tar1 << " " << (pass+i)->tar2 << endl;
	}

	pthread_t* prcs;
	prcs = (pthread_t*)malloc(sizeof(pthread_t)*num_proc);
	for (int i=0;i<num_proc;i++)
	{
		if(pthread_create((prcs+i),NULL,&processor,pass+i)) fprintf(stderr, "Error creating prober thread\n");
	}
	for (int i=0;i<num_proc;i++) pthread_join(*(prcs+i),NULL);

	int timeUp = 1;	
	while (timeUp<=timeOut and comt == 0 and abrt == 0)
	{
		usleep(50000);
		timeUp++;
	}

	double tempDub;
	char msg[64];
	bzero(msg,sizeof(msg));
	string oktemp, ok;
	double resTemp;
	for (int i=0;i<num_proc;i++)
	{
		if ((be_info+i)->lve == 1)
		{
			resTemp = (be_info+i)->res;
		}
	}

	if (op == 1) ok = "OK " + to_string((unsigned int)resTemp);
	else
	{
		oktemp = "OK " + to_string(resTemp);
		ok = oktemp.substr(0,oktemp.find('.')+3);
	}
	
	string err = "ERR Account " + to_string((unsigned int)tar1) + " does not exist.";
	string tmt = "Transaction timed out";

	if (timeUp == timeOut)
	{
		strcpy(msg,tmt.c_str());
		n = write(newsockfd, msg, 64);
	}	
	else if (comt == 1)
	{
		strcpy(msg,ok.c_str());
		n = write(newsockfd, msg, 64);
	}
	else if (abrt == 1)
	{
		strcpy(msg,err.c_str());
		n = write(newsockfd, msg, 64);
	}

	for (int i=0;i<num_proc;i++)
	{
		(be_info+i)->lve = 1;
		(be_info+i)->cmt = 0;
	}
	abrt = 0;
	comt = 0;
}

// WORKER //
void *worker(void* ptr)
{
	while(1)
	{
		//cout << "Worker looped" << endl;
		pthread_t timr;
		if(pthread_create(&timr,NULL,&timer,NULL)) fprintf(stderr, "Error creating timer thread\n");
		pthread_join(timr,NULL);
	}
}

/* LISTENER */
void *listener(void* fd)
{
	int sockfd = *(int*)fd;	
	struct sockaddr_in clientaddr;
	socklen_t clientlen = sizeof(clientaddr);

	while(1)
	{
		int newsockfd = accept(sockfd,(struct sockaddr*)&clientaddr,&clientlen);
		if(newsockfd < 0) fprintf(stderr, "Error accepting client request\n");

		string op, tar1, tar2;
		char rcv[32];
		int n = read(newsockfd, rcv, 32);

		op.assign(rcv,rcv+8);
		tar1.assign(rcv+8,rcv+20);
		tar2.assign(rcv+20,rcv+32);

		int opint;
		if (op.substr(0,6) == "create") opint = 1;
		else if (op.substr(0,6) == "update") opint = 2;
		else if (op.substr(0,5) == "query") opint = 3;

		pthread_mutex_lock(&mut_que);
		while (fll >= queueSize) pthread_cond_wait(&cvFull,&mut_que);
		if (tmpP-queue>=queueSize) tmpP=queue;
		tmpP->sfd = newsockfd;
		tmpP->op = opint;
		tmpP->tar1 = stod(tar1);
		if (opint == 2) tmpP->tar2 = stod(tar2);
		else tmpP->tar2 = 0;
		//cout << "Listener pushed request " << tmpP->sfd << " " << tmpP->op << " " << tmpP->tar1 << " " << tmpP->tar2 << endl;
		fll++;
		tmpP++;
		pthread_cond_signal(&cvEmpty);
		pthread_mutex_unlock(&mut_que);
	}
}

// MAIN //
int main (int argc, char* argv[])
{
	if (argc < 4 or argc != atoi(argv[2])+3)
	{
		fprintf(stderr, "Usage: ./server <port_number> <num_proc> <proc_1_port> <proc_2_port> ...\n");
		exit(0);
	}

	num_proc = atoi(argv[2]);
	be_info = (struct beSrvr*)malloc(num_proc*sizeof(beSrvr));
	for (int i=0;i<num_proc;i++)
	{
		(be_info+i)->prt = atoi(argv[3+i]);
		(be_info+i)->lve = 1;
		(be_info+i)->cmt = 0;
	}
	for (int i=0;i<num_proc;i++) cout << "Backend port detected " << (be_info+i)->prt << endl;

	int socketfd, portno;
	struct sockaddr_in serveradd;

	socketfd = socket(AF_INET,SOCK_STREAM, 0);
	if(socketfd < 0) fprintf(stderr, "Error creating socket\n");

	bzero((char *)&serveradd, sizeof(serveradd));
	portno = atoi(argv[1]);
	serveradd.sin_family = AF_INET;
	serveradd.sin_addr.s_addr = INADDR_ANY;
	serveradd.sin_port = htons(portno);
	
	if(bind(socketfd,(struct sockaddr*)&serveradd,sizeof(serveradd)) < 0) fprintf(stderr, "Error binding socket\n");
	listen(socketfd, numRequests);

	pthread_t lstnr;
	if(pthread_create(&lstnr,NULL,&listener,&socketfd)) fprintf(stderr, "Error creating listener thread\n");

	pthread_t wrkr;
	if(pthread_create(&wrkr,NULL,&worker,NULL)) fprintf(stderr, "Error creating worker thread\n");

	while(1)
	{
		sleep(5);/*
		}*/
	}
	return 0;
}

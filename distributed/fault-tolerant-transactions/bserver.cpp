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

pthread_mutex_t mut_que = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cvFull = PTHREAD_COND_INITIALIZER, cvEmpty = PTHREAD_COND_INITIALIZER;

struct trans queue[queueSize];
struct trans* tmpP = queue;
struct trans* tmpC = queue;
sem_t *semFull, *semEmpty;
int num_proc, fll = 0;
string port_num;

/* WORKER */
void *worker(void* ptr)
{
	double tar1tmpD, tar2tmpD;
	string tar1, tar2, tar1tmp, tar2tmp;
	int newsockfd, n, op;
	double buff[sizeof(double)*3];

	while(1)
	{
		pthread_mutex_lock(&mut_que);
		while (fll == 0) pthread_cond_wait(&cvEmpty,&mut_que);
		if (tmpC-queue>=queueSize) tmpC=queue;
		newsockfd = tmpC->sfd;
		op = tmpC->op;
		tar1tmpD = tmpC->tar1;
		tar2tmpD = tmpC->tar2;
		fll--;
		tmpC++;
		pthread_cond_signal(&cvFull);
		pthread_mutex_unlock(&mut_que);

		if (op == 1)
		{
			tar1tmp = to_string(tar1tmpD);
			tar1 = tar1tmp.substr(0,tar1tmp.find('.')+3);
			tar2 = "NONE";
		}
		else if (op == 2)
		{
			tar1 = to_string((int)tar1tmpD);
			tar2tmp = to_string(tar2tmpD);
			tar2 = tar2tmp.substr(0,tar2tmp.find('.')+3);
		}
		else if (op == 3)
		{
			tar1 = to_string((int)tar1tmpD);
			tar2 = "NONE";
		}

		cout << "Worker popped request " << op << " " << tar1 << " " << tar2 << endl;

		string temp = "", tempOut, tempQry = "";
		char ltr, spc = ' ', bks = '\n';
		int vld = 1, mch = 0, ldx = 0, amt = 0, ldxOut;
		string txt = port_num + ".txt";
		string txt2 = port_num + "_tmp.txt";

		ifstream rdr(txt);
		while (!rdr.eof())
		{
			rdr.get(ltr);
			if (ltr == bks)
			{
				vld = 1;
				amt = 0;
				ldx++;
			
			}	
			else if (ltr == spc)
			{
				tempOut = temp;
				if (tempOut == tar1) { mch = 1; ldxOut = ldx; amt = 1;}
				temp = "";
				vld = 0;
			}
			else if (vld == 1)
			{
				temp += ltr;
			}
			else if (amt == 1)
			{
				tempQry += ltr;
			}	
		}
		rdr.close();
	
		if (op == 1)
		{
			bzero(buff,sizeof(buff));
			buff[0] = 100;
			n = write(newsockfd, buff, sizeof(buff));
			bzero(buff,sizeof(buff));
			n = read(newsockfd, buff, sizeof(buff));
			if (buff[0] == 200)
			{
				unsigned long nid = stoul(tempOut) + 1;
				ofstream wrtr(txt, ios::app);
				wrtr << nid << " " << tar1 << endl;
				wrtr.close();
				buff[0] = 100;
				buff[1] = double(nid);
				n = write(newsockfd, buff, sizeof(buff));
			}
		}
		else if (op == 2 and mch == 0)
		{
			bzero(buff,sizeof(buff));
			buff[0] = 101;
			n = write(newsockfd, buff, sizeof(buff));
		}
		else if (op == 2 and mch == 1)
		{
			bzero(buff,sizeof(buff));
			buff[0] = 100;
			n = write(newsockfd, buff, sizeof(buff));
			bzero(buff,sizeof(buff));
			n = read(newsockfd, buff, sizeof(buff));
			
			if (buff[0] == 200)
			{
				ifstream rdr(txt);
				ofstream wrtr(txt2);
				int ldxNew = 0;
				while (!rdr.eof())
				{
					rdr.get(ltr);
					if (ltr != bks)
					{
						wrtr << ltr;
					}
					else if (ldxNew != ldx-1)
					{
						ldxNew++;
						wrtr << '\n';
					}
				}
				rdr.close();
				wrtr.close();
		
				ofstream wrtr2(txt);
				ifstream rdr2(txt2);
				ldx = 0;
				temp = "";
				while (!rdr2.eof())
				{
					rdr2.get(ltr);
					if (ltr == bks)
					{
						wrtr2 << temp;
						temp = "";
						ldx++;
					}
					if (ldx == ldxOut)
					{
						temp = "\n" + tar1 + " " + tar2;
					}
					if (ldx != ldxOut)
					{
						temp += ltr;
					}
				}
				wrtr2.close();
				rdr2.close();	
			}
			buff[0] = 100;
			buff[1] = stod(tar2);
			n = write(newsockfd, buff, sizeof(buff));
		}
		else if (op == 3 and mch == 0)
		{
			bzero(buff,sizeof(buff));
			buff[0] = 101;
			n = write(newsockfd, buff, sizeof(buff));
		}
		else if (op == 3 and mch == 1)
		{
			bzero(buff,sizeof(buff));
			buff[0] = 100;
			n = write(newsockfd, buff, sizeof(buff));
			n = read(newsockfd, buff, sizeof(buff));
			buff[1] = stod(tempQry);
			if (buff[0] == 200) n = write(newsockfd, buff, sizeof(buff));
		}
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

		double buff[sizeof(double)*3];
		int n = read(newsockfd, buff, sizeof(buff));

		pthread_mutex_lock(&mut_que);
		while (fll >= queueSize) pthread_cond_wait(&cvFull,&mut_que);
		if (tmpP-queue>=queueSize) tmpP=queue;
		tmpP->sfd = newsockfd;
		tmpP->op = (int)buff[0];
		tmpP->tar1 = buff[1];
		tmpP->tar2 = buff[2];
		fll++;
		tmpP++;
		pthread_cond_signal(&cvEmpty);
		pthread_mutex_unlock(&mut_que);	
	}
}

// MAIN //
int main (int argc, char* argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage: ./server <port_number>\n");
		exit(0);
	}

	port_num = argv[1];

	ofstream wrtr(port_num+".txt");
	wrtr << "99 0\n";
	wrtr.close();

	semEmpty = sem_open("/semEmpty",O_CREAT,0644,queueSize);
	semFull = sem_open("/semFull",O_CREAT,0644,queueSize);
	*(int*)semEmpty = queueSize;
	*(int*)semFull = 0;

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

	pthread_t* lstnr;
	lstnr = (pthread_t*)malloc(sizeof(pthread_t)*numListeners);
	for (int i=0; i<numListeners; i++)
	{
		if(pthread_create((lstnr+i),NULL,&listener,&socketfd)) fprintf(stderr, "Error creating listener thread\n");
	}
	
	pthread_t wrkr;
	if(pthread_create(&wrkr,NULL,&worker,NULL)) fprintf(stderr, "Error creating worker thread\n");

	float val;
	while(1)
	{
		sleep(5);/*
		}*/
	}
	return 0;
}

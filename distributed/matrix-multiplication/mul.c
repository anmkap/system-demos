#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <pthread.h>
#include <sys/time.h>

struct toThread {
unsigned int lw_bnd;
unsigned int up_bnd;
};

unsigned int m,n,p;
long *mx1,*mx2,*mx3;

void *multiplier(void *bnd)
{	
	struct toThread *m = (struct toThread*)bnd;
	for (unsigned int mx=m->lw_bnd; mx<m->up_bnd; mx++)
	{
		for (unsigned int px=0; px<p; px++)
		{
			long sum = 0;
			for (unsigned int nx=0; nx<n; nx++)
			{
				long sm1 = *(mx1+((mx*n)+nx));
				long sm2 = *(mx2+((nx*p)+px));
				sum += sm1*sm2;
			}	
			*(mx3+((mx*p)+px)) = sum;
		}
	}
}

int main (int argc, char* argv[])
{
	if (argc < 3)
	{
		fprintf(stderr,"Usage: ./mul <number-of-threads> <input-file-path>\n");
		exit(0);
	}

	FILE* fp;
	fp = fopen(argv[2],"r");
	if (fp == 0)
	{
		fprintf(stderr,"Error opening input file for reading\n");
		exit(0);
	}
	
	fscanf(fp,"%u %u",&m,&n);
	mx1 = (long*)malloc(m*n*sizeof(long));
	for (int i=0;i<m*n;i++) fscanf(fp,"%li",mx1+i);

	fscanf(fp,"%u %u",&n,&p);
	mx2 = (long*)malloc(n*p*sizeof(long));
	for (int i=0;i<n*p;i++) fscanf(fp,"%li",mx2+i);

	mx3 = (long*)malloc(m*p*sizeof(long));

	unsigned int num = atoi(argv[1]);
	pthread_t* thread;
	thread = (pthread_t*)malloc(sizeof(pthread_t)*num);
	struct toThread *bnd;
	bnd = (struct toThread*)malloc(num*sizeof(struct toThread));

	struct timeval t1, t2;
	gettimeofday(&t1,NULL);

	if (num>m)
	{
		fprintf(stderr,"Number of threads should be less or equal to %u\n",m);
		exit(0);
	}

	unsigned int div;
	if (m%num) div = m+num-1;
	else div = m;

	for (unsigned int i=0;i<num;i++)
	{
		(bnd+i)->lw_bnd = i*div/num;
		(bnd+i)->up_bnd = (i+1)*div/num;
		if ((bnd+i)->up_bnd > m) (bnd+i)->up_bnd = m;
   		if(pthread_create(thread+i,NULL,&multiplier,bnd+i)) fprintf(stderr,"Error creating thread\n");
	}
	
	for (int i=0;i<num;i++) pthread_join(*(thread+i),NULL);
	gettimeofday(&t2,NULL);

	printf("========\n");
	for (int xx=0;xx<m;xx++)
	{
		for (int yy=0;yy<p;yy++) printf("%li ",*(mx3+(xx*p)+yy));
		printf("\n");
	}
	
	free(mx1); free(mx2); free(mx3); free(bnd); free(thread);
	printf("\nTime Elapsed = %li milliseconds\n",(((t2.tv_sec*1000000)+t2.tv_usec)-((t1.tv_sec*1000000)+t1.tv_usec))/1000);
}

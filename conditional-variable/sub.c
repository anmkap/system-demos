#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <pthread.h>

#define producerSleep 1
#define consumerSleep 3
#define bufferSize 4
#define someValue 99

pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER; // or use pthread_mutex_init(&mx,NULL)
pthread_cond_t cvFull = PTHREAD_COND_INITIALIZER, cvEmpty = PTHREAD_COND_INITIALIZER; // or use pthread_cond_init(&cv,NULL)
int queue[bufferSize], fll = 0, *tmpP = queue, *tmpC = queue;

void *producer()
{
    while(1)
    {
        pthread_mutex_lock(&mx);
	printf("Producer locked mutex\n");
	while (fll >= bufferSize)
	{	
		printf("Buffer is full. Producer will wait for consumer signal\n");
		pthread_cond_wait(&cvFull,&mx);
		printf("Producer was signalled by a consumer\n");
	}
	if (tmpP-queue>=bufferSize) tmpP = queue;
        *tmpP = someValue;
	fll++;
	printf("Producer put a %d. Fill count is %d\n",someValue,fll);
	tmpP++;
	printf("Producer is signalling the consumer\n");
	pthread_cond_signal(&cvEmpty);
	pthread_mutex_unlock(&mx);
	printf("Producer released mutex\n");
        sleep(producerSleep);
    }
}

void *consumer()
{
    while(1)
    {	
        pthread_mutex_lock(&mx);
	printf("Consumer locked mutex\n");
	while (fll == 0)
	{
		printf("Buffer is empty. Consumer will wait for producer signal\n");
		pthread_cond_wait(&cvEmpty,&mx);
		printf("Consumer was signalled by the producer\n");
	}
	if (tmpC-queue>=bufferSize) tmpC = queue;
        int consumerVar = *tmpC;
	fll--;
	printf("Consumer got a %d. Fill count is %d\n",consumerVar,fll);
	tmpC++;
	printf("Consumer is signalling the producer\n");
	pthread_cond_signal(&cvFull);
	pthread_mutex_unlock(&mx);
	printf("Consumer released mutex\n");
        sleep(consumerSleep);
    }
}

void main()
{
	pthread_t producerThread, consumerThread;
	int producerError, consumerError;
	int ret = pthread_cond_init(&cvEmpty,NULL);

	if(consumerError = pthread_create(&consumerThread, NULL, &consumer, NULL)) printf("Consumer thread creation failed: %d\n", consumerError);
	if(producerError=pthread_create(&producerThread, NULL, &producer, NULL)) printf("Producer thread creation failed: %d\n", producerError);
  
	pthread_join(producerThread, NULL);
	pthread_join(consumerThread, NULL);
	exit(0);
}
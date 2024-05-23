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

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int queue[bufferSize], *tmpP = queue, *tmpC = queue;
sem_t *semFull, *semEmpty;

void *producerFunction()
{
    while (1)
    {
	printf("Empty slots in queue: %d\n",*(int*)semEmpty);
        sem_wait(semEmpty);
        pthread_mutex_lock(&mutex1);
	if (tmpP-queue>=bufferSize) tmpP = queue;
	printf("***Producer's Buffer Pointer: %p\n",tmpP);
        *tmpP = someValue;
        printf("Producer put a %d\n",someValue);
        tmpP++;
        pthread_mutex_unlock(&mutex1);
        sem_post(semFull);
        sleep(producerSleep);
    }
}

void *consumerFunction()
{
    while (1)
    {
        sem_wait(semFull);
        pthread_mutex_lock(&mutex1);
	if (tmpC-queue>=bufferSize) tmpC = queue;
	printf("***Consumer's Buffer Pointer: %p\n",tmpC);
        int consumerVar = *tmpC;
        printf("Consumer got a %d\n",consumerVar);
        tmpC++;
        pthread_mutex_unlock(&mutex1);
        sem_post(semEmpty);
        sleep(consumerSleep);
    }
}

void main()
{
    pthread_t producerThread, consumerThread;
    int producerError, consumerError;

    semFull = sem_open("/semFull",O_CREAT,0644,bufferSize);
    semEmpty = sem_open("/semEmpty",O_CREAT,0644,bufferSize);

    *(int*)semEmpty = bufferSize;
    *(int*)semFull = 0;

    if( (consumerError=pthread_create(&consumerThread, NULL, &consumerFunction, NULL)) )
    {
    printf("Consumer thread creation failed: %d\n", consumerError);
    }

    if( (producerError=pthread_create(&producerThread, NULL, &producerFunction, NULL)) )
    {
    printf("Producer thread creation failed: %d\n", producerError);
    }
    
    pthread_join(producerThread, NULL);
    pthread_join(consumerThread, NULL);
    exit(0);
}
#include "HTTPThreader.h"
#include <pthread.h>

//static so its not visible outside this file
pthread_cond_t recieved = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex =  PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

void init(Dispatch* dispatcher) {
    const bool debug = false;
    if (debug == true) {
        printf("Creating Threads...\n");
    }
    for (int x = 0; x < dispatcher->threadcount; x++) {
        // create threads
        pthread_create(&dispatcher->threads[x], NULL, run, (void*)dispatcher);
        if (debug == true) {
            printf("Thread: #%ld Online at Index %d\n",dispatcher->threads[x],x);
        }
    }
}

void* run(void* data) {
    const bool debug = true;
    Dispatch* dispatcher = (Dispatch*) data;
    const int id = *dispatcher->current;
    (*dispatcher->current)++;
    client* newclient;
    pthread_mutex_lock(&mutex);
    while (1) {
        if (*dispatcher->waiting <= 0) {
			*dispatcher->waiting = 0;
            pthread_cond_wait(&recieved, &mutex);
        }
        newclient = popclient(dispatcher);
		printf("now waiting: %d\n",*dispatcher->waiting);
		if (*dispatcher->waiting < 0) {
			exit(EXIT_FAILURE);
		}
        if (newclient != NULL) {
            printf("thread #%d responding to client #%ld\n",id,newclient->fd);
            accumulator(newclient->fd,dispatcher);
        }
        free(newclient);
    }
    return NULL;
}

client* popclient(Dispatch* dispatcher) {
    client* newclient;
    int rc = pthread_mutex_lock(&mutex);
    if (*dispatcher->waiting > 0) {
        newclient = dispatcher->waitlist;
        dispatcher->waitlist = newclient->next;
        if (dispatcher->waitlist == NULL) { /* this was the last request on the list */
            dispatcher->newest = NULL;
        }
		(*dispatcher->waiting)--;
    } else {
        newclient = NULL;
    }
    rc = pthread_mutex_unlock(&mutex);
    return newclient;
}

void addclient(Dispatch* dispatcher, ssize_t client_socket) {
    client* newclient = malloc(sizeof(client));
    newclient->fd = client_socket;
    newclient->next = NULL;
    int rc = pthread_mutex_lock(&mutex);
    if (*dispatcher->waiting == 0) {
        dispatcher->waitlist = newclient;
        dispatcher->newest = newclient;
    } else {
        dispatcher->newest->next = newclient;
        dispatcher->newest = newclient;
    }
    (*dispatcher->waiting)++;
	printf("waiting: %d\n",*dispatcher->waiting);
    rc = pthread_mutex_unlock(&mutex);
    rc = pthread_cond_signal(&recieved);
}

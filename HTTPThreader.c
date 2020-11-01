#include "HTTPThreader.h"
#include <pthread.h>
Dispatch newdispatch(Parameters params) {
    Dispatch dispatcher = {
        .threadcount = params.threads,
        // logging
        .log = params.log,
        .logname = params.logname,
        // execution
        .version = params.version,
    };
    static int wait = 0;
    dispatcher.waiting = &wait;
    static int current = 0;
    dispatcher.current = &current;
    return dispatcher;
}

void init(Dispatch dispatcher) {
    const bool debug = true;
    if (debug == true) {
        printf("Creating Threads...\n");
    }
    pthread_t threads[dispatcher.threadcount];
    dispatcher.threads = threads;
    for (int x = 0; x < dispatcher.threadcount; x++) {
        // create threads
        pthread_create(&threads[x], NULL, run, (void*)&dispatcher);
        if (debug == true) {
            printf("Thread: #%ld Online at Index %d\n",threads[x],x);
        }
    }
}

void* run(void* data) {
    const bool debug = true;
    Dispatch* dispatcher = (Dispatch*) data;
    const int id = *dispatcher->current;
    *dispatcher->current++;
    if (debug == true) {
        printf("Thread info:\n");
        printf("Thread ID: %d\n",id);
        printf("dispatcher waiting: %d\n",*dispatcher->waiting);
    }
    client* newclient;
    pthread_mutex_lock(&mutex);
    while (1) {
        if (*dispatcher->waiting == 0) {
            if (debug == true) {
                printf("Thread #%d going to sleep...\n",id);
            }
            pthread_cond_wait(&recieved, &mutex);
            if (debug == true) {
                printf("Thread #%d is woken...\n",id);
            }
        }
        if (debug == true) {
            printf("thread #%d getting client...\n",id);
        }
        newclient = popclient(dispatcher);
        dispatcher->waiting--;
        if (debug == true) {
            printf("thread #%d responding...\n",id);
        }
        if (newclient != NULL) {
            accumulator(newclient->fd,dispatcher);
        } else {
            if (debug == true) {
                printf("client is null...\n");
            } 
        }
    }
    return NULL;
}

client* popclient(Dispatch* dispatcher) {
    const bool debug = true;
    if (debug == true) {
        printf("getting client from waitlist...\n");
        printf("waiting: %d\n",*dispatcher->waiting);
    }
    client* newclient;
    int code = pthread_mutex_lock(&mutex);
    if (debug == true) {
        printf("popclient lock code: %d\n",code);
    }
    if (dispatcher->waiting == 0) {
        return NULL;
    } else if (*dispatcher->waiting == 1) {
        if (debug == true) {
            printf("1 found...\n");
        }
        newclient = dispatcher->waitlist;
        dispatcher->newest = NULL;
    } else {
        if (debug == true) {
            printf("many found...\n");
        }
        newclient = dispatcher->waitlist;
        dispatcher->waitlist = newclient->next;
    }
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&recieved);
    return newclient;
}

void addclient(Dispatch dispatcher,ssize_t client_socket) {
    const bool debug = true;
    if (debug == true) {
        printf("adding client...\n");
        printf("client fd: %ld\n",client_socket);
    }
    client* newclient = (client*) malloc(sizeof(client));
    newclient->fd = client_socket;
    newclient->next = NULL;
    if (debug == true) {
        printf("client object:\n");
        printf("fd: %ld\n",newclient->fd);
    }
    int code = pthread_mutex_lock(&mutex);
    if (code < 0) {
        printf("code: %d\n",code);
    }
    int temp = dispatcher.waiting;
    printf("WAIT VALUE: %d",temp);
    switch (0) {
        case 0:
            dispatcher.waitlist = newclient;
            dispatcher.newest = newclient;
            break;
        default:
            dispatcher.newest->next = newclient;
            dispatcher.newest = newclient;
            break;
    }
    dispatcher.waiting++;
    if (debug == true) {
        printf("Queue Size: %d\n",*dispatcher.waiting);
    }
    code = pthread_mutex_unlock(&mutex);
    code = pthread_cond_signal(&recieved);
    if (debug == true) {
        printf("awake signal sent...\n");
    }
}
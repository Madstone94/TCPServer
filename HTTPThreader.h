#ifndef HTTPThreader_H_
#define HTTPThreader_H_
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "Parameters.h"
#include <unistd.h>
#include <stdatomic.h>

// from kent state example, superior to my version
typedef struct client {
    ssize_t fd;
    // needed struct to work
    struct client* next;
}client;

typedef struct dispatchObj {
    // waitlist
    atomic_int* entries;
    atomic_int* fails;
    atomic_int* waiting;
    client* waitlist;
    client* newest;
    // logging
    bool log;
    int fd;
    off_t* offset;
    char* logname;
    char* version;
    // used to identify threads
    int* current;
    int threadcount;
    pthread_t threads[];
}Dispatch;

typedef struct Thread {
    int id;
    Dispatch dispatcher;
}Thread;

void init(Dispatch* dispatcher);
// thread processes
void* run(void* data);
client* popclient(Dispatch* dispatcher);
void addclient(Dispatch* dispatcher,ssize_t client_socket);
#endif
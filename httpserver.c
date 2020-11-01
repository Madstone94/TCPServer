#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <errno.h> // error codes
#include <unistd.h> // write
#include <string.h> // memset
#include <stdlib.h> // atoi
#include <stdbool.h> // booleans
#include <stdint.h> // read writer

#include "Parameters.h"
#include "HTTPThreader.h"
#include "HTTParse.h"

// prototypes
// SERVER FUNCTIONS - DO NOT TOUCH
void create_server(Parameters params);
void create_socket(struct sockaddr_in server_address, socklen_t addresslength, Parameters params);
void connectclient(ssize_t server_socket, Parameters params);

#define BUFFER_SIZE 4096


int main(int argc, char** argv) {
	const char* version = "HTTP/1.1";
    if (argc > 1) {
        Parameters params = newparams(argc,argv,version);
        create_server(params);
    } else {
        printf("port not given\n");
    }
    return 0;
}

// server

void create_server(Parameters params) {
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(params.port));
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    socklen_t addresslength = sizeof(server_address);
	create_socket(server_address,addresslength,params);
}

void create_socket(struct sockaddr_in server_address, socklen_t addresslength, Parameters params) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        exit(EXIT_FAILURE);
    }
    int enable = 1;
    int ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    ret = bind(server_socket, (struct sockaddr *) &server_address, addresslength);
    if (ret < 0) {
        exit(EXIT_FAILURE);
    }
    ret = listen(server_socket, 5); // 5 should be enough, if not use SOMAXCONN
    if (ret < 0) {
        exit(EXIT_FAILURE);
    }
	connectclient(server_socket,params);
}

void connectclient(ssize_t server_socket, const Parameters params) {
    Dispatch* dispatcher = malloc(sizeof(Dispatch) + (params.threads * sizeof(pthread_t)));
    dispatcher->threadcount = params.threads;
    dispatcher->log = params.log;
    dispatcher->logname = params.logname;
    dispatcher->version = params.version;
    static atomic_int wait = 0;
    dispatcher->waiting = &wait;
    static int current = 0;
    dispatcher->current = &current;
    static atomic_int entries = 0;
    dispatcher->entries = &entries;
    static atomic_int fails = 0;
    dispatcher->fails = &fails;
    static off_t offset = 0;
    dispatcher->offset = &offset;
    init(dispatcher);
    if (dispatcher->log == true) {
        dispatcher->fd = open(dispatcher->logname, O_CREAT | O_TRUNC | O_RDWR | O_APPEND,0644);
    }
    while (1) {
        printf("[+] server is waiting...\n");
        struct sockaddr client_address;
        socklen_t client_addresslength = sizeof(client_address);
        ssize_t client_socket = accept(server_socket, &client_address, &client_addresslength);
        printf("NEW CLIENT: %ld\n",client_socket);
        printf("ERROR: %d\n",errno);
        addclient(dispatcher,client_socket);
        (*dispatcher->entries)++;
        printf("total requests accepted: %d\n",(*dispatcher->entries));
    }
}
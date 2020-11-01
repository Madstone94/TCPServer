#ifndef HTTParse_H_
#define HTTParse_H_
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h> // read writer
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include "HTTPThreader.h"

#define BUFFER_SIZE 4096

// exists because single file was too cumbersome
// easier than passing a bunch of variables
typedef struct commObj{
	char* request;
	char* filename;
	char* cversion;
    char* version;
    // makes things faster
    bool check;
    bool validver;
    int length;
    char* body;
    // bytes read might be deprecated
    int bytes;
    // logging
    bool log;
    char* logname;
    // for logging 
    int temp;
}Command;

//constructor
Command* parserequest(char* orig, char* clean, Dispatch* dispatcher,int bytes);
//parsing
//void accumulator(ssize_t client_socket,Dispatch* dispatcher);
char* cleancommand(char command[]);
char* removesugar(char command[], const char substring[],char replace[]);
//order parsing
Command* parseheader(char* header, Dispatch* dispatcher,int bytes);
char* getplace(char* command,const int place);
int getlength(char* command);
char* getbody(char* command);
//misc
bool validatefile(char* file);
bool isnumeric(char* number);
int checkfile(char* file, int mode);
int accesschecker();
int filesize(char* file);
off_t offsetsize(char* file);
void calculate(const int tempfd, char* header, Dispatch* dispatcher, off_t size);
off_t transform(int fd, unsigned char* buffer,int read, off_t offset);
//execute
void execute(Command* order, ssize_t client_socket, Dispatch* dispatcher);
int exehead(Command* order);
int exeget(Command* order);
int exeput(Command* order);
// IO
void writelog(int mode, Command* order, Dispatch* dispatcher);
void codewriter(ssize_t client_socket, Command* order, int code, Dispatch* dispatcher);
void errorwriter(Command* order, ssize_t client_socket, Dispatch* dispatcher);
#endif
#ifndef parameters_H_
#define parameters_H_
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
// exists because single file was too cumbersome
// easier than passing a bunch of variables
typedef struct Params {
    int threads;
    bool log;
    char* logname;
    char* port;
    char* version;
}Parameters;
// constructor
Parameters newparams(int argc, char** argv,const char* version);
char* findport(int argc,char**argv);
bool checknumber(char* number);
bool isflag(char* argument);
#endif
#include "Parameters.h"
#include "HTTParse.h"
#include <stdio.h>

// constructor
Parameters newparams(int argc, char** argv,const char* version) {
    const bool debug = false;
    Parameters params = {
        .threads = 4,
        .log = false,
        .logname = '\0',
        .version = version,
        .port = findport(argc,argv),
    };
    int flag;
    while ( (flag = getopt(argc, argv,"N:l:") ) > -1) {
        switch (flag) {
            case 'N':
                if (debug == true) {
                    printf("N: ");
                    printf("%s\n",optarg);
                }
                if (checknumber(optarg) == true) {
                    params.threads = atoi(optarg);
                }
                break;
            case 'l':
                if (debug == true) {
                    printf("L: ");
                    printf("%s\n",optarg);
                }
                params.log = true;
                params.logname = optarg;
                break;
            case ':':
            case '?':
                printf("? found\n");
                printf("optarg: %s\n",optarg);
                break;
        }
    }
    if (debug == true) {
        printf("returned parameters:\n");
        printf("threads: %d\n",params.threads);
        if (params.log == true) {
            printf("log: true\n");
            printf("logname: %s\n",params.logname);
        } else {
            printf("log: false\n");
        }
        printf("port: %s\n",params.port);
        printf("version: %s\n",params.version);
    }
    return params;
}

char* findport(int argc,char**argv) {
    for (int x = 1; x < argc; x++) {
        if ( (isflag(argv[x-1]) == true) || (isflag(argv[x]) == true) ) {
            continue;
        } else {
            return argv[x];
        }
    }
    return '\0';
}

bool checknumber(char* number) {
    const bool debug = false;
    if (debug == true) {
        printf("Value: ");
    }
    if (strspn(number,"0123456789") == strlen(number)) {
        if (debug == true) {
            printf("Valid\n");
        }
        return true;
    } else {
        if (debug == true) {
            printf("Invalid\n");
        }
        return false;
    }
}

bool isflag(char* argument) {
    const bool debug = false;
    if (debug == true) {
        printf("flag: ");
    }
    if (strspn(argument,"-") > 0) {
        if (debug == true) {
            printf("true\n");
        }
        return true;
    } else {
        if (debug == true) {
            printf("false\n");
        }
        return false;
    }
}
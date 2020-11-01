#include "HTTParse.h"

typedef struct CommObj {
    
}CommObj;#include "HTTParse.h"

void accumulator(ssize_t client_socket,Dispatch dispatcher) {
    const debug = true;
    if (debug == true) {
        printf("Accumulating header...\n");
    }
    uint8_t buffer[BUFFER_SIZE + 1];
    buffer[BUFFER_SIZE + 1] = 0;
    int bytes = 1;
    int tbytes;
    // gets header and possibly a small portion of the body
    char* header = malloc(2);
    while (bytes > 0) {
		bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
		if (bytes < 0) {
            //print 500 error to client
            errorwriter(client_socket,dispatcher.version);
        } else {
            tbytes += bytes;
            realloc(header,strlen(header) + bytes + 1);
            memcpy(header + strlen(header), buffer, bytes + 1);
            if (strstr(buffer,"\r\n\r\n") != NULL) {
                //execute(parseheader(header,dispatcher.version,tbytes),client_socket,dispatcher.version);
                parseheader(header,dispatcher,tbytes);
                break;
            }
        }
	}
    free(header);
}

char* cleancommand(char command[]) {
    const bool debug = true;
    if (debug == true) {
        printf("cleaning header...\n");
    }
	// if header exists, it removes it and every possible \r\n
	if (strstr(command,"\r\n\r\n") != NULL) {
		return removesugar(command,"\r\n",' ');
	} else {
		// returns NULL for error as headers are required
		return NULL;
	}
}

char* removesugar(char command[], const char substring[],char replace[]) {
    int length = strlen(substring);
	char* pointer = command;
    if (length > 0) {
        while ( (pointer = strstr(pointer,substring)) != NULL ) {
			memset(pointer,replace,length);
			pointer += length;
		}
    }
	char* clean_command = command;
    return clean_command;
}

Command parseheader(char* header, Dispatch dispatcher,int bytes) {
    char line[strlen(header)+1];
    line[strlen(header)+1] = '\0';
    strcpy(line,header);
    char* cleansed = cleancommand(line);
    return parserequest(header,cleansed,dispatcher,bytes);
}

Command parserequest(char* orig, char* clean, Dispatch dispatcher,int bytes) {
    const bool debug = true;
    if (debug == true) {
        printf("Collecting metadata...\n");
    }
    Command order = {
        // server
        .log = dispatcher.log,
        .logname = dispatcher.logname,
        .cversion = dispatcher.version,
        // parsing
        .request = getplace(clean,1),
        .filename = getplace(clean,2),
        .version = getplace(clean,3),
        .length = getlength(clean),
        .body = getbody(orig),
        .bytes = bytes
    };
    if (debug == true) {
        printf("Returned Struct:\n");
        printf("log: %d\n",order.log);
        printf("logname: %s\n",order.logname);
        printf("current version: %s\n",order.cversion);
        printf("Request: %s\n",order.request);
        printf("File Name: %s\n",order.filename);
        printf("Version: %s\n",order.version);
        printf("Content-Length: %d\n",order.length);
    }
    return order;
}

// morphs 3 functions into 1
char* getplace(char* command,const int place) {
    int length = strlen(command);
	if (length <= 0) {
		return '\0';
	} else {
		// solves strcpy flaw of no terminator
		char line[length+1];
		strcpy(line,command);
        char* found;
        char* temp;
        switch(place) {
            case 1:
                found = strtok_r(line," ",temp);
                break;
            default:
                found = strtok_r(line," ");
                for (int x = 1; x < place; x++) {
                    found = strtok_r(NULL," ",temp);
                }
                break;
        }
        if (found != NULL) {
            if ( (place == 2) & (validatefile(found) == true) ) {
                return found;
            } else {
                return '\0';
            }
        } else {
            return '\0';
        }
	}
}

bool validatefile(char* file) {
    printf("validating file...\n");
    const char* valid = "-_0123456789AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz";
    if (strspn(file,valid) == strlen(file)) {
        printf("file valid...\n");
        return true;
    } else {
        printf("validity:\nexpected: %d\nfound: %d\n",strlen(file),strspn(file,valid));
        return false;
    }
}

bool isnumeric(char* number) {
    if (strspn(number,"0123456789") == strlen(number)) {
        return true;
    } else {
        return false;
    }
}

int getlength(char* command) {
	int length = strlen(command);
	char line[length+1];
	strcpy(line,command);
    line[length+1] = "\0";
	if (strstr(line,"Content-Length:") != NULL) {
		char* len = strtok(line," ");
		while ( strcmp(len,"Content-Length:") != 0 ) {
			len = strtok(NULL," ");
		}
		// goes to Content-Length
		len = strtok(NULL," ");
		// if valid, return
		if ( len != NULL ) {
			if (isnumeric(len) == true) {
				return atoi(len);
			} else {
				return -1;
			}
		} else {
			// 400 error needs content
			return -1;
		}
	} else {
		// 400 error needs content
		return -1;
	}
}

char* getbody(char* command) {
	// gets whole body, will read later
	int length = strlen(command);
	char line[length+1];
	strcpy(line,command);
	if (strstr(line,"\r\n\r\n") != NULL) {
		char* body = strtok(line,"\r\n\r\n");
		if ( (body != NULL) ) {
			char* body = strtok(NULL,"\r\n\r\n");
			if (body != NULL) {
				return body;
			} else {
				// no body
				return '\0';
			}
		} else {
			// 400
			return '\0';
		}
	} else {
		// 400 error
		return '\0';
	}
}

void errorwriter(ssize_t client_socket, const char* version) {
    printf("responding...\n");
    // severe errors
    char * buffer = malloc(BUFFER_SIZE);
    int bytes;
    switch (errno) {
        case 13:
            printf("403\n");
            bytes = sprintf(buffer,"403 Forbidden %s\r\n\r\n",version);
            send(client_socket,buffer,bytes,0);
            break;
        case 21:
            printf("403\n");
            bytes = sprintf(buffer,"403 Forbidden %s\r\n\r\n",version);
            send(client_socket,buffer,bytes,0);
            break;
        default:
        // 500
            printf("500\n");
            bytes = sprintf(buffer,"500 Internal Server Error %s\r\n\r\n",version);
            send(client_socket,buffer,bytes,0);
            break;
    }
}
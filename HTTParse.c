#include "HTTParse.h"
#include <unistd.h>
// protects offset
static pthread_mutex_t loglock =  PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

void accumulator(ssize_t client_socket,Dispatch* dispatcher) {
    uint8_t* buffer = malloc(BUFFER_SIZE);
    ssize_t bytes = recv(client_socket,buffer,BUFFER_SIZE,0);
    if (strstr(buffer,"\r\n\r\n") != NULL) {
        execute(parseheader(buffer,dispatcher,bytes),client_socket,dispatcher);
    } else {
        // return 400;
    }
}

char* cleancommand(char command[]) {
	if (strstr(command,"\r\n\r\n") != NULL) {
		return removesugar(command,"\r\n",' ');
	} else {
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

Command* parseheader(char* header, Dispatch* dispatcher,int bytes) {
    char line[strlen(header)+1];
    line[strlen(header)+1] = '\0';
    strcpy(line,header);
    char* cleansed = cleancommand(line);
    return parserequest(header,cleansed,dispatcher,bytes);
}

Command* parserequest(char* orig, char* clean, Dispatch* dispatcher,int bytes) {
    Command* order = malloc(sizeof(Command));
    order->log = dispatcher->log;
    order->logname = dispatcher->logname;
    order->cversion = dispatcher->version;
    order->request = getplace(clean,1);
    order->filename = getplace(clean,2);
    order->version = getplace(clean,3);
    order->length = getlength(clean);
    order->body = getbody(orig);
    order->bytes = 0;
    order->check = false;
    if (strcmp(order->version,dispatcher->version) == 0) {
        order->validver = true;
    } else {
        order->validver = false;
    }
    if (strcmp(order->filename,"healthcheck") == 0) {
        order->check = true;
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
        char* temp = &line;
        switch(place) {
            case 1:
                found = strtok_r(line," ",&temp);
                break;
            default:
                found = strtok_r(line," ",&temp);
                for (int x = 1; x < place; x++) {
                    found = strtok_r(NULL," ",&temp);
                }
                break;
        }
        if ((found != NULL) & (strlen(found) <= 28)) {
            if (place != 2) {
                char* copy = malloc(28*sizeof(char));
                strcpy(copy,found);
                return copy;
            } else {
                memmove(found, found+1, strlen(found));
                if (validatefile(found) == true) {
                    char* copy = malloc(strlen(found));
                    strcpy(copy,found);
                    return copy;
                } else {
                    return '\0';
                }
            }
        } else {
            return '\0';
        }  
        return found;
	}
}

bool validatefile(char* file) {
    const char* valid = "-_0123456789AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz";
    if (strspn(file,valid) == strlen(file)) {
        return true;
    } else {
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
    char* temp;
	if (strstr(line,"Content-Length:") != NULL) {
		char* len = strtok_r(line," ",&temp);
		while ( strcmp(len,"Content-Length:") != 0 ) {
			len = strtok_r(NULL," ",&temp);
		}
		// goes to Content-Length
		len = strtok_r(NULL," ",&temp);
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
    char* temp;
	strcpy(line,command);
	if (strstr(line,"\r\n\r\n") != NULL) {
		char* body = strtok_r(line,"\r\n\r\n",&temp);
		if ( (body != NULL) ) {
			char* body = strtok_r(NULL,"\r\n\r\n",&temp);
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


// execution

void execute(Command* order, ssize_t client_socket, Dispatch* dispatcher) {
	printf("GOT TO EXECUTE\n");
    int code = 0;
    // gets the response code
    if (strcmp(order->request,"HEAD") == 0) {
        if (order->check == false) {
            code = exehead(order);
        }
    } else if (strcmp(order->request,"GET") == 0) {
        code = exeget(order);
    } else if (strcmp(order->request,"PUT") == 0) {
        if (order->check == false) {
            code = exeput(order);
        }
    } else {
        code = 400;
    }
    // 0 for serious errors, everything else is code
    if (code > 0) {
        codewriter(client_socket, order, code, dispatcher);
    } else {
        errorwriter(order,client_socket, dispatcher);
    }
}

int exehead(Command* order) {
    if (order->check == 0) {
        if (order->validver == true) {
            if ( strlen(order->filename) > 0 ) {
                if ( validatefile(order->filename) == true ) {
                    return checkfile(order->filename,R_OK);
                } else {
                    return 400;
                }
            } else {
                return 400;
            }
        } else {
            return 400;
        }
    } else {
        return -1;
    }
}

int exeget(Command* order) {
    if (order->check != true) {
        if (order->validver == true) {
            if ( strlen(order->filename) > 0 ) {
                if (validatefile(order->filename) == true) {
                    return checkfile(order->filename,R_OK);
                } else {
                    return 400;
                }
            } else {
                return 400;
            }
        } else {
            return 400;
        }
    } else {
        if (order->log == true) {
            return 200;
        } else {
            return 404;
        }
    }
}

int exeput(Command* order) {
    if (order->validver == true) {
            if ( strlen(order->filename) > 0 ) {
                if (validatefile(order->filename) == true) {
                    int result = checkfile(order->filename,F_OK);
                    if ( result == 404 ) {
                        return 201;
                    } else {
                        return result;
                    }
                } else {
                    return 400;
                }
            } else {
                // not valid file
                return 400;
            }
        } else {
            // wrong version
            return 400;
        }
    }

int filesize(char* file) {
    struct stat temp;
    stat(file,&temp);
    return temp.st_size;
}

int checkfile(char* file, int mode) {
    struct stat temp;
    stat(file,&temp);
    // checks existence
    if (stat(file,&temp) == 0) {
        // check permission
        int result = access(file,mode);
        if (result == 0) {
            return 200;
        } else {
            return accesschecker();
        }
        return accesschecker();
    } else {
        return 404;
    }
}

int accesschecker() {
    switch(errno) {
        case EROFS:
            // write permission denied
            return 403;
            break;
        case EACCES:
            // forbidden path
            return 403;
            break;
        default:
            // other error, likely server
            return 0;
            break;
    }
}

void sendfile(Command* order, ssize_t client_socket,Dispatch* dispatcher) {
    int fd = open(order->filename,O_RDONLY,0644);
    uint8_t buffer[16384];
    ssize_t bytes = 1;
    ssize_t result = 1;
    ssize_t tbytes = 0;
    while ((bytes > 0) && (result > 0)) {
        bytes = read(fd,buffer,16384);
        result = send(client_socket,buffer,bytes,0);
        if (result < 0) {
            printf("errno: %d\n",errno);
        }
        tbytes += result;
    }
    close(fd);
    close(client_socket);
}

void createfile(Command* order, size_t client_socket) {
        int fd = open(order->filename,O_CREAT | O_RDWR | O_TRUNC,0644);
        uint8_t* buffer = malloc(BUFFER_SIZE);
        ssize_t bytes = 1;
        ssize_t result = 1;
		int total;
        while ((order->bytes < order->length) && bytes > 0 && result > 0) {
            bytes = recv(client_socket, buffer,BUFFER_SIZE,0);
            result = write(fd,buffer,bytes);
            order->bytes += result;
			total += bytes;
        }
		printf("total written: %d\n",total);
        close(fd);
}

// gets offset to write and updates offset
off_t getoffset(const int tempfd, char* header, off_t size, Dispatch* dispatcher) {
    pthread_mutex_lock(&loglock);
	off_t offset = *dispatcher->offset;
    calculate(tempfd,header,dispatcher,size);
	pthread_mutex_unlock(&loglock);
    return offset;
}

// helper
void calculate(const int tempfd, char* header, Dispatch* dispatcher, off_t size) {
    //char* footer = "========\n";
    switch(tempfd) {
        // error cases or HEAD
        case -1:
            *(dispatcher->offset) += strlen(header)+1;
            *(dispatcher->offset) += 10;
            break;
        default:
            *(dispatcher->offset) += strlen(header)+1;
            *(dispatcher->offset) += size;
            *(dispatcher->offset) += 10;
            break;
    }
}

void writelog(int mode, Command* order, Dispatch* dispatcher) {
    uint8_t * buffer = malloc(BUFFER_SIZE);
    int bytes;
    switch(mode) {
        // for HEAD
		off_t offset;
        case 200:
            bytes = sprintf(buffer,"HEAD /%s length %d\n========\n", order->filename, order->length);
			offset = getoffset(-1,buffer,dispatcher,0);
            pwrite(dispatcher->fd,buffer,bytes,offset);
            break;
        case 201:
            logfile(order,dispatcher);
            break;
        case 400:
            bytes = sprintf(buffer,"FAIL: %s /%s HTTP/1.1 --- response 400\n========\n", order->request, order->filename);
			offset = getoffset(-1,buffer,dispatcher,0);
            pwrite(dispatcher->fd,buffer,bytes,offset);
            break;
        case 403:
            bytes = sprintf(buffer,"FAIL: %s /%s HTTP/1.1 --- response 403\n========\n", order->request, order->filename);
            offset = getoffset(-1,buffer,dispatcher,0);
			pwrite(dispatcher->fd,buffer,bytes,offset);
            break;
        case 404:
            bytes = sprintf(buffer,"FAIL: %s /%s HTTP/1.1 --- response 404\n========\n", order->request, order->filename);
            offset = getoffset(-1,buffer,dispatcher,0);
			pwrite(dispatcher->fd,buffer,bytes,offset);
            break;
        case 500:
            bytes = sprintf(buffer,"FAIL: %s /%s HTTP/1.1 --- response 500\n========\n", order->request, order->filename);
            offset = getoffset(-1,buffer,dispatcher,0);
			pwrite(dispatcher->fd,buffer,bytes,offset);
            break;
        default:
            logfile(order,dispatcher);
            break;
    }
}

void logfile(Command* order, Dispatch* dispatcher) {
    unsigned char* buffer = malloc(6000);
    unsigned char* header = malloc(69);
    int fd = open(order->filename,O_RDONLY,0777);
    snprintf(header,69,"%s /%s length %d\n", order->request, order->filename,filesize(order->filename));
    int head = strlen(header);
	off_t file = filesize(order->filename);
	off_t size = (floor(file/20)*69);
    if (file%20 != 0) {
        size += ((file%20)*3)+9;
    }
	off_t offset = getoffset(1,header,dispatcher,size);
    int bytes = pwrite(dispatcher->fd,header,head,offset);
	offset += bytes+1;
    while (bytes > 0) {
        bytes = read(fd,buffer,6000);
		printf("Error: in function \"%s\" line number %d\n", __func__, __LINE__);
        offset = transform(dispatcher->fd,buffer,bytes,offset);
    }
    free(buffer);
	free(header);
    close(fd);
}

// used to transform file names 
off_t transform(int fd, unsigned char* buffer,int read,off_t offset) {
    int parsed = 0;
    int x = 0;
    int tbytes;
    while (parsed < read) {
        unsigned char* character = malloc(4);
        unsigned char* temp = malloc(80);
        for (x = 0; (x < 20) & (parsed < read); x++) {
            snprintf(character,4," %02x",buffer[parsed]);
            strcat(temp,character);
            parsed += 1;
        }
        unsigned char* converted = malloc(89);
        snprintf(converted,89,"%08d%s",tbytes,temp);
        tbytes += x;
        int bytes = pwrite(fd,converted,strlen(converted),offset);
		offset += bytes + 1;
        bytes = pwrite(fd,"\n",1,offset);
		offset += bytes + 1;
        free(character);
        free(temp);
        free(converted);
    }
    return offset;
}

void codewriter(ssize_t client_socket, Command* order, int code, Dispatch* dispatcher) {
    // response codes
    uint8_t * buffer = malloc(BUFFER_SIZE);
    int bytes;
    off_t offset;
    switch (code) {
        case 200:
            printf("200\n");
            if (strcmp(order->request,"HEAD") == 0 ) {
                bytes = sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n",filesize(order->filename));
                send(client_socket,buffer,bytes,0);
                if (dispatcher->log == true) {
                    writelog(200,order,dispatcher);
                }
            } else if (strcmp(order->request,"GET") == 0 ) {
                if (order->check == true) {
                    uint8_t* temp = malloc(BUFFER_SIZE);
                    sprintf(temp,"%d\n%d", *dispatcher->fails, (*dispatcher->entries)-1);
                    sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n%s",strlen(temp),temp);
                    send(client_socket,buffer,strlen(buffer),0);
                    if (dispatcher->log == true) {
                        writelog(-1,order,dispatcher);
                    }
                    free(temp);
                } else {
                    int size = filesize(order->filename);
                    bytes = sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n",size);
                    send(client_socket,buffer,bytes,0);
                    sendfile(order,client_socket,dispatcher);
                    if (dispatcher->log == true && (strcmp(order->filename, dispatcher->logname)) != 0 ) {
                        writelog(-1,order,dispatcher);
                    }
                }
            } else if ((strcmp(order->request,"PUT")) == 0) {
                createfile(order,client_socket);
                bytes = sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n");
                send(client_socket,buffer,bytes,0);
                close(client_socket);
                if (dispatcher->log == true) {
                    writelog(-1,order,dispatcher);
                }
            }
            break;
        case 201:
            printf("201\n");
            bytes = sprintf(buffer,"HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n");
            createfile(order,client_socket);
            send(client_socket,buffer,bytes,0);
            close(client_socket);
            if (dispatcher->log == true) {
                writelog(201,order,dispatcher);
            }
            break;
        case 400:
            printf("400\n");
            *(dispatcher->fails) += 1;
            bytes = sprintf(buffer,"HTTP/1.1 400 Bad Request\r\n\r\n");
            send(client_socket,buffer,bytes,0);
            if (dispatcher->log == true) {
                writelog(400,order,dispatcher);
            }
            break;
        case 404:
            printf("404\n");
            *(dispatcher->fails) += 1;
            bytes = sprintf(buffer,"HTTP/1.1 404 File Not Found\r\n\r\n");
            send(client_socket,buffer,bytes,0);
            if (dispatcher->log == true) {
                writelog(404,order,dispatcher);
            }
            break;
        default:
            printf("500\n");
            *(dispatcher->fails)+= 1;
            bytes = sprintf(buffer,"HTTP/1.1 500 Internal Server Error\r\n\r\n",dispatcher->version);
            send(client_socket,buffer,bytes,0);
            if (dispatcher->log == true) {
                writelog(500,order,dispatcher);
            }
            break;
    }
    free(buffer);
}

void errorwriter(Command* order, ssize_t client_socket, Dispatch* dispatcher) {
    uint8_t * buffer = malloc(BUFFER_SIZE);
    int bytes;
    off_t offset = 0;
    switch (errno) {
        case 13:
            *(dispatcher->fails)+= 1;
            bytes = sprintf(buffer,"HTTP/1.1 403 Forbidden\r\n\r\n");
            send(client_socket,buffer,bytes,0);
            if (dispatcher->log == true) {
                writelog(403,order,dispatcher);
            }
            break;
        case 21:
            *(dispatcher->fails)+= 1;
            bytes = sprintf(buffer,"HTTP/1.1 403 Forbidden\r\n\r\n");
            send(client_socket,buffer,bytes,0);
            if (dispatcher->log == true) {
                writelog(403,order,dispatcher);
            }
            break;
        default:
            *(dispatcher->fails)+= 1;
            // now 403 for HEAD and PUT healthchecks
            sprintf(buffer,"HTTP/1.1 403 Forbidden\r\n\r\n");
            send(client_socket,buffer,strlen(buffer),0);
            if (dispatcher->log == true) {
                writelog(403,order,dispatcher);
            }
            break;
    }
    close(client_socket);
}
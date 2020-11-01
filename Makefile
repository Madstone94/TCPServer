#------------------------------------------------------------------------------
# Makefile for CSE 130 Programming Assignment 2
#------------------------------------------------------------------------------

httpserver : httpserver.o HTTParse.o Parameters.o HTTPThreader.o
	gcc -o httpserver httpserver.o HTTParse.o Parameters.o HTTPThreader.o -g -lpthread -D_GNU_SOURCE

httpserver.o : httpserver.c
	gcc -c -std=c99 -Wall httpserver.c -g -lpthread -D_GNU_SOURCE
	
HTTPThreader.o : HTTPThreader.c HTTPThreader.h
	gcc -c -std=c99 -Wall HTTPThreader.c -g -lpthread -D_GNU_SOURCE

Parameters.o : Parameters.c Parameters.h
	gcc -c -std=c99 -Wall Parameters.c -lpthread -D_GNU_SOURCE

HTTParse.o : HTTParse.c HTTParse.h
	gcc -c -std=c99 -Wall HTTParse.c -lpthread -D_GNU_SOURCE

clean :
	rm -f httpserver httpserver.o  HTTParse.o Parameters.o HTTPThreader.o

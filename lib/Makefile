
include ../Makefile.inc

all: picohttpparser.o

picohttpparser.o: picohttpparser/picohttpparser.h
	$(ECHO) Entering into picohttpparser
	cd picohttpparser && $(CC) $(CFLAGS) -v -c -o picohttpparser.o picohttpparser.c


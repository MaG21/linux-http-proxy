#
#lhps stand for Linux Http Proxy Server.
CC=gcc
CFLAGS=-ggdb

all: linux-http-proxy

linux-http-proxy: http.o net.o utils.o main.o
	$(CC) $(CFLAGS) -o lhps http.o net.o utils.o main.o
http:
	$(CC) $(CFLAGS) -c -o http.o http.c
net:
	$(CC) $(CFLAGS) -c -o net.o net.c
utils:
	$(CC) $(CFLAGS) -c -o utils.o utils.c
main:
	$(CC) $(CFLAGS) -c -o main.o main.c

clean:
	rm -f *.o proxy-server


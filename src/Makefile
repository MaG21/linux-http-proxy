include ../Makefile.inc

OBJ     = http.o net.o utils.o main.o
INCLUDE = -I../include

all: $(OBJ)

http.o: ../include/http.h ../include/net.h ../include/utils.h
	$(CC) $(CFLAGS) $(INCLUDE) -c -o http.o http.c

net.o: ../include/net.h
	$(CC) $(CFLAGS) $(INCLUDE) -c -o net.o net.c

utils.o: ../include/utils.h
	$(CC) $(CFLAGS) $(INCLUDE) -c -o utils.o utils.c

main.o: http.o net.o utils.o
	$(CC) $(CFLAGS) $(INCLUDE) -c -o main.o main.c

clean:
	$(RM) -f *.o


include ../Makefile.inc

OBJ = http.o net.o utils.o main.o

all: $(OBJ)

http.o: ../include/http.h ../include/net.h ../include/utils.h
	$(CC) $(CFLAGS) -c -o http.o http.c

net.o: ../include/net.h
	$(CC) $(CFLAGS) -c -o net.o net.c

utils.o: ../include/utils.h
	$(CC) $(CFLAGS) -c -o utils.o utils.c

main.o: http.o net.o utils.o main.o
	$(CC) $(CFLAGS) -c -o main.o main.c

clean:
	$(RM) -f *.o


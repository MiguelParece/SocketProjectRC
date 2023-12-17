# Makefile for AS.c

CC = gcc
CFLAGS = -pthread

all: USER AS
#reset the server and cleans the executables
reset: 
	cd ASDIR && $(MAKE) reset
	$(MAKE) clean
AS: 
	cd ASDIR && $(MAKE)


USER: user.c
	$(CC) $(CFLAGS) -o user user.c

clean:
	rm -f user
	cd ASDIR && $(MAKE) clean
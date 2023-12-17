# Makefile for AS.c

CC = gcc
CFLAGS = -pthread

all: USER AS

AS: 
	cd ASDIR && $(MAKE)


USER: user.c
	$(CC) $(CFLAGS) -o user user.c

clean:
	rm -f user
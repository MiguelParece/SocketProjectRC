# Makefile for AS.c

CC = gcc
CFLAGS = -pthread

all: AS

AS: AS.c
	$(CC) $(CFLAGS) -o AS AS.c

clean:
	rm -f AS
reset:
	rm -f -r AUCTIONS/*
	rm -f -r USERS/*
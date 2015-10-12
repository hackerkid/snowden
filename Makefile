CC=g++
CFLAGS= -g -Wall -Werror -pthread
LDFLAGS= -pthread
LIBS ?= -pthread

all: proxy

proxy: snowden.cpp
	$(CC) $(CFLAGS) -o proxy_parse.o -c proxy_parse.c
	$(CC) $(CFLAGS) -o proxy.o -c snowden.cpp
	$(CC) $(CFLAGS) -o proxy proxy_parse.o proxy.o

clean:
	rm -f proxy *.o

tar:
	tar -cvzf ass1.tgz snowden.cpp README Makefile proxy_parse.c proxy_parse.h

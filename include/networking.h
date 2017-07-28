#pragma once

#include <ev.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include "socks.h"
//-------------------------------------------------------------------

enum {
	SMALL_BUF = 2048,
	LARGE_BUF = 8192
};

typedef struct server {
	ev_io io;
	int sock;
} server;

//-------------------------------------------------------------------

int setnonblock(int d);

void print_addr(FILE *f, int af, const void *addr);

int blocking_recv(int sock, void *buf, size_t sz, int flags);

int send_data(int from, int to);

int nonblock_recv(int sock, void *buf, size_t sz, int fl);

//-------------------------------------------------------------------

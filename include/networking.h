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

//-------------------------------------------------------------------

struct context {
	ev_io io;
	int sock;
	struct sockaddr_storage addr;
	struct session *session;
};
typedef struct context server;
typedef struct context client;
typedef struct context host;

typedef struct session {
	struct context client;
	struct context host;
	struct socks_request req;
	void *data;
} session;

session * session_new();

void delete_session(session *);

//-------------------------------------------------------------------

int setnonblock(int d);

void print_addr(FILE *f, int af, const void *addr);

int blocking_recv(int sock, void *buf, size_t sz, int flags);

int send_data(int from, int to);

//-------------------------------------------------------------------

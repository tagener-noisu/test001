#pragma once

#include <ev.h>
#include <sys/socket.h>
#include <arpa/inet.h>
//-------------------------------------------------------------------

struct context {
	ev_io io;
	int sock;
	struct sockaddr_in addr;
};
typedef struct context server;
typedef struct context client;

void server_init(server* s, int sock, struct sockaddr_in addr);

client* client_new();

//-------------------------------------------------------------------

int setnonblock(int d);

void
set_sockaddr(struct sockaddr_in* addr, int fam, int s_addr, uint16_t port);

//-------------------------------------------------------------------

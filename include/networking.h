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

enum socks_stat {
	GRANTED = 0x5A,
	REJECTED,
	OTHER1,
	OTHER2
};

struct socks_reply {
	uint8_t ver;
	uint8_t stat;
	uint8_t pad[6];
};

struct socks_reply new_socks_reply(enum socks_stat stat);

//-------------------------------------------------------------------

int setnonblock(int d);

void
set_sockaddr(struct sockaddr_in* addr, int fam, int s_addr, uint16_t port);

//-------------------------------------------------------------------

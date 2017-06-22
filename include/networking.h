#pragma once

#include <ev.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
//-------------------------------------------------------------------

enum {
	SMALL_BUF = 2048,
	LARGE_BUF = 4096
};

//-------------------------------------------------------------------

struct context {
	ev_io io;
	int sock;
};
typedef struct context server;
typedef struct context client;

client* client_new();

//-------------------------------------------------------------------

enum socks_stat {
	GRANTED = 0x5A,
	REJECTED = 0x5B,
	OTHER1 = 0x5C,
	OTHER2 = 0x5D
};

enum socks_comm {
	STREAM_CON = 0x1,
	PORT_BIND = 0x2,
};

struct socks_reply {
	uint8_t ver;
	uint8_t stat;
	uint16_t port;
	uint32_t ipv4;
};

struct socks_request {
	uint8_t ver;
	uint8_t comm;
	uint16_t port;
	uint32_t ipv4;
};

struct socks_reply
socks_reply_new(enum socks_stat stat, uint32_t ipv4, uint16_t port);

//-------------------------------------------------------------------

int setnonblock(int d);

void print_addr(FILE *f, int af, const void *addr);

int blocking_recv(int sock, void *buf, size_t sz, int flags);

//-------------------------------------------------------------------

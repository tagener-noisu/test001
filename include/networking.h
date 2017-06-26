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

struct context {
	ev_io io;
	int sock;
	struct sockaddr_storage addr;
};
typedef struct context server;
typedef struct context client;

typedef struct {
	struct context client;
	struct context host;
	struct socks_request req;
	void *data;
} session;

session * session_new();

//-------------------------------------------------------------------

int setnonblock(int d);

void print_addr(FILE *f, int af, const void *addr);

int blocking_recv(int sock, void *buf, size_t sz, int flags);

//-------------------------------------------------------------------

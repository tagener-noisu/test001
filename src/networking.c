#include <fcntl.h>
#include <stdlib.h>
#include "networking.h"
#include "callbacks.h"
//-------------------------------------------------------------------

int setnonblock(int d) {
	int flags = fcntl(d, F_GETFL);
	if (flags == -1)
		return flags;
	flags |= O_NONBLOCK;

	return fcntl(d, F_SETFD, flags);
}

void
set_sockaddr(struct sockaddr_in* addr, int fam, int s_addr, uint16_t port) {
	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = fam;
	addr->sin_addr.s_addr = s_addr;
	addr->sin_port = htons(port);
}

//-------------------------------------------------------------------

struct socks_reply
new_socks_reply(enum socks_stat stat, uint32_t ipv4, uint16_t port) {
	struct socks_reply r;
	r.ver = 0;
	r.stat = stat;
	r.ipv4 = ipv4;
	r.port = port;
	return r;
}

//-------------------------------------------------------------------

void server_init(server* s, int sock, struct sockaddr_in addr) {
	s->sock = sock;
	s->addr = addr;
}

client* client_new(void (*callback)(EV_P_ ev_io *, int)) {
	client* cli = (client*) malloc(sizeof(client));
	memset(cli, 0, sizeof(client));
	ev_io_init(&cli->io, callback, cli->sock, EV_WRITE);
	return cli;
}

//-------------------------------------------------------------------

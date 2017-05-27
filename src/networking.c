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

void server_init(server* s, int sock, struct sockaddr_in addr) {
	s->sock = sock;
	s->addr = addr;
}

client* client_new() {
	client* cli = (client*) malloc(sizeof(client));
	memset(cli, 0, sizeof(client));
	ev_io_init(&cli->io, send_cb, cli->sock, EV_WRITE);
	return cli;
}

//-------------------------------------------------------------------

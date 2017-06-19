#include <fcntl.h>
#include <stdlib.h>
#include "networking.h"
//-------------------------------------------------------------------

int setnonblock(int d) {
	int flags = fcntl(d, F_GETFL);
	if (flags == -1)
		return flags;
	flags |= O_NONBLOCK;

	return fcntl(d, F_SETFL, flags);
}

void print_addr(FILE *f, int af, const void *addr) {
	char buf[INET6_ADDRSTRLEN];
	const char *ip = NULL;
	uint16_t port = 0;

	if (af == AF_INET) {
		struct sockaddr_in *addr_in = (struct sockaddr_in *) addr;
		ip = inet_ntop(af, &addr_in->sin_addr, buf, sizeof(buf));
		port = ntohs(((struct sockaddr_in *)addr)->sin_port);
	}
	else if (af == AF_INET6) {
		struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *) addr;
		ip = inet_ntop(af, &addr_in6->sin6_addr, buf, sizeof(buf));
		port = ntohs(((struct sockaddr_in6 *)addr)->sin6_port);
	}

	if (ip != NULL)
		fprintf(f, "%s:%u", ip, port);
}

//-------------------------------------------------------------------

struct socks_reply
socks_reply_new(enum socks_stat stat, uint32_t ipv4, uint16_t port) {
	struct socks_reply r;
	r.ver = 0;
	r.stat = stat;
	r.ipv4 = ipv4;
	r.port = port;
	return r;
}

//-------------------------------------------------------------------

client* client_new(void (*callback)(EV_P_ ev_io *, int)) {
	client* cli = (client*) malloc(sizeof(client));
	memset(cli, 0, sizeof(client));
	ev_io_init(&cli->io, callback, cli->sock, EV_WRITE);
	return cli;
}

//-------------------------------------------------------------------

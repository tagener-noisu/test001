#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
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

int blocking_recv(int sock, void *buf, size_t sz, int flags) {
	int len;
	do {
		len = recv(sock, buf, sz, flags);
	} while (len == -1 && errno == EAGAIN);

	return len;
}

int send_data(int from, int to) {
	char buf[LARGE_BUF];
	int recvd = recv(from, buf, sizeof(buf), MSG_PEEK);

	if (recvd > 0) {
		int sent = send(to, buf, recvd, MSG_NOSIGNAL);
		if (sent != -1)
			recv(from, buf, sent, MSG_WAITALL);
		else
			return sent;
	}

	return recvd;
}

int nonblock_recv(int sock, void *buf, size_t sz, int fl) {
	int stat;
	stat = recv(sock, buf, sz, MSG_PEEK);
	if (stat < sz) {
		if (stat == 0)
			errno = ENODATA;
		if (stat > 0)
			errno = EAGAIN;

		return -1;
	}
	stat = recv(sock, buf, sz, 0);
	return 0;
}

//-------------------------------------------------------------------

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "callbacks.h"
#include "networking.h"
//-------------------------------------------------------------------
enum {
	SMALL_BUF = 2048
};

int blocking_recv(int sock, void *buf, size_t sz, int flags) {
	int len;
	do {
		len = recv(sock, buf, sz, flags);
	} while (len == -1 && errno == EAGAIN);

	return len;
}

void socks_request_cb(struct ev_loop *loop, ev_io *w, int revents) {
	int stat;
	struct socks_request r;
	client* cli = (client*) w;

	stat = recv(cli->sock, &r, sizeof(r), 0);

	if (stat == -1 && errno == EAGAIN)
		return;

	shutdown(cli->sock, SHUT_RD);
	if (stat != -1 && r.ver == 4) {
		struct in_addr ip;
		struct socks_reply repl;
		char buf[SMALL_BUF];

		stat = blocking_recv(cli->sock, buf, sizeof(buf), MSG_WAITALL);
		if (stat != -1) {
			buf[stat - 1] = '\0';

			ip.s_addr = r.ipv4;
			fprintf(stderr, "REQUEST(%x), host: %s:%u, from: %s\n",
				r.comm,
				inet_ntoa(ip),
				ntohs(r.port),
				buf
			);

			repl = socks_reply_new(REJECTED, r.ipv4, r.port);
			stat = send(cli->sock, &repl, sizeof(repl), 0);
		}
	}

	if (stat == -1)
		fprintf(stderr,
			"Error: socks_request_cb(), %s\n",
			strerror(errno));

	fprintf(stderr, "Connection closed.\n");
	close(cli->sock);
	ev_io_stop(loop, w);
	free(cli);
}

void accept_cb (struct ev_loop *loop, ev_io *w, int revents) {
	int sock;
	struct sockaddr_in addr;
	server* serv = (server*) w;
	int addrsz = sizeof(struct sockaddr_in);

	sock = accept(
		serv->sock,
		(struct sockaddr*)&addr,
		(socklen_t*)&addrsz);

	if (sock != -1) {
		setnonblock(sock);
		client* cli = client_new(socks_request_cb);
		cli->sock = sock;

		ev_io_start(loop, &cli->io);

		fprintf(stderr,
			"Connection from: %s:%u\n",
			inet_ntoa(addr.sin_addr),
			ntohs(addr.sin_port));
		return;
	}

	fprintf(stderr, "Error in accept(); %s\n", strerror(errno));
	ev_io_stop(loop, w);
	ev_break(loop, EVBREAK_ALL);
	close(sock);
}

void sigint_cb(struct ev_loop *loop, ev_signal *w, int revents) {
	ev_break(loop, EVBREAK_ALL);
	fprintf(stderr, "Interrupt\n");
}

//-------------------------------------------------------------------

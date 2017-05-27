#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "callbacks.h"
#include "networking.h"
//-------------------------------------------------------------------

void send_cb (struct ev_loop *loop, ev_io *w, int revents) {
	int stat;
	static char* http_ok = "HTTP/1.1 200 OK\r\n\r\n";
	client* cli = (client*) w;
	stat = send(cli->sock, http_ok, strlen(http_ok), 0);
	if (stat == -1)
		fprintf(stderr, "Error: send(), %s\n", strerror(errno));

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
		client* cli = client_new();
		cli->sock = sock;
		cli->addr = addr;

		ev_io_start(loop, &cli->io);

		fprintf(stderr,
			"Connection from: %s:%u\n",
			inet_ntoa(cli->addr.sin_addr),
			ntohs(cli->addr.sin_port));
		return;
	}

	fprintf(stderr, "Error in accept(); %s\n", strerror(errno));
	ev_io_stop(loop, w);
	ev_break(loop, EVBREAK_ALL);
	close(sock);
}

//-------------------------------------------------------------------

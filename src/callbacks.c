#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "callbacks.h"
#include "networking.h"
//-------------------------------------------------------------------

void client_cb (struct ev_loop *loop, ev_io *w, int revents) {
	int stat;
	client* cli = (client*) w;
	struct socks_request r;
	char buf[2048];

	stat = recv(cli->sock, &r, sizeof(r), 0);
	if (stat != -1 && r.ver == 4) {
		struct in_addr ip;
		struct socks_reply repl;

		stat = recv(cli->sock, buf, 2048, 0);
		buf[2047] = 0;

		ip.s_addr = r.ipv4;
		fprintf(stderr, "REQUEST(%x), host: %s:%u, id: %s\n",
			r.comm,
			inet_ntoa(ip),
			ntohs(r.port),
			buf
		);

		repl = new_socks_reply(GRANTED, r.ipv4, r.port);
		stat = send(cli->sock, &repl, sizeof(repl), 0);
	}

	if (stat == -1)
		fprintf(stderr, "Error: client_cb(), %s\n", strerror(errno));

	shutdown(cli->sock, SHUT_RDWR);
	stat = recv(cli->sock, buf, 2048, 0);
	if (stat == -1)
		fprintf(stderr,
			"Error: recv() waiting FIN, %s", strerror(errno));
	else if (stat > 0)
		fprintf(stderr,
			"Error: recv() waiting FIN, trailing data.");

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
		client* cli = client_new(client_cb);
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

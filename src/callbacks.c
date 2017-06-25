#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include "error.h"
#include "callbacks.h"
#include "networking.h"
//-------------------------------------------------------------------

void connect_to_host(session *s) {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	s->host.sock = sock;

	if (sock != -1) {
		int stat;
		struct sockaddr_in * addr =
			(struct sockaddr_in *) &s->host.addr;

		struct socks_reply repl = socks_reply_new(
			REJECTED,
			addr->sin_addr.s_addr,
			addr->sin_port);

		//setnonblock(sock);
		stat = connect(
			sock,
			(struct sockaddr *) addr,
			sizeof(struct sockaddr_in));

		if (stat == 0)
			repl.stat = GRANTED;
		else if (stat == -1)
			error();

		stat = send(s->client.sock, &repl, sizeof(repl), 0);
	}

	close(s->client.sock);
	close(sock);
	free(s);
}

void socks_request_cb(struct ev_loop *loop, ev_io *w, int revents) {
	int stat;
	struct socks_request r;
	session *s = (session *) w;
	int sock = s->client.sock;

	stat = recv(sock, &r, sizeof(r), 0);
	if (stat == -1 && errno == EAGAIN)
		return;

	if (stat != -1 && r.ver == 4) {
		char userid[SMALL_BUF];

		stat = blocking_recv(sock, userid, sizeof(userid), MSG_WAITALL);
		if (stat != -1) {
			struct sockaddr_in *host =
				(struct sockaddr_in *) &s->host.addr;

			userid[stat - 1] = '\0';

			memset(host, 0, sizeof(struct sockaddr_in));
			host->sin_family = AF_INET;
			host->sin_addr.s_addr = r.ipv4;
			host->sin_port = r.port;

			fprintf(stderr, "SOCKS REQUEST(%x), host: ", r.comm);
			print_addr(stderr, AF_INET, host);
			fprintf(stderr, ", from: %s\n", userid);

			ev_io_stop(loop, &s->client.io);
			connect_to_host(s);
			ev_break(loop, EVBREAK_ALL);
			return;
		}
	}

	if (stat == -1)
		fprintf(stderr,
			"Error: socks_request_cb(), %s\n",
			strerror(errno));

	fprintf(stderr, "Connection closed.\n");
	close(sock);
	ev_io_stop(loop, w);
	free(s);
}

void accept_cb (struct ev_loop *loop, ev_io *w, int revents) {
	session *s = session_new();
	client *client = &s->client;
	int addrsz = sizeof(client->addr);
	server* serv = (server*) w;

	client->sock = accept(
		serv->sock,
		(struct sockaddr*) &client->addr,
		(socklen_t*)&addrsz);

	if (client->sock != -1) {
		setnonblock(client->sock);

		ev_io_init(
			&client->io,
			socks_request_cb,
			client->sock,
			EV_READ);
		ev_io_start(loop, &client->io);

		fprintf(stderr, "Connection from: ");
		print_addr(stderr, client->addr.ss_family, &client->addr);
		putc('\n', stderr);
		return;
	}

	fprintf(stderr, "Error in accept(); %s\n", strerror(errno));
	close(client->sock);
	free(s);
}

void sigint_cb(struct ev_loop *loop, ev_signal *w, int revents) {
	ev_break(loop, EVBREAK_ALL);
	fprintf(stderr, "Interrupt\n");
}

//-------------------------------------------------------------------

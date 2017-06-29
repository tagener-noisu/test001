#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include "error.h"
#include "callbacks.h"
#include "networking.h"
//-------------------------------------------------------------------

void client_to_host_cb(struct ev_loop *loop, ev_io *w, int revents) {
	int len_recv;
	client *c = (client *) w;
	session *s = c->io.data;

	host *host = &s->host;
	char buf[LARGE_BUF];

	len_recv = recv(c->sock, buf, sizeof(buf), 0);
	if (len_recv != -1) {
		int total_sent = 0;
		while (total_sent < len_recv) {
			int sent = send(host->sock, buf, len_recv, 0);
			if (sent == -1) {
				if (errno == EAGAIN)
					continue;
				else
					break;
			}
			total_sent += sent;
		}
		if (total_sent < len_recv)
			error();
	}
}

void socks_resp_cb(struct ev_loop *loop, ev_io *w, int revents) {
	int stat;
	client *c = (client *) w;
	session *s = c->io.data;
	struct socks_reply *reply = s->data;

	stat = send(c->sock, reply, sizeof(struct socks_reply), 0);
	if (stat != -1) {
		fprintf(stderr, "SOCKS RESPONSE sent\n");
		ev_io_stop(loop, &c->io);
		ev_io_init(&c->io, client_to_host_cb, c->sock, EV_READ);
		ev_io_start(loop, &c->io);
		return;
	}
	else if (errno == EAGAIN) {
		return;
	}

	ev_io_stop(loop, &c->io);
	close(c->sock);
	close(s->host.sock);
	if (reply != NULL)
	free(reply);
	free(s);
}

void connect_to_host(session *s, struct ev_loop *loop) {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	s->host.sock = sock;

	if (sock != -1) {
		int stat;
		struct sockaddr_in * addr =
			(struct sockaddr_in *) &s->host.addr;

		struct socks_reply *repl = malloc(sizeof(struct socks_reply));
		repl->ver = 0;
		repl->stat = REJECTED;
		repl->port = addr->sin_port;
		repl->ipv4 = addr->sin_addr.s_addr;
		s->data = repl;

		//setnonblock(sock);
		stat = connect(
			sock,
			(struct sockaddr *) addr,
			sizeof(struct sockaddr_in));

		if (stat == 0)
			repl->stat = GRANTED;
		else if (stat == -1)
			error();

		ev_io_stop(loop, &s->client.io);
		ev_io_init(
			&s->client.io,
			socks_resp_cb,
			s->client.sock,
			EV_WRITE);
		ev_io_start(loop, &s->client.io);
		return;
	}

	close(s->client.sock);
	close(sock);
	free(s);
}

void socks_request_cb(struct ev_loop *loop, ev_io *w, int revents) {
	int stat;
	struct socks_request r;
	client *c = (client *) w;
	session *s = c->io.data;

	stat = recv(c->sock, &r, sizeof(r), 0);
	if (stat == -1 && errno == EAGAIN)
		return;

	if (stat != -1 && r.ver == 4) {
		char userid[SMALL_BUF];

		stat = blocking_recv(c->sock, userid, sizeof(userid), MSG_PEEK);
		if (stat != -1) {
			int id_len = strlen(userid);
			blocking_recv(c->sock, userid, id_len + 1, MSG_WAITALL);
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

			ev_io_stop(loop, &c->io);
			connect_to_host(s, loop);
			return;
		}
	}

	if (stat == -1)
		fprintf(stderr,
			"Error: socks_request_cb(), %s\n",
			strerror(errno));

	fprintf(stderr, "Connection closed.\n");
	close(c->sock);
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
		client->io.data = s;
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

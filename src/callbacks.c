#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include "log.h"
#include "callbacks.h"
#include "networking.h"
//-------------------------------------------------------------------

void host_to_client_cb(struct ev_loop *loop, ev_io *w, int revents) {
	int stat;
	host *h = (host *) w;
	session *s = h->session;

	if ((stat = send_data(s->host.sock, s->client.sock)) < 1) {
		if (stat == -1) {
			if (errno == EAGAIN)
				return;
			else
				log_errno(__FILE__, __LINE__, errno);
		}
		ev_io_stop(loop, &s->client.io);
		ev_io_stop(loop, &s->host.io);
		delete_session(s);
	}
}

void client_to_host_cb(struct ev_loop *loop, ev_io *w, int revents) {
	int stat;
	client *c = (client *) w;
	session *s = c->session;

	if ((stat = send_data(s->client.sock, s->host.sock)) < 1) {
		if (stat == -1) {
			if (errno == EAGAIN)
				return;
			else
				log_errno(__FILE__, __LINE__, errno);
		}
		ev_io_stop(loop, &s->client.io);
		ev_io_stop(loop, &s->host.io);
		delete_session(s);
	}
}

void socks_resp_cb(struct ev_loop *loop, ev_io *w, int revents) {
	int stat;
	client *c = (client *) w;
	session *s = c->session;
	host *h = &s->host;
	struct socks_reply reply = *(struct socks_reply *)s->data;

	free(s->data);
	s->data = NULL;

	stat = send(c->sock, &reply, sizeof(struct socks_reply), 0);
	if (stat != -1) {
		log_msg(LOG,
			__FILE__, __LINE__, "SOCKS RESPONSE sent\n");
		ev_io_stop(loop, &c->io);
		ev_io_stop(loop, &c->io);
		ev_io_init(&c->io, client_to_host_cb, c->sock, EV_READ);
		ev_io_init(&h->io, host_to_client_cb, h->sock, EV_READ);
		ev_io_start(loop, &c->io);
		ev_io_start(loop, &h->io);
		return;
	}
	else if (errno == EAGAIN) {
		return;
	}

	ev_io_stop(loop, &c->io);
	close(c->sock);
	close(s->host.sock);
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

		stat = connect(
			sock,
			(struct sockaddr *) addr,
			sizeof(struct sockaddr_in));

		if (stat == 0)
			repl->stat = GRANTED;
		else if (stat == -1)
			log_errno(__FILE__, __LINE__, errno);

		setnonblock(sock);
		ev_io_stop(loop, &s->client.io);
		ev_io_init(
			&s->client.io,
			socks_resp_cb,
			s->client.sock,
			EV_WRITE);
		ev_io_start(loop, &s->client.io);

		if (stat != -1)
			return;
	}

	ev_io_stop(loop, &s->client.io);
	close(s->client.sock);
	close(sock);
	free(s);
}

void socks_request_cb(struct ev_loop *loop, ev_io *w, int revents) {
	int stat;
	struct socks_request r;
	client *c = (client *) w;
	session *s = c->session;

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

			log_msg(LOG, __FILE__, __LINE__,
				"SOCKS REQUEST(%x), host: ", r.comm);
			print_addr(stderr, AF_INET, host);
			fprintf(stderr, ", from: %s\n", userid);

			ev_io_stop(loop, &c->io);
			connect_to_host(s, loop);
			return;
		}
	}

	if (stat == -1)
		log_errno(__FILE__, __LINE__, errno);

	log_msg(LOG, __FILE__, __LINE__, "Connection closed.\n");
	close(c->sock);
	ev_io_stop(loop, w);
	free(s);
}

void accept_cb (struct ev_loop *loop, ev_io *w, int revents) {
	server* serv = (server*) w;

	client client;
	int addrsz = sizeof(client.addr);

	client.sock = accept(
		serv->sock,
		(struct sockaddr*) &client.addr,
		(socklen_t*)&addrsz);

	if (client.sock != -1) {
		session *s = session_new();
		s->client.sock = client.sock;
		s->client.addr = client.addr;

		setnonblock(s->client.sock);

		ev_io_init(
			&s->client.io,
			socks_request_cb,
			s->client.sock,
			EV_READ);

		ev_io_start(loop, &s->client.io);

		log_msg(LOG, __FILE__, __LINE__, "Connection from: ");
		print_addr(stderr, s->client.addr.ss_family, &s->client.addr);
		putc('\n', stderr);
		return;
	}

	log_errno(__FILE__, __LINE__, errno);
	close(client.sock);
}

void sigint_cb(struct ev_loop *loop, ev_signal *w, int revents) {
	ev_break(loop, EVBREAK_ALL);
	log_msg(LOG_WARNING, __FILE__, __LINE__, "Interrupt\n");
}

//-------------------------------------------------------------------

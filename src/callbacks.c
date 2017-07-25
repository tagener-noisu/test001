#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include "log.h"
#include "callbacks.h"
#include "networking.h"
#include "session.h"
//-------------------------------------------------------------------

void session_timeout_cb(struct ev_loop *loop, ev_timer *w, int revents) {
	log_msg(LOG,
		__FILE__, __LINE__, "Session timed out\n");
	session_set_state((session *) w->data, SHUTDOWN);
}

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
		session_set_state(s, SHUTDOWN);
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
		session_set_state(s, SHUTDOWN);
	}
}

void socks_resp_cb(struct ev_loop *loop, ev_io *w, int revents) {
	int stat;
	client *c = (client *) w;
	session *s = c->session;
	struct sockaddr_in *addr = (struct sockaddr_in*) &s->host.addr;

	struct socks_reply reply;
	reply.ver = 0;
	reply.stat = s->host.status;
	reply.port = addr->sin_port;
	reply.ipv4 = addr->sin_addr.s_addr;

	stat = send(c->sock, &reply, sizeof(struct socks_reply), 0);
	if (stat != -1) {
		log_msg(LOG,
			__FILE__, __LINE__, "SOCKS RESPONSE sent\n");
		session_set_state(s, SOCKS_COMM);
		return;
	}
	else if (errno == EAGAIN) {
		return;
	}

	session_set_state(s, SHUTDOWN);
}

void connect_to_host_cb(struct ev_loop *loop, ev_io *w, int revents) {
	host *h = (host *) w;
	log_msg(LOG, __FILE__, __LINE__,
		"Connect to host!\n");
	session_set_state(h->session, SHUTDOWN);
}

void connect_to_host(session *s, struct ev_loop *loop) {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	s->host.sock = sock;

	if (sock != -1) {
		int stat;
		s->host.status = REJECTED;

		stat = connect(
			sock,
			(struct sockaddr *) &s->host.addr,
			sizeof(s->host.addr));

		if (stat == 0)
			s->host.status = GRANTED;
		else if (stat == -1)
			log_errno(__FILE__, __LINE__, errno);

		setnonblock(sock);
		session_set_state(s, SOCKS_RESP);

		if (stat != -1)
			return;
	}

	session_set_state(s, SHUTDOWN);
}

void socks_request_cb(struct ev_loop *loop, ev_io *w, int revents) {
	int stat;
	struct socks_request r;
	client *c = (client *) w;
	session *s = c->session;

	stat = recv(c->sock, &r, sizeof(r), MSG_PEEK);
	if (stat < sizeof(r)) {
		if (stat == -1 || stat == 0) {
			if (stat == -1 && errno == EAGAIN)
				return;
			else
				session_set_state(s, SHUTDOWN);
		}
		return;
	}
	stat = recv(c->sock, &r, sizeof(r), MSG_WAITALL);

	if (stat != -1 && r.ver == 4) {
		char userid[SMALL_BUF + 1];

		stat = blocking_recv(c->sock, userid, sizeof(userid), MSG_PEEK);
		if (stat != -1) {
			int sz = strlen(userid);
			blocking_recv(c->sock, userid, sz + 1, MSG_WAITALL);
			struct sockaddr_in *haddr =
				(struct sockaddr_in *) &s->host.addr;

			userid[stat] = '\0';

			memset(haddr, 0, sizeof(struct sockaddr_in));
			haddr->sin_family = AF_INET;
			haddr->sin_addr.s_addr = r.ipv4;
			haddr->sin_port = r.port;

			log_msg(LOG, __FILE__, __LINE__,
				"SOCKS REQUEST(%x), host: ", r.comm);
			print_addr(stderr, AF_INET, haddr);
			fprintf(stderr, ", from: %s\n", userid);

			s->host.sock = socket(AF_INET, SOCK_STREAM, 0);
			if (s->host.sock == -1) {
				log_errno(__FILE__, __LINE__, errno);
				session_set_state(s, SHUTDOWN);
				return;
			}
			setnonblock(s->host.sock);

			stat = connect(
				s->host.sock,
				(struct sockaddr *) haddr,
				sizeof(s->host.addr));

			if (stat == 0)
				session_set_state(s, SOCKS_RESP);
			else if (stat == -1) {
				if (errno == EINPROGRESS) {
					session_set_state(s, SOCKS_HOSTCONN);
					return;
				}
				log_errno(__FILE__, __LINE__, errno);
				session_set_state(s, SHUTDOWN);
			}
			return;
		}
	}

	if (stat == -1)
		log_errno(__FILE__, __LINE__, errno);

	session_set_state(s, SHUTDOWN);
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
		session *s = session_new(loop);
		setnonblock(client.sock);
		s->client.sock = client.sock;
		s->client.addr = client.addr;

		session_set_state(s, SOCKS_REQ);

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

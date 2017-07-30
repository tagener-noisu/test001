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
	log_msg(LOG_WARNING,
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
		if (s->host.status == GRANTED)
			return;
	}
	else if (errno == EAGAIN) {
		return;
	}

	session_set_state(s, SHUTDOWN);
}

void connect_to_host_cb(struct ev_loop *loop, ev_io *w, int revents) {
	int stat;
	int conn_stat = REJECTED;
	host *h = (host *) w;

	stat = connect(
		h->sock,
		(struct sockaddr *) &h->addr,
		sizeof(h->addr));

	if (stat == 0)
		conn_stat = GRANTED;

	if (stat == -1 && errno == EALREADY)
		return;

	h->status = conn_stat;
	session_set_state(h->session, SOCKS_RESP);
}

void socks4_req_header_cb(struct ev_loop *loop, ev_io *w, int revents) {
	int stat;
	client *c = (client *) w;
	session *s = c->session;
	struct socks4_request r;

	stat = nonblock_recv(c->sock, &r, sizeof(r), 0);
	if (stat == -1) {
		if (errno == ENODATA)
			session_set_state(s, SHUTDOWN);
		else if (errno != EAGAIN) {
			log_errno(__FILE__, __LINE__, errno);
			session_set_state(s, SHUTDOWN);
		}
		return;
	}

	if (r.ver == 4) {
		struct sockaddr_in *haddr = (struct sockaddr_in *)&s->host.addr;
		haddr->sin_family = AF_INET;
		haddr->sin_addr.s_addr = r.ip;
		haddr->sin_port = r.port;

		log_msg(LOG, __FILE__, __LINE__,
			"SOCKS request(%x), host: ", r.comm);
		print_addr(stderr, AF_INET, haddr);
		fprintf(stderr, "\n");

	}

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

		session_set_state(s, SOCKS_REQ_HEADER);

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

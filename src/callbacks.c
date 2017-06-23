#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include "error.h"
#include "callbacks.h"
#include "networking.h"
//-------------------------------------------------------------------

void connect_to_host_cb(struct ev_loop *loop, ev_io *w, int revents) {
	int sock;
	client* cli = (client*) w;
	sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock != -1) {
		int stat;
		struct sockaddr_in * addr = cli->data;
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

		stat = send(cli->sock, &repl, sizeof(repl), 0);
		free(addr);
	}

	ev_io_stop(loop, w);
	close(sock);
	close(cli->sock);
	free(cli);
	ev_break(loop, EVBREAK_ALL);
}

void socks_request_cb(struct ev_loop *loop, ev_io *w, int revents) {
	int stat;
	struct socks_request r;
	client* cli = (client*) w;

	stat = recv(cli->sock, &r, sizeof(r), 0);

	if (stat == -1 && errno == EAGAIN)
		return;

	if (stat != -1 && r.ver == 4) {
		char buf[SMALL_BUF];

		stat = blocking_recv(cli->sock, buf, sizeof(buf), MSG_WAITALL);
		if (stat != -1) {
			struct sockaddr_in *host =
				malloc(sizeof(struct sockaddr_in));

			memset(host, 0, sizeof(struct sockaddr_in));
			host->sin_family = AF_INET;
			host->sin_addr.s_addr = r.ipv4;
			host->sin_port = r.port;

			buf[stat - 1] = '\0';

			fprintf(stderr, "SOCKS REQUEST(%x), host: ", r.comm);
			print_addr(stderr, AF_INET, host);
			fprintf(stderr, ", from: %s\n", buf);

			cli->data = host;
			ev_io_stop(loop, w);
			ev_init(w, connect_to_host_cb);
			ev_io_start(loop, w);
			return;
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
	struct sockaddr_storage addr;
	int addrsz = sizeof(addr);
	server* serv = (server*) w;

	sock = accept(
		serv->sock,
		(struct sockaddr*)&addr,
		(socklen_t*)&addrsz);

	if (sock != -1) {
		setnonblock(sock);
		client* cli = client_new(socks_request_cb);
		cli->sock = sock;

		ev_io_start(loop, &cli->io);

		fprintf(stderr, "Connection from: ");
		print_addr(stderr, addr.ss_family, &addr);
		putc('\n', stderr);
		return;
	}

	fprintf(stderr, "Error in accept(); %s\n", strerror(errno));
	close(sock);
}

void sigint_cb(struct ev_loop *loop, ev_signal *w, int revents) {
	ev_break(loop, EVBREAK_ALL);
	fprintf(stderr, "Interrupt\n");
}

//-------------------------------------------------------------------

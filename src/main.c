#include <ev.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

struct context {
	ev_io io;
	int sock;
	struct sockaddr_in addr;
};
typedef struct context server;
typedef struct context client;

static void send_cb (struct ev_loop *loop, ev_io *w, int revents);
static void accept_cb (struct ev_loop *loop, ev_io *w, int revents);

void
set_sockaddr(struct sockaddr_in* addr, int fam, int s_addr, uint16_t port) {
	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = fam;
	addr->sin_addr.s_addr = s_addr;
	addr->sin_port = htons(port);
}

int setnonblock(int d) {
	int flags = fcntl(d, F_GETFL);
	if (flags == -1)
		return flags;
	flags |= O_NONBLOCK;

	return fcntl(d, F_SETFD, flags);
}

void server_init(server* s, int sock, struct sockaddr_in addr) {
	s->sock = sock;
	s->addr = addr;
}

client* client_new() {
	client* cli = (client*) malloc(sizeof(client));
	memset(cli, 0, sizeof(client));
	ev_io_init(&cli->io, send_cb, cli->sock, EV_WRITE);
	return cli;
}

static void send_cb (struct ev_loop *loop, ev_io *w, int revents) {
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

static void accept_cb (struct ev_loop *loop, ev_io *w, int revents) {
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

int server_loop() {
	server serv;
	int sock;
	int stat;
	struct sockaddr_in addr;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		return sock;

	stat = setnonblock(sock);
	if (stat == -1)
		return stat;

	set_sockaddr(&addr, AF_INET, INADDR_ANY, 1337);
	stat = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
	if (stat != 0)
		return stat;

	listen(sock, 0);
	server_init(&serv, sock, addr);

	struct ev_loop *loop = EV_DEFAULT;
	ev_io_init(&serv.io, accept_cb, serv.sock, EV_READ);
	ev_io_start(loop, &serv.io);

	ev_run(loop, 0);

	ev_loop_destroy(loop);
	return 0;
}

int main() {
	int stat = server_loop();
	if (stat != 0)
		fprintf(stderr, "Error: %s", strerror(errno));
	return stat;
}

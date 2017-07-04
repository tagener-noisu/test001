#include <ev.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>

#include "networking.h"
#include "callbacks.h"
#include "log.h"
//-------------------------------------------------------------------

#define PORT "1337"

int bind_locally() {
	int stat, sock;
	struct addrinfo hints;
	struct addrinfo *list, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	stat = getaddrinfo(NULL, PORT, &hints, &list);
	if (stat != 0)
		return stat;

	for (p = list; p != NULL; p = p->ai_next) {
		sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sock != -1) {
			int reuse = 1;

			setsockopt(
				sock,
				SOL_SOCKET,
				SO_REUSEADDR,
				&reuse,
				sizeof(reuse));

			stat = bind(sock, p->ai_addr, p->ai_addrlen);
			if (stat == 0)
				break; // SUCCESS
		}
		close(sock);
	}
	freeaddrinfo(list);

	if (sock == -1) {
		log_errno(__FILE__, __LINE__, errno);
		return sock;
	}

	if (stat == -1) {
		log_errno(__FILE__, __LINE__, errno);
		return stat;
	}

	return sock;
}

int server_loop() {
	server serv;
	int sock;

	sock = bind_locally();
	if (sock == -1)
		return sock;

	setnonblock(sock);

	listen(sock, 0);
	serv.sock = sock;

	struct ev_loop *loop = EV_DEFAULT;
	ev_io_init(&serv.io, accept_cb, serv.sock, EV_READ);
	ev_io_start(loop, &serv.io);

	// interrupt handler
	ev_signal int_handler;
	ev_signal_init(&int_handler, sigint_cb, SIGINT);
	ev_signal_start(loop, &int_handler);

	ev_run(loop, 0);

	ev_loop_destroy(loop);
	close(sock);
	return 0;
}

int main() {
	int stat = server_loop();
	return stat;
}

#include <ev.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "networking.h"
#include "callbacks.h"
//-------------------------------------------------------------------

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
	if (stat != 0)
		fprintf(stderr, "Error: %s\n", strerror(errno));
	return stat;
}

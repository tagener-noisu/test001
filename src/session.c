#include <stdlib.h>
#include <unistd.h>
#include "session.h"
#include "callbacks.h"
//-------------------------------------------------------------------

void session_cleanup(struct ev_loop *l, ev_cleanup *c, int e);

session * session_new(struct ev_loop *loop) {
	session * s = (session *) malloc(sizeof(session));
	memset(s, 0, sizeof(session));

	s->loop = loop;
	ev_cleanup_init(&s->cleanup, session_cleanup);
	ev_cleanup_start(s->loop, &s->cleanup);

	s->client.session = s;
	s->host.session = s;
	s->state = IDLE;
	return s;
}

void session_cleanup(struct ev_loop * l, ev_cleanup *c, int e) {
	delete_session((session *) c);
}

void delete_session(session * s) {
	ev_cleanup_stop(s->loop, &s->cleanup);
	ev_io_stop(s->loop, &s->client.io);
	ev_io_stop(s->loop, &s->host.io);
	close(s->client.sock);
	close(s->host.sock);
	free(s);
}

void session_set_state(session *s, int state) {
	switch (state) {
		case IDLE:
			return;
		case SOCKS_REQ:
			s->state = state;
			ev_io_stop(s->loop, &s->client.io);
			ev_io_init(
				&s->client.io,
				socks_request_cb,
				s->client.sock,
				EV_READ);
			ev_io_start(s->loop, &s->client.io);
			return;
		case SOCKS_RESP:
			ev_io_stop(s->loop, &s->client.io);
			ev_io_init(
				&s->client.io,
				socks_resp_cb,
				s->client.sock,
				EV_WRITE);
			ev_io_start(s->loop, &s->client.io);
			return;
		case SHUTDOWN:
			delete_session(s);
			return;
	}
}

//-------------------------------------------------------------------

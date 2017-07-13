#include <stdlib.h>
#include <unistd.h>
#include "session.h"
//-------------------------------------------------------------------

void session_cleanup(struct ev_loop * l, ev_cleanup *c, int e);

session * session_new() {
	session * s = (session *) malloc(sizeof(session));
	memset(s, 0, sizeof(session));
	ev_cleanup_init(&s->cleanup, session_cleanup);
	s->client.session = s;
	s->host.session = s;

	s->state = IDLE;
	return s;
}

void session_cleanup(struct ev_loop * l, ev_cleanup *c, int e) {
	delete_session((session *) c);
}

void delete_session(session * s) {
	ev_io_stop(s->loop, &s->client.io);
	ev_io_stop(s->loop, &s->host.io);
	close(s->client.sock);
	close(s->host.sock);
	free(s);
}

//-------------------------------------------------------------------

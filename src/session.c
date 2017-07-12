#include <stdlib.h>
#include <unistd.h>
#include "session.h"
//-------------------------------------------------------------------

session * session_new() {
	session * s = (session *) malloc(sizeof(session));
	memset(s, 0, sizeof(session));
	ev_cleanup_init(&s->cleanup, session_cleanup);
	s->client.session = s;
	s->host.session = s;

	s->state = IDLE;
	return s;
}

void delete_session(session * s) {
	ev_io_stop(s->loop, &s->client.io);
	ev_io_stop(s->loop, &s->host.io);
	close(s->client.sock);
	close(s->host.sock);
	free(s);
}

//-------------------------------------------------------------------

#include <stdlib.h>
#include <unistd.h>
#include "session.h"
//-------------------------------------------------------------------

session * session_new() {
	session * s = (session *) malloc(sizeof(session));
	memset(s, 0, sizeof(session));
	s->client.session = s;
	s->host.session = s;
	return s;
}

void delete_session(session * s) {
	close(s->client.sock);
	close(s->host.sock);
	if (s->data != NULL)
		free(s->data);
	free(s);
}

//-------------------------------------------------------------------

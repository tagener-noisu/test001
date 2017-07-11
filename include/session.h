#pragma once
#include <ev.h>
#include <netdb.h>
#include "socks.h"
//-------------------------------------------------------------------

struct context {
	ev_io io;
	int sock;
	struct sockaddr_storage addr;
	struct session *session;
};
typedef struct context client;
typedef struct context host;

typedef struct session {
	struct context client;
	struct context host;
	void *data;
} session;

session * session_new();

void delete_session(session *);

//-------------------------------------------------------------------

#pragma once
#include <ev.h>
#include <netdb.h>
#include "socks.h"
//-------------------------------------------------------------------

enum s_state {
	IDLE,
	SOCKS_REQ,
	SHUTDOWN
};

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
	struct ev_loop *loop;
	enum s_state state;
	void *data;
} session;

session * session_new();

void delete_session(session *);

void session_set_state(session *, int state);

//-------------------------------------------------------------------

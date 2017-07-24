#pragma once
#include <ev.h>
#include <netdb.h>
#include "socks.h"
//-------------------------------------------------------------------

enum s_state {
	IDLE,
	SOCKS_REQ,
	SOCKS_RESP,
	SOCKS_COMM,
	SOCKS_HOSTCONN,
	SHUTDOWN
};

struct context {
	ev_io io;
	int sock;
	struct sockaddr_storage addr;
	struct session *session;
};

struct host_context {
	ev_io io;
	int sock;
	struct sockaddr_storage addr;
	struct session *session;
	int status;
};
typedef struct context client;
typedef struct host_context host;

typedef struct session {
	ev_cleanup cleanup;
	ev_timer timer;
	client client;
	host host;
	struct ev_loop *loop;
	enum s_state state;
} session;

session * session_new();

void delete_session(session *);

void session_set_state(session *, int);

//-------------------------------------------------------------------

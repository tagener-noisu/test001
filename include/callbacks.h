#pragma once

#include <ev.h>
//-------------------------------------------------------------------

void accept_cb (struct ev_loop *loop, ev_io *w, int revents);

void sigint_cb(struct ev_loop *loop, ev_signal *w, int revents);

void socks_request_cb(struct ev_loop *loop, ev_io *w, int revents);

void socks_resp_cb(struct ev_loop *loop, ev_io *w, int revents);

void client_to_host_cb(struct ev_loop *loop, ev_io *w, int revents);

void host_to_client_cb(struct ev_loop *loop, ev_io *w, int revents);

//-------------------------------------------------------------------

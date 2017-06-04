#pragma once

#include <ev.h>
//-------------------------------------------------------------------

void send_cb (struct ev_loop *loop, ev_io *w, int revents);

void accept_cb (struct ev_loop *loop, ev_io *w, int revents);

void sigint_cb(struct ev_loop *loop, ev_signal *w, int revents);

//-------------------------------------------------------------------

#pragma once
#include <stdint.h>
//-------------------------------------------------------------------

enum socks_stat {
	GRANTED = 0x5A,
	REJECTED = 0x5B,
	OTHER1 = 0x5C,
	OTHER2 = 0x5D
};

enum socks_comm {
	STREAM_CON = 0x1,
	PORT_BIND = 0x2,
};

struct socks_reply {
	uint8_t ver;
	uint8_t stat;
	uint16_t port;
	uint32_t ipv4;
};

struct socks4_request {
	uint8_t ver;
	uint8_t comm;
	uint16_t port;
	uint32_t ip;
};

//-------------------------------------------------------------------

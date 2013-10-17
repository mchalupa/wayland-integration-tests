#ifndef __WIT_GLOBAL_H__
#define __WIT_GLOBAL_H__

/**
 * Definitions visible for both - server and client
 */

enum optype {
	/* arguments: int can (0 or 1)*/
	CAN_CONTINUE = 1,	/* client can continue */

	/* arguments: int count*/
	EVENT_COUNT,	   	/* how many events can display emit */

	/* arguments: struct wit_event, union wl_argument *args */
	EVENT_EMIT,		/* ask for single event */

	/* arguments: none */
	RUN_FUNC,		/* run user's func */

	/* arguments: size_t size, void *mem */
	SEND_BYTES,		/* send raw bytestream to the other side */

	/* arguments: none */
	BARRIER,		/* sync client with display */
};

enum side {
	CLIENT = 0,
	DISPLAY = 1
};


#include <wayland-client-protocol.h>
const struct wl_registry_listener registry_default_listener;


/* write with assert check */
int
asswrite(int fd, void *src, size_t size);

int
assread(int fd, void *dest, size_t size);

void
send_message(int fd, enum optype op, ...);
#endif  /* __WIT_UTIL_H__ */

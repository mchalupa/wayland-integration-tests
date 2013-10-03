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
	SEND_BYTES		/* send raw bytestream to the other side */
};

#include <wayland-client-protocol.h>
const struct wl_registry_listener registry_default_listener;

#endif  /* __WIT_UTIL_H__ */

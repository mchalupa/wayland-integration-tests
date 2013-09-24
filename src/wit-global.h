#ifndef __WIT_GLOBAL_H__
#define __WIT_GLOBAL_H__

/**
 * Definitions visible for both - server and client
 */


enum optype {
	CAN_CONTINUE = 1,	/* client can continue */
	EVENT_COUNT,	   	/* how many events can display emit */
	RUN_FUNC		/* run user's func */
};

#include <wayland-client-protocol.h>
const struct wl_registry_listener registry_default_listener;

#endif  /* __WIT_UTIL_H__ */

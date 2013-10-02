#ifndef __WIT_EVENTS_H__
#define __WIT_EVENTS_H__

#include <wayland-util.h>

struct wit_display;

#define MAX_ARGS_NO 15
#define MAX_EVENTS 100

/**
 * Usage:
 *  struct wit_event *send_motion = wit_event_define(&wl_pointer_interface, WL_POINTER_SEND_MOTION);
 *
 *  wit_eventarray_add(send_motion, arg1, arg2, arg3, arg4);
 *  wit_eventarray_add(send_motion, 1, 2, 3, 4);
 *
 *  ...
 *  ...
 */

struct wit_event {
	const struct wl_interface *interface;
	uint32_t opcode;
};

struct event;
struct wit_eventarray {
	struct event *events[MAX_EVENTS];

	unsigned count;
	unsigned index;
};

/* we use pointer in all functions, so create event as opaque structure
 * and "return" pointer */
#define WIT_EVENT_DEFINE(eventname, intf, opcode) 				\
	assertf((intf), "WIT_EVENT_DEFINE: Interface cannot be NULL");		\
	assertf((opcode) < ((struct wl_interface *) (intf))->event_count,	\
		"WIT_EVENT_DEFINE: Event opcode is illegal (%d for '%s')",	\
		(opcode),((struct wl_interface *) (intf))->name);		\
	const static struct wit_event __wit_event_##eventname##_		\
							= {(intf), (opcode)};	\
	const struct wit_event *(eventname) = &__wit_event_##eventname##_;

/* the same as WIT_EVENT_DEFINE but with eventarray, initialize it with nuls */
#define WIT_EVENTARRAY_DEFINE(name)						\
	static struct wit_eventarray __wit_eventarray_##name##_ = {{0}, 0, 0};	\
	struct wit_eventarray *(name) = &__wit_eventarray_##name##_;

unsigned int
wit_eventarray_add(struct wit_eventarray *ea, const struct wit_event *event, ...);

int
wit_eventarray_emit_one(struct wit_display *d, struct wit_eventarray *ea);

int
wit_eventarray_compare(struct wit_eventarray *a, struct wit_eventarray *b);

void
wit_eventarray_free(struct wit_eventarray *ea);
#endif /* __WIT_EVENTS_H__ */

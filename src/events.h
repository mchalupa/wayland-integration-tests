#ifndef __WIT_EVENTS_H__
#define __WIT_EVENTS_H__

#include <wayland-util.h>
#include <wit-global.h>

struct wit_display;

#define MAX_ARGS_NO 15
#define MAX_EVENTS 100

/**
 * Usage:
 *  WIT_EVENT_DEFINE(send_motion, &wl_pointer_interface, WL_POINTER_MOTION);
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
	assertf((opcode) < ((struct wl_interface *) (intf))->event_count,	\
		"WIT_EVENT_DEFINE: Event opcode is illegal (%d for '%s')",	\
		(opcode),((struct wl_interface *) (intf))->name);		\
	static const struct wit_event __wit_event_##eventname##_		\
							= {(intf), (opcode)};	\
	const struct wit_event *(eventname) = &__wit_event_##eventname##_;

struct wit_eventarray *
wit_eventarray_create();

/*
 * side = {CLIENT|DISPLAY}
 */
unsigned int
wit_eventarray_add(struct wit_eventarray *ea, enum side side,
			const struct wit_event *event, ...);

int
wit_eventarray_emit_one(struct wit_display *d, struct wit_eventarray *ea);

int
wit_eventarray_compare(struct wit_eventarray *a, struct wit_eventarray *b);

void
wit_eventarray_free(struct wit_eventarray *ea);
#endif /* __WIT_EVENTS_H__ */

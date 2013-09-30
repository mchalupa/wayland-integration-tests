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

struct wit_event *
wit_event_define(const struct wl_interface *intf, uint32_t opcode);

void
wit_eventarray_add(struct wit_eventarray *ea, struct wit_event *event, ...);

int
wit_eventarray_emit_one(struct wit_display *d, struct wit_eventarray *ea);

void
wit_eventarray_free(struct wit_eventarray *ea);
#endif /* __WIT_EVENTS_H__ */

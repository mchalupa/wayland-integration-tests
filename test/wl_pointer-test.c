/*
 * Copyright © 2013 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include <assert.h>
#include <wayland-client.h>
#include <wayland-server.h>

#include "test-runner.h"
#include "wit.h"

/* -----------------------------------------------------------------------------
    Pointer listener
   -------------------------------------------------------------------------- */
static void
pointer_handle_enter(void *data, struct wl_pointer *pointer, uint32_t serial,
		     struct wl_surface *surface, wl_fixed_t surface_x,
		     wl_fixed_t surface_y)
{
	assert(pointer);

	struct wit_client *c = data;
	WIT_EVENT_DEFINE(e, &wl_pointer_interface, WL_POINTER_ENTER);

	wit_eventarray_add(c->events, CLIENT, e, serial, surface, surface_x, surface_y);
}

static void
pointer_handle_leave(void *data, struct wl_pointer *pointer, uint32_t serial,
		     struct wl_surface *surface)
{
	assert(pointer);

	struct wit_client *c = data;
	WIT_EVENT_DEFINE(e, &wl_pointer_interface, WL_POINTER_LEAVE);

	wit_eventarray_add(c->events, CLIENT, e, serial, surface);
}

static void
pointer_handle_motion(void *data, struct wl_pointer *pointer, uint32_t time,
		      wl_fixed_t x, wl_fixed_t y)
{
	assert(pointer);

	struct wit_client *c = data;
	WIT_EVENT_DEFINE(e, &wl_pointer_interface, WL_POINTER_MOTION);

	wit_eventarray_add(c->events, CLIENT, e, time, x, y);
}

static void
pointer_handle_button(void *data, struct wl_pointer *pointer, uint32_t serial,
		      uint32_t time, uint32_t button, uint32_t state)
{
	assert(pointer);

	struct wit_client *c = data;
	WIT_EVENT_DEFINE(e, &wl_pointer_interface, WL_POINTER_BUTTON);

	wit_eventarray_add(c->events, CLIENT, e, serial, time, button, state);
}

static void
pointer_handle_axis(void *data, struct wl_pointer *pointer, uint32_t time,
		    uint32_t axis, wl_fixed_t value)
{
	assert(pointer);

	struct wit_client *c = data;
	WIT_EVENT_DEFINE(e, &wl_pointer_interface, WL_POINTER_AXIS);

	wit_eventarray_add(c->events, CLIENT, e, time, axis, value);
}

static const struct wl_pointer_listener pointer_default_listener = {
	pointer_handle_enter,
	pointer_handle_leave,
	pointer_handle_motion,
	pointer_handle_button,
	pointer_handle_axis
};

/*
 * Test if each event come and if it comes with the right arguments.
 * I know that it is not very useful test of wl_pointer,
 * but I'm working on better :)
 */
static int
pointer_test_each_once_main(int sock)
{
	struct wit_eventarray *recived_events = wit_eventarray_create();
	struct wit_client *c = wit_client_populate(sock);
	struct wl_surface *surface
				= wl_compositor_create_surface(
					(struct wl_compositor *) c->compositor.proxy);
	assert(surface);
	dbg("Created surface id %u\n",
	    wl_proxy_get_id((struct wl_proxy *) surface));

	wit_client_add_listener(c, "wl_pointer", &pointer_default_listener);
	c->events = recived_events;

	wl_display_roundtrip(c->display);
	wit_client_barrier(c);

	wit_client_ask_for_events(c, 0);
	wl_display_roundtrip(c->display);

	wit_client_send_eventarray(c, recived_events);

	wit_eventarray_free(recived_events);
	wl_surface_destroy(surface);
	wit_client_free(c);
	return EXIT_SUCCESS;
}

TEST(pointer_each_once_tst)
{
	struct wit_display *d = wit_display_create(NULL);

	/* this is one way how to use eventarray. Create it on both side
	 * and then send it from client to server and compare.
	 * The other way is to create it only in client, then send it
	 * to display, let it to be emitted and compare with events that came */
	struct wit_eventarray *events = wit_eventarray_create();
	wit_display_add_events(d, events);
	wit_display_create_client(d, pointer_test_each_once_main);
	wit_display_run(d);

	/* barrier, wait for client to create resource */
	wit_display_barrier(d);

	/* I'm lazy, use this one variable for all events (modify it manually) */
	struct wit_event pointer_event = {&wl_pointer_interface, WL_POINTER_MOTION};

	/* fill eventarray */
	wit_eventarray_add(events, DISPLAY, &pointer_event, 3, 0, 0);
	pointer_event.opcode = WL_POINTER_BUTTON;
	wit_eventarray_add(events, DISPLAY, &pointer_event, 4, 4, 0, 0);
	pointer_event.opcode = WL_POINTER_AXIS;
	wit_eventarray_add(events, DISPLAY, &pointer_event, 5, 0, 0);
	pointer_event.opcode = WL_POINTER_ENTER;
	wit_eventarray_add(d->events, DISPLAY, &pointer_event, 111,
			  d->resources.surface, 0, 0);
	pointer_event.opcode = WL_POINTER_LEAVE;
	wit_eventarray_add(d->events, DISPLAY, &pointer_event, 111, d->resources.surface);

	/* client is calling for events */
	wit_display_emit_events(d);

	/* recieve client's eventarray and compare */
	wit_display_recieve_eventarray(d);
	assert(wit_eventarray_compare(d->events, events) == 0);

	wit_eventarray_free(events);
	wit_display_destroy(d);
}
/* TODO create more sophisticated tests */

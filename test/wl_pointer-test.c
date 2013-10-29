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

/* -----------------------------------------------------------------------------
    Pointer implementation
   -------------------------------------------------------------------------- */
static void
pointer_set_cursor(struct wl_client *client, struct wl_resource *resource,
		   uint32_t serial, struct wl_resource *surface,
		   int32_t hotspot_x, int32_t hotspot_y)
{
	 dbg("client: %p, res: %p, serial: %u, surf: %p, x: %d, y: %d\n",
	 client, resource, serial, surface, hotspot_x, hotspot_y);
	 /*
	 * This request only takes effect if the
	 * pointer focus for this device is one of the requesting client's
	 * surfaces or the surface parameter is the current pointer
	 * surface. If there was a previous surface set with this request
	 * it is replaced. If surface is NULL, the pointer image is hidden.
	 *
	 * The parameters hotspot_x and hotspot_y define the position of
	 * the pointer surface relative to the pointer location. Its
	 * top-left corner is always at (x, y) - (hotspot_x, hotspot_y),
	 * where (x, y) are the coordinates of the pointer location, in
	 * surface local coordinates.
	 *
	 * On surface.attach requests to the pointer surface, hotspot_x and
	 * hotspot_y are decremented by the x and y parameters passed to
	 * the request. Attach must be confirmed by wl_surface.commit as
	 * usual.
	 *
	 * The hotspot can also be updated by passing the currently set
	 * pointer surface to this request with new values for hotspot_x
	 * and hotspot_y.
	 *
	 * The current and pending input regions of the wl_surface are
	 * cleared, and wl_surface.set_input_region is ignored until the
	 * wl_surface is no longer used as the cursor. When the use as a
	 * cursor ends, the current and pending input regions become
	 * undefined, and the wl_surface is unmapped.
	 */
}

static const struct wl_pointer_interface pointer_implementation  = {
	.set_cursor = pointer_set_cursor
};

static int
pointer_set_cursor_main(int s)
{
	struct wit_client *c = wit_client_populate(s);
	struct wl_surface *surf =
		wl_compositor_create_surface((struct wl_compositor *) c->compositor.proxy);
	wl_display_roundtrip(c->display);

	/* stop display, so that it can set implementation */
	wit_client_barrier(c);

	wl_pointer_set_cursor((struct wl_pointer *) c->pointer.proxy,
			      0, surf, 5, 5);
	wl_pointer_set_cursor((struct wl_pointer *) c->pointer.proxy,
			      0, surf, 5, 5);

	wit_client_free(c);
	return EXIT_SUCCESS;
}

TEST(pointer_set_cursor_tst)
{
	struct wit_config conf = {CONF_SEAT | CONF_COMPOSITOR, ~(CONF_KEYBOARD | CONF_TOUCH), 0};
	struct wit_display *d = wit_display_create(&conf);

	wit_display_create_client(d, pointer_set_cursor_main);
	wit_display_run(d);

	wl_resource_set_implementation(d->resources.pointer,
				       &pointer_implementation, d, NULL);

	wit_display_barrier(d);

	wit_display_destroy(d);
}

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

	wit_display_recieve_eventarray(d);
	assert(wit_eventarray_compare(d->events, events) == 0);

	wit_eventarray_free(events);
	wit_display_destroy(d);
}

/* TODO create more sophisticated tests */

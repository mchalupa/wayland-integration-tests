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

#include <stdio.h>
#include <string.h>

#include <wayland-client.h>
#include <wayland-server.h>

#include "client.h"
#include "wit-assert.h"

/* -----------------------------------------------------------------------------
    Seat listener
   -------------------------------------------------------------------------- */
static void
seat_handle_caps(void *data, struct wl_seat *seat, enum wl_seat_capability caps)
{
	assertf(data, "No data when catched seat");
	assertf(seat, "No seat when catched seat");

	dbg("Seat handle caps\n");

	struct wit_client *cl = data;

	if (caps & WL_SEAT_CAPABILITY_POINTER) {
		if (cl->pointer != NULL)
			wl_pointer_destroy(cl->pointer);

		cl->pointer = wl_seat_get_pointer(seat);
		assertf(cl->pointer,
			"wl_seat_get_pointer returned NULL in seat_listener function");

		if (cl->listener.pointer)
			wl_pointer_add_listener(cl->pointer, cl->listener.pointer, cl);
	}

	if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
		if (cl->keyboard != NULL)
			wl_keyboard_destroy(cl->keyboard);

		cl->keyboard = wl_seat_get_keyboard(seat);
		assertf(cl->keyboard,
			"Got no keyboard from seat");

		if (cl->listener.keyboard)
			wl_keyboard_add_listener(cl->keyboard, cl->listener.keyboard, cl);
	}

	if (caps & WL_SEAT_CAPABILITY_TOUCH) {
		if (cl->touch != NULL)
			wl_touch_destroy(cl->touch);

		cl->touch= wl_seat_get_touch(seat);
		assertf(cl->touch,
			"Got no touch from seat");

		if (cl->listener.touch)
			wl_touch_add_listener(cl->touch, cl->listener.touch, cl);
	}

	/* block until synced, or client will end too early */
	if (caps)
		wl_display_roundtrip(cl->display);

	assertf(wl_display_get_error(cl->display) == 0,
		"An error in display occured");
}

/*
static void
seat_handle_name(void *data, struct wl_seat *wl_seat, const char *name)
{
	struct client *c = data;
}
*/

const struct wl_seat_listener seat_default_listener = {
	seat_handle_caps,
	NULL /* seat_handle_name */
};

/* -----------------------------------------------------------------------------
    Registry listener
   -------------------------------------------------------------------------- */
static void
registry_handle_global(void *data, struct wl_registry *registry,
		       uint32_t id, const char *interface, uint32_t version)
{
	struct wit_client *cl = data;
	if (strcmp(interface, "wl_seat") == 0) {
		cl->seat = wl_registry_bind(registry, id, &wl_seat_interface,
					    version);
		assertf(cl->seat, "Binding to registry for seat failed");

		wit_client_add_listener(cl, "wl_seat", &seat_default_listener);
		assertf(cl->listener.seat, "Failed adding listener");

		wl_display_roundtrip(cl->display);
		assertf(wl_display_get_error(cl->display) == 0,
			"An error in display occured");
	}
}

const struct wl_registry_listener registry_default_listener = {
	registry_handle_global,
	NULL /* TODO */
};

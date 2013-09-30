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
#include "wayland-server.h"
#include "wit-assert.h"
#include "server.h"

/* -----------------------------------------------------------------------------
 *  Seat default implementation
 * -------------------------------------------------------------------------- */
static void
seat_get_pointer(struct wl_client *client, struct wl_resource *resource,
		 uint32_t id)
{
	struct wl_resource *res;
	struct wit_display *d = wl_resource_get_user_data(resource);
	assertf(d, "No user data in resource");

	if (!(d->config.resources & CONF_POINTER)) {
		dbg("Creating pointer resource suppressed\n");
		return;
	}

	res = wl_resource_create(client, &wl_pointer_interface, 1, id);
	assertf(res, "Failed creating resource for pointer");
	wl_resource_set_user_data(res, d);

	d->resources.pointer = res;
}

static void
seat_get_keyboard(struct wl_client *client, struct wl_resource *resource,
		 uint32_t id)
{
	struct wl_resource *res;
	struct wit_display *d = wl_resource_get_user_data(resource);
	assertf(d, "No user data in resource");

	if (!(d->config.resources & CONF_KEYBOARD)) {
		dbg("Creating keyboard resource suppressed\n");
		return;
	}

	res = wl_resource_create(client, &wl_keyboard_interface, 1, id);
	assertf(res, "Failed creating resource for keyboard");
	wl_resource_set_user_data(res, d);

	d->resources.keyboard = res;
}

static void
seat_get_touch(struct wl_client *client, struct wl_resource *resource,
		 uint32_t id)
{
	struct wl_resource *res;
	struct wit_display *d = wl_resource_get_user_data(resource);
	assertf(d, "No user data in resource");

	if (!(d->config.resources & CONF_TOUCH)) {
		dbg("Creating touch resource suppressed\n");
		return;
	}

	res = wl_resource_create(client, &wl_touch_interface, 1, id);
	assertf(res, "Failed creating resource for touch");
	wl_resource_set_user_data(res, d);

	d->resources.touch = res;
}

const struct wl_seat_interface seat_default_implementation = {
	seat_get_pointer,
	seat_get_keyboard,
	seat_get_touch
};

void
seat_bind(struct wl_client *client, void *data,
	      uint32_t version, uint32_t id)
{
	struct wit_display *d = data;
	enum wl_seat_capability cap = 0;

	if (!(d->config.resources & CONF_SEAT)) {
		dbg("Creating seat resource suppressed\n");
		return;
	}

	/* set capabilities according to configuration. Can be changed later */
	if (d->config.resources & CONF_POINTER)
		cap |= WL_SEAT_CAPABILITY_POINTER;
	if (d->config.resources & CONF_KEYBOARD)
		cap |= WL_SEAT_CAPABILITY_KEYBOARD;
	if (d->config.resources & CONF_TOUCH)
		cap |= WL_SEAT_CAPABILITY_TOUCH;

	d->resources.seat =
		wl_resource_create(client,
				   &wl_seat_interface, version, id);
	assertf(d->resources.seat, "Failed creating resource for seat");
	wl_resource_set_implementation(d->resources.seat,
				       &seat_default_implementation, data, NULL);

	/* trigger handle_seat */
	wl_seat_send_capabilities(d->resources.seat, cap);
}


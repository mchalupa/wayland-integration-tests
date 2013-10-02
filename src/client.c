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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <wayland-client.h>

#include "wit-global.h"
#include "wit-assert.h"
#include "client.h"


/*
 * Allow comfortably add listener into client structure
 */
void
wit_client_add_listener(struct wit_client *cl, const char *interface,
			void *listener)
{
	assertf(cl);
	assert(interface);

	ifdbg(listener == NULL, "Adding NULL listener (%s)\n", interface);

	if (strcmp(interface, "wl_pointer") == 0) {
		ifdbg(cl->listener.pointer, "Rewriting pointer listener (%p)\n",
		      cl->listener.pointer);
		cl->listener.pointer = (struct wl_pointer_listener *) listener;

		if (cl->pointer)
			wl_pointer_add_listener(cl->pointer,
						cl->listener.pointer, cl);
		else
			dbg("Not adding listener. Pointer proxy hasn't been created yet.\n");
	} else if (strcmp(interface, "wl_keyboard") == 0) {
		ifdbg(cl->listener.keyboard, "Rewriting keyboard listener (%p)\n",
		      cl->listener.keyboard);
		cl->listener.keyboard = (struct wl_keyboard_listener *) listener;

		if (cl->keyboard)
			wl_keyboard_add_listener(cl->keyboard,
						 cl->listener.keyboard, cl);
		else
			dbg("Not adding listener. Keyboard proxy hasn't been created yet.\n");
	} else if (strcmp(interface, "wl_touch") == 0) {
		ifdbg(cl->listener.touch, "Rewriting touch listener (%p)\n",
		      cl->listener.touch);
		cl->listener.touch = (struct wl_touch_listener *) listener;

		if (cl->touch)
			wl_touch_add_listener(cl->touch, cl->listener.touch, cl);
		else
			dbg("Not adding listener. Touch proxy hasn't been created yet.\n");
	} else if (strcmp(interface, "wl_seat") == 0) {
		ifdbg(cl->listener.seat, "Rewriting seat listener (%p)\n",
		      cl->listener.seat);
		cl->listener.seat = (struct wl_seat_listener *) listener;

		if (cl->seat)
			wl_seat_add_listener(cl->seat, cl->listener.seat, cl);
		else
			dbg("Not adding listener. Seat proxy hasn't been created yet.\n");
	} else if (strcmp(interface, "wl_registry") == 0) {
		ifdbg(cl->listener.registry, "Rewriting registry listener (%p)\n",
		      cl->listener.pointer);
		cl->listener.registry = (struct wl_registry_listener *) listener;

		if (cl->registry)
			wl_registry_add_listener(cl->registry, cl->listener.registry, cl);
		else
			dbg("Not adding listener. Registry proxy hasn't been created yet.\n");
	}
	else {
		assertf(0, "Unknown type of interface");
	}
}

struct wit_client *
wit_client_populate(int sock)
{
	struct wit_client *c = calloc(1, sizeof *c);
	assert(c && "Out of memory");

	c->sock = sock;

	c->display = wl_display_connect(NULL);
	assertf(c->display, "Couldn't connect to display");

	c->registry = wl_display_get_registry(c->display);
	assertf(c->registry, "Couldn't get registry");

	wit_client_add_listener(c, "wl_registry", &registry_default_listener);
	wl_display_dispatch(c->display);

	assertf(wl_display_get_error(c->display) == 0,
		"An error in display occured");

	return c;
}

void
wit_client_free(struct wit_client *c)
{
	assertf(c, "Wrong pointer");
	assertf(wl_display_get_error(c->display) == 0,
		"An error in display occured");

	wl_registry_destroy(c->registry);

	if(c->seat)
		wl_seat_destroy(c->seat);
	if(c->pointer)
		wl_pointer_destroy(c->pointer);
	if(c->keyboard)
		wl_keyboard_destroy(c->keyboard);
	if(c->touch)
		wl_touch_destroy(c->touch);

	wl_display_disconnect(c->display);

	close(c->sock);

	free(c);
}

void
wit_client_call_user_func(struct wit_client *cl)
{
	int stat;

	assertf(cl, "No client's structure passed");

	stat = kill(getppid(), SIGUSR1);
	assertf(stat == 0,
		"Failed sending signal to display");

	enum optype op = RUN_FUNC;
	stat = write(cl->sock, &op, sizeof(op));
	assertf(stat == sizeof(op), "Sent %d instead of %lu bytes (sendig optype)",
		stat, sizeof(op));
}

void
wit_client_ask_for_events(struct wit_client *cl, int n)
{
	int stat;

	assertf(cl, "No client's structure passed");
	assertf(n >= 0, "Asked for negative number of events");

	/* kick to display to have its attention */
	stat = kill(getppid(), SIGUSR1);
	assertf(stat == 0,
		"Failed sending signal to start emitting events");

	enum optype op = EVENT_COUNT;
	stat = write(cl->sock, &op, sizeof(op));
	assertf(stat == sizeof(op), "Sent %d instead of %lu bytes (sendig optype)",
		stat, sizeof(op));

	stat = write(cl->sock, &n, sizeof(int));
	assertf(stat == sizeof(op), "Sent %d instead of %lu bytes (sending count)",
		stat, sizeof(op));

	cl->emitting = 1;

}

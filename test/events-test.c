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

#include <string.h>

#include <wayland-server.h>
#include <wayland-client.h>

#include "test-runner.h"
#include "wit.h"


/*
#
void
wit_eventarray_add(struct wit_eventarray *ea, struct wit_event *event, ...);

int
wit_eventarray_emit_one(struct wit_display *d, struct wit_eventarray *ea);

int
wit_eventarray_compare(struct wit_eventarray *a, struct wit_eventarray *b);

void
wit_eventarray_free(struct wit_eventarray *ea);
*/

TEST(define_event_tst)
{
	WIT_EVENT_DEFINE(pointer_motion, &wl_pointer_interface, WL_POINTER_MOTION);
	WIT_EVENT_DEFINE(touch_frame, &wl_touch_interface, WL_TOUCH_FRAME);

	/* pointer_motion and touch_frame structures should be accessible now */
	/* otherwise it fails during compilation */
	assert(pointer_motion->interface == &wl_pointer_interface);
	assert(touch_frame->interface == &wl_touch_interface);

	/* methods */
	assertf(strcmp(pointer_motion->interface->events[pointer_motion->opcode].name,
		"motion") == 0, "Wrong method assigend to pointer_motion");
	assertf(strcmp(touch_frame->interface->events[touch_frame->opcode].name,
		"frame") == 0, "Wrong method assigend to touch_frame");


}

const static struct wl_interface intf = {"wl_test_interface", 1, 0, NULL, 3, NULL};

TEST(define_edge_event_opcode_tst)
{
	/* opcode is the lower edge value */
	WIT_EVENT_DEFINE(event, &intf, 2);
	exit(! &event); /* suppress compiler warning */
}

FAIL_TEST(define_illegal_event_1_tst)
{
	/* opcode is the higher edge value */
	WIT_EVENT_DEFINE(event, &intf, 3);
	exit(! &event); /* suppress compiler warning */
}

FAIL_TEST(define_illegal_event_2_tst)
{
	/* too big opcode */
	WIT_EVENT_DEFINE(event, &intf, 4);
	exit(! &event); /* suppress compiler warning */
}

FAIL_TEST(define_illegal_event_3_tst)
{
	/* NULL interface */
	WIT_EVENT_DEFINE(event, NULL, 0);
	exit(! &event); /* suppress compiler warning */
}

TEST(eventarray_init_tst)
{
	int i;

	/* tea = test event array, but you probably know what I was drinking
	 * at the moment :) */
	WIT_EVENTARRAY_DEFINE(tea);
	WIT_EVENTARRAY_DEFINE(teabag);

	for (i = 0; i < MAX_EVENTS; i++) {
		assertf(tea->events[i] == 0, "Field no. %d is not inizialized", i);
		assertf(teabag->events[i] == 0, "Field no. %d is not inizialized"
			" (teabag)", i);
	}

	assertf(tea->count == 0, "Count not initialized");
	assertf(tea->index == 0, "Index not initialized");
}

TEST(eventarray_add_tst)
{
	unsigned count = -1;

	WIT_EVENTARRAY_DEFINE(tea);
	WIT_EVENT_DEFINE(key, &wl_keyboard_interface, WL_KEYBOARD_KEY);

	count = wit_eventarray_add(tea, key, 0, 0, 0, 0);
	assertf(tea->count == 1, "Count not increased");
	assertf(tea->count == count, "Count returned wrong count");
	assertf(tea->index == 0, "Index should have not been increased");
	assertf(tea->events[0] != NULL, "Event not saved");
	assertf(tea->events[1] == NULL, "Wrong memory state");

	count = wit_eventarray_add(tea, key, 1, 1, 1, 1);
	assertf(tea->count == 2, "Count not increased");
	assertf(tea->count == count, "Count returned wrong count");
	assertf(tea->index == 0, "Index should have not been increased");
	assertf(tea->events[1] != NULL, "Event not saved");
	assertf(tea->events[2] == NULL, "Wrong memory state");

	wit_eventarray_free(tea);
}

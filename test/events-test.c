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

static const struct wl_interface intf = {"wl_test_interface", 1, 0, NULL, 3, NULL};

TEST(define_edge_event_opcode_tst)
{
	/* opcode is the lower edge value */
	WIT_EVENT_DEFINE(event, &intf, 2);
	exit(! event->interface); /* suppress compiler warning */
}

FAIL_TEST(define_illegal_event_1_tst)
{
	/* opcode is the higher edge value */
	WIT_EVENT_DEFINE(event, &intf, 3);
	exit(! event->interface); /* suppress compiler warning */
}

FAIL_TEST(define_illegal_event_2_tst)
{
	/* too big opcode */
	WIT_EVENT_DEFINE(event, &intf, 4);
	exit(! event->interface); /* suppress compiler warning */
}

TEST(eventarray_init_tst)
{
	int i;

	/* tea = test event array, but you probably know what I was drinking
	 * at the moment :) */
	struct wit_eventarray *tea = wit_eventarray_create();
	struct wit_eventarray *teabag = wit_eventarray_create();

	for (i = 0; i < MAX_EVENTS; i++) {
		assertf(tea->events[i] == 0, "Field no. %d is not inizialized", i);
		assertf(teabag->events[i] == 0, "Field no. %d is not inizialized"
			" (teabag)", i);
	}

	assertf(tea->count == 0, "Count not initialized");
	assertf(tea->index == 0, "Index not initialized");

	wit_eventarray_free(tea);
	wit_eventarray_free(teabag);
}

FAIL_TEST(eventarray_add_wrong_event_tst)
{
	struct wit_eventarray *tea = wit_eventarray_create();
	wit_eventarray_add(tea, 0, NULL);
	wit_eventarray_free(tea);
}

FAIL_TEST(eventarray_add_wrong_ea_tst)
{
	WIT_EVENT_DEFINE(e, &wl_pointer_interface, WL_POINTER_MOTION);
	wit_eventarray_add(NULL, 0, e);
}

TEST(eventarray_add_tst)
{
	unsigned count = -1;

	struct wit_eventarray *tea = wit_eventarray_create();
	WIT_EVENT_DEFINE(key, &wl_keyboard_interface, WL_KEYBOARD_KEY);

	count = wit_eventarray_add(tea, DISPLAY, key, 0, 0, 0, 0);
	assertf(tea->count == 1, "Count not increased");
	assertf(tea->count == count, "Count returned wrong count");
	assertf(tea->index == 0, "Index should have not been increased");
	assertf(tea->events[0] != NULL, "Event not saved");
	assertf(tea->events[1] == NULL, "Wrong memory state");

	count = wit_eventarray_add(tea, DISPLAY, key, 1, 1, 1, 1);
	assertf(tea->count == 2, "Count not increased");
	assertf(tea->count == count, "Count returned wrong count");
	assertf(tea->index == 0, "Index should have not been increased");
	assertf(tea->events[1] != NULL, "Event not saved");
	assertf(tea->events[2] == NULL, "Wrong memory state");

	wit_eventarray_free(tea);
}


/* just define some events, no matter what events and do it manually, so it can
 * is global */
struct wit_event touch_e = {&wl_touch_interface, WL_TOUCH_FRAME};
struct wit_event pointer_e = {&wl_pointer_interface, WL_POINTER_BUTTON};
struct wit_event keyboard_e = {&wl_keyboard_interface, WL_KEYBOARD_KEY};
struct wit_event seat_e = {&wl_seat_interface, WL_SEAT_NAME};

static int
eventarray_emit_main(int sock)
{
	struct wit_client *c = wit_client_populate(sock);
	struct wl_surface *surface
			= wl_compositor_create_surface(
				(struct wl_compositor *) c->compositor.proxy);
	assert(surface);
	wl_display_roundtrip(c->display);

	wit_client_ask_for_events(c, 0);

	wl_display_roundtrip(c->display);
	assert(strcmp(c->seat.data, "Cool name") == 0);

	wl_surface_destroy(surface);
	wit_client_free(c);
	return EXIT_SUCCESS;
}

TEST(eventarray_emit_tst)
{
	struct wit_eventarray *tea = wit_eventarray_create();

	struct wit_display *d = wit_display_create(NULL);
	wit_display_create_client(d, eventarray_emit_main);
	wit_display_add_events(d, tea);

	wit_display_run(d);

	/* just test if we won't get SIGSEGV because of bad resource */
	wit_eventarray_add(tea, DISPLAY, &touch_e);
	wit_eventarray_add(tea, DISPLAY, &pointer_e, 0, 0, 0, 0);
	wit_eventarray_add(tea, DISPLAY, &keyboard_e, 0, 0, 0, 0);
	wit_eventarray_add(tea, DISPLAY, &seat_e, "Cool name");

	assert(tea->count == 4 && tea->index == 0);

	/* process request for events */
	wit_display_process_request(d);

	assert(tea->count == 4);
	assertf(tea->index == 4, "Index is set wrong (%d)", tea->index);

	/* how many resources we tested?
	 * it's just for me..
	int resources_tst = tea->count + 1; // +1 = compositor doesn't have events
	int resources_no = sizeof(d->resources) / sizeof(struct wl_resource *);

	assertf(resources_tst == resources_no, "Missing tests for new resources");
	*/

	wit_display_destroy(d);
}

TEST(eventarray_compare_tst)
{
	struct wit_eventarray *e1 = wit_eventarray_create();
	struct wit_eventarray *e2 = wit_eventarray_create();
	WIT_EVENT_DEFINE(pointer_motion, &wl_pointer_interface, WL_POINTER_MOTION);
	WIT_EVENT_DEFINE(seat_caps, &wl_seat_interface, WL_SEAT_CAPABILITIES);

	assertf(wit_eventarray_compare(e1, e1) == 0,
		"The same eventarrays are not equal");
	assertf(wit_eventarray_compare(e1, e2) == 0
		&& wit_eventarray_compare(e2, e1) == 0,
		"Empty eventarrays are not equal");

	wit_eventarray_add(e1, DISPLAY, pointer_motion, 1, 2, 3, 4);
	wit_eventarray_add(e2, DISPLAY, pointer_motion, 1, 2, 3, 4);
	assertf(wit_eventarray_compare(e1, e2) == 0);

	wit_eventarray_add(e1, DISPLAY, seat_caps, 4);
	assertf(wit_eventarray_compare(e1, e2) != 0);
	assertf(wit_eventarray_compare(e2, e1) != 0);

	wit_eventarray_add(e2, DISPLAY, seat_caps, 4);
	assertf(wit_eventarray_compare(e2, e1) == 0);
	assertf(wit_eventarray_compare(e1, e2) == 0);

	wit_eventarray_add(e2, DISPLAY, pointer_motion,  0, 0, 0);
	assertf(wit_eventarray_compare(e1, e2) != 0);

	wit_eventarray_add(e1, DISPLAY, pointer_motion,  0, 0, 0);
	assertf(wit_eventarray_compare(e1, e2) == 0);

	assertf(wit_eventarray_compare(e1, e1) == 0);
	assertf(wit_eventarray_compare(e2, e2) == 0);

	wit_eventarray_free(e1);
	wit_eventarray_free(e2);
}

/* test adding arguments which are dynamically allocated:
 * string and array */
TEST(ea_add_dynamic)
{

	WIT_EVENT_DEFINE(seat_name, &wl_seat_interface, WL_SEAT_NAME);
	WIT_EVENT_DEFINE(keyboard_enter, &wl_keyboard_interface, WL_KEYBOARD_ENTER);

	/* try string */
	struct wit_eventarray *ea = wit_eventarray_create();
	wit_eventarray_add(ea, DISPLAY, seat_name, "Cool name");
	/* test-runner will assert on leaked memory */
	wit_eventarray_free(ea);

	/* try array */
	ea = wit_eventarray_create();

	/* I need a proxy for keyboard enter */
	struct wit_config conf = {0,0,0};
	struct wit_display *d = wit_display_create(&conf);

	struct wl_array a;
	wl_array_init(&a);
	wl_array_add(&a, 10);
	strcpy(a.data, "Cool array");

	wit_eventarray_add(ea, DISPLAY, keyboard_enter,
			   0x5e41a1, d->display, &a);

	wit_eventarray_free(ea);

	/* try both */
	ea = wit_eventarray_create();

	wit_eventarray_add(ea, DISPLAY, seat_name, "Cool name");
	wit_eventarray_add(ea, DISPLAY, keyboard_enter, 0x5e41a1, d->display, &a);

	wit_display_destroy(d);
	wl_array_release(&a);
	wit_eventarray_free(ea);
}


/* test events which don't allocate it's own memory */
static int
send_ea_basic_main(int sock)
{
	struct wit_client *c = wit_client_populate(sock);
	struct wit_eventarray *ea = wit_eventarray_create();

	/* try events with arguments: uint32_t, int32_t, wl_fixed_t and
	 * object (it will be send as id */
	WIT_EVENT_DEFINE(touch_motion, &wl_touch_interface, WL_TOUCH_MOTION);

	wit_eventarray_add(ea, CLIENT, touch_motion, 0x0131, -5,
			   wl_fixed_from_int(45), wl_fixed_from_double(2.74));

	wit_client_send_eventarray(c, ea);

	wit_eventarray_free(ea);
	wit_client_free(c);

	return EXIT_SUCCESS;
}

TEST(send_eventarray_basic_events_tst)
{
	struct wit_eventarray *ea = wit_eventarray_create();
	struct wit_display *d = wit_display_create(NULL);

	wit_display_create_client(d, send_ea_basic_main);
	wit_display_run(d);

	wit_display_recieve_eventarray(d);
	assert(d->events);

	WIT_EVENT_DEFINE(touch_motion, &wl_touch_interface, WL_TOUCH_MOTION);

	wit_eventarray_add(ea, DISPLAY, touch_motion, 0x0131, -5,
			   wl_fixed_from_int(45), wl_fixed_from_double(2.74));

	assert(wit_eventarray_compare(d->events, ea) == 0);

	wit_eventarray_free(ea);
	wit_display_destroy(d);
}

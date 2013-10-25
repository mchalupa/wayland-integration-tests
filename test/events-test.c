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

/* the compilation will fail if this macro is wrong */
WIT_EVENT_DEFINE_GLOBAL(anevent, &wl_pointer_interface,
			WL_POINTER_MOTION);
TEST(define_global_event)
{
	/* define the same event locally and compare */
	WIT_EVENT_DEFINE(othevent, &wl_pointer_interface,
			 WL_POINTER_MOTION);
	assertf(anevent->interface == othevent->interface,
		"Interfaces differs");
	assertf(anevent->opcode == othevent->opcode,
		"Opcodes differs");
	assertf(strcmp(anevent->interface->events[anevent->opcode].name,
			othevent->interface->events[othevent->opcode].name) == 0,
		"Events have different methods");
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
 * be global */
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

	wit_eventarray_add(tea, DISPLAY, &touch_e);
	wit_eventarray_add(tea, DISPLAY, &pointer_e, 0, 0, 0, 0);
	wit_eventarray_add(tea, DISPLAY, &keyboard_e, 0, 0, 0, 0);
	wit_eventarray_add(tea, DISPLAY, &seat_e, "Cool name");

	assert(tea->count == 4 && tea->index == 0);

	/* just test if we won't get SIGSEGV because of bad resource */
	wit_display_event_count(d);

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
	wit_eventarray_add(ea, CLIENT, touch_motion, 0x0131, -5,
			   wl_fixed_from_int(45), wl_fixed_from_double(2.74));
	wit_eventarray_add(ea, CLIENT, touch_motion, 0x0131, -5,
			   wl_fixed_from_int(45), wl_fixed_from_double(2.74));

	wit_client_send_eventarray(c, ea);
	wit_client_ask_for_events(c, 3);

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
	wit_eventarray_add(ea, DISPLAY, touch_motion, 0x0131, -5,
			   wl_fixed_from_int(45), wl_fixed_from_double(2.74));
	wit_eventarray_add(ea, DISPLAY, touch_motion, 0x0131, -5,
			   wl_fixed_from_int(45), wl_fixed_from_double(2.74));

	/* try emit commited eventarray, don't catch it. Only test if we won't get
	 * some error. That the eventarray is same we'll test by compare() */
	wit_display_event_count(d);

	assert(wit_eventarray_compare(d->events, ea) == 0);

	wit_eventarray_free(ea);
	wit_display_destroy(d);
}


static void
pointer_handle_button(void *data, struct wl_pointer *pointer, uint32_t serial,
		      uint32_t time, uint32_t button, uint32_t state)
{
	assert(data);
	assert(pointer);
	assert(serial == 0xbee);
	assert(time == 0xdead);
	assert(button == 0);
	assert(state == 1);

	struct wit_client *c = data;
	c->data = (void *) 0xb00;
}

static void
pointer_handle_enter(void *data, struct wl_pointer *pointer, uint32_t serial,
		     struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y)
{
	assert(data);
	assert(pointer);
	assert(serial == 0);
	assert(surface);
	assert(wl_fixed_to_int(x) == 13);
	assert(wl_fixed_to_int(y) == 43);
	assert(wl_proxy_get_user_data(surface) ==
	       (void *) wl_proxy_get_id((struct wl_proxy *) surface));

	struct wit_client *c = data;
	(*((int *) c->data))++;
}

static void
pointer_handle_leave(void *data, struct wl_pointer *pointer,
		     uint32_t serial, struct wl_surface *surface)
{
	assert(data && pointer);
	assert(surface);

	struct wit_client *c = data;
	(*((int *) c->data))++;
}


static const struct wl_pointer_listener pointer_listener = {
	.enter = pointer_handle_enter,
	.leave = pointer_handle_leave,
	.button = pointer_handle_button,
};

static void
keyboard_handle_enter(void *data, struct wl_keyboard *keyboard,
		      uint32_t serial, struct wl_surface *surface,
		      struct wl_array *array)
{
	assert(data && keyboard && surface && array);
	assert(serial == 0);

	assert(strcmp(array->data, "Cool array") == 0);

	struct wit_client *c = data;
	(*((int *) c->data))++;
}

static const struct wl_keyboard_listener keyboard_listener = {
	.enter = keyboard_handle_enter
};

static int
send_one_event_main(int sock)
{
	struct wit_client *c = wit_client_populate(sock);
	wit_client_add_listener(c, "wl_pointer", (void *) &pointer_listener);

	WIT_EVENT_DEFINE(pointer_button, &wl_pointer_interface, WL_POINTER_BUTTON);
	wit_client_trigger_event(c, pointer_button, 0xbee, 0xdead, 0, 1);
	wl_display_dispatch(c->display);

	assert(c->data == (void *) 0xb00);

	wit_client_free(c);
	return EXIT_SUCCESS;
}

TEST(send_one_event_tst)
{
	struct wit_display *d = wit_display_create(NULL);
	wit_display_create_client(d, send_one_event_main);

	wit_display_run(d);
	wit_display_event_emit(d);

	wit_display_destroy(d);
}

static int
send_one_event2_main(int sock)
{
	struct wl_surface *surf;
	struct wit_client *c = wit_client_populate(sock);
	wit_client_add_listener(c, "wl_pointer", (void *) &pointer_listener);
	surf = wl_compositor_create_surface((struct wl_compositor *) c->compositor.proxy);
	wl_display_roundtrip(c->display);
	assert(surf);
	wl_proxy_set_user_data(surf, (void *) wl_proxy_get_id(surf));

	int count = 0;
	c->data = &count;

	WIT_EVENT_DEFINE(pointer_enter, &wl_pointer_interface, WL_POINTER_ENTER);
	wit_client_trigger_event(c, pointer_enter,
				 0, surf,
				 wl_fixed_from_int(13), wl_fixed_from_int(43));
	wl_display_dispatch(c->display);

	assertf(count == 1, "Called only %d callback (instead of 1)", count);

	wl_surface_destroy(surf);
	wit_client_free(c);
	return EXIT_SUCCESS;
}

TEST(send_one_event2_tst)
{
	struct wit_display *d = wit_display_create(NULL);
	wit_display_create_client(d, send_one_event2_main);

	wit_display_run(d);
	wit_display_event_emit(d);

	wit_display_destroy(d);
}

static int
trigger_multiple_event_main(int s)
{
	/* define events */
	WIT_EVENT_DEFINE(pointer_enter, &wl_pointer_interface, WL_POINTER_ENTER);
	WIT_EVENT_DEFINE(pointer_leave, &wl_pointer_interface, WL_POINTER_LEAVE);
	WIT_EVENT_DEFINE(keyboard_enter, &wl_keyboard_interface, WL_KEYBOARD_ENTER);

	/* get and create objects */
	struct wit_client *c = wit_client_populate(s);
	struct wl_array array;
	struct wl_surface *surf;
	surf = wl_compositor_create_surface((struct wl_compositor *) c->compositor.proxy);
	wl_proxy_set_user_data(surf, (void *) wl_proxy_get_id(surf));

	/* counter of callbacks called */
	int count = 0;
	c->data = &count;

	/* add listeners */
	wit_client_add_listener(c, "wl_pointer", (void *) &pointer_listener);
	wit_client_add_listener(c, "wl_keyboard", (void *) &keyboard_listener);

	wl_array_init(&array);
	wl_array_add(&array, 15);
	strcpy(array.data, "Cool array");

	/* trigger emitting events */
	wit_client_trigger_event(c, pointer_enter,
				 0, surf,
				 wl_fixed_from_int(13), wl_fixed_from_int(43));
	wit_client_trigger_event(c, pointer_leave, 0, surf);
	wit_client_trigger_event(c, keyboard_enter, 0, surf, &array);
	wl_display_dispatch(c->display);

	assertf(count == 3, "Called only %d callback (instead of 3)", count);

	wl_array_release(&array);
	wl_surface_destroy(surf);
	wit_client_free(c);
	return EXIT_SUCCESS;
}

TEST(trigger_multiple_event_tst)
{
	struct wit_display *d
		= wit_display_create_and_run(NULL, trigger_multiple_event_main);

	wit_display_event_emit(d); /* enter */
	wit_display_event_emit(d); /* leave */
	wit_display_event_emit(d); /* keyboard enter */

	wit_display_destroy(d);
}

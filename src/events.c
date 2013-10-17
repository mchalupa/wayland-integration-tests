/**
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
#include <wayland-util.h>

#include "events.h"
#include "wit-assert.h"
#include "server.h"
#include "client.h"
#include "wit-global.h"

/* structure for inner use by wit_eventarray */
struct event {
	struct wit_event event;
	union wl_argument args[MAX_ARGS_NO];

	/* if we are sending the event via socket, we need to know
	 * the arguments' size */
	size_t args_size[MAX_ARGS_NO];

	/* we wouldn't need this one, but it's simpler than go through signature
	 * again (when sending) */
	int args_no;
};

/**
 * Serves the client to create evenets and then ask to display to emit them
 * When this function is used on display side, side argument has to be set to
 * DISPLAY
 */
unsigned int
wit_eventarray_add(struct wit_eventarray *ea, enum side side,
		   const struct wit_event *event, ...)
{

	assertf(ea, "wit_eventarray is NULL");
	assert(event);

	/* check if event exist */
	assert(event->interface);
	assertf(event->opcode < (unsigned ) event->interface->event_count,
		"Event opcode is illegal (%d for %s)",
		event->opcode, event->interface->name);

	va_list vl;
	int index = 0;
	int i = 0;
	const char *tmp;
	struct wl_array *array, *tmp_array;
	struct wl_proxy *proxy;
	struct wl_resource *resource;

	const char *signature
			= event->interface->events[event->opcode].signature;
	assert(signature);

	struct event *e = calloc(1, sizeof *e);
	assert(e && "Out of memory");

	/* copy event */
	e->event = *event;

	/* copy arguments */
	va_start(vl, event);
	while(signature[i]) {
		assertf(index < MAX_ARGS_NO , "Too much arguments (wit issue, not wayland)");

		/* index = position in arguments array,
		 *     i = position in signature */
		switch(signature[i]) {
			case 'i':
				e->args[index].i = va_arg(vl, int32_t);
				e->args_size[index] = sizeof(int32_t);
				index++;
				break;
			case 'u':
				e->args[index].u = va_arg(vl, uint32_t);
				e->args_size[index] = sizeof(uint32_t);
				index++;
				break;
			case 'f':
				e->args[index].f = va_arg(vl, wl_fixed_t);
				e->args_size[index] = sizeof(wl_fixed_t);
				index++;
				break;
			case 's':
				tmp = va_arg(vl, const char *);
				assertf(tmp, "No string passed");
				/* this one we'll need to send whole */
				e->args_size[index] = strlen(tmp);

				e->args[index].s = malloc(e->args_size[index] + 1);
				assert(e->args[index].s && "Out of memory");
				strcpy((char *) e->args[index].s, tmp);

				index++;
				break;
			case 'n':
			case 'o':
				/* save only object's id */
				if (side == CLIENT) {
					proxy = va_arg(vl, struct wl_proxy *);
					e->args[index].n = wl_proxy_get_id(proxy);
				} else {
					resource = va_arg(vl, struct wl_resource *);
					e->args[index].u = wl_resource_get_id(resource);
				}

				e->args_size[index] = sizeof(uint32_t);
				index++;
				break;
			case 'a':
				tmp_array = va_arg(vl, struct wl_array *);
				assertf(tmp_array, "No array passed");

				array = malloc(sizeof(struct wl_array));
				assert(array && "Out of memory");
				wl_array_init(array);

				wl_array_copy(array, tmp_array);
				e->args_size[index] = array->alloc;
				e->args[index].a = array;

				index++;
				break;
			case 'h':
				e->args[index++].h = va_arg(vl, int32_t);
				e->args_size[index] = sizeof(int32_t);
				index++;
				break;
			default:
				break;
		}
		i++;
	}
	va_end(vl);

	/* save how many arguments event has */
	e->args_no = index;

	ea->events[ea->count] = e;
	ea->count++;

	return ea->count;
}

/* return pointer to the next type in the signature */
/* (that means skip all non-type parts of signature */
static const char *
get_next_signature(const char *sig)
{
	while (*sig) {
		switch(*sig) {
			case 'i':
			case 'u':
			case 'f':
			case 's':
			case 'n':
			case 'o':
			case 'a':
			case 'h':
				return sig;
			default:
				sig++;
		}
	}

	/* possible terminating zero */
	return sig;
}

static void
convert_ids_to_objects(struct wit_display *d, struct event *e)
{
	int i;
	const char *signature
			= e->event.interface->events[e->event.opcode].signature;

	for(i = 0; i < e->args_no; i++) {
		signature = get_next_signature(signature);
		if (*signature == 'o') {
			e->args[i].o =
				(struct wl_object *) wl_client_get_object(d->client,
									  e->args[i].u);
			assertf(e->args[i].o, "No object like that");
		}

		signature++;
	}
}

static void
convert_objects_to_ids(struct event *e)
{
	int i;
	const char *signature
			= e->event.interface->events[e->event.opcode].signature;

	for(i = 0; i < e->args_no; i++) {
		signature = get_next_signature(signature);
		if (*signature == 'o') {
			e->args[i].o =
				(struct wl_resource *)
				wl_resource_get_id((struct wl_resource *) e->args[i].o);
			assertf(e->args[i].o, "No object like that");
		}

		signature++;
	}
}

int
wit_eventarray_emit_one(struct wit_display *d, struct wit_eventarray *ea)
{
	assertf(ea->index < ea->count,
		"Index (%d) in wit_eventarray is greater than count (%d)",
		ea->index, ea->count);

	struct wl_resource *resource = NULL;
	struct event *e = ea->events[ea->index];

	/* choose right resource */
	if (e->event.interface == &wl_seat_interface)
		resource = d->resources.seat;
	else if (e->event.interface == &wl_pointer_interface)
		resource = d->resources.pointer;
	else if (e->event.interface ==  &wl_keyboard_interface)
		resource = d->resources.keyboard;
	else if (e->event.interface == &wl_touch_interface)
		resource = d->resources.touch;
	else if (e->event.interface == &wl_surface_interface)
		resource = (struct wl_resource *) e->args[0].o;
	else
		assertf(0, "Unsupported interface");

	assertf(resource, "Resource is not present in the display (%s)",
		e->event.interface->name);

	wl_resource_post_event_array(resource, e->event.opcode, e->args);
	ea->index++;

	/* return how many events left */
	return ea->count - ea->index;
}

static const char *
event_name_string(struct wit_event *e)
{
	return e->interface->events[e->opcode].name;
}

static char *
print_bytes(void *src, int n)
{
	assert(n < 250 && "Asking for too much bytes");

	char *str = calloc(1, 1000);
	int i = 0, printed = 0, cur_pos = 0;
	char *pos = str;

	while (i < n) {
		sprintf(pos + cur_pos, "%#x%n ", *((char *) src + n - i - 1), &printed);
		i++;
		cur_pos += printed + 1;

		if (pos - str >= 1000)
			break;
	}

	return str;
}

static int
compare_event_arguments(struct event *e1, struct event *e2, unsigned pos)
{
	int nok = 0;
	int i;
	char *bytes1, *bytes2;

	assert(e1);
	assert(e2);

	for (i = 0; i < MAX_ARGS_NO; i++) {
		if (memcmp(&e1->args[i], &e2->args[i], sizeof e1->args[i]) != 0) {
			bytes1 = print_bytes(&e1->args[i], sizeof e1->args[i]);
			bytes2 = print_bytes(&e2->args[i], sizeof e2->args[i]);
			dbg("Event on position %d (%s): "
				"different argument %d\n "
				"Bytes: %s != %s\n"
				"String: '%*s' != '%*s'\n",
				pos, event_name_string(&e1->event), i,
				bytes1, bytes2,
				(int) sizeof(e1->args[i]), (char *) &e1->args[i],
				(int) sizeof(e2->args[i]), (char *) &e2->args[i]);

			free(bytes1);
			free(bytes2);

			nok = 1;
		}
	}

	return nok;
}

#define MIN(a,b) (((a)<(b))?(a):(b))
// compare two wit_eventarray and give out description if something differs
int
wit_eventarray_compare(struct wit_eventarray *a, struct wit_eventarray *b)
{
	unsigned n;
	int nok = 0;
	int wrong_count = 0;
	struct event *e1, *e2;

	if (a == b)
		return 0;

	assert(a);
	assert(b);

	if (a->count != b->count) {
		dbg("Different number of events in %s wit_eventarray (first %d and second %d)\n",
		    (a->count < b->count) ? "second" : "first", a->count, b->count);

		nok = 1;
		wrong_count = 1;
	}

	for (n = 0; n < MIN(a->count, b->count); n++) {
		e1 = a->events[n];
		e2 = b->events[n];

		if (e1->event.interface != e2->event.interface)
			dbg("Different interfaces on position %d: (%s and %s)\n", n,
			    e1->event.interface->name, e2->event.interface->name);
		if (e1->event.opcode != e2->event.opcode) {
			dbg("Different event opcode on position %d: "
			    "have %d (%s->%s) and %d (%s->%s)\n", n, e1->event.opcode, e1->event.interface->name,
			    event_name_string(&e1->event), e2->event.opcode, e2->event.interface->name,
			    event_name_string(&e2->event));
			nok = 1;
		} else if (compare_event_arguments(e1, e2, n) != 0)
			nok = 1;
	}

	// Print info of extra events
	if (nok && wrong_count) {
		if (a->count < b->count) {
			for(; n < b->count; n++)
				dbg("Extra event on position %d (%s->%s)\n",
				    n, b->events[n]->event.interface->name,
				    event_name_string(&b->events[n]->event));
		} else {
			for(; n < a->count; n++)
				dbg("Extra event on position %d (%s->%s)\n",
				    n, a->events[n]->event.interface->name,
				    event_name_string(&a->events[n]->event));
		}
	}

	return nok;
}

void
wit_eventarray_free(struct wit_eventarray *ea)
{
	unsigned i;

	assert(ea);

	for (i = 0; i < ea->count; i++)
		free(ea->events[i]);

struct wit_eventarray *
wit_eventarray_create()
{
	struct wit_eventarray *ea = calloc(1, sizeof(struct wit_eventarray));
	assert(ea && "Out of memory");

	return ea;
}

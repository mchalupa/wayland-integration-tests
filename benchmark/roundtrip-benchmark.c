/*
 * Copyright © 2013 Marek Chalupa
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
#include <time.h>

#include <wayland-server.h>
#include <wayland-client.h>

#include "wit.h"

struct wl_measure;

/**
 * Request is used to begin measurement with given id.
 * In reaction on request is sent the event, which will carry time information
 */
const struct wl_interface wl_measure_interface;

static const struct wl_interface *types[] = {
	&wl_measure_interface,
};

#define REQUESTS_NO 1
static const struct wl_message measure_requests[] = {
	{ "measure_begin", "uuu",  types + 0},
};

#define EVENTS_NO 1
static const struct wl_message measure_events[] = {
	{ "measure_return", "uuuuu",  types + 0},
};

const struct wl_interface wl_measure_interface = {
	"wl_measure", 1,
	EVENTS_NO, measure_requests,
	REQUESTS_NO, measure_events,
};

struct wl_measure_interface {
	void (*measure_begin) (struct wl_client *client, struct wl_resource *resource,
			       unsigned int id, unsigned int sec, unsigned int nsec);
};

struct wl_measure_listener {
	void (*measure_return) (void *data, struct wl_measure *wl_measure,
			 	unsigned int id, unsigned int sec, unsigned int nsec,
				unsigned int marshal_sec, unsigned int marshal_nsec);
};

#define MEASURE_BEGIN  0
#define MEASURE_RETURN 0

static unsigned long long avg_marshal_sec = 0;
static unsigned long long avg_post_sec = 0;
static unsigned long long avg_all_sec = 0;

static unsigned long long avg_marshal_nsec = 0;
static unsigned long long avg_post_nsec = 0;
static unsigned long long avg_all_nsec = 0;

static void
measure_begin(struct wl_client *client, struct wl_resource *resource,
	      unsigned int id, unsigned int start_sec, unsigned int start_nsec)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);

	/* we will count difference, so don't care we can get cropped from long to int */
	wl_resource_post_event(resource, MEASURE_RETURN, id, t.tv_sec, t.tv_nsec,
			       start_sec, start_nsec);

}

static const struct wl_measure_interface measure_interface = {
	measure_begin
};

/*
 * sec, nsec = time when measure_begin was invoked
 * start_sec, start_nsec = time taken before proxy_start
 */
static void
measure_return(void *data, struct wl_measure *wl_measure,
	       unsigned int id, unsigned int begin_sec, unsigned int begin_nsec,
	       unsigned int start_sec, unsigned int start_nsec)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);

	/* this is time from posting event till invoking this function */
	avg_post_sec += t.tv_sec - begin_sec;
	avg_post_nsec += t.tv_nsec - begin_nsec;

	/* this is time from marshal to invoking this function */
	/* it have to be counted here because of forking processes */
	avg_marshal_sec += begin_sec - start_sec;
	avg_marshal_nsec += begin_nsec - start_nsec;

	avg_all_sec += t.tv_sec - start_sec;
	avg_all_nsec += t.tv_nsec - start_nsec;
}

static const struct wl_measure_listener measure_listener = {
	measure_return
};

static void
registry_handle_global(void *data, struct wl_registry *registry,
		       uint32_t id, const char *interface, uint32_t version)
{
	/* a little hack - pass display in data, but then use this var to
	 * store new proxy */
	struct wl_display *d = *((struct wl_display **) data);
	struct wl_proxy **p = data;
	if (strcmp(interface, "wl_measure") == 0) {
		*p = wl_registry_bind(registry, id,
					 &wl_measure_interface, 0);
		assertf(*p, "Binding to registry failed");

		wl_proxy_add_listener((struct wl_proxy *) *p,
					(void *)&measure_listener, NULL);
		wl_display_roundtrip(d);
	}

}

const struct wl_registry_listener registry_listener = {
	registry_handle_global,
	NULL
};


static int
measure_roundtrip_main(int sock)
{
	int i;
	struct wl_registry *r;
	struct wl_display *d = wl_display_connect(NULL);
	assert(d);

	r = wl_display_get_registry(d);
	assert(r);

	struct wl_proxy *p = d;
	wl_registry_add_listener(r, &registry_listener, &p);
	wl_display_dispatch(d);

	assert(p != NULL);

	/* invoke measuring */
	struct timespec t;
	for (i = 0; i < 1000; i++) {
		clock_gettime(CLOCK_MONOTONIC, &t);
		wl_proxy_marshal(p, MEASURE_BEGIN, i, t.tv_sec, t.tv_nsec);
		wl_display_dispatch(d);
	}

	/* barrier */
	wl_display_roundtrip(d);

	avg_marshal_sec /= 1000;
	avg_marshal_nsec /= 1000;
	avg_post_sec /= 1000;
	avg_post_nsec /= 1000;
	avg_all_sec /= 1000;
	avg_all_nsec /= 1000;

	printf("Average of 1000 measurments:\n"
	       " marshal -> invoke: %llu seconds %llu nanoseconds\n"
	       " post    -> invoke: %llu seconds %llu nanoseconds\n"
	       " total roundtrip  : %llu seconds %llu nanoseconds\n",
	       avg_marshal_sec, avg_marshal_nsec, avg_post_sec, avg_post_nsec,
	       avg_all_sec, avg_all_nsec);

	wl_proxy_destroy(p);
	wl_display_disconnect(d);

	return EXIT_SUCCESS;
}

static void
measure_bind(struct wl_client *c, void *data, uint32_t ver, uint32_t id)
{
	assert(c && data);
	struct wit_display *d = data;

	d->data = wl_resource_create(d->client, &wl_measure_interface, ver, id);
	assertf(d->data, "Failed creating resource for wl_measure");
	wl_resource_set_implementation(d->data, &measure_interface, d, NULL);
}

int
main(void)
{
	int stat;
	struct wit_display *d = wit_display_create(NULL);
	struct wl_global *measure_global = wl_global_create(d->display,
							    &wl_measure_interface,
							    1, d, measure_bind);

	assert(measure_global);
	wit_display_create_client(d, measure_roundtrip_main);

	stat = wit_display_run(d);

	wl_global_destroy(measure_global);
	wit_display_destroy(d);

	return 0;
}

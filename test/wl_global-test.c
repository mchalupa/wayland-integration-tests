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
#include <string.h>
#include <wayland-client.h>
#include <wayland-server.h>

#include "test-runner.h"
#include "wit.h"

static void
registry_handle_global(void *data, struct wl_registry *registry,
			uint32_t id, const char *interface, uint32_t version)
{
	assert(data && registry && interface);
	struct wit_client *cl = data;
	uint32_t *max_id = cl->data;

	if (*max_id < id)
		*max_id = id;
}

const struct wl_registry_listener registry_listener = {
	registry_handle_global,
	NULL
};

static int
global_bind_wrong_id_main(int s)
{
	int stat;
	uint32_t max_id = 0;
	struct wit_client c;

	memset(&c, 0, sizeof c);
	c.data = &max_id;

	c.display = wl_display_connect(NULL);
	assert(c.display);

	c.registry.proxy = (struct wl_proxy *) wl_display_get_registry(c.display);
	assert(c.registry.proxy);

	wl_registry_add_listener((struct wl_registry *) c.registry.proxy,
				 &registry_listener, &c);
	wl_display_dispatch(c.display);

	/* try bind to invalid global */
	struct wl_compositor *comp =
		wl_registry_bind((struct wl_registry *) c.registry.proxy,
				 max_id + 1, &wl_compositor_interface,
				 wl_compositor_interface.version);
	/* should get display error now */
	wl_display_roundtrip(c.display);
	stat = wl_display_get_error(c.display);

	if (comp)
		wl_compositor_destroy(comp);
	wl_registry_destroy((struct wl_registry *) c.registry.proxy);
	wl_display_disconnect(c.display);

	/* if we got error, everything's alright */
	return !stat;
}

/*
 * test binding to non-existing global
 */
TEST(global_bind_wrong_id_tst)
{
	struct wit_display *d
		= wit_display_create_and_run(NULL, global_bind_wrong_id_main);
	wit_display_destroy(d);
}

static int
create_more_same_singletons_main(int s)
{
	struct wit_client *c = wit_client_populate(s);
	wit_client_barrier(c);

	wl_display_roundtrip(c->display);
	wit_client_free(c);
	return EXIT_SUCCESS;
}

TEST(create_more_same_singletons_tst)
{
	struct wl_global *g1, *g2;

	/* create only display */
	struct wit_config conf = {0, 0, 0};
	struct wit_display *d = wit_display_create(&conf);
	wit_display_create_client(d, create_more_same_singletons_main);
	wit_display_run(d);

	g1 = wl_global_create(d->display, &wl_display_interface,
			      wl_display_interface.version, NULL, NULL);
	g2 = wl_global_create(d->display, &wl_display_interface,
			      wl_display_interface.version, NULL, NULL);
	wit_display_barrier(d);

	/* XXX ask about that */
	ifdbg(g1 || g2,
		"Display is stated a singleton but it's possible to create it "
		"more times. Dunno if it's bug..");

	wl_global_destroy(g1);
	wl_global_destroy(g2);
	wit_display_destroy(d);
}

create_wrong_version_global_main(int s)
{
	struct wit_client c = {.sock = s};
	c.display = wl_display_connect(NULL);
	assert(c.display);

	wit_client_barrier(&c);

	wl_display_disconnect(c.display);
	return EXIT_SUCCESS;
}

TEST(create_wrong_version_global_tst)
{
	struct wit_config conf = {0, 0, 0};
	struct wl_global *g1, *g2;
	struct wit_display *d
		= wit_display_create_and_run(&conf,
					     create_wrong_version_global_main);
	g1 = wl_global_create(d->display, &wl_compositor_interface,
			      wl_compositor_interface.version + 1, NULL, NULL);
	assertf(g1 == NULL,
		"Global created even with wrong version");

	wit_display_barrier(d);
	wit_display_destroy(d);
}

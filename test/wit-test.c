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

#include "test-runner.h"
#include "wit.h"

TEST(compositor_create)
{
	/* since I plan adding objects, keep here last default config
	 * and check, if default config have changed since last time.
	 * So that I know that the tests are out of date */
	const struct wit_config wit_old_default_config = {
		CONF_SEAT | CONF_COMPOSITOR,
		CONF_ALL,
		0
	};

	struct wit_display *d = wit_display_create(NULL);

	/* check configuration version */
	assertf(wit_old_default_config.globals == d->config.globals &&
		wit_old_default_config.resources == d->config.resources &&
		wit_old_default_config.options == d->config.options,
		"Config tests are out of date. Default config changed");

	/* Following tests should be covered by asserts in code, but what if I
	 * forget or will change something? */
	assertf(d->display, "Display wasn't created");
	assertf(d->client == NULL, "Client is not NULL before calling client_create");
	assertf(d->sigchld, "Event source (SIGCHLD signal) is NULL");
	assertf(d->sigusr1, "Event source (SIGUSR1 signal) is NULL");
	assertf(d->loop, "Got no event loop");
	assertf(d->client_pid == 0, "Client pid is set even though we "
					"haven't created client yet");
	assertf(d->client_exit_code == 0, "Client exit code differs from 0 "
						"after initizalization");

	assertf(d->data == NULL, "Client non-NULL before setting");
	assertf(d->user_func == NULL, "User func is NULL before setting");

	/* check for default configs */
	assert(d->config.globals == CONF_SEAT | CONF_COMPOSITOR);
	assert(d->config.resources == CONF_ALL);
	assert(d->config.options == ~CONF_ALL);
	/* ~CONF_ALL == 0 */
	assert(~CONF_ALL == 0); /* sanity test */

	wit_display_destroy(d);
	exit(EXIT_SUCCESS);
}

static int
client_main(int s)
{
	assert(s >= 0);
	return 42;
}

TEST(client_create)
{
	struct wit_display *c = wit_display_create(NULL);
	int stat;

	wit_display_create_client(c, client_main);

	assertf(c->client, "Client is NULL");
	assertf(c->client_pid != 0, "Client pid is weird (%d)..", c->client_pid);

	stat = wit_display_run(c);
	assertf(stat == 42, "The value returned in client_main doesn't mach the"
				" value returned by wit_display_run (%d != 42)", stat);

	wit_display_destroy(c);
	exit(EXIT_SUCCESS);
}

TEST(user_data_without_destr)
{
	struct wit_display *d = wit_display_create(NULL);
	/* program shouldn't crash with NULL destructor */
	wit_display_add_user_data(d, (void *) 0xbee, NULL);
	assertf(wit_display_get_user_data(d) == (void *) 0xbee,
		"Got %p instead of 0xbee", wit_display_get_user_data(d));

	wit_display_destroy(d);
}

static _Bool destroy_bee_called = 0;
static void
destroy_bee(void *data)
{
	destroy_bee_called = 1;
	assertf(data == (void *) 0xbee,
		"Passed wrong data in data's destructor");
}

TEST(user_data_with_destr)
{
	struct wit_display *d = wit_display_create(NULL);
	wit_display_add_user_data(d, (void *) 0xbee, destroy_bee);
	wit_display_destroy(d);

	assertf(destroy_bee_called, "Destructor wasn't called");
}

static _Bool user_func_called = 0;
static void
user_func(void *data)
{
	user_func_called = 1;
	assertf(data = (void *) 0xdeadbee,
		"data should be 0xdeadbee but is %p", data);
}

static int
user_func_main(int sock)
{
	struct wit_client c;
	c.sock = sock;

	wit_client_call_user_func(&c);

	return EXIT_SUCCESS;
}

TEST(user_func_tst)
{
	struct wit_display *d = wit_display_create(NULL);
	int stat;

	wit_display_create_client(d, user_func_main);
	wit_display_add_user_func(d, user_func, (void *) 0xdeadbee);

	stat = wit_display_run(d);
	assertf(user_func_called, "User function wasn't called");

	wit_display_destroy(d);
	exit(stat);

}

TEST(config_tst)
{
	struct wit_config conf = {
		.globals = CONF_SEAT,
		.resources = CONF_ALL
	};

	struct wit_display *d = wit_display_create(&conf);

	assert(d->config.globals == CONF_SEAT);
	assert(d->config.resources == CONF_ALL);
	assert(d->config.options == ~CONF_ALL);

	wit_display_destroy(d);
}

static int
client_populate_main(int sock)
{
	struct wit_client *c = wit_client_populate(sock);
	assert(c);
	assert(c->display);
	assert(c->registry);

	/* we have default settings, check it */
	assert(c->compositor);
	assert(c->seat);
	assert(c->pointer);
	assert(c->keyboard);
	assert(c->touch);

	wit_client_free(c);

	return EXIT_SUCCESS;
}

TEST(client_populate_tst)
{
	struct wit_display *d = wit_display_create(NULL);
	wit_display_create_client(d, client_populate_main);

	int stat = wit_display_run(d);

	/* we have default settings */
	assert(d->globals.compositor);
	assert(d->globals.seat);
	assert(d->globals.global == NULL);
	assert(d->resources.compositor);
	assert(d->resources.seat);
	assert(d->resources.pointer);
	assert(d->resources.keyboard);
	assert(d->resources.touch);
	wit_display_destroy(d);

	exit(stat);
}

static const struct wl_pointer_listener *dummy_pointer_listener = (void *) 0xBED;
static const struct wl_keyboard_listener *dummy_keyboard_listener = (void *) 0xB00;
static const struct wl_touch_listener *dummy_touch_listener = (void *) 0xBEAF;

static int
add_listener_main(int sock)
{
	int listeners_tested = 5;
	int listeners_no;

	struct wit_client *c = wit_client_populate(sock);

	assertf(c->listener.registry != NULL,
		"In populate should have been default registry listener assigned");
	assertf(c->listener.seat == NULL,
		"We didn't created seat so the default seat listener shouldn't be assigned");


	wit_client_add_listener(c, "wl_pointer",
				(void *) dummy_pointer_listener);
	assertf(c->listener.pointer == dummy_pointer_listener,
		"Failed adding pointer listener");

	wit_client_add_listener(c, "wl_keyboard",
				(void *) dummy_keyboard_listener);
	assertf(c->listener.keyboard == dummy_keyboard_listener,
		"Failed adding keyboard listener");

	wit_client_add_listener(c, "wl_touch",
				(void *) dummy_touch_listener);
	assertf(c->listener.touch == dummy_touch_listener,
		"Failed adding touch listener");

	/* Do we have tested all? */
	listeners_no = sizeof(c->listener) / sizeof(struct wl_listener *);
	assertf(listeners_tested == listeners_no,
		"Missing tests for assigning listeners");

	wit_client_free(c);
	return EXIT_SUCCESS;
}

TEST(add_listener_tst)
{
	/* don't create resources, otherwise we'll get SIGSEGV */
	struct wit_config conf = {0, 0, 0};
	struct wit_display *d = wit_display_create(&conf);
	wit_display_create_client(d, add_listener_main);

	assert(wit_display_run(d) == EXIT_SUCCESS);

	wit_display_destroy(d);
}

FAIL_TEST(add_unknown_interface_listener_tst)
{
	struct wit_client c = {0};

	/* should abort from inside the function */
	wit_client_add_listener(&c, "wl_unknown_interface_!@#$",
				(void *) dummy_pointer_listener);

	/* only print, assert would pass the test, because this test
	 * is expected to fail */
	fprintf(stderr, "We should have been aborted by now...");

	exit(EXIT_SUCCESS);
}

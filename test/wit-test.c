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

#include <assert.h>
#include <wayland-client.h>

#include "test-runner.h"
#include "wit.h"

TEST(compositor_create)
{
	struct wit_display *c = wit_display_create(NULL);

	/* Following tests should be covered by asserts in code, but what if I
	 * forget or will change something? */
	assertf(c->display, "Display wasn't created");
	assertf(c->client == NULL, "Client is not NULL before calling client_create");
	assertf(c->sigchld, "Event source (SIGCHLD signal) is NULL");
	assertf(c->sigusr1, "Event source (SIGUSR1 signal) is NULL");
	assertf(c->loop, "Got no event loop");
	assertf(c->client_pid == 0, "Client pid is set even though we "
					"haven't created client yet");
	assertf(c->client_exit_code == 0, "Client exit code differs from 0 "
						"after initizalization");

	assertf(c->data == NULL, "Client non-NULL before setting");
	assertf(c->user_func == NULL, "User func is NULL before setting");

	/* check for default configs */
	assert(c->config.globals == CONF_SEAT);
	assert(c->config.resources == CONF_ALL);
	assert(c->config.options == ~CONF_ALL);
	/* ~CONF_ALL == 0 */
	assert(~CONF_ALL == 0); /* sanity test */

	wit_display_destroy(c);
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
	assert(d->globals.seat);
	assert(d->globals.global == NULL);
	assert(d->resources.seat);
	assert(d->resources.pointer);
	assert(d->resources.keyboard);
	assert(d->resources.touch);

	wit_display_destroy(d);

	exit(stat);
}

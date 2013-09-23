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
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
//#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>

#include <wayland-server.h>

#include "wit-assert.h"
#include "wit-global.h"
#include "server.h"

/*
 * Terminate display when client exited
 */
static int
handle_sigchld(int signum, void *data)
{
	assertf(signum == SIGCHLD,
		"Got other signal than SIGCHLD from loop\n");
	assertf(data, "Got SIGCHLD with NULL data\n");

	int status, stat;
	struct wit_display *disp = data;

	wl_display_terminate(disp->display);
	dbg("Display terminated\n--\n");

	stat = waitpid(disp->client_pid, &status, WNOHANG);
	assertf(stat != -1, "Waiting for child failed");

	disp->client_exit_code = WEXITSTATUS(status);
	return 0;
}

/* When display gets this signal it reads from socket what client want */
static int
handle_sigusr1(int signum, void *data)
{
	int stat;
	enum optype op;
	struct wit_display *disp = data;

	assertf(signum == SIGUSR1,
		"Expected signal %d (SIGUSR1) but got %d", SIGUSR1, signum);
	assertf(data, "Got SIGUSR1 with NULL data\n");

	stat = read(disp->client_sock[1], &op, sizeof(op));
	assertf(stat == sizeof(op),
		"Reading pipe error, read %d byte(s) instead of %lu",
		stat, sizeof(op));

	switch(op) {
		case CAN_CONTINUE:
			assertf(0, "Got CAN_CONTINUE from child");
		case RUN_FUNC:
			dbg("Running user's function\n");
			disp->user_func(disp->user_func_data);
			break;
		default:
			assertf(0, "Unknown operation");
	}

	return 0;
}


/* Send messages to client */
static void
send_client(struct wit_display *disp, enum optype op, ...)
{
	int stat;
	va_list vl;
	_Bool cont;

	/* enum optype is defined from 1 */
	assertf(op > 0, "Wrong operation");
	assertf(disp, "No compositor passed");

	int fd = disp->client_sock[1];

	va_start(vl, op);

	switch (op) {
		case CAN_CONTINUE:
			cont = va_arg(vl, int);
			stat = write(fd, &op, sizeof(op));
			stat += write(fd, &cont, sizeof(int));
			assertf(stat == (sizeof(op) + sizeof(int)),
				"Failed signal client. Supposed to "
				"send %lu byte(s), but sent %d byte(s)",
				sizeof(op) + sizeof(int), stat);
			break;
		default:
			assertf(0, "Unknown operation");
	}

	va_end(vl);
}


/* Since tests can run parallely, we need unique socket names
 * for each test. Otherwise test can fail on wl_display_add_socket.
 * Also test would fail on this function when some other test failed and socket
 * would remain undeleted.
 * Not re-entrant nor thread-safe */
static char *
get_socket_name(void)
{
	struct timeval tv;
	static char retval[108];

	gettimeofday(&tv, NULL);
	snprintf(retval, sizeof retval, "wayland-test-%ld%ld",
		 tv.tv_sec, tv.tv_usec);

	return retval;
}

struct wit_display *
wit_display_create(void)
{
	struct wit_display *d = NULL;
	const char *socket_name;
	int stat = 0;

	d = calloc(1, sizeof *d);
	assert(d && "Out of memory");

	d->display = wl_display_create();
	assertf(d->display, "Creating display failed [display: %p]", d->display);

	/* hope path won't be longer than 108 .. */
	socket_name = get_socket_name();
	stat = wl_display_add_socket(d->display, socket_name);
	assertf(stat == 0,
		"Failed to add socket '%s' to display. "
		"If everything seems ok, check if path of socket is"
		" shorter than 108 chars or if socket already exists.",
		socket_name);
	dbg("Added socket: %s\n", socket_name);

	d->loop = wl_display_get_event_loop(d->display);
	assertf(d->loop, "Failed to get loop from display");

	d->sigchld = wl_event_loop_add_signal(d->loop, SIGCHLD,
						handle_sigchld, d);
	assertf(d->sigchld,
		"Couldn't add SIGCHLD signal handler to loop");

	d->sigusr1 = wl_event_loop_add_signal(d->loop, SIGUSR1,
						handle_sigusr1, d);
	assertf(d->sigusr1,
		"Couldn't add SIGUSR1 signal handler to loop");

	stat = socketpair(AF_UNIX, SOCK_STREAM, 0, d->client_sock);
	assertf(stat == 0,
		"Cannot create socket for comunication "
		"between client and server");

	return d;
}

void
wit_display_destroy(struct wit_display *d)
{
	assert(d && "Invalid pointer given to destroy_compositor");

	if (d->data && d->data_destroy_func)
		d->data_destroy_func(d->data);

	close(d->client_sock[1]);

	wl_event_source_remove(d->sigchld);
	wl_event_source_remove(d->sigusr1);

	wl_display_destroy(d->display);

	free(d);
}

int
wit_display_run(struct wit_display *d)
{
	assert(d && "Wrong pointer");
	send_client(d, CAN_CONTINUE, 1);

	wl_display_run(d->display);

	return d->client_exit_code;
}


static int
run_client(int (*client_main)(int), int wayland_sock, int client_sock)
{
	int stat;
	char s[32];
	enum optype op;
	_Bool can_continue = 0;

	stat = read(client_sock, &op, sizeof(op));
	stat += read(client_sock, &can_continue, sizeof(int));
	assertf(stat == (sizeof(op) + sizeof(int)),
		"Reading pipe error, read %d byte(s) instead of %lu",
		stat, sizeof(op) + sizeof(int));

	assertf(op == CAN_CONTINUE,
		"Got request for another operation (%u) than CAN_CONTINUE (%d)",
		op, CAN_CONTINUE);

	if (can_continue == 0)
		return EXIT_FAILURE;

	/* for wl_display_connect() */
	snprintf(s, sizeof s, "%d", wayland_sock);
	setenv("WAYLAND_SOCKET", s, 0);

	return client_main(client_sock);
}

static inline void
handle_child_abort(int signum)
{
	exit(SIGABRT);
}

void
wit_display_create_client(struct wit_display *disp,
			  int (*client_main)(int))
{
	int sockv[2];
	pid_t pid;
	int stat;

	stat = socketpair(AF_UNIX, SOCK_STREAM, 0, sockv);
	assertf(stat == 0,
		"Failed to create socket pair");

	pid = fork();
	assertf(pid != -1, "Fork failed");

	if (pid == 0) {
		close(sockv[1]);
		close(disp->client_sock[1]);

		/* abort() itself doesn't imply failing test when it's forked,
		 * we need call exit after abort() */
		signal(SIGABRT, handle_child_abort);
		stat = run_client(client_main, sockv[0],
				  disp->client_sock[0]);
		exit(stat);
	} else {
		close(sockv[0]);
		close(disp->client_sock[0]);

		disp->client_pid = pid;

		disp->client = wl_client_create(disp->display, sockv[1]);
		if (!disp->client) {
			send_client(disp, CAN_CONTINUE, 0);
			assertf(disp->client, "Couldn't create wayland client");
		}
	}
}

/**
 * Set user's data and it's destructor
 * (destructor will be called in wit_display_destroy)
 */
void
wit_display_add_user_data(struct wit_display *disp, void *data,
			  void (*destroy_func)(void *))
{
	assert(disp);

	if (disp->data)
		dbg("Overwriting user data\n");

	disp->data = data;
	disp->data_destroy_func = destroy_func;
}

inline void *
wit_display_get_user_data(struct wit_display *disp)
{
	return disp->data;
}

inline void
wit_display_add_user_func(struct wit_display *disp,
			  void (*func) (void *), void *data)
{
	disp->user_func = func;
	disp->user_func_data = data;
}

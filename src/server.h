#ifndef __WIT_SERVER_H__
#define __WIT_SERVER_H__

#include <unistd.h>

#include "configuration.h"
#include "events.h"

/* ===
 *  Compositor
   === */
struct wit_display {
	struct wl_display *display;
	struct wl_client *client;

	struct wl_event_loop *loop;

	struct {
		struct wl_global *wl_seat;
		struct wl_global *wl_compositor;
		struct wl_global *global; /* one for user's arbitrary use */
	} globals;

	struct {
		struct wl_resource *compositor;
		struct wl_resource *seat;
		struct wl_resource *pointer;
		struct wl_resource *keyboard;
		struct wl_resource *touch;
		struct wl_resource *surface;
	} resources;

	int client_sock[2];
	struct wl_event_source *sigchld;
	struct wl_event_source *sigusr1;

	int client_exit_code;
	pid_t client_pid;

	/* user data */
	void *data;
	void (*data_destroy_func)(void *);

	/* user defined func */
	void (*user_func)(void *);
	void *user_func_data;

	struct wit_eventarray *events;

	struct wit_config config;

	/* sigusr1 sets this when action from display is required */
	int request;
};

int
wit_compositor(int (*client_main)(int));

struct wit_display *
wit_display_create(struct wit_config *);

void
wit_display_run(struct wit_display *d);

void
wit_display_destroy(struct wit_display *d);

void
wit_display_create_client(struct wit_display *disp,
			  int (*client_main)(int));
void
wit_display_add_user_data(struct wit_display *d, void *data,
			  void (*destroy_func)(void *));

void *
wit_display_get_user_data(struct wit_display *d);

void
wit_display_add_user_func(struct wit_display *d,
			  void (*func) (void *), void *data);

void
wit_display_add_events(struct wit_display *d, struct wit_eventarray *e);

/**
 * Process request from client
 */
void
wit_display_process_request(struct wit_display *d);
#endif /* __WIT_SERVER_H__ */

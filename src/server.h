#ifndef __WIT_SERVER_H__
#define __WIT_SERVER_H__

#include <unistd.h>

#include "configuration.h"
#include "events.h"

struct wit_surface {
	struct wl_list link;

	struct wl_resource *resource;
	uint32_t id;
};

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
		struct wl_global *wl_shm;
		struct wl_global *global; /* one for user's arbitrary use */
	} globals;

	struct {
		struct wl_resource *compositor;
		struct wl_resource *seat;
		struct wl_resource *pointer;
		struct wl_resource *keyboard;
		struct wl_resource *touch;
		struct wl_resource *shm;
		struct wl_resource *surface; /* last resource created */
	} resources;

	/* list of wit_surfaces */
	struct wl_list surfaces;

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

struct wit_display *
wit_display_create_and_run(struct wit_config *,
			   int (*client_main)(int));

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

/* create aliases of opcodes for better readability */
#define wit_display_event_count		wit_display_process_request
#define wit_display_event_emit		wit_display_process_request
#define wit_display_user_func		wit_display_process_request
#define wit_display_send_bytes		wit_display_process_request
#define wit_display_barrier		wit_display_process_request

/**
 * Recive eventarray from client. It will be saved into d->events
 */
void
wit_display_recieve_eventarray(struct wit_display *d);
#endif /* __WIT_SERVER_H__ */

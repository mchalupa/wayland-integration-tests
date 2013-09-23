#ifndef __WIT_SERVER_H__
#define __WIT_SERVER_H__

#include <unistd.h>

#include "configuration.h"


/* Following macro goes through anonymous structures globals of structure wit_display */
#define for_each_global(ptr, l)								\
		struct wl_global **__cur_g; size_t __i_g; struct wl_global *(l);	\
		for(__i_g = 0, __cur_g = (struct wl_global **) &(ptr), l = *(__cur_g);	\
			__i_g < ((sizeof (ptr)) / (sizeof (struct wl_global *)));	\
			(__cur_g)++, __i_g++, l = *(__cur_g))

/* ===
 *  Compositor
   === */
struct wit_display {
	struct wl_display *display;
	struct wl_client *client;

	struct wl_event_loop *loop;

	struct {
		struct wl_global *seat;
		struct wl_global *global; /* one for user's arbitrary use */
	} globals;

	struct {
		struct wl_resource *seat;
		struct wl_resource *pointer;
		struct wl_resource *keyboard;
		struct wl_resource *touch;
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

	struct wit_config config;
};

int
wit_compositor(int (*client_main)(int));

struct wit_display *
wit_display_create(struct wit_config *);

int
wit_display_run(struct wit_display *);

void
wit_display_destroy(struct wit_display *);

void
wit_display_create_client(struct wit_display *disp,
			  int (*client_main)(int));
void
wit_display_add_user_data(struct wit_display *c, void *data,
			  void (*destroy_func)(void *));

void *
wit_display_get_user_data(struct wit_display *c);

void
wit_display_add_user_func(struct wit_display *c,
			  void (*func) (void *), void *data);

#endif /* __WIT_SERVER_H__ */

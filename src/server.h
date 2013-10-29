#ifndef __WIT_SERVER_H__
#define __WIT_SERVER_H__

#include <unistd.h>

#include "configuration.h"
#include "events.h"

/* container for wl_surface (it is stored in wl_list)*/
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

/**
 * Create display
 *
 * Create display and bind it to the socket. Use conf to
 * suppres/allow creating of objects. When conf is NULL
 * then default configuration is used (defined in server.c).
 *
 * Usual usage is:
 *
 * d = wit_display_create(NULL);
 * ... do sth (this is adventage against wit_display_create_and_run) ...
 * wit_display_add_client(d, client_main)
 * wit_display_run(d)
 * ...
 * wit_display_destory(d)
 *
 * @param conf    configuration, NULL means default
 * @return        filled struct wit_display
 */
struct wit_display *
wit_display_create(struct wit_config *conf);

/**
 * Create display with client and run display
 *
 * This is the sequence of following operations:
 * wit_display_create()
 * wit_display_add_client()
 * wit_display_run()
 *
 * @param conf         configuration
 * @param client_main  client's main function
 * @return             display
 */
struct wit_display *
wit_display_create_and_run(struct wit_config *conf,
			   int (*client_main)(int));

/**
 * Signalize client that it can continue and run wl_display
 *
 * Client waits until display finish initialization. This function
 * let client know that it can continue and call wl_display_run
 * to make wl_display alive.
 *
 * Usual usage: @see wit_display_create()
 *
 * @param d    display
 */
void
wit_display_run(struct wit_display *d);

/**
 * Free allocated memory and destroy wayland objects
 *
 * Moreover this function checks for errors and client's exit code
 * and therefore it should be called at the end of each display's life
 * to check for client's exit status.
 *
 * @param d    display
 * @return     nothig (aborts on error or on client returning non-0)
 */
void
wit_display_destroy(struct wit_display *d);

/**
 * Create subprocess and run client's main function in it
 *
 * Subprocess waits with running client's main function until
 * display gives signal using wit_display_run().
 * Client's main function has prototype:
 *   int main(int socket);
 * Into socket parameter will be passed socket for client<->display
 * communication (note that it's not the wayland socket). Via this socket are
 * requests (for wit_display, not wl_display) sent and acknowledgement of each
 * request sent back.
 *
 * @param disp    display struct
 * @param client_main  client's main function
 */
void
wit_display_create_client(struct wit_display *disp,
			  int (*client_main)(int));

/**
 * Add user data into wit_display struct
 *
 * Data are stored in wit_display.data element. This function does (along with
 * some checks):
 * d->data = data;
 * d->data_destroy_func = destroy_func;
 *
 * @param d            display's struct
 * @param data         data to be stored
 * @param destroy_func function used for freeing data (can be NULL)
 */
void
wit_display_add_user_data(struct wit_display *d, void *data,
			  void (*destroy_func)(void *));

/**
 * Get user data
 *
 * @param d    display's struct
 * @return     user data (wit_display.data)
 */
void *
wit_display_get_user_data(struct wit_display *d);

/**
 * Register user function
 *
 * This function is called when client invokes wit_client_call_user_func()
 * (display must call wit_display_run_user_func() accordingly)
 *
 * @param d      display
 * @param func   pointer to the function
 * @param data   data to be passed to the function
 */
void
wit_display_add_user_func(struct wit_display *d,
			  void (*func) (void *), void *data);

/**
 * Assign eventarray to be used for emitting events
 *
 * Events contained in this eventarray will be emitted after client
 * calls wit_client_ask_for_events() and display answers by calling
 * wit_display_emit_events() (on the same position in code of course)
 *
 * @param d    display's struct
 * @param e    eventarray
 */
void
wit_display_add_events(struct wit_display *d, struct wit_eventarray *e);

/**
 * Process request from client
 *
 * Any request from client can be (is) processed by this function.
 * (Except for recieve eventarray). However, there are defined aliases
 * for each request (wit_display_{emit_events, emit_event, run_user_func} etc.
 *
 * This function must be called as an answer for each request from client.
 * It serves as a barrier, because each request is acknowledged when it's done.
 * Request is invoked and served as follows:
 *
 *        == DISPLAY ==                       == CLIENT ==
 *  display calls run() (blocks)         |
 *       ... [blocking] ...              |
 *  SIGUSR1 (terminate display)       <--|-  kick_display()
 *          => stop blocking =>          |
 *             process code until        |
 *             process_request()         |
 *  get request                       <--|-  send request
 *  process request                      |
 *  acknowledge request                --|-> wait for acknowledgement
 *  run() (blocks)                       |
 *           [blocking]                  |
 *
 *
 * @param d    display's struct
 */
void
wit_display_process_request(struct wit_display *d);

/* create aliases of opcodes for better readability */
#define wit_display_emit_events		wit_display_process_request
#define wit_display_emit_event		wit_display_process_request
#define wit_display_run_user_func	wit_display_process_request
#define wit_display_recieve_data	wit_display_process_request
#define wit_display_barrier		wit_display_process_request

/**
 * Recive eventarray from client.
 *
 * It is answer for wit_client_send_eventarray and
 * recieved eventarray will be saved into d->events.
 *
 * @param d    display's struct
 */
void
wit_display_recieve_eventarray(struct wit_display *d);

#endif /* __WIT_SERVER_H__ */

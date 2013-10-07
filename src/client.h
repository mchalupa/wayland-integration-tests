#ifndef __WIT_CLIENT_H__
#define __WIT_CLIENT_H__

/*
 * Allow saving object along with some helper data and object's listener
 */
struct wit_client_object {
	struct wl_proxy *proxy;
	const struct wl_listener *listener;
	void *data;
	void (*data_destr)(void *); /* data destructor */
};

/**
 * Data (which can be) used by client.
 */
struct wit_client {
	struct wl_display *display;

	struct wit_client_object registry;
	struct wit_client_object compositor;
	struct wit_client_object seat;
	struct wit_client_object pointer;
	struct wit_client_object keyboard;
	struct wit_client_object touch;
	struct wit_client_object shm;

	int sock;

	/* here we can store events if we need (no need to pass more arguments
	 * or create global variables */
	struct wit_eventarray *events;

	/* set value 1 here, when client asked for emitting events */
	int emitting;

	/* data for user's arbitrary use */
	void *data;
};

struct wit_client *
wit_client_populate(int sock);

void
wit_client_free(struct wit_client *c);

void
wit_client_call_user_func(struct wit_client *cl);

int
wit_client_ask_for_events(struct wit_client *cl, int n);

void
wit_client_add_listener(struct wit_client *cl, const char *interface,
			const void *listener);
void
wit_client_state(struct wit_client *cl);

void
wit_client_barrier(struct wit_client *cl);
#endif /* __WIT_CLIENT_H__ */

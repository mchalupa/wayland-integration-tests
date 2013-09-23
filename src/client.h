#ifndef __WIT_CLIENT_H__
#define __WIT_CLIENT_H__


/**
 * Data (which can be) used by client.
 */
struct wit_client {
	struct wl_display *display;
	struct wl_registry *registry;

	struct wl_seat *seat;
	struct wl_pointer *pointer;
	struct wl_keyboard *keyboard;
	struct wl_touch *touch;

	int sock;

	struct {
		struct wl_pointer_listener *pointer;
		struct wl_keyboard_listener *keyboard;
		struct wl_touch_listener *touch;
	} listener;
};

/*
void
wit_client_add_listener(struct wit_client *cl, const char *interface,
			void *listener);

struct wit_client *
wit_client_populate(int sock);

void
wit_client_free(struct wit_client *c);
*/

void
wit_client_call_user_func(struct wit_client *cl);
#endif /* __WIT_CLIENT_H__ */

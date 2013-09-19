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

#endif /* __WIT_CLIENT_H__ */

lib_LTLIBRARIES = libwit-server.la libwit-client.la

libwit_server_la_LIBADD = $(WAYLAND_SERVER_LIBS)
libwit_server_la_SOURCES =	\
	wit-server-protocol.c 	\
	server.c

libwit_client_la_LIBADD = $(WAYLAND_CLIENT_LIBS)
libwit_client_la_SOURCES =	\
	wit-client-protocol.c	\
	client.c

AM_CFLAGS = $(GCC_CFLAGS) $(WAYLAND_SERVER_CFLAGS) $(WAYLAND_CLIENT_CFLAGS)

lib_LIBRARIES = libwit-server.a libwit-client.a
noinst_LTLIBRARIES = libwit-global.la


libwit_server_a_LIBADD = libwit-global.la
libwit_server_a_SOURCES =	\
	wit-server-protocol.c 	\
	server.c		\
	events.c

libwit_client_a_LIBADD = libwit-global.la
libwit_client_a_SOURCES =	\
	wit-client-protocol.c	\
	client.c

libwit_global_la_SOURCES =	\
	wit-global.c
AM_CFLAGS = $(WAYLAND_SERVER_CFLAGS) $(WAYLAND_CLIENT_CFLAGS)

debug:
	@echo -e "client_la_LIBADD = ${libwit_client_la_LIBADD}\n\
	sever_la_LIBADD=${libwit_server_la_LIBADD}\n\
	AM_CFLAGS=${AM_CFLAGS}\n"

lib_LIBRARIES = libwit-server.a libwit-client.a

libwit_server_a_SOURCES =	\
	wit-server-protocol.c 	\
	server.c

libwit_client_a_SOURCES =	\
	wit-client-protocol.c	\
	client.c

AM_CFLAGS = $(COMPILER_CFLAGS) $(WAYLAND_SERVER_CFLAGS) $(WAYLAND_CLIENT_CFLAGS)

debug:
	@echo -e "client_la_LIBADD = ${libwit_client_la_LIBADD}\n\
	sever_la_LIBADD=${libwit_server_la_LIBADD}\n\
	AM_CFLAGS=${AM_CFLAGS}\n"

TESTS =						\
	wit-test

check_PROGRAMS =				\
	$(TESTS)

test_runner_src = test-runner.c test-runner.h test-helpers.c

wit_test_SOURCES = wit-test.c $(test_runner_src)

AM_CPPFLAGS = -I$(top_builddir)/src -I$(top_srcdir)/src -I$(top_builddir)/
AM_CFLAGS = $(COMPILER_CFLAGS) $(TESTS_CFLAGS)# -ldl

LDADD = $(top_builddir)/src/libwit-server.a \
	$(top_builddir)/src/libwit-client.a \
	$(TESTS_LIBS) -ldl


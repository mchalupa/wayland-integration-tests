SUBDIRS = test-runner

TESTS =						\
	wit-test

check_PROGRAMS =				\
	$(TESTS)

test_runner_dir = $(top_builddir)/test/test-runner

wit_test_SOURCES = wit-test.c

AM_CPPFLAGS = -I$(top_builddir)/src -I$(top_srcdir)/src -I$(top_builddir)/ -I$(test_runner_dir)/
AM_CFLAGS = $(COMPILER_CFLAGS) $(TESTS_CFLAGS)# -ldl

LDADD = $(top_builddir)/src/libwit-server.a \
	$(top_builddir)/src/libwit-client.a \
	$(test_runner_dir)/lib-test-runner.la
	$(TESTS_LIBS) -ldl

debug:
	@echo -e "LDADD = ${LDADD}\nAM_CPPFLAGS=${AM_CPPFLAGS}\nAM_CFLAGS=${AM_CFLAGS}\n"

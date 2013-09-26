lib_LTLIBRARIES = lib-test-runner.la lib-test-helpers.la

lib_test_runner_la_LIBADD = lib-test-helpers.la $(TESTS_LIBS) -ldl
lib_test_runner_la_SOURCES =	\
	test-runner.c

lib_test_helpers_la_SOURCES =	\
	test-helpers.c

AM_CPPFLAGS = -I$(top_builddir)/src -I$(top_srcdir)/src -I$(top_builddir)/
AM_CFLAGS = $(COMPILER_CFLAGS) $(TESTS_CFLAGS)

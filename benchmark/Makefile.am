lib_LTLIBRARIES = lib-benchmark-runner.la
benchmarkdir = $(top_builddir)/benchmark

lib_benchmark_runner_la_SOURCES =	\
	benchmark-runner.c

benchmark_PROGRAMS = \
	roundtrip-benchmark		\
	wayland-benchmarks

roundtrip_benchmark_SOURCES = roundtrip-benchmark.c
wayland_benchmarks_SOURCES = wayland-benchmarks.c

AM_CPPFLAGS = -I$(top_builddir)/src -I$(top_srcdir)/src -I$(top_builddir)/
AM_CFLAGS = $(COMPILER_CFLAGS) $(TESTS_CFLAGS)

LDADD = lib-benchmark-runner.la	\
	$(top_builddir)/src/libwit-server.a \
	$(top_builddir)/src/libwit-client.a \
	$(TESTS_LIBS)

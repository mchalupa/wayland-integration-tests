/*
 * Copyright © 2012 Intel Corporation
 * Copyright © 2013 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <err.h>
#include <dlfcn.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>

#include "config.h"

/* support for printing backtrace upon SIGSEGV */
#include <signal.h>
#ifdef HAVE_LIBUNWIND
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#endif

#include "test-runner.h"
#include "wit-assert.h"

static int num_alloc;
static void* (*sys_malloc)(size_t);
static void (*sys_free)(void*);
static void* (*sys_realloc)(void*, size_t);
static void* (*sys_calloc)(size_t, size_t);

int leak_check_enabled;

extern const struct test __start_test_section, __stop_test_section;

__attribute__ ((visibility("default"))) void *
malloc(size_t size)
{
	num_alloc++;
	return sys_malloc(size);
}

__attribute__ ((visibility("default"))) void
free(void* mem)
{
	if (mem != NULL)
		num_alloc--;
	sys_free(mem);
}

__attribute__ ((visibility("default"))) void *
realloc(void* mem, size_t size)
{
	if (mem == NULL)
		num_alloc++;
	return sys_realloc(mem, size);
}

__attribute__ ((visibility("default"))) void *
calloc(size_t nmemb, size_t size)
{
	if (sys_calloc == NULL)
		return NULL;

	num_alloc++;

	return sys_calloc(nmemb, size);
}

static const struct test *
find_test(const char *name)
{
	const struct test *t;

	for (t = &__start_test_section; t < &__stop_test_section; t++)
		if (strcmp(t->name, name) == 0)
			return t;

	return NULL;
}

static void
usage(const char *name, int status)
{
	const struct test *t;

	fprintf(stderr, "Usage: %s [TEST]\n\n"
		"With no arguments, run all test.  Specify test case to run\n"
		"only that test without forking.  Available tests:\n\n",
		name);

	for (t = &__start_test_section; t < &__stop_test_section; t++)
		fprintf(stderr, "  %s\n", t->name);

	fprintf(stderr, "\n");

	exit(status);
}

static void
run_test(const struct test *t)
{
	int cur_alloc = num_alloc;
	int cur_fds, num_fds;
	struct timespec start, end;
	char bench_name[255];
	const char *head;
	FILE *f;

	cur_fds = count_open_fds();

	clock_gettime(CLOCK_MONOTONIC, &start);
	t->run();
	clock_gettime(CLOCK_MONOTONIC, &end);

	if (leak_check_enabled) {
		if (cur_alloc != num_alloc) {
			fprintf(stderr, "Memory leak detected in test. "
				"Allocated %d blocks, unfreed %d\n", num_alloc,
				num_alloc - cur_alloc);
			abort();
		}
		num_fds = count_open_fds();
		if (cur_fds != num_fds) {
			fprintf(stderr, "fd leak detected in test. "
				"Opened %d files, unclosed %d\n", num_fds,
				num_fds - cur_fds);
			fprintf(stderr, "Still open filedescriptors:\n");
			print_open_fds();
			abort();
		}
	}

	/* save benchmark tests */
	snprintf(bench_name, 255, "benchmarks/%s.benchmark", t->name);
	f = fopen(bench_name, "a");
	if (f == NULL)
		errx(EXIT_FAILURE, "Opening '%s' failed", bench_name);

	head = get_head_commit();
	ifdbg(head == NULL, "Failed getting HEAD commit\n");

	if (fprintf(f, "%lu %s %lu %lu\n",
		    time(NULL), head ? head : "xxx",
		    end.tv_sec - start.tv_sec,
                    end.tv_nsec - start.tv_nsec) < 0) {
		 fclose(f);
		 errx(EXIT_FAILURE, "Writing to %s failed", bench_name);
	}

	fclose(f);

	exit(EXIT_SUCCESS);
}

/* Print backtrace.
 * Taken from weston */
#ifdef HAVE_LIBUNWIND

static void
print_backtrace(int signum)
{
	unw_cursor_t cursor;
	unw_context_t context;
	unw_word_t off;
	unw_proc_info_t pip;
	int ret, i = 0;
	char procname[256];
	const char *filename;
	Dl_info dlinfo;

	dbg("Backtrace for signal %d:\n", signum);

	pip.unwind_info = NULL;
	ret = unw_getcontext(&context);
	if (ret) {
		dbg("unw_getcontext: %d\n", ret);
		return;
	}

	ret = unw_init_local(&cursor, &context);
	if (ret) {
		dbg("unw_init_local: %d\n", ret);
		return;
	}

	ret = unw_step(&cursor);
	while (ret > 0) {
		ret = unw_get_proc_info(&cursor, &pip);
		if (ret) {
			dbg("unw_get_proc_info: %d\n", ret);
			break;
		}

		ret = unw_get_proc_name(&cursor, procname, 256, &off);
		if (ret && ret != -UNW_ENOMEM) {
			if (ret != -UNW_EUNSPEC)
				dbg("unw_get_proc_name: %d\n", ret);
			procname[0] = '?';
			procname[1] = 0;
		}

		if (dladdr((void *)(pip.start_ip + off), &dlinfo) &&
		    dlinfo.dli_fname &&
		    *dlinfo.dli_fname)
			filename = dlinfo.dli_fname;
		else
			filename = "?";

		fprintf(stderr, "  %u: %s (%s%s+0x%x) [%p]\n", i++, filename, procname,
		    ret == -UNW_ENOMEM ? "..." : "", (int)off,
		    (void *)(pip.start_ip + off));

		ret = unw_step(&cursor);
		if (ret < 0)
			dbg("unw_step: %d\n", ret);
	}

	exit(signum);
}

#else

static void
print_backtrace(int signum)
{
	void *buffer[32];
	int i, count;
	Dl_info info;

	dbg("Backtrace for signal %d:\n", signum);

	count = backtrace(buffer, 32);
	for (i = 0; i < count; i++) {
		dladdr(buffer[i], &info);
		fprintf(stderr, "  [%016lx]  %s  (%s)\n",
		    (long) buffer[i],
		    info.dli_sname ? info.dli_sname : "--",
		    info.dli_fname);
	}

	exit(signum);
}
#endif /* HAVE_LIBUNWIND */

int main(int argc, char *argv[])
{
	const struct test *t;
	pid_t pid;
	int total, pass;
	siginfo_t info;

	/* print backtrace upon SIGSEGV */
	signal(SIGSEGV, print_backtrace);

	/* Load system malloc, free, and realloc */
	sys_calloc = dlsym(RTLD_NEXT, "calloc");
	sys_realloc = dlsym(RTLD_NEXT, "realloc");
	sys_malloc = dlsym(RTLD_NEXT, "malloc");
	sys_free = dlsym(RTLD_NEXT, "free");

	leak_check_enabled = !getenv("NO_ASSERT_LEAK_CHECK");

	if (argc == 2 && strcmp(argv[1], "--help") == 0)
		usage(argv[0], EXIT_SUCCESS);

	if (argc == 2) {
		t = find_test(argv[1]);
		if (t == NULL) {
			fprintf(stderr, "unknown test: \"%s\"\n", argv[1]);
			usage(argv[0], EXIT_FAILURE);
		}

		run_test(t);
	}

	pass = 0;
	for (t = &__start_test_section; t < &__stop_test_section; t++) {
		int success = 0;

		pid = fork();
		assert(pid >= 0);

		if (pid == 0)
			run_test(t); /* never returns */

		if (waitid(P_ALL, 0, &info, WEXITED)) {
			fprintf(stderr, "waitid failed: %m\n");
			abort();
		}

		fprintf(stderr, "test \"%s\":\t", t->name);
		switch (info.si_code) {
		case CLD_EXITED:
			fprintf(stderr, "exit status %d", info.si_status);
			if (info.si_status == EXIT_SUCCESS)
				success = 1;
			break;
		case CLD_KILLED:
		case CLD_DUMPED:
			fprintf(stderr, "signal %d", info.si_status);
			break;
		}

		if (t->must_fail)
			success = !success;

		if (success) {
			pass++;
			fprintf(stderr, ", pass.\n");
		} else
			fprintf(stderr, ", fail.\n");

		fprintf(stderr, "---------------------------------------"
				"-------------------------------------\n");
	}

	total = &__stop_test_section - &__start_test_section;
	fprintf(stderr, "%d tests, %d pass, %d fail\n",
		total, pass, total - pass);

	return pass == total ? EXIT_SUCCESS : EXIT_FAILURE;
}

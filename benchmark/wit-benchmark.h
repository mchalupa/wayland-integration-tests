#ifndef __WIT_BENCHMARK_H__
#define __WIT_BENCHMARK_H__

#define BENCHMARK_TEMPLATE "benchmark-"

#include <time.h>

struct timespec
benchmark(const char *name, void (*func)(void *), void *data);

struct timespec
benchmark_statistics(const char *name);
#endif /* __WIT_BENCHMARK_H__a */

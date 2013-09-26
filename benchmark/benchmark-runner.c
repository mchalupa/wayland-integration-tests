#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>

#include "wit-benchmark.h"

/*
 * Run benchmark and return comparsion to the last run
 */
struct timespec
benchmark(const char *name, void (*func)(void *), void *data)
{
	char buff[255];
	struct timespec start, end, last = {-1, -1};
	int record_shift;
	FILE *f;

	clock_gettime(CLOCK_MONOTONIC, &start);
	func(data);
	clock_gettime(CLOCK_MONOTONIC, &end);

	snprintf(buff, 255, "%s%s", BENCHMARK_TEMPLATE, name);
	f = fopen(buff, "a");
	if (f == NULL) {
		perror("Failed opening/creating file");
		return last;
	}

	last.tv_sec = end.tv_sec - start.tv_sec;
	last.tv_nsec = end.tv_nsec - start.tv_nsec;
	snprintf(buff, 255, "%lu %lu\n%n", last.tv_sec, last.tv_nsec,
		 &record_shift);

	fwrite(&buff, sizeof(char), strlen(buff), f);

	fclose(f);

	return last;
}


/*
 * Count statistics from the benchmark given
 */
struct timespec
benchmark_statistics(const char *name)
{
	char buff[255];
	struct timespec t = {-1, -1};
	struct timespec tmp;
	int count = 0;

	snprintf(buff, 255, "%s%s", BENCHMARK_TEMPLATE, name);
	FILE *f = fopen(buff, "rt");
	if (f == NULL) {
		perror("Failed opening file");
		return t;
	}

	while(fgets(buff, 254, f) != NULL) {
		sscanf(buff, "%lu %lu\n", &tmp.tv_sec, &tmp.tv_nsec);

		t.tv_sec += tmp.tv_sec;
		t.tv_nsec += tmp.tv_nsec;
		count++;
	}

	fclose(f);

	t.tv_sec /= count;
	t.tv_nsec /= count;

	printf("Benchmark %s: average time %lu sec %lu nsec (%d measurements)\n---\n",
		name, t.tv_sec, t.tv_nsec, count);

	return t;
}

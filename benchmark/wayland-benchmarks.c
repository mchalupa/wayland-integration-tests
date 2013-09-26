#include "wit.h"
#include "wit-benchmark.h"

#include <wayland-client.h>

static inline void
roundtrip_benchmark(void *data)
{
	struct wl_display *display = data;
	wl_display_roundtrip(display);
}

static int
benchmarks_main(int sock)
{
	struct wl_display *d = wl_display_connect(NULL);
	assert(d);

	benchmark("wl_roundtrip_benchmark", roundtrip_benchmark, d);

	wl_display_disconnect(d);
	return 0;
}

static void
statistics(void)
{
	benchmark_statistics("wl_roundtrip_benchmark");
}

int main(void)
{
	struct wit_display *d = wit_display_create(NULL);
	wit_display_create_client(d, benchmarks_main);

	wit_display_run(d);
	wit_display_destroy(d);

	statistics();
	return 0;
}

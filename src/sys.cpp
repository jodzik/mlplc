#include <mlplc/sys/sys.hpp>
#include <zephyr/sys/printk.h>

namespace mlplc {
namespace sys {

namespace _private {

}

static void print_sys_memory_stats(struct sys_heap *hp)
{
	struct sys_memory_stats stats;

	sys_heap_runtime_stats_get(hp, &stats);

	printk("allocated %zu, free %zu, max allocated %zu\n",
		stats.allocated_bytes, stats.free_bytes,
		stats.max_allocated_bytes);
}

static void print_all_heaps(void)
{
	struct sys_heap **ha;
	size_t i, n;

	n = sys_heap_array_get(&ha);
	printk("There are %zu heaps allocated:\n", n);

	for (i = 0; i < n; i++) {
		printk("\t%zu - address %p ", i, ha[i]);
		print_sys_memory_stats(ha[i]);
	}
}

std::size_t mem_usage() {
    print_all_heaps();
    return 0;
}

}
}

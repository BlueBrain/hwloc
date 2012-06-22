#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
	hwloc_topology_t topology;

	printf(">init...\n");
	hwloc_topology_init(&topology);

	printf(">switching sysfs fsroot...\n");
	hwloc_topology_set_fsroot(topology, "/");

	printf(">switching sysfs fsroot and loading...\n");
	hwloc_topology_set_fsroot(topology, "/");
	hwloc_topology_load(topology);
	fprintf(stderr, "**test file: After load\n");

	printf(">destroy...\n");
	hwloc_topology_destroy(topology);

	return 0;
}

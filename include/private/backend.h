#ifndef HWLOC_BACKEND_H
#define HWLOC_BACKEND_H

typedef enum {
	HWLOC_BACKEND_GLOBAL, /* XML, Synthetic, ... */ 
	HWLOC_BACKEND_BASE, /* OS */
	HWLOC_BACKEND_IO /* PCI, CUDA, ... */
} hwloc_backend_type;

struct hwloc_backend_st{
	hwloc_backend_type type;
	void (*hwloc_look)(struct hwloc_topology); /* Fill the bind functions pointers 
										 		* at least the set_cpubind one */
	void (*hwloc_set_hooks)(struct hwloc_topology); /* Set binding hooks */
	int (*hwloc_backend_init)(void*, ...); /* One topology should be passed in parameter */
	void (*hwloc_backend_exit)(struct hwloc_topology);

};


#endif /* HWLOC_BACKEND_H */

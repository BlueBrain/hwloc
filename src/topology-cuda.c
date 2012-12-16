/*
 * Copyright © 2011 Université Bordeaux 1
 * Copyright © 2012 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>
#include <hwloc.h>
#include <hwloc/plugins.h>
#include <hwloc/cudart.h>

/* private headers allowed for convenience because this plugin is built within hwloc */
#include <private/misc.h>
#include <private/debug.h>

#include <cuda.h>
#include <cuda_runtime_api.h>

static int hwloc_cuda_cores_per_MP(int major, int minor)
{
  /* TODO: use nvml instead? */
  switch (major) {
    case 1:
      switch (minor) {
        case 0:
        case 1:
        case 2:
        case 3: return 8;
      }
      break;
    case 2:
      switch (minor) {
        case 0: return 32;
        case 1: return 48;
      }
      break;
  }
  hwloc_debug("unknown compute capability %u.%u, disabling core display.\n", major, minor);
  return 0;
}

static hwloc_obj_t hwloc_topology_get_pcidev(hwloc_topology_t topology, hwloc_obj_t parent, int domain, int bus, int dev)
{
  hwloc_obj_t child;

  if (parent->type == HWLOC_OBJ_PCI_DEVICE
      && parent->attr->pcidev.domain == domain
      && parent->attr->pcidev.bus == bus
      && parent->attr->pcidev.dev == dev)
    return parent;

  for (child = parent->first_child; child; child = child->next_sibling) {
    hwloc_obj_t found;
    found = hwloc_topology_get_pcidev(topology, child, domain, bus, dev);
    if (found)
      return found;
  }

  return NULL;
}

static
int hwloc_look_cuda(struct hwloc_backend *backend)
{
  struct hwloc_topology *topology = backend->topology;
  int cnt, device;
  cudaError_t cures;
  struct cudaDeviceProp prop;
  int res = 0;

  if (!(hwloc_topology_get_flags(topology) & (HWLOC_TOPOLOGY_FLAG_IO_DEVICES|HWLOC_TOPOLOGY_FLAG_WHOLE_IO)))
    return 0;

  if (!hwloc_topology_is_thissystem(topology)) {
    hwloc_debug("%s", "\nno CUDA detection (not thissystem)\n");
    return 0;
  }

  cures = cudaGetDeviceCount(&cnt);
  if (cures)
    return -1;

  for (device = 0; device < cnt; device++) {
    int domain, bus, dev;
    char cuda_name[32];
    hwloc_obj_t pci_card, cuda_device, memory, space;
    unsigned i, j, cores;

    hwloc_debug("cuda %d\n", device);

    cures = cudaGetDeviceProperties(&prop, device);
    if (cures)
      continue;

    if (hwloc_cudart_get_device_pci_ids(topology, device, &domain, &bus, &dev))
      continue;

    cores = hwloc_cuda_cores_per_MP(prop.major, prop.minor);

    pci_card = hwloc_topology_get_pcidev(topology, hwloc_get_root_obj(topology), domain, bus, dev);
    if (!pci_card)
      continue;

    cuda_device = hwloc_alloc_setup_object(HWLOC_OBJ_OS_DEVICE, -1);
    snprintf(cuda_name, sizeof(cuda_name), "cuda%d", device);
    cuda_device->name = strdup(cuda_name);
    cuda_device->depth = (unsigned) HWLOC_TYPE_DEPTH_UNKNOWN;
    cuda_device->attr->osdev.type = HWLOC_OBJ_OSDEV_GPU;

    hwloc_obj_add_info(cuda_device, "Backend", "CUDA");
    hwloc_obj_add_info(cuda_device, "Vendor", "NVIDIA Corporation");
    hwloc_obj_add_info(cuda_device, "Name", prop.name);

    hwloc_insert_object_by_parent(topology, pci_card, cuda_device);

    space = memory = hwloc_alloc_setup_object(HWLOC_OBJ_NODE, -1);
    memory->name = strdup("Global");
    memory->memory.total_memory = memory->memory.local_memory = prop.totalGlobalMem;
    hwloc_insert_object_by_parent(topology, cuda_device, memory);

#ifdef HWLOC_HAVE_CUDA_L2CACHESIZE
    if (prop.l2CacheSize) {
      hwloc_obj_t cache = hwloc_alloc_setup_object(HWLOC_OBJ_CACHE, i);

      hwloc_debug("%d KiB cache\n", prop.l2CacheSize >> 10);

      cache->attr->cache.size = prop.l2CacheSize;
      cache->attr->cache.depth = 2;
      cache->attr->cache.linesize = 0; /* TODO */
      cache->attr->cache.associativity = 0; /* TODO */
      hwloc_insert_object_by_parent(topology, memory, cache);
      space = cache;
    }
#endif

    hwloc_debug("%d MP(s)\n", prop.multiProcessorCount);
    if (hwloc_topology_get_flags(topology) & HWLOC_TOPOLOGY_FLAG_WHOLE_ACCELERATORS) {
      for (i = 0; i < (unsigned) prop.multiProcessorCount; i++) {
        hwloc_obj_t shared = hwloc_alloc_setup_object(HWLOC_OBJ_NODE, -1);
        hwloc_obj_t group = hwloc_alloc_setup_object(HWLOC_OBJ_GROUP, -1);
        group->attr->group.tight = 1;

        shared->name = strdup("Shared");
        shared->memory.total_memory = shared->memory.local_memory = prop.sharedMemPerBlock;
        group->name = strdup("MP");

        hwloc_insert_object_by_parent(topology, space, shared);
        hwloc_insert_object_by_parent(topology, shared, group);

        hwloc_debug("%d cores\n", cores);
        for (j = 0; j < cores; j++) {
          hwloc_obj_t core = hwloc_alloc_setup_object(HWLOC_OBJ_CORE, j);
          hwloc_insert_object_by_parent(topology, group, core);
        }
      }
    } else {
      char name[32];
      hwloc_obj_t coreset = hwloc_alloc_setup_object(HWLOC_OBJ_MISC, -1);
      snprintf(name, sizeof(name), "%d x (%d cores + %uKB)", prop.multiProcessorCount, cores, (unsigned) (prop.sharedMemPerBlock / 1024));
      coreset->name = strdup(name);
      hwloc_insert_object_by_parent(topology, space, coreset);
    }
#if 0
    printf("Clock %0.3fGHz\n", (float)(float)  prop.clockRate / (1 << 20));
#endif
    res++;
  }

  return res;
}

static struct hwloc_backend *
hwloc_cuda_component_instantiate(struct hwloc_disc_component *component,
                                 const void *_data1 __hwloc_attribute_unused,
                                 const void *_data2 __hwloc_attribute_unused,
                                 const void *_data3 __hwloc_attribute_unused)
{
  struct hwloc_backend *backend;

  /* thissystem may not be fully initialized yet, we'll check flags in discover() */

  backend = hwloc_backend_alloc(component);
  if (!backend)
    return NULL;
  backend->discover = hwloc_look_cuda;
  return backend;
}

static struct hwloc_disc_component hwloc_cuda_disc_component = {
  HWLOC_DISC_COMPONENT_TYPE_ADDITIONAL,
  "cuda",
  HWLOC_DISC_COMPONENT_TYPE_GLOBAL,
  hwloc_cuda_component_instantiate,
  19, /* after libpci */
  NULL
};

#ifdef HWLOC_INSIDE_PLUGIN
HWLOC_DECLSPEC extern const struct hwloc_component hwloc_cuda_component;
#endif

const struct hwloc_component hwloc_cuda_component = {
  HWLOC_COMPONENT_ABI,
  HWLOC_COMPONENT_TYPE_DISC,
  0,
  &hwloc_cuda_disc_component
};

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

struct hwloc_cuda_backend_data_s {
  unsigned nr_devices; /* -1 when unknown yet, first callback will setup */
  struct hwloc_cuda_device_info_s {
    int idx;
    unsigned pcidomain, pcibus, pcidev, pcifunc;
  } * devices;
};

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

/* query all PCI bus ids for later */
static void
hwloc_cuda_query_devices(struct hwloc_cuda_backend_data_s *data)
{
  cudaError_t cures;
  int nb, i;

  /* mark the number of devices as 0 in case we fail below,
   * so that we don't try again later.
   */
  data->nr_devices = 0;

  cures = cudaGetDeviceCount(&nb);
  if (cures)
    return;

  /* allocate structs */
  data->devices = malloc(nb * sizeof(*data->devices));
  if (!data->devices)
    return;

  for (i = 0; i < nb; i++) {
    struct hwloc_cuda_device_info_s *info = &data->devices[data->nr_devices];
    int domain, bus, dev;

    if (hwloc_cudart_get_device_pci_ids(NULL /* topology unused */, i, &domain, &bus, &dev))
      continue;

    info->idx = i;
    info->pcidomain = (unsigned) domain;
    info->pcibus = (unsigned) bus;
    info->pcidev = (unsigned) dev;
    info->pcifunc = 0;

    /* validate this device */
    data->nr_devices++;
  }

  return;
}

static int
hwloc_cuda_backend_notify_new_object(struct hwloc_backend *backend, struct hwloc_backend *caller __hwloc_attribute_unused,
				     struct hwloc_obj *pcidev)
{
  struct hwloc_topology *topology = backend->topology;
  struct hwloc_cuda_backend_data_s *data = backend->private_data;
  unsigned i;

  if (!(hwloc_topology_get_flags(topology) & (HWLOC_TOPOLOGY_FLAG_IO_DEVICES|HWLOC_TOPOLOGY_FLAG_WHOLE_IO)))
    return 0;

  if (!hwloc_topology_is_thissystem(topology)) {
    hwloc_debug("%s", "\nno CUDA detection (not thissystem)\n");
    return 0;
  }

  if (HWLOC_OBJ_PCI_DEVICE != pcidev->type)
    return 0;

  if (data->nr_devices == (unsigned) -1) {
    /* first call, lookup all devices */
    hwloc_cuda_query_devices(data);
    /* if it fails, data->nr_devices = 0 so we won't do anything below and in next callbacks */
  }

  if (!data->nr_devices)
    /* found no devices */
    return 0;

  for(i=0; i<data->nr_devices; i++) {
    struct hwloc_cuda_device_info_s *info = &data->devices[i];
    char cuda_name[32];
    struct cudaDeviceProp prop;
    hwloc_obj_t cuda_device, memory, space;
    unsigned i, j, cores;
    cudaError_t cures;

    if (info->pcidomain != pcidev->attr->pcidev.domain)
      continue;
    if (info->pcibus != pcidev->attr->pcidev.bus)
      continue;
    if (info->pcidev != pcidev->attr->pcidev.dev)
      continue;
    if (info->pcifunc != pcidev->attr->pcidev.func)
      continue;

    hwloc_debug("cuda %d\n", info->idx);

    cures = cudaGetDeviceProperties(&prop, info->idx);
    if (cures)
      continue;

    cores = hwloc_cuda_cores_per_MP(prop.major, prop.minor);

    cuda_device = hwloc_alloc_setup_object(HWLOC_OBJ_OS_DEVICE, -1);
    snprintf(cuda_name, sizeof(cuda_name), "cuda%d", info->idx);
    cuda_device->name = strdup(cuda_name);
    cuda_device->depth = (unsigned) HWLOC_TYPE_DEPTH_UNKNOWN;
    cuda_device->attr->osdev.type = HWLOC_OBJ_OSDEV_GPU;

    hwloc_obj_add_info(cuda_device, "Backend", "CUDA");
    hwloc_obj_add_info(cuda_device, "Vendor", "NVIDIA Corporation");
    hwloc_obj_add_info(cuda_device, "Name", prop.name);

    hwloc_insert_object_by_parent(topology, pcidev, cuda_device);

    space = memory = hwloc_alloc_setup_object(HWLOC_OBJ_NODE, -1);
    memory->name = strdup("Global");
    memory->memory.total_memory = memory->memory.local_memory = prop.totalGlobalMem;
    hwloc_insert_object_by_parent(topology, cuda_device, memory);

#ifdef HWLOC_HAVE_CUDA_L2CACHESIZE
    if (prop.l2CacheSize) {
      hwloc_obj_t cache = hwloc_alloc_setup_object(HWLOC_OBJ_CACHE, info->idx);

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

    return 1;
  }

  return 0;
}

static void
hwloc_cuda_backend_disable(struct hwloc_backend *backend)
{
  struct hwloc_cuda_backend_data_s *data = backend->private_data;
  free(data->devices);
  free(data);
}

static struct hwloc_backend *
hwloc_cuda_component_instantiate(struct hwloc_disc_component *component,
                                 const void *_data1 __hwloc_attribute_unused,
                                 const void *_data2 __hwloc_attribute_unused,
                                 const void *_data3 __hwloc_attribute_unused)
{
  struct hwloc_backend *backend;
  struct hwloc_cuda_backend_data_s *data;

  /* thissystem may not be fully initialized yet, we'll check flags in discover() */

  backend = hwloc_backend_alloc(component);
  if (!backend)
    return NULL;

  data = malloc(sizeof(*data));
  if (!data) {
    free(backend);
    return NULL;
  }
  /* the first callback will initialize those */
  data->nr_devices = (unsigned) -1; /* unknown yet */
  data->devices = NULL;

  backend->private_data = data;
  backend->disable = hwloc_cuda_backend_disable;

  backend->notify_new_object = hwloc_cuda_backend_notify_new_object;
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

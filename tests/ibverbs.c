/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <stdio.h>
#include <assert.h>
#include <infiniband/verbs.h>
#include <hwloc.h>
#include <hwloc/ibverbs.h>

/* check the ibverbs helpers */

int main() {
  struct ibv_device **dev_list, *dev;
  int count, i;

  dev_list = ibv_get_device_list(&count);
  if (!dev_list) {
    fprintf(stderr, "ibv_get_device_list failed\n");
    return 0;
  }
  printf("ibv_get_device_list found %d devices\n", count);

  for(i=0; i<count; i++) {
    hwloc_cpuset_t set;
    dev = dev_list[i];

    if (hwloc_ibverbs_get_device_cpuset(dev, &set) < 0) {
      printf("failed to get cpuset for device %d (%s)\n",
	     i, ibv_get_device_name(dev));
    } else {
      char *cpuset_string = NULL;
      hwloc_cpuset_asprintf(&cpuset_string, set);
      printf("got cpuset %s for device %d (%s)\n",
	     cpuset_string, i, ibv_get_device_name(dev));
      free(cpuset_string);
    }
  }

  ibv_free_device_list(dev_list);

  return 0;
}

/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

/* Check the is_thissystem flag behavior */

static void result(const char *msg, int err)
{
  if (err)
    printf("%-30s: FAILED (%d, %s)\n", msg, errno, strerror(errno));
  else
    printf("%-30s: OK\n", msg);
}

int main(void)
{
  hwloc_topology_t topology;
  hwloc_cpuset_t cpuset;
  int err;

  /* check the OS topology */
  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);
  assert(hwloc_topology_is_thissystem(topology));

  cpuset = hwloc_cpuset_dup(hwloc_topology_get_complete_cpuset(topology));
  result("Binding with OS backend", hwloc_set_cpubind(topology, cpuset, 0));

  hwloc_topology_destroy(topology);

  /* We're assume there is a real processor numbered 0 */
  hwloc_cpuset_zero(cpuset);
  hwloc_cpuset_set(cpuset, 0);

  /* check a synthetic topology */
  hwloc_topology_init(&topology);
  hwloc_topology_set_synthetic(topology, "1");
  hwloc_topology_load(topology);
  assert(!hwloc_topology_is_thissystem(topology));

  err = hwloc_set_cpubind(topology, cpuset, 0);
  result("Binding with synthetic backend", err);
  assert(!err);

  hwloc_topology_destroy(topology);

  /* check a synthetic topology but assuming it's the system topology */
  hwloc_topology_init(&topology);
  hwloc_topology_set_flags(topology, HWLOC_TOPOLOGY_FLAG_IS_THISSYSTEM);
  hwloc_topology_set_synthetic(topology, "1");
  hwloc_topology_load(topology);
  assert(hwloc_topology_is_thissystem(topology));

  result("Binding with synthetic backend faking is_thissystem", hwloc_set_cpubind(topology, cpuset, 0));

  hwloc_topology_destroy(topology);

  hwloc_cpuset_free(cpuset);

  return 0;
}

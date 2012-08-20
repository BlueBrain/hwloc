/*
 * Copyright © 2012 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <hwloc.h>
#include <assert.h>

int main(void)
{
  struct hwloc_topology *topology;
  unsigned depth, width;
  hwloc_obj_t child;
  float vals[12];
  unsigned idxs[12];
  struct hwloc_valarray_s *valarray;
  unsigned i;
  int err;

  err = hwloc_topology_init(&topology);
  assert(!err);
  err = hwloc_topology_load(topology);
  assert(!err);

  depth = hwloc_topology_get_depth(topology);
  width = hwloc_get_nbobjs_by_depth(topology, depth-1);
  child = hwloc_get_obj_by_depth(topology, depth-1, width-1);
  assert(child);

  for(i=0; i<12; i++) {
    vals[i] = 3.4*i-i*i;
    idxs[i] = i*i;
  }
  hwloc_obj_add_valarray(child, "Test1", 5, &vals[0], NULL);
  hwloc_obj_add_valarray(child, "Test2", 7, &vals[5], &idxs[5]);

  valarray = hwloc_obj_get_valarray_by_name(child, "Test2");
  assert(valarray);
  err = strcmp(valarray->name, "Test2");
  assert(!err);
  assert(valarray->nb == 7);
  err = memcmp(valarray->values, &vals[5], 7*sizeof(float));
  assert(!err);
  err = memcmp(valarray->idx, &idxs[5], 7*sizeof(unsigned));
  assert(!err);

  valarray = hwloc_obj_get_valarray_by_name(child, "Test1");
  assert(valarray);
  err = strcmp(valarray->name, "Test1");
  assert(!err);
  assert(valarray->nb == 5);
  err = memcmp(valarray->values, &vals[0], 5*sizeof(float));
  assert(!err);
  assert(valarray->idx[0] == 0);
  assert(valarray->idx[1] == 1);
  assert(valarray->idx[2] == 2);
  assert(valarray->idx[3] == 3);
  assert(valarray->idx[4] == 4);

  valarray = hwloc_obj_get_valarray_by_name(child, "Test1");
  assert(valarray);

  hwloc_topology_destroy(topology);
  return 0;
}

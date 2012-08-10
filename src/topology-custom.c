/*
 * Copyright © 2011-2012 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <hwloc.h>
#include <private/private.h>

hwloc_obj_t
hwloc_custom_insert_group_object_by_parent(struct hwloc_topology *topology, hwloc_obj_t parent, int groupdepth)
{
  hwloc_obj_t obj = hwloc_alloc_setup_object(HWLOC_OBJ_GROUP, -1);
  obj->attr->group.depth = groupdepth;

  if (topology->backend_type != HWLOC_BACKEND_CUSTOM || topology->is_loaded) {
    errno = EINVAL;
    return NULL;
  }

  hwloc_insert_object_by_parent(topology, parent, obj);

  return obj;
}

int
hwloc_backend_custom_init(struct hwloc_topology *topology)
{
  assert(topology->backend_type == HWLOC_BACKEND_NONE);

  topology->levels[0][0]->type = HWLOC_OBJ_SYSTEM;
  topology->is_thissystem = 0;
  topology->backend_type = HWLOC_BACKEND_CUSTOM;
  return 0;
}

int
hwloc_custom_insert_topology(struct hwloc_topology *newtopology,
			     struct hwloc_obj *newparent,
			     struct hwloc_topology *oldtopology,
			     struct hwloc_obj *oldroot)
{
  if (newtopology->backend_type != HWLOC_BACKEND_CUSTOM || newtopology->is_loaded || !oldtopology->is_loaded) {
    errno = EINVAL;
    return -1;
  }

  hwloc__duplicate_objects(newtopology, newparent, oldroot ? oldroot : oldtopology->levels[0][0]);
  return 0;
}

void
hwloc_backend_custom_exit(struct hwloc_topology *topology)
{
  assert(topology->backend_type == HWLOC_BACKEND_CUSTOM);

  /* undo what we did in instantiate */
  topology->levels[0][0]->type = HWLOC_OBJ_MACHINE;
  topology->is_thissystem = 1;

  topology->backend_type = HWLOC_BACKEND_NONE;
}

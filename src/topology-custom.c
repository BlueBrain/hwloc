/*
 * Copyright © 2011-2012 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>
#include <hwloc.h>
#include <private/private.h>

hwloc_obj_t
hwloc_custom_insert_group_object_by_parent(struct hwloc_topology *topology, hwloc_obj_t parent, int groupdepth)
{
  hwloc_obj_t obj = hwloc_alloc_setup_object(HWLOC_OBJ_GROUP, -1);
  obj->attr->group.depth = groupdepth;

  /* FIXME: retrieve the backend instead of assuming it's the first one */
  if (!topology->backend->is_custom || topology->is_loaded) {
    errno = EINVAL;
    return NULL;
  }

  hwloc_insert_object_by_parent(topology, parent, obj);

  return obj;
}

int
hwloc_custom_insert_topology(struct hwloc_topology *newtopology,
			     struct hwloc_obj *newparent,
			     struct hwloc_topology *oldtopology,
			     struct hwloc_obj *oldroot)
{
  /* FIXME: retrieve the backend instead of assuming it's the first one */
  if (!newtopology->backend->is_custom || newtopology->is_loaded || !oldtopology->is_loaded) {
    errno = EINVAL;
    return -1;
  }

  hwloc__duplicate_objects(newtopology, newparent, oldroot ? oldroot : oldtopology->levels[0][0]);
  return 0;
}

static int
hwloc_look_custom(struct hwloc_backend *backend)
{
  struct hwloc_topology *topology = backend->topology;

  assert(!topology->levels[0][0]->cpuset);

  if (!topology->levels[0][0]->first_child) {
    errno = EINVAL;
    return -1;
  }

  topology->levels[0][0]->type = HWLOC_OBJ_SYSTEM;
  return 1;
}

static struct hwloc_backend *
hwloc_custom_component_instantiate(struct hwloc_topology *topology,
				   struct hwloc_core_component *component,
				   const void *_data1 __hwloc_attribute_unused,
				   const void *_data2 __hwloc_attribute_unused,
				   const void *_data3 __hwloc_attribute_unused)
{
  struct hwloc_backend *backend;
  backend = hwloc_backend_alloc(topology, component);
  if (!backend)
    return NULL;
  backend->discover = hwloc_look_custom;
  backend->is_custom = 1;
  backend->is_thissystem = 0;
  return backend;
}

static struct hwloc_core_component hwloc_custom_core_component = {
  HWLOC_CORE_COMPONENT_TYPE_GLOBAL,
  "custom",
  HWLOC_CORE_COMPONENT_TYPE_OS | HWLOC_CORE_COMPONENT_TYPE_GLOBAL | HWLOC_CORE_COMPONENT_TYPE_ADDITIONAL,
  hwloc_custom_component_instantiate,
  30,
  NULL
};

const struct hwloc_component hwloc_custom_component = {
  HWLOC_COMPONENT_ABI,
  HWLOC_COMPONENT_TYPE_CORE,
  0,
  &hwloc_custom_core_component
};

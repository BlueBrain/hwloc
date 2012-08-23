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
  if (!newtopology->backend->is_custom || newtopology->is_loaded || !oldtopology->is_loaded) {
    errno = EINVAL;
    return -1;
  }

  hwloc__duplicate_objects(newtopology, newparent, oldroot ? oldroot : oldtopology->levels[0][0]);
  return 0;
}

static int
hwloc_look_custom(struct hwloc_topology *topology __hwloc_attribute_unused)
{
  if (!topology->levels[0][0]->first_child) {
    errno = EINVAL;
    return -1;
  }

  topology->levels[0][0]->type = HWLOC_OBJ_SYSTEM;
  topology->is_thissystem = 0;
  return 1;
}

static int
hwloc_custom_component_instantiate(struct hwloc_topology *topology,
				   struct hwloc_component *component,
				   const void *_data1 __hwloc_attribute_unused,
				   const void *_data2 __hwloc_attribute_unused,
				   const void *_data3 __hwloc_attribute_unused)
{
  struct hwloc_backend *backend;

  backend = hwloc_backend_alloc(topology, component);
  if (!backend)
    goto out;

  backend->discover = hwloc_look_custom;
  backend->is_custom = 1;
  hwloc_backend_enable(topology, backend);
  return 0;

 out:
  free(backend);
  return -1;
}

static struct hwloc_component hwloc_custom_component = {
  HWLOC_COMPONENT_TYPE_GLOBAL,
  "custom",
  hwloc_custom_component_instantiate,
  NULL, /* no hooks for HWLOC_COMPONENT_TYPE_GLOBAL */
  10,
  NULL
};

void
hwloc_core_custom_component_register(void)
{
  hwloc_component_register(&hwloc_custom_component);
}

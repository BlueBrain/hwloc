/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2012 Inria.  All rights reserved.
 * Copyright © 2009-2012 Université Bordeaux 1
 * Copyright © 2009-2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>
#include <hwloc.h>
#include <private/private.h>

static int
hwloc_look_noos(struct hwloc_topology *topology, struct hwloc_backend *backend __hwloc_attribute_unused)
{
  hwloc_alloc_obj_cpusets(topology->levels[0][0]);
  hwloc_setup_pu_level(topology, hwloc_fallback_nbprocessors(topology));
  if (topology->is_thissystem)
    hwloc_add_uname_info(topology);
  return 1;
}

static struct hwloc_backend *
hwloc_noos_component_instantiate(struct hwloc_topology *topology,
				 struct hwloc_core_component *component,
				 const void *_data1 __hwloc_attribute_unused,
				 const void *_data2 __hwloc_attribute_unused,
				 const void *_data3 __hwloc_attribute_unused)
{
  struct hwloc_backend *backend;
  backend = hwloc_backend_alloc(topology, component);
  if (!backend)
    return NULL;
  backend->discover = hwloc_look_noos;
  return backend;
}

static void
hwloc_set_noos_hooks(struct hwloc_topology *topology __hwloc_attribute_unused)
{
}

static struct hwloc_core_component hwloc_noos_core_component = {
  HWLOC_CORE_COMPONENT_TYPE_OS,
  "no_os",
  hwloc_noos_component_instantiate,
  hwloc_set_noos_hooks,
  0, /* lowest priority */
  NULL
};

const struct hwloc_component hwloc_noos_component = {
  HWLOC_COMPONENT_ABI,
  HWLOC_COMPONENT_TYPE_CORE,
  0,
  &hwloc_noos_core_component
};
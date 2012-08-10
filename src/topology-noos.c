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

int
hwloc_look_noos(struct hwloc_topology *topology)
{
  hwloc_alloc_obj_cpusets(topology->levels[0][0]);
  hwloc_setup_pu_level(topology, hwloc_fallback_nbprocessors(topology));
  if (topology->is_thissystem)
    hwloc_add_uname_info(topology);
  return 0;
}

static int
hwloc_noos_component_instantiate(struct hwloc_topology *topology,
				 struct hwloc_component *component,
				 const void *_data1 __hwloc_attribute_unused,
				 const void *_data2 __hwloc_attribute_unused,
				 const void *_data3 __hwloc_attribute_unused)
{
  struct hwloc_backend *backend;
  backend = hwloc_backend_alloc(topology, component);
  if (!backend)
    return -1;
  hwloc_backend_enable(topology, backend);
  return 0;
}

static struct hwloc_component hwloc_noos_component = {
  HWLOC_COMPONENT_TYPE_OS,
  "none",
  hwloc_noos_component_instantiate,
  NULL
};

void
hwloc_noos_component_register(struct hwloc_topology *topology)
{
  hwloc_component_register(topology, &hwloc_noos_component);
}

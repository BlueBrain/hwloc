/*
 * Copyright © 2012 Blue Brain Project, EPFL. All rights reserved.
 * Copyright © 2012 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#ifndef HWLOC_GL_H
#define HWLOC_GL_H

#include <hwloc.h>

/* FIXME: don't include this header everywhere else */
/* FIXME: rename into display.h ? */

#ifdef __cplusplus
extern "C" {
#endif

/** \brief Queries a display defined by its port and device in the
 * string format ":[port].[device]", and returns a hwloc_obj_t of
 * type HWLOC_OBJ_PCI_DEVICE containg the desired pci parameters
 * (bus,device id, domain, function) representing the GPU connected
 * to this display.
 */
HWLOC_DECLSPEC hwloc_obj_t hwloc_gl_query_display(hwloc_topology_t topology, char* displayName);

/** \brief Find the PCI device object matching the GPU connected to the
 * display defined by its port and device as [:][port][.][device]
 */
static __hwloc_inline hwloc_obj_t
hwloc_get_pcidev_by_display(hwloc_topology_t topology, int port, int device)
{
  char x_display [10];
  snprintf(x_display,sizeof(x_display),":%d.%d", port, device);
  return hwloc_gl_query_display(topology, x_display);
}

/** \brief Returns a DISPLAY for a given GPU defined by pcidev_obj.
 */
HWLOC_DECLSPEC int hwloc_gl_get_gpu_display(hwloc_topology_t topology, hwloc_obj_t pcidev_obj, unsigned *port, unsigned *device);

/** \brief Returns an object of type HWLOC_OBJ_PCI_DEVICE
 * representing the GPU connected to the display defined by
 * its port and device.
 */
HWLOC_DECLSPEC hwloc_obj_t hwloc_gl_get_gpu_by_display(hwloc_topology_t topology, int port, int device);

/** \brief Returns the cpuset of the socket connected to the
 * host bridge connecting the GPU attached to the display
 * defined by its input port and device.
 */
HWLOC_DECLSPEC hwloc_bitmap_t hwloc_gl_get_display_cpuset(hwloc_topology_t topology, int port, int device);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HWLOC_GL_H */


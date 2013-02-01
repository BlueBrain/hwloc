/*
 * Copyright © 2012 Blue Brain Project, BBP/EPFL. All rights reserved.
 * Copyright © 2012 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>
#include <hwloc.h>
#include <hwloc/plugins.h>
#include <hwloc/helper.h>
#include <stdarg.h>
#include <errno.h>
#include <hwloc/gl.h>
#include <X11/Xlib.h>
#include <NVCtrl/NVCtrl.h>
#include <NVCtrl/NVCtrlLib.h>

/*****************************************************************
 * Queries a display defined by its port and device numbers in the
 * string format ":[port].[device]", and
 * returns a hwloc_obj_t containg the desired pci parameters (bus,
 * device id, domain, function)
 ****************************************************************/
hwloc_obj_t hwloc_gl_query_display(hwloc_topology_t topology, char* displayName)
{
#ifdef HWLOC_HAVE_GL
  hwloc_obj_t display_obj = NULL;
  Display* display;
  int opcode, event, error;
  int default_screen_number;
  unsigned int *ptr_binary_data;
  int data_lenght;
  int gpu_number;
  int nv_ctrl_pci_bus;
  int nv_ctrl_pci_device;
  int nv_ctrl_pci_domain;
  int nv_ctrl_pci_func;
  int err;

  display = XOpenDisplay(displayName);
  if (display == 0) {
    return display_obj;
  }

  /* Check for NV-CONTROL extension */
  if( !XQueryExtension(display, "NV-CONTROL", &opcode, &event, &error))
  {
    XCloseDisplay( display);
    return display_obj;
  }

  default_screen_number = DefaultScreen(display);

  /* Gets the GPU number attached to the default screen. */
  /* For further details, see the <NVCtrl/NVCtrlLib.h> */
  err = XNVCTRLQueryTargetBinaryData (display, NV_CTRL_TARGET_TYPE_X_SCREEN, default_screen_number, 0,
				      NV_CTRL_BINARY_DATA_GPUS_USED_BY_XSCREEN,
				      (unsigned char **) &ptr_binary_data, &data_lenght);
  if (!err)
    goto out_display;

  gpu_number = ptr_binary_data[1];
  free(ptr_binary_data);

  /* Gets the ID's of the GPU defined by gpu_number
   * For further details, see the <NVCtrl/NVCtrlLib.h> */
  err = XNVCTRLQueryTargetAttribute(display, NV_CTRL_TARGET_TYPE_GPU, gpu_number, 0,
				    NV_CTRL_PCI_BUS, &nv_ctrl_pci_bus);
  if (!err)
    goto out_display;

  err = XNVCTRLQueryTargetAttribute(display, NV_CTRL_TARGET_TYPE_GPU, gpu_number, 0,
				    NV_CTRL_PCI_DEVICE, &nv_ctrl_pci_device);
  if (!err)
    goto out_display;

  err = XNVCTRLQueryTargetAttribute(display, NV_CTRL_TARGET_TYPE_GPU, gpu_number, 0,
				    NV_CTRL_PCI_DOMAIN, &nv_ctrl_pci_domain);
  if (!err)
    goto out_display;

  err = XNVCTRLQueryTargetAttribute(display, NV_CTRL_TARGET_TYPE_GPU, gpu_number, 0,
				    NV_CTRL_PCI_FUNCTION, &nv_ctrl_pci_func);
  if (!err)
    goto out_display;

  display_obj = hwloc_get_pcidev_by_busid(topology,
					  (unsigned) nv_ctrl_pci_domain,
					  (unsigned) nv_ctrl_pci_bus,
					  (unsigned) nv_ctrl_pci_device,
					  (unsigned) nv_ctrl_pci_func);

  XCloseDisplay(display);
  return display_obj;

out_display:
  XCloseDisplay(display);
  errno = EINVAL;
  return NULL;

#else
  errno = ENOSYS;
  return NULL;
#endif
}

/*****************************************************************
 * Returns a DISPLAY for a given GPU defined by its pcidev_obj
 * which contains the pci info required for doing the matching a
 * pci device in the topology.
 ****************************************************************/
int hwloc_gl_get_gpu_display(hwloc_topology_t topology, hwloc_obj_t pcidev_obj, unsigned *port, unsigned *device)
{
  hwloc_obj_t query_display_obj;

  unsigned x_server_max;
  unsigned x_screen_max;
  unsigned i,j;

  /* Try the first 10 servers with 10 screens */
  /* For each x server, if the first x screen fails move to the next x server */
  x_server_max = 10;
  x_screen_max = 10;

  for (i = 0; i < x_server_max; ++i) {
    for (j = 0; j < x_screen_max; ++j) {

      /* Formulate the display string with the format "[:][x_server][.][x_screen]" */
      char x_display [10];
      snprintf(x_display,sizeof(x_display),":%d.%d", i, j);

      /* Retrieve an object matching the x_display */
      query_display_obj = hwloc_gl_query_display(topology, x_display);

      if (query_display_obj == NULL)
        break;

      if (query_display_obj->attr->pcidev.bus == pcidev_obj->attr->pcidev.bus &&
          query_display_obj->attr->pcidev.device_id == pcidev_obj->attr->pcidev.device_id &&
          query_display_obj->attr->pcidev.domain == pcidev_obj->attr->pcidev.domain &&
          query_display_obj->attr->pcidev.func == pcidev_obj->attr->pcidev.func) {

        *port = i;
        *device = j;

        return 0;
      }
    }
  }
  return -1;
}

/*****************************************************************
 * Returns a hwloc_obj_t HWLOC_OBJ_PCI_DEVICE representing the GPU
 * connected to a display defined by its port and device
 * parameters.
 * Returns NULL if no GPU was connected to the give port and
 * device or for non exisiting display.
 ****************************************************************/
hwloc_obj_t hwloc_gl_get_gpu_by_display(hwloc_topology_t topology, int port, int device)
{
  char x_display [10];
  hwloc_obj_t display_obj;

  /* Formulate the display string */
  snprintf(x_display, sizeof(x_display), ":%d.%d", port, device);
  display_obj = hwloc_gl_query_display(topology, x_display);

  if (display_obj != NULL)
    return display_obj;
  else
    return NULL;
}

static int
hwloc_gl_backend_notify_new_object(struct hwloc_backend *backend, struct hwloc_backend *caller __hwloc_attribute_unused,
				   struct hwloc_obj *pcidev)
{
  struct hwloc_topology *topology = backend->topology;
  unsigned port, device;
  int err;

  /* Getting the display info [:port.device] */
  err = hwloc_gl_get_gpu_display(topology, pcidev, &port, &device);

  /* If GPU, Appending the display as a children to the GPU
   * and add a display object with the display name */
  if (!err) {
    struct hwloc_obj *obj;
    char display_name[64];
    snprintf(display_name, sizeof(display_name), ":%d.%d", port, device);

    obj = hwloc_alloc_setup_object(HWLOC_OBJ_OS_DEVICE, -1);
    obj->name = strdup(display_name);
    obj->logical_index = -1;
    obj->attr->osdev.type = HWLOC_OBJ_OSDEV_GPU;
    hwloc_insert_object_by_parent(topology, pcidev, obj);
    return 1;
  } else
    return 0;
}

static struct hwloc_backend *
hwloc_gl_component_instantiate(struct hwloc_disc_component *component,
			       const void *_data1 __hwloc_attribute_unused,
			       const void *_data2 __hwloc_attribute_unused,
			       const void *_data3 __hwloc_attribute_unused)
{
  struct hwloc_backend *backend;

  /* thissystem may not be fully initialized yet, we'll check flags in discover() */

  backend = hwloc_backend_alloc(component);
  if (!backend)
    return NULL;

  backend->notify_new_object = hwloc_gl_backend_notify_new_object;
  return backend;
}

static struct hwloc_disc_component hwloc_gl_disc_component = {
  HWLOC_DISC_COMPONENT_TYPE_ADDITIONAL,
  "gl",
  HWLOC_DISC_COMPONENT_TYPE_GLOBAL,
  hwloc_gl_component_instantiate,
  19, /* after libpci */
  NULL
};

#ifdef HWLOC_INSIDE_PLUGIN
HWLOC_DECLSPEC extern const struct hwloc_component hwloc_gl_component;
#endif

const struct hwloc_component hwloc_gl_component = {
  HWLOC_COMPONENT_ABI,
  HWLOC_COMPONENT_TYPE_DISC,
  0,
  &hwloc_gl_disc_component
};

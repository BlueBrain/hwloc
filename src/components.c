/*
 * Copyright © 2009-2012 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>
#include <hwloc.h>
#include <private/private.h>

int
hwloc_component_register(struct hwloc_topology *topology, struct hwloc_component *component)
{
  struct hwloc_component **prev, *new;

  new = malloc(sizeof(*new));
  if (!new)
    return -1;
  memcpy(new, component, sizeof(*new));

  prev = &topology->components;
  while (NULL != *prev) {
    prev = &((*prev)->next);
  }
  *prev = new;
  component->next = NULL;
  return 0;
}

void
hwloc_components_init(struct hwloc_topology *topology)
{
  topology->components = NULL;

#ifdef HWLOC_LINUX_SYS
# define HAVE_OS_SUPPORT
  hwloc_linux_component_register(topology);
#endif
#ifdef HWLOC_AIX_SYS
# define HAVE_OS_SUPPORT
  hwloc_aix_component_register(topology);
#endif /* HWLOC_AIX_SYS */
#ifdef HWLOC_OSF_SYS
# define HAVE_OS_SUPPORT
  hwloc_osf_component_register(topology);
#endif /* HWLOC_OSF_SYS */
#ifdef HWLOC_SOLARIS_SYS
# define HAVE_OS_SUPPORT
  hwloc_solaris_component_register(topology);
#endif /* HWLOC_SOLARIS_SYS */
#ifdef HWLOC_WIN_SYS
# define HAVE_OS_SUPPORT
  hwloc_windows_component_register(topology);
#endif /* HWLOC_WIN_SYS */
#ifdef HWLOC_DARWIN_SYS
# define HAVE_OS_SUPPORT
  hwloc_darwin_component_register(topology);
#endif /* HWLOC_DARWIN_SYS */
#ifdef HWLOC_FREEBSD_SYS
# define HAVE_OS_SUPPORT
  hwloc_freebsd_component_register(topology);
#endif /* HWLOC_FREEBSD_SYS */
#ifdef HWLOC_HPUX_SYS
# define HAVE_OS_SUPPORT
  hwloc_hpux_component_register(topology);
#endif /* HWLOC_HPUX_SYS */
#ifndef HAVE_OS_SUPPORT
  hwloc_noos_component_register(topology);
#endif /* Unsupported OS */

  hwloc_xml_component_register(topology);
  hwloc_synthetic_component_register(topology);
  hwloc_custom_component_register(topology);

#ifdef HWLOC_HAVE_LIBPCI
  hwloc_libpci_component_register(topology);
#endif

  topology->backend = NULL;
  topology->additional_backends = NULL;
}

struct hwloc_component *
hwloc_component_find_next(struct hwloc_topology *topology,
			  int type /* hwloc_component_type_t or -1 if any */,
			  const char *name /* name of NULL if any */,
			  struct hwloc_component *prev)
{
  struct hwloc_component *comp;
  comp = prev ? prev->next : topology->components;
  while (NULL != comp) {
    if ((-1 == type || type == (int) comp->type)
       && (NULL == name || !strcmp(name, comp->name)))
      return comp;
    comp = comp->next;
  }
  return NULL;
}

struct hwloc_component *
hwloc_component_find(struct hwloc_topology *topology,
		     int type /* hwloc_component_type_t or -1 if any */,
		     const char *name /* name of NULL if any */)
{
  return hwloc_component_find_next(topology, type, name, NULL);
}

void
hwloc_components_destroy_all(struct hwloc_topology *topology)
{
  struct hwloc_component *comp = topology->components, *next;
  while (NULL != comp) {
    next = comp->next;
    free(comp);
    comp = next;
  }
  topology->components = NULL;
}

struct hwloc_backend *
hwloc_backend_alloc(struct hwloc_topology *topology __hwloc_attribute_unused,
		    struct hwloc_component *component)
{
  struct hwloc_backend * backend = malloc(sizeof(*backend));
  if (!backend) {
    errno = ENOMEM;
    return NULL;
  }
  backend->component = component;
  backend->discover = NULL;
  backend->get_obj_cpuset = NULL;
  backend->notify_new_object = NULL;
  backend->disable = NULL;
  backend->is_custom = 0;
  backend->priority = 0; /* additional backends have to define this */
  backend->next = NULL;
  return backend;
}

void
hwloc_backend_enable(struct hwloc_topology *topology, struct hwloc_backend *backend)
{
  switch (backend->component->type) {

  case HWLOC_COMPONENT_TYPE_OS:
  case HWLOC_COMPONENT_TYPE_GLOBAL:
    if (NULL != topology->backend) {
      /* only one base/global backend simultaneously */
      if (topology->backend->disable)
	topology->backend->disable(topology, topology->backend);
      free(topology->backend);
      if (topology->is_loaded) {
	hwloc_topology_clear(topology);
	hwloc_distances_destroy(topology);
	hwloc_topology_setup_defaults(topology);
	topology->is_loaded = 0;
      }
    }
    topology->backend = backend;
    break;

  case HWLOC_COMPONENT_TYPE_ADDITIONAL: {
    struct hwloc_backend **pprev = &topology->additional_backends;
    while (NULL != *pprev) {
      if ((*pprev)->priority < backend->priority)
        break;
      pprev = &((*pprev)->next);
    }
    backend->next = *pprev;
    *pprev = backend;
    break;
  }

  default:
    assert(0);
  }
}

int
hwloc_backends_notify_new_object(struct hwloc_topology *topology, struct hwloc_obj *obj)
{
  struct hwloc_backend *backend;
  int res = 0;

  backend = topology->backend;
  if (backend->notify_new_object)
    res += backend->notify_new_object(topology, obj);

  backend = topology->additional_backends;
  while (NULL != backend) {
    if (backend->notify_new_object)
      res += backend->notify_new_object(topology, obj);
    backend = backend->next;
  }

  return res;
}

void
hwloc_backends_disable_all(struct hwloc_topology *topology)
{
  struct hwloc_backend *backend;

  if (NULL != (backend = topology->backend)) {
    assert(NULL == backend->next);
    if (backend->disable)
      backend->disable(topology, backend);
    free(backend);
    topology->backend = NULL;
  }

  while (NULL != (backend = topology->additional_backends)) {
    struct hwloc_backend *next = backend->next;
    if (backend->disable)
     backend->disable(topology, backend);
    free(backend);
    topology->additional_backends = next;
  }
}

/*
 * Copyright © 2012 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#ifndef PRIVATE_COMPONENTS_H
#define PRIVATE_COMPONENTS_H 1

struct hwloc_backend;

/**************
 * Components *
 **************/

typedef enum hwloc_component_type_e {
  HWLOC_COMPONENT_TYPE_OS, /* OS backend, and no-OS support.
		       * that's where we take hooks from when is_this_system=1.
		       */
  HWLOC_COMPONENT_TYPE_GLOBAL, /* xml, synthetic or custom.
			   * no additional backend is used.
			   */
  HWLOC_COMPONENT_TYPE_ADDITIONAL, /* pci, etc.
			       */
  /* This value is only here so that we can end the enum list without
     a comma (thereby preventing compiler warnings) */
  HWLOC_COMPONENT_TYPE_MAX
} hwloc_component_type_t;

typedef void (*hwloc_component_init_fn_t)(void);

struct hwloc_component {
  hwloc_component_type_t type;
  const char *name;
  int (*instantiate)(struct hwloc_topology *topology, struct hwloc_component *component, const void *data1, const void *data2, const void *data3);
  void (*set_hooks)(struct hwloc_topology *topology); /* only used if HWLOC_COMPONENT_TYPE_OS */

  unsigned priority; /* used to sort topology->components and topology->additional_backends, higher priority first */
  struct hwloc_component * next; /* used internally to list components by priority on topology->components */
};

HWLOC_DECLSPEC int hwloc_component_register(struct hwloc_component *component);
extern void hwloc_components_init(struct hwloc_topology *topology); /* increases plugins refcount, should be called exactly once per topology (during init) */
extern void hwloc_components_destroy_all(struct hwloc_topology *topology); /* decreases plugins refcount, should be called exactly once per topology (during destroy) */
extern struct hwloc_component * hwloc_component_find(int type, const char *name);
extern struct hwloc_component * hwloc_component_find_next(int type, const char *name, struct hwloc_component *prev);

/************
 * Backends *
 ************/

struct hwloc_backend {
  struct hwloc_component * component;

  /* main discovery callback.
   * returns > 0 if it modified the topology tree, -1 on error, 0 otherwise.
   * maybe NULL if type is HWLOC_COMPONENT_TYPE_ADDITIONAL. */
  int (*discover)(struct hwloc_topology *topology);

  /* used by the libpci backend to retrieve pci device locality from the OS backend */
  int (*get_obj_cpuset)(struct hwloc_topology *topology, struct hwloc_obj *obj, hwloc_bitmap_t cpuset); /* may be NULL */

  /* used by additional backends to notify other backend when new objects are added.
   * returns > 0 if it modified the topology tree, 0 otherwise. */
  int (*notify_new_object)(struct hwloc_topology *topology, struct hwloc_obj *obj); /* may be NULL */

  void (*disable)(struct hwloc_topology *topology, struct hwloc_backend *backend); /* may be NULL */
  void * private_data;
  int is_custom; /* shortcut on !strcmp(..->component->name, "custom") */

  struct hwloc_backend * next; /* used internally to list additional backends by priority on topology->additional_backends.
				* unused (NULL) for other backends (on topology->backend).
				*/
};

HWLOC_DECLSPEC struct hwloc_backend * hwloc_backend_alloc(struct hwloc_topology *topology, struct hwloc_component *component);
HWLOC_DECLSPEC void hwloc_backend_enable(struct hwloc_topology *topology, struct hwloc_backend *backend);
extern void hwloc_backends_disable_all(struct hwloc_topology *topology);
HWLOC_DECLSPEC int hwloc_backends_notify_new_object(struct hwloc_topology *topology, struct hwloc_obj *obj);

/***********
 * Plugins *
 ***********/

#ifdef HWLOC_HAVE_PLUGINS

#define HWLOC_PLUGIN_ABI 1

struct hwloc_plugin {
  unsigned abi;
  hwloc_component_init_fn_t init;
};

#endif /* HWLOC_HAVE_PLUGINS */

/****************************************
 * Misc component registration routines *
 ****************************************/

#if defined(HWLOC_LINUX_SYS)
extern void hwloc_core_linux_component_register(void);
#endif /* HWLOC_LINUX_SYS */

extern void hwloc_core_xml_component_register(void);

#ifdef HWLOC_SOLARIS_SYS
extern void hwloc_core_solaris_component_register(void);
#endif /* HWLOC_SOLARIS_SYS */

#ifdef HWLOC_AIX_SYS
extern void hwloc_core_aix_component_register(void);
#endif /* HWLOC_AIX_SYS */

#ifdef HWLOC_OSF_SYS
extern void hwloc_core_osf_component_register(void);
#endif /* HWLOC_OSF_SYS */

#ifdef HWLOC_WIN_SYS
extern void hwloc_core_windows_component_register(void);
#endif /* HWLOC_WIN_SYS */

#ifdef HWLOC_DARWIN_SYS
extern void hwloc_core_darwin_component_register(void);
#endif /* HWLOC_DARWIN_SYS */

#ifdef HWLOC_FREEBSD_SYS
extern void hwloc_core_freebsd_component_register(void);
#endif /* HWLOC_FREEBSD_SYS */

#ifdef HWLOC_HPUX_SYS
extern void hwloc_core_hpux_component_register(void);
#endif /* HWLOC_HPUX_SYS */

#ifdef HWLOC_HAVE_LIBPCI
extern void hwloc_core_libpci_component_register(void);
#endif /* HWLOC_HAVE_LIBPCI */

extern void hwloc_core_synthetic_component_register(void);

extern void hwloc_core_noos_component_register(void);

extern void hwloc_core_custom_component_register(void);


extern void hwloc_xml_nolibxml_component_register(void);
#ifdef HWLOC_HAVE_LIBXML2
extern void hwloc_xml_libxml_component_register(void);
#endif

#endif /* PRIVATE_COMPONENTS_H */


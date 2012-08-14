/*
 * Copyright © 2009      CNRS
 * Copyright © 2009-2011 inria.  All rights reserved.
 * Copyright © 2009-2012 Université Bordeaux 1
 * Copyright © 2009-2011 Cisco Systems, Inc.  All rights reserved.
 *
 * See COPYING in top-level directory.
 */

/* Internal types and helpers. */

#ifndef HWLOC_PRIVATE_H
#define HWLOC_PRIVATE_H

#include <private/autogen/config.h>
#include <hwloc.h>
#include <hwloc/bitmap.h>
#include <private/debug.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#include <string.h>

#ifdef HWLOC_HAVE_ATTRIBUTE_FORMAT
# if HWLOC_HAVE_ATTRIBUTE_FORMAT
#  define __hwloc_attribute_format(type, str, arg)  __attribute__((__format__(type, str, arg)))
# else
#  define __hwloc_attribute_format(type, str, arg)
# endif
#else
# define __hwloc_attribute_format(type, str, arg)
#endif

enum hwloc_ignore_type_e {
  HWLOC_IGNORE_TYPE_NEVER = 0,
  HWLOC_IGNORE_TYPE_KEEP_STRUCTURE,
  HWLOC_IGNORE_TYPE_ALWAYS
};

#define HWLOC_DEPTH_MAX 128

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

struct hwloc_backend;

struct hwloc_component {
  hwloc_component_type_t type;
  const char *name;
  int (*instantiate)(struct hwloc_topology *topology, struct hwloc_component *component, const void *data1, const void *data2, const void *data3);
  void (*set_hooks)(struct hwloc_topology *topology); /* only used if HWLOC_COMPONENT_TYPE_OS */
  struct hwloc_component * next; /* hwloc internal use only */
};

extern int hwloc_component_register(struct hwloc_topology *topology, struct hwloc_component *component);
extern void hwloc_components_register_all(struct hwloc_topology *topology);
extern void hwloc_components_destroy_all(struct hwloc_topology *topology);
extern struct hwloc_component * hwloc_find_component(struct hwloc_topology *topology, int type, const char *name);

struct hwloc_backend {
  struct hwloc_component * component;

  /* main discovery callback.
   * returns > 0 if it modified the topology tree, -1 on error, 0 otherwise. */
  int (*discover)(struct hwloc_topology *topology);

  /* used by the libpci backend to retrieve pci device locality from the OS backend */
  int (*get_obj_cpuset)(struct hwloc_topology *topology, struct hwloc_obj *obj, hwloc_bitmap_t cpuset); /* may be NULL */

  /* used by additional backends to notify other backend when new objects are added */
  void (*notify_new_object)(struct hwloc_topology *topology, struct hwloc_obj *obj); /* may be NULL */

  void (*disable)(struct hwloc_topology *topology, struct hwloc_backend *backend); /* may be NULL */
  void * private_data;
  int is_custom; /* shortcut on !strcmp(..->component->name, "custom") */
  struct hwloc_backend * next;
};

extern struct hwloc_backend * hwloc_backend_alloc(struct hwloc_topology *topology, struct hwloc_component *component);
extern void hwloc_backend_enable(struct hwloc_topology *topology, struct hwloc_backend *backend);
extern void hwloc_backends_disable_all(struct hwloc_topology *topology);

struct hwloc__xml_import_state_s;

struct hwloc_topology {
  unsigned nb_levels;					/* Number of horizontal levels */
  unsigned next_group_depth;				/* Depth of the next Group object that we may create */
  unsigned level_nbobjects[HWLOC_DEPTH_MAX]; 		/* Number of objects on each horizontal level */
  struct hwloc_obj **levels[HWLOC_DEPTH_MAX];		/* Direct access to levels, levels[l = 0 .. nblevels-1][0..level_nbobjects[l]] */
  unsigned long flags;
  int type_depth[HWLOC_OBJ_TYPE_MAX];
  enum hwloc_ignore_type_e ignored_types[HWLOC_OBJ_TYPE_MAX];
  int is_thissystem;
  int is_loaded;
  hwloc_pid_t pid;                                      /* Process ID the topology is view from, 0 for self */

  unsigned bridge_nbobjects;
  struct hwloc_obj **bridge_level;
  struct hwloc_obj *first_bridge, *last_bridge;
  unsigned pcidev_nbobjects;
  struct hwloc_obj **pcidev_level;
  struct hwloc_obj *first_pcidev, *last_pcidev;
  unsigned osdev_nbobjects;
  struct hwloc_obj **osdev_level;
  struct hwloc_obj *first_osdev, *last_osdev;

  int (*set_thisproc_cpubind)(hwloc_topology_t topology, hwloc_const_cpuset_t set, int flags);
  int (*get_thisproc_cpubind)(hwloc_topology_t topology, hwloc_cpuset_t set, int flags);
  int (*set_thisthread_cpubind)(hwloc_topology_t topology, hwloc_const_cpuset_t set, int flags);
  int (*get_thisthread_cpubind)(hwloc_topology_t topology, hwloc_cpuset_t set, int flags);
  int (*set_proc_cpubind)(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_const_cpuset_t set, int flags);
  int (*get_proc_cpubind)(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_cpuset_t set, int flags);
#ifdef hwloc_thread_t
  int (*set_thread_cpubind)(hwloc_topology_t topology, hwloc_thread_t tid, hwloc_const_cpuset_t set, int flags);
  int (*get_thread_cpubind)(hwloc_topology_t topology, hwloc_thread_t tid, hwloc_cpuset_t set, int flags);
#endif

  int (*get_thisproc_last_cpu_location)(hwloc_topology_t topology, hwloc_cpuset_t set, int flags);
  int (*get_thisthread_last_cpu_location)(hwloc_topology_t topology, hwloc_cpuset_t set, int flags);
  int (*get_proc_last_cpu_location)(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_cpuset_t set, int flags);

  int (*set_thisproc_membind)(hwloc_topology_t topology, hwloc_const_nodeset_t nodeset, hwloc_membind_policy_t policy, int flags);
  int (*get_thisproc_membind)(hwloc_topology_t topology, hwloc_nodeset_t nodeset, hwloc_membind_policy_t * policy, int flags);
  int (*set_thisthread_membind)(hwloc_topology_t topology, hwloc_const_nodeset_t nodeset, hwloc_membind_policy_t policy, int flags);
  int (*get_thisthread_membind)(hwloc_topology_t topology, hwloc_nodeset_t nodeset, hwloc_membind_policy_t * policy, int flags);
  int (*set_proc_membind)(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_const_nodeset_t nodeset, hwloc_membind_policy_t policy, int flags);
  int (*get_proc_membind)(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_nodeset_t nodeset, hwloc_membind_policy_t * policy, int flags);
  int (*set_area_membind)(hwloc_topology_t topology, const void *addr, size_t len, hwloc_const_nodeset_t nodeset, hwloc_membind_policy_t policy, int flags);
  int (*get_area_membind)(hwloc_topology_t topology, const void *addr, size_t len, hwloc_nodeset_t nodeset, hwloc_membind_policy_t * policy, int flags);
  /* This has to return the same kind of pointer as alloc_membind, so that free_membind can be used on it */
  void *(*alloc)(hwloc_topology_t topology, size_t len);
  /* alloc_membind has to always succeed if !(flags & HWLOC_MEMBIND_STRICT).
   * see hwloc_alloc_or_fail which is convenient for that.  */
  void *(*alloc_membind)(hwloc_topology_t topology, size_t len, hwloc_const_nodeset_t nodeset, hwloc_membind_policy_t policy, int flags);
  int (*free_membind)(hwloc_topology_t topology, void *addr, size_t len);

  struct hwloc_topology_support support;

  struct hwloc_os_distances_s {
    hwloc_obj_type_t type;
    int nbobjs;
    unsigned *indexes; /* array of OS indexes before we can convert them into objs. always available.
			*/
    struct hwloc_obj **objs; /* array of objects, in the same order as above.
			      * either given (by a backend) together with the indexes array above.
			      * or build from the above indexes array when not given (by the user).
			      */
    float *distances; /* distance matrices, ordered according to the above indexes/objs array.
		       * distance from i to j is stored in slot i*nbnodes+j.
		       * will be copied into the main logical-index-ordered distance at the end of the discovery.
		       */
    int forced; /* set if the user forced a matrix to ignore the OS one */

    struct hwloc_os_distances_s *prev, *next;
  } *first_osdist, *last_osdist;

  struct hwloc_component * components;
  struct hwloc_backend * backend;
};


extern void hwloc_alloc_obj_cpusets(hwloc_obj_t obj);
extern void hwloc_setup_pu_level(struct hwloc_topology *topology, unsigned nb_pus);
extern int hwloc_get_sysctlbyname(const char *name, int64_t *n);
extern int hwloc_get_sysctl(int name[], unsigned namelen, int *n);
extern unsigned hwloc_fallback_nbprocessors(struct hwloc_topology *topology);
extern void hwloc_connect_children(hwloc_obj_t obj);
extern int hwloc_connect_levels(hwloc_topology_t topology);

extern void hwloc_topology_setup_defaults(struct hwloc_topology *topology);
extern void hwloc_topology_clear(struct hwloc_topology *topology);

#if defined(HWLOC_LINUX_SYS)
extern void hwloc_linux_component_register(struct hwloc_topology *topology);
#endif /* HWLOC_LINUX_SYS */

extern void hwloc_xml_component_register(struct hwloc_topology *topology);

#ifdef HWLOC_SOLARIS_SYS
extern void hwloc_solaris_component_register(struct hwloc_topology *topology);
#endif /* HWLOC_SOLARIS_SYS */

#ifdef HWLOC_AIX_SYS
extern void hwloc_aix_component_register(struct hwloc_topology *topology);
#endif /* HWLOC_AIX_SYS */

#ifdef HWLOC_OSF_SYS
extern void hwloc_osf_component_register(struct hwloc_topology *topology);
#endif /* HWLOC_OSF_SYS */

#ifdef HWLOC_WIN_SYS
extern void hwloc_windows_component_register(struct hwloc_topology *topology);
#endif /* HWLOC_WIN_SYS */

#ifdef HWLOC_DARWIN_SYS
extern void hwloc_darwin_component_register(struct hwloc_topology *topology);
#endif /* HWLOC_DARWIN_SYS */

#ifdef HWLOC_FREEBSD_SYS
extern void hwloc_freebsd_component_register(struct hwloc_topology *topology);
#endif /* HWLOC_FREEBSD_SYS */

#ifdef HWLOC_HPUX_SYS
extern void hwloc_hpux_component_register(struct hwloc_topology *topology);
#endif /* HWLOC_HPUX_SYS */

extern void hwloc_look_x86(struct hwloc_topology *topology, unsigned nbprocs);

#ifdef HWLOC_HAVE_LIBPCI
extern void hwloc_look_libpci(struct hwloc_topology *topology);
#endif /* HWLOC_HAVE_LIBPCI */

extern void hwloc_synthetic_component_register(struct hwloc_topology *topology);

extern void hwloc_noos_component_register(struct hwloc_topology *topology);

extern void hwloc_custom_component_register(struct hwloc_topology *topology);

/*
 * Add an object to the topology.
 * It is sorted along the tree of other objects according to the inclusion of
 * cpusets, to eventually be added as a child of the smallest object including
 * this object.
 *
 * If the cpuset is empty, the type of the object (and maybe some attributes)
 * must be enough to find where to insert the object. This is especially true
 * for NUMA nodes with memory and no CPUs.
 *
 * The given object should not have children.
 *
 * This shall only be called before levels are built.
 *
 * In case of error, hwloc_report_os_error() is called.
 */
extern void hwloc_insert_object_by_cpuset(struct hwloc_topology *topology, hwloc_obj_t obj);

/* Error reporting */
typedef void (*hwloc_report_error_t)(const char * msg, int line);
extern void hwloc_report_os_error(const char * msg, int line);
extern int hwloc_hide_errors(void);
/*
 * Add an object to the topology and specify which error callback to use
 */
extern int hwloc__insert_object_by_cpuset(struct hwloc_topology *topology, hwloc_obj_t obj, hwloc_report_error_t report_error);

/*
 * Insert an object somewhere in the topology.
 *
 * It is added as the last child of the given parent.
 * The cpuset is completely ignored, so strange objects such as I/O devices should
 * preferably be inserted with this.
 *
 * The given object may have children.
 *
 * Remember to call topology_connect() afterwards to fix handy pointers.
 */
extern void hwloc_insert_object_by_parent(struct hwloc_topology *topology, hwloc_obj_t parent, hwloc_obj_t obj);

/* Insert uname-specific names/values in the object infos array */
extern void hwloc_add_uname_info(struct hwloc_topology *topology);

#ifdef HWLOC_INSIDE_LIBHWLOC
/** \brief Return a locally-allocated stringified bitmap for printf-like calls. */
static __hwloc_inline char *
hwloc_bitmap_printf_value(hwloc_const_bitmap_t bitmap)
{
  char *buf;
  hwloc_bitmap_asprintf(&buf, bitmap);
  return buf;
}

static __hwloc_inline struct hwloc_obj *
hwloc_alloc_setup_object(hwloc_obj_type_t type, signed idx)
{
  struct hwloc_obj *obj = malloc(sizeof(*obj));
  memset(obj, 0, sizeof(*obj));
  obj->type = type;
  obj->os_index = idx;
  obj->os_level = -1;
  obj->attr = malloc(sizeof(*obj->attr));
  memset(obj->attr, 0, sizeof(*obj->attr));
  /* do not allocate the cpuset here, let the caller do it */
  return obj;
}
#endif

/* Free obj and its attributes assuming it doesn't have any children/parent anymore */
extern void hwloc_free_unlinked_object(hwloc_obj_t obj);

/* Duplicate src and its children under newparent in newtopology */
extern void hwloc__duplicate_objects(struct hwloc_topology *newtopology, struct hwloc_obj *newparent, struct hwloc_obj *src);

/* This can be used for the alloc field to get allocated data that can be freed by free() */
void *hwloc_alloc_heap(hwloc_topology_t topology, size_t len);

/* This can be used for the alloc field to get allocated data that can be freed by munmap() */
void *hwloc_alloc_mmap(hwloc_topology_t topology, size_t len);

/* This can be used for the free_membind field to free data using free() */
int hwloc_free_heap(hwloc_topology_t topology, void *addr, size_t len);

/* This can be used for the free_membind field to free data using munmap() */
int hwloc_free_mmap(hwloc_topology_t topology, void *addr, size_t len);

/* Allocates unbound memory or fail, depending on whether STRICT is requested
 * or not */
static __hwloc_inline void *
hwloc_alloc_or_fail(hwloc_topology_t topology, size_t len, int flags)
{
  if (flags & HWLOC_MEMBIND_STRICT)
    return NULL;
  return hwloc_alloc(topology, len);
}

extern void hwloc_distances_init(struct hwloc_topology *topology);
extern void hwloc_distances_clear(struct hwloc_topology *topology);
extern void hwloc_distances_destroy(struct hwloc_topology *topology);
extern void hwloc_distances_set(struct hwloc_topology *topology, hwloc_obj_type_t type, unsigned nbobjs, unsigned *indexes, hwloc_obj_t *objs, float *distances, int force);
extern void hwloc_distances_set_from_env(struct hwloc_topology *topology);
extern void hwloc_distances_restrict_os(struct hwloc_topology *topology);
extern void hwloc_distances_restrict(struct hwloc_topology *topology, unsigned long flags);
extern void hwloc_distances_finalize_os(struct hwloc_topology *topology);
extern void hwloc_distances_finalize_logical(struct hwloc_topology *topology);
extern void hwloc_clear_object_distances(struct hwloc_obj *obj);
extern void hwloc_clear_object_distances_one(struct hwloc_distances_s *distances);
extern void hwloc_group_by_distances(struct hwloc_topology *topology);

#ifdef HAVE_USELOCALE
#include "locale.h"
#ifdef HAVE_XLOCALE_H
#include "xlocale.h"
#endif
#define hwloc_localeswitch_declare locale_t __old_locale = (locale_t)0, __new_locale
#define hwloc_localeswitch_init() do {                     \
  __new_locale = newlocale(LC_ALL_MASK, "C", (locale_t)0); \
  if (__new_locale != (locale_t)0)                         \
    __old_locale = uselocale(__new_locale);                \
} while (0)
#define hwloc_localeswitch_fini() do { \
  if (__new_locale != (locale_t)0) {   \
    uselocale(__old_locale);           \
    freelocale(__new_locale);          \
  }                                    \
} while(0)
#else /* HAVE_USELOCALE */
#define hwloc_localeswitch_declare int __dummy_nolocale __hwloc_attribute_unused
#define hwloc_localeswitch_init()
#define hwloc_localeswitch_fini()
#endif /* HAVE_USELOCALE */

#if !HAVE_DECL_FABSF
#define fabsf(f) fabs((double)(f))
#endif

#if HAVE_DECL__SC_PAGE_SIZE
#define hwloc_getpagesize() sysconf(_SC_PAGE_SIZE)
#elif HAVE_DECL__SC_PAGESIZE
#define hwloc_getpagesize() sysconf(_SC_PAGESIZE)
#elif defined HAVE_GETPAGESIZE
#define hwloc_getpagesize() getpagesize()
#else
#undef hwloc_getpagesize
#endif

#endif /* HWLOC_PRIVATE_H */

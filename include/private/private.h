/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

/* Internal types and helpers. */

#ifndef HWLOC_PRIVATE_H
#define HWLOC_PRIVATE_H

#include <private/config.h>
#include <hwloc.h>
#include <hwloc/cpuset.h>
#include <private/debug.h>

#include <assert.h>
#include <string.h>

enum hwloc_ignore_type_e {
  HWLOC_IGNORE_TYPE_NEVER = 0,
  HWLOC_IGNORE_TYPE_KEEP_STRUCTURE,
  HWLOC_IGNORE_TYPE_ALWAYS,
};

/* Maximal value of an object type */
#define HWLOC_OBJ_TYPE_MAX (HWLOC_OBJ_MISC+1)
#define HWLOC_DEPTH_MAX 128

typedef enum hwloc_backend_e {
  HWLOC_BACKEND_NONE,
  HWLOC_BACKEND_SYNTHETIC,
#ifdef LINUX_SYS
  HWLOC_BACKEND_SYSFS,
#endif
#ifdef HAVE_XML
  HWLOC_BACKEND_XML,
#endif
} hwloc_backend_t;

struct hwloc_topology {
  unsigned nb_levels;					/* Number of horizontal levels */
  unsigned level_nbobjects[HWLOC_DEPTH_MAX]; 		/* Number of objects on each horizontal level */
  struct hwloc_obj **levels[HWLOC_DEPTH_MAX];		/* Direct access to levels, levels[l = 0 .. nblevels-1][0..level_nbobjects[l]] */
  unsigned long flags;
  int type_depth[HWLOC_OBJ_TYPE_MAX];
  enum hwloc_ignore_type_e ignored_types[HWLOC_OBJ_TYPE_MAX];
  int is_thissystem;
  int is_loaded;

  int (*set_cpubind)(hwloc_topology_t topology, hwloc_cpuset_t set, int strict);
  int (*set_thisproc_cpubind)(hwloc_topology_t topology, hwloc_cpuset_t set, int strict);
  int (*set_thisthread_cpubind)(hwloc_topology_t topology, hwloc_cpuset_t set, int strict);
  int (*set_proc_cpubind)(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_cpuset_t set, int strict);
#ifdef hwloc_thread_t
  int (*set_thread_cpubind)(hwloc_topology_t topology, hwloc_thread_t tid, hwloc_cpuset_t set, int strict);
#endif

  hwloc_backend_t backend_type;
  union hwloc_backend_params_u {
#ifdef LINUX_SYS
    struct hwloc_backend_params_sysfs_s {
      /* sysfs backend parameters */
      int root_fd; /* The file descriptor for the file system root, used when browsing, e.g., Linux' sysfs and procfs. */
    } sysfs;
#endif /* LINUX_SYS */
#if defined(OSF_SYS) || defined(HWLOC_COMPILE_PORTS)
    struct hwloc_backend_params_osf {
      int nbnodes;
    } osf;
#endif /* OSF_SYS */
#ifdef HAVE_XML
    struct hwloc_backend_params_xml_s {
      /* xml backend parameters */
      void *doc;
    } xml;
#endif /* HAVE_XML */
    struct hwloc_backend_params_synthetic_s {
      /* synthetic backend parameters */
#define HWLOC_SYNTHETIC_MAX_DEPTH 128
      unsigned arity[HWLOC_SYNTHETIC_MAX_DEPTH];
      hwloc_obj_type_t type[HWLOC_SYNTHETIC_MAX_DEPTH];
      unsigned id[HWLOC_SYNTHETIC_MAX_DEPTH];
      unsigned depth[HWLOC_SYNTHETIC_MAX_DEPTH]; /* For cache/misc */
    } synthetic;
  } backend_params;
};


extern void hwloc_setup_proc_level(struct hwloc_topology *topology, unsigned nb_processors, hwloc_cpuset_t online_cpuset);
extern void hwloc_setup_misc_level_from_distances(struct hwloc_topology *topology, unsigned nbobjs, struct hwloc_obj *objs[nbobjs], unsigned distances[nbobjs][nbobjs]);
extern unsigned hwloc_fallback_nbprocessors(void);

#if defined(LINUX_SYS)
extern void hwloc_look_linux(struct hwloc_topology *topology);
extern void hwloc_set_linux_hooks(struct hwloc_topology *topology);
extern int hwloc_backend_sysfs_init(struct hwloc_topology *topology, const char *fsroot_path);
extern void hwloc_backend_sysfs_exit(struct hwloc_topology *topology);
#endif /* LINUX_SYS */

#ifdef HAVE_XML
extern int hwloc_backend_xml_init(struct hwloc_topology *topology, const char *xmlpath);
extern void hwloc_look_xml(struct hwloc_topology *topology);
extern void hwloc_backend_xml_exit(struct hwloc_topology *topology);
#endif /* HAVE_XML */

#ifdef SOLARIS_SYS
extern void hwloc_look_solaris(struct hwloc_topology *topology);
extern void hwloc_set_solaris_hooks(struct hwloc_topology *topology);
#endif /* SOLARIS_SYS */

#ifdef AIX_SYS
extern void hwloc_look_aix(struct hwloc_topology *topology);
extern void hwloc_set_aix_hooks(struct hwloc_topology *topology);
#endif /* AIX_SYS */

#ifdef OSF_SYS
extern void hwloc_look_osf(struct hwloc_topology *topology);
extern void hwloc_set_osf_hooks(struct hwloc_topology *topology);
#endif /* OSF_SYS */

#ifdef WIN_SYS
extern void hwloc_look_windows(struct hwloc_topology *topology);
extern void hwloc_set_windows_hooks(struct hwloc_topology *topology);
#endif /* WIN_SYS */

#ifdef DARWIN_SYS
extern void hwloc_look_darwin(struct hwloc_topology *topology);
extern void hwloc_set_darwin_hooks(struct hwloc_topology *topology);
#endif /* DARWIN_SYS */

#ifdef HPUX_SYS
extern void hwloc_look_hpux(struct hwloc_topology *topology);
extern void hwloc_set_hpux_hooks(struct hwloc_topology *topology);
#endif /* HPUX_SYS */

extern int hwloc_backend_synthetic_init(struct hwloc_topology *topology, const char *description);
extern void hwloc_backend_synthetic_exit(struct hwloc_topology *topology);
extern void hwloc_look_synthetic (struct hwloc_topology *topology);

extern void hwloc_add_object(struct hwloc_topology *topology, hwloc_obj_t obj);


/** \brief Return a locally-allocated stringified cpuset for printf-like calls. */
static inline char *
hwloc_cpuset_printf_value(hwloc_cpuset_t cpuset)
{
  char *buf;
  hwloc_cpuset_asprintf(&buf, cpuset);
  return buf;
}

static inline struct hwloc_obj *
hwloc_alloc_setup_object(hwloc_obj_type_t type, signed index)
{
  struct hwloc_obj *obj = malloc(sizeof(*obj));
  assert(obj);
  memset(obj, 0, sizeof(*obj));
  obj->type = type;
  obj->os_index = index;
  obj->os_level = -1;
  obj->attr = malloc(sizeof(*obj->attr));
  /* do not allocate the cpuset here, let the caller do it */
  return obj;
}

#define hwloc_object_cpuset_from_array(l, _value, _array, _max) do {	\
		struct hwloc_obj *__l = (l);				\
		unsigned int *__a = (_array);				\
		int k;							\
		__l->cpuset = hwloc_cpuset_alloc();			\
		for(k=0; k<_max; k++)					\
			if (__a[k] == _value)				\
				hwloc_cpuset_set(__l->cpuset, k);	\
	} while (0)

/* Configures an array of NUM objects of type TYPE with physical IDs OSPHYSIDS
 * and for which processors have ID PROC_PHYSIDS, and add them to the topology.
 * */
static __inline void
hwloc_setup_level(int procid_max, unsigned num, unsigned *osphysids, unsigned *proc_physids, struct hwloc_topology *topology, hwloc_obj_type_t type)
{
  struct hwloc_obj *obj;
  int j;

  hwloc_debug("%d %s\n", num, hwloc_obj_type_string(type));

  for (j = 0; j < num; j++)
    {
      obj = hwloc_alloc_setup_object(type, osphysids[j]);
      hwloc_object_cpuset_from_array(obj, j, proc_physids, procid_max);
      hwloc_debug_2args_cpuset("%s %d has cpuset %s\n",
		 hwloc_obj_type_string(type),
		 j, obj->cpuset);
      hwloc_add_object(topology, obj);
    }
  hwloc_debug("\n");
}

#endif /* HWLOC_PRIVATE_H */

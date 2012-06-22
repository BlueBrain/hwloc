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
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#include <string.h>
#include <hwloc/topology.h>

#ifdef HWLOC_HAVE_ATTRIBUTE_FORMAT
# if HWLOC_HAVE_ATTRIBUTE_FORMAT
#  define __hwloc_attribute_format(type, str, arg)  __attribute__((__format__(type, str, arg)))
# else
#  define __hwloc_attribute_format(type, str, arg)
# endif
#else
# define __hwloc_attribute_format(type, str, arg)
#endif

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
#define hwloc_localeswitch_declare locale_t __old_locale, __new_locale
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

#endif /* HWLOC_PRIVATE_H */

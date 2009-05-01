/* Copyright 2009 INRIA, Université Bordeaux 1  */

/* Miscellaneous topology helpers to be used the libtopology's core.  */

#ifndef LIBTOPOLOGY_HELPER_H
#define LIBTOPOLOGY_HELPER_H

#include <config.h>
#include <libtopology/private.h>


#if defined(LINUX_SYS) || defined(HAVE_LIBKSTAT)
extern void lt_setup_die_level(int procid_max, unsigned numdies, unsigned *osphysids, unsigned *proc_physids, topo_topology_t topology);
extern void lt_setup_core_level(int procid_max, unsigned numcores, unsigned *oscoreids, unsigned *proc_coreids, topo_topology_t topology);
#endif /* LINUX_SYS || HAVE_LIBKSTAT */
#if defined(LINUX_SYS)
extern void lt_setup_cache_level(int cachelevel, enum topo_level_type_e topotype, int procid_max, unsigned *numcaches, unsigned *cacheids, unsigned long *cachesizes, topo_topology_t topology);
extern void look_linux(struct topo_topology *topology);
extern int lt_set_fsys_root(struct topo_topology *topology, const char *fsys_root_path);
#endif /* LINUX_SYS */

#ifdef HAVE_LIBLGRP
extern void look_lgrp(topo_topology_t topology);
#endif /* HAVE_LIBLGRP */
#ifdef HAVE_LIBKSTAT
extern void look_kstat(topo_topology_t topology);
#endif /* HAVE_LIBKSTAT */

#ifdef AIX_SYS
extern void look_aix(struct topo_topology *topology);
#endif /* AIX_SYS */

#ifdef OSF_SYS
extern void look_osf(struct topo_topology *topology);
#endif /* OSF_SYS */

#ifdef WIN_SYS
extern void look_windows(struct topo_topology *topology);
#endif /* WIN_SYS */

#ifdef __GLIBC__
#if (__GLIBC__ > 2) || (__GLIBC_MINOR__ >= 4)
# define HAVE_OPENAT
#endif
#endif


#define lt_set_empty_os_numbers(l) do { \
		struct topo_level *___l = (l); \
		int i; \
		for(i=0; i<TOPO_LEVEL_MAX; i++) \
		  ___l->physical_index[i] = -1; \
	} while(0)

#define lt_set_os_numbers(l, _type, _val) do { \
		struct topo_level *__l = (l); \
		lt_set_empty_os_numbers(l); \
		__l->physical_index[_type] = _val; \
	} while(0)

#define lt_setup_level(l, _type) do {	       \
		struct topo_level *__l = (l);    \
		__l->type = _type;		\
		topo_cpuset_zero(&__l->cpuset);	\
		__l->arity = 0;			\
		__l->children = NULL;		\
		__l->father = NULL;		\
		__l->admin_disabled = 0;	\
	} while (0)


#define lt_setup_machine_level(l) do {					\
		struct topo_level **__p = (l);                          \
		struct topo_level *__l1 = &(__p[0][0]);			\
		struct topo_level *__l2 = &(__p[0][1]);			\
		lt_setup_level(__l1, TOPO_LEVEL_MACHINE);		\
		__l1->level = 0;					\
		__l1->number = 0;					\
		__l1->index = 0;					\
		lt_set_empty_os_numbers(__l1);                          \
		__l1->physical_index[TOPO_LEVEL_MACHINE] = 0;		\
		topo_cpuset_fill(&__l1->cpuset);			\
		topo_cpuset_zero(&__l2->cpuset);			\
  } while (0)


#define lt_level_cpuset_from_array(l, _value, _array, _max) do { \
		struct topo_level *__l = (l); \
		unsigned int *__a = (_array); \
		int k; \
		for(k=0; k<_max; k++) \
			if (__a[k] == _value) \
				topo_cpuset_set(&__l->cpuset, k); \
	} while (0)


#endif /* LIBTOPOLOGY_HELPER_H */

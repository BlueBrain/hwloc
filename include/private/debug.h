/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

/* The configuration file */

#ifndef HWLOC_DEBUG_H
#define HWLOC_DEBUG_H

#include <private/config.h>

#ifdef TOPO_DEBUG
#define topo_debug(s, ...) fprintf(stderr, s, ##__VA_ARGS__) 
#else
#define topo_debug(s, ...) do { }while(0)
#endif

#endif /* HWLOC_DEBUG_H */

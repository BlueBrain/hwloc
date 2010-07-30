/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <private/config.h>
#include <hwloc.h>
#include <private/private.h>
#include <hwloc/helper.h>
#ifdef HAVE_MALLOC_H
#  include <malloc.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

/* TODO: HWLOC_GNU_SYS, HWLOC_IRIX_SYS,
 * IRIX: see _DSM_MUSTRUN
 *
 * We could use glibc's sched_setaffinity generically when it is available
 *
 * Darwin and OpenBSD don't seem to have binding facilities.
 */

static hwloc_const_cpuset_t
hwloc_fix_cpubind(hwloc_topology_t topology, hwloc_const_cpuset_t set)
{
  hwloc_const_cpuset_t topology_set = hwloc_topology_get_topology_cpuset(topology);
  hwloc_const_cpuset_t complete_set = hwloc_topology_get_complete_cpuset(topology);

  if (!topology_set) {
    /* The topology is composed of several systems, the cpuset is ambiguous. */
    errno = EXDEV;
    return NULL;
  }

  if (!hwloc_cpuset_isincluded(set, complete_set)) {
    errno = EINVAL;
    return NULL;
  }

  if (hwloc_cpuset_isincluded(topology_set, set))
    set = complete_set;

  return set;
}

int
hwloc_set_cpubind(hwloc_topology_t topology, hwloc_const_cpuset_t set, int policy)
{
  set = hwloc_fix_cpubind(topology, set);
  if (!set)
    return -1;

  if (policy & HWLOC_CPUBIND_PROCESS) {
    if (topology->set_thisproc_cpubind)
      return topology->set_thisproc_cpubind(topology, set, policy);
  } else if (policy & HWLOC_CPUBIND_THREAD) {
    if (topology->set_thisthread_cpubind)
      return topology->set_thisthread_cpubind(topology, set, policy);
  } else {
    if (topology->set_thisproc_cpubind)
      return topology->set_thisproc_cpubind(topology, set, policy);
    else if (topology->set_thisthread_cpubind)
      return topology->set_thisthread_cpubind(topology, set, policy);
  }

  errno = ENOSYS;
  return -1;
}

int
hwloc_get_cpubind(hwloc_topology_t topology, hwloc_cpuset_t set, int policy)
{
  if (policy & HWLOC_CPUBIND_PROCESS) {
    if (topology->get_thisproc_cpubind)
      return topology->get_thisproc_cpubind(topology, set, policy);
  } else if (policy & HWLOC_CPUBIND_THREAD) {
    if (topology->get_thisthread_cpubind)
      return topology->get_thisthread_cpubind(topology, set, policy);
  } else {
    if (topology->get_thisproc_cpubind)
      return topology->get_thisproc_cpubind(topology, set, policy);
    else if (topology->get_thisthread_cpubind)
      return topology->get_thisthread_cpubind(topology, set, policy);
  }

  errno = ENOSYS;
  return -1;
}

int
hwloc_set_proc_cpubind(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_const_cpuset_t set, int policy)
{
  set = hwloc_fix_cpubind(topology, set);
  if (!set)
    return -1;

  if (topology->set_proc_cpubind)
    return topology->set_proc_cpubind(topology, pid, set, policy);

  errno = ENOSYS;
  return -1;
}

int
hwloc_get_proc_cpubind(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_cpuset_t set, int policy)
{
  if (topology->get_proc_cpubind)
    return topology->get_proc_cpubind(topology, pid, set, policy);

  errno = ENOSYS;
  return -1;
}

#ifdef hwloc_thread_t
int
hwloc_set_thread_cpubind(hwloc_topology_t topology, hwloc_thread_t tid, hwloc_const_cpuset_t set, int policy)
{
  set = hwloc_fix_cpubind(topology, set);
  if (!set)
    return -1;

  if (topology->set_thread_cpubind)
    return topology->set_thread_cpubind(topology, tid, set, policy);

  errno = ENOSYS;
  return -1;
}

int
hwloc_get_thread_cpubind(hwloc_topology_t topology, hwloc_thread_t tid, hwloc_cpuset_t set, int policy)
{
  if (topology->get_thread_cpubind)
    return topology->get_thread_cpubind(topology, tid, set, policy);

  errno = ENOSYS;
  return -1;
}
#endif

static hwloc_const_cpuset_t
hwloc_fix_membind(hwloc_topology_t topology, hwloc_const_cpuset_t set)
{
  hwloc_const_cpuset_t topology_set = hwloc_topology_get_topology_cpuset(topology);

  if (!topology_set) {
    /* The topology is composed of several systems, the set is ambiguous. */
    errno = EXDEV;
    return NULL;
  }

  /* TODO */
  return set;
}

int
hwloc_set_membind(hwloc_topology_t topology, hwloc_const_cpuset_t set, int policy)
{
  set = hwloc_fix_membind(topology, set);
  if (!set)
    return -1;

  if (topology->set_membind)
    return topology->set_membind(topology, set, policy);

  errno = ENOSYS;
  return -1;
}

int
hwloc_get_membind(hwloc_topology_t topology, hwloc_cpuset_t set, int * policy)
{
  if (topology->get_membind)
    return topology->get_membind(topology, set, policy);

  errno = ENOSYS;
  return -1;
}

int
hwloc_set_proc_membind(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_const_cpuset_t set, int policy)
{
  set = hwloc_fix_membind(topology, set);
  if (!set)
    return -1;

  if (topology->set_proc_membind)
    return topology->set_proc_membind(topology, pid, set, policy);

  errno = ENOSYS;
  return -1;
}

int
hwloc_get_proc_membind(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_cpuset_t set, int * policy)
{
  if (topology->get_proc_membind)
    return topology->get_proc_membind(topology, pid, set, policy);

  errno = ENOSYS;
  return -1;
}

int
hwloc_set_area_membind(hwloc_topology_t topology, const void *addr, size_t len, hwloc_const_cpuset_t set, int policy)
{
  set = hwloc_fix_membind(topology, set);
  if (!set)
    return -1;

  if (topology->set_area_membind)
    return topology->set_area_membind(topology, addr, len, set, policy);

  errno = ENOSYS;
  return -1;
}

int
hwloc_get_area_membind(hwloc_topology_t topology, const void *addr, size_t len, hwloc_cpuset_t set, int * policy)
{
  if (topology->get_area_membind)
    return topology->get_area_membind(topology, addr, len, set, policy);

  errno = ENOSYS;
  return -1;
}

void *
hwloc_alloc_membind(hwloc_topology_t topology, size_t len, hwloc_const_cpuset_t set, int policy)
{
  set = hwloc_fix_membind(topology, set);
  if (!set)
    return NULL;

  if (topology->alloc_membind)
    return topology->alloc_membind(topology, len, set, policy);
  else if (topology->set_area_membind) {
    void *p;
#if defined(HAVE_GETPAGESIZE) && defined(HAVE_POSIX_MEMALIGN)
    errno = posix_memalign(&p, getpagesize(), len);
    if (errno)
      p = NULL;
#elif defined(HAVE_GETPAGESIZE) && defined(HAVE_MEMALIGN)
    p = memalign(getpagesize(), len);
#else
    p = malloc(len);
#endif
    if (!topology->set_area_membind(topology, p, len, set, policy)) {
      int error = errno;
      free(p);
      errno = error;
      return NULL;
    }
    return p;
  }

  errno = ENOSYS;
  return NULL;
}

int
hwloc_free_membind(hwloc_topology_t topology, void *addr, size_t len)
{
  if (topology->free_membind)
    return topology->free_membind(topology, addr, len);
  else if (topology->set_area_membind)
    free(addr);

  errno = ENOSYS;
  return -1;
}

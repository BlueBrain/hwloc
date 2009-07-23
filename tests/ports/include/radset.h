/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 *
 * This software is a computer program whose purpose is to provide
 * abstracted information about the hardware topology.
 *
 * This software is governed by the CeCILL-B license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-B
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL-B license and that you accept its terms.
 */

#ifndef LIBTOPOLOGY_RADSET_H
#define LIBTOPOLOGY_RADSET_H

#include <limits.h>

typedef int radset_cursor_t;
typedef int radid_t;
#define RAD_NONE -1
typedef struct {
} radset_t;
typedef unsigned int nsgid_t;
#define NSG_NONE -1

extern int radsetcreate(radset_t *set);

extern int rademptyset(radset_t set);
extern int radaddset(radset_t set, radid_t radid);
extern int radismember(radset_t set, radid_t radid);
extern int rad_foreach(radset_t radset, int flags, radset_cursor_t *cursor);


extern int radsetdestroy(radset_t *set);


int rad_attach_pid(pid_t pid, radset_t radset, unsigned long flags);
int pthread_rad_attach(pthread_t thread, radset_t radset, unsigned long flags);
int rad_detach_pid(pid_t pid);
int pthread_rad_detach(pthread_t thread);


/* (strict) */
#define RAD_INSIST 0x01
#define RAD_WAIT 0x08
int rad_bind_pid(pid_t pid, radset_t radset, unsigned long flags);
int pthread_rad_bind(pthread_t thread, radset_t radset, unsigned long flags);

typedef union rsrcdescr {
  radset_t rd_radset;
  int rd_fd;
  char *rd_pathname;
  int rd_shmid;
  pid_t rd_pid;
  void *rd_addr;
  nsgid_t rd_nsg;
} rsrcdescr_t;

typedef struct numa_attr {
  unsigned nattr_type;
#define R_RAD 0

  rsrcdescr_t nattr_descr;

  unsigned long nattr_distance;
#define RAD_DIST_LOCAL 100
#define RAD_DIST_REMOTE INT_MAX

  unsigned long nattr_flags;
} numa_attr_t;

typedef enum memalloc_policy {
  MPOL_DIRECTED,
  MPOL_THREAD,
  MPOL_REPLICATED,
  MPOL_STRIPPED,
  MPOL_INVALID
} memalloc_policy_t;
#define MPOL_NO_MIGRATE 0x100

int nloc(numa_attr_t *numa_attr, radset_t radset);

#endif /* LIBTOPOLOGY_RADSET_H */

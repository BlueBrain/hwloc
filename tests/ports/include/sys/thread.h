/* Copyright 2009 INRIA, Université Bordeaux 1  */

#ifndef LIBTOPOLOGY_SYS_THREAD_H
#define LIBTOPOLOGY_SYS_THREAD_H

typedef long tid_t;
tid_t thread_self();
struct __pthrdsinfo {
  tid_t __pi_tid;
};
#define PTHRDSINFO_QUERY_TID 0x10
int pthread_getthrds_np (pthread_t * thread, int mode, struct __pthrdsinfo * buf, int bufsize, void * regbuf, int * regbufsize);

#endif /* LIBTOPOLOGY_SYS_THREAD_H */

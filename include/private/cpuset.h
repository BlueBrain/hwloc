/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

/* Internals for cpuset API.  */

#ifndef HWLOC_PRIVATE_CPUSET_H
#define HWLOC_PRIVATE_CPUSET_H

#include <hwloc/config.h>
#include <private/config.h>

#include <sys/types.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>


/**
 * Cpuset internals.
 */

/* size and count of subsets within a set */
#define HWLOC_CPUSUBSET_SIZE	HWLOC_BITS_PER_LONG
#define HWLOC_CPUSUBSET_COUNT	((HWLOC_NBMAXCPUS+HWLOC_CPUSUBSET_SIZE-1)/HWLOC_CPUSUBSET_SIZE)

/* magic number */
#define HWLOC_CPUSET_MAGIC 0x20091007

/* actual opaque type internals */
struct hwloc_cpuset_s {
	unsigned long s[HWLOC_CPUSUBSET_COUNT];
#ifdef HWLOC_DEBUG
	int magic;
#endif
};

/* extract a subset from a set using an index or a cpu */
#define HWLOC_CPUSUBSET_SUBSET(set,x)		((set).s[x])
#define HWLOC_CPUSUBSET_INDEX(cpu)		((cpu)/(HWLOC_CPUSUBSET_SIZE))
#define HWLOC_CPUSUBSET_CPUSUBSET(set,cpu)	HWLOC_CPUSUBSET_SUBSET(set,HWLOC_CPUSUBSET_INDEX(cpu))

/* predefined subset values */
#define HWLOC_CPUSUBSET_VAL(cpu)		(1UL<<((cpu)%(HWLOC_CPUSUBSET_SIZE)))
#define HWLOC_CPUSUBSET_ZERO		0UL
#define HWLOC_CPUSUBSET_FULL		~0UL

/* Strings always use 32bit groups */
#define HWLOC_PRIxCPUSUBSET		"%08lx"
#define HWLOC_CPUSET_SUBSTRING_SIZE	32
#define HWLOC_CPUSET_SUBSTRING_COUNT	((HWLOC_NBMAXCPUS+HWLOC_CPUSET_SUBSTRING_SIZE-1)/HWLOC_CPUSET_SUBSTRING_SIZE)
#define HWLOC_CPUSET_SUBSTRING_LENGTH	(HWLOC_CPUSET_SUBSTRING_SIZE/4)


/**
 * ffs helpers.
 */

#ifdef __GNUC__

#  if (__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4))
     /* Starting from 3.4, gcc has a long variant.  */
#    define hwloc_ffsl(x) __builtin_ffsl(x)
#  else
#    define hwloc_ffs(x) __builtin_ffs(x)
#    define HWLOC_NEED_FFSL
#  endif

#elif defined(HWLOC_HAVE_FFSL)

#  define hwloc_ffsl(x) ffsl(x)

#elif defined(HWLOC_HAVE_FFS)

#  ifndef HWLOC_HAVE_DECL_FFS
extern int ffs(int);
#  endif

#  define hwloc_ffs(x) ffs(x)
#  define HWLOC_NEED_FFSL

#else /* no ffs implementation */

static __inline int hwloc_ffsl(unsigned long x)
{
	int i;

	if (!x)
		return 0;

	i = 1;
#if HWLOC_BITS_PER_LONG >= 64
	if (!(x & 0xfffffffful)) {
		x >>= 32;
		i += 32;
	}
#endif
	if (!(x & 0xffffu)) {
		x >>= 16;
		i += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		i += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		i += 4;
	}
	if (!(x & 0x3)) {
		x >>= 2;
		i += 2;
	}
	if (!(x & 0x1)) {
		x >>= 1;
		i += 1;
	}

	return i;
}

#endif

#ifdef HWLOC_NEED_FFSL

/* We only have an int ffs(int) implementation, build a long one.  */

/* First make it 32 bits if it was only 16.  */
static __inline int hwloc_ffs32(unsigned long x)
{
#if HWLOC_BITS_PER_INT == 16
	int low_ffs, hi_ffs;

	low_ffs = hwloc_ffs(x & 0xfffful);
	if (low_ffs)
		return low_ffs;

	hi_ffs = hwloc_ffs(x >> 16);
	if (hi_ffs)
		return hi_ffs + 16;

	return 0;
#else
	return hwloc_ffs(x);
#endif
}

/* Then make it 64 bit if longs are.  */
static __inline int hwloc_ffsl(unsigned long x)
{
#if HWLOC_BITS_PER_LONG == 64
	int low_ffs, hi_ffs;

	low_ffs = hwloc_ffs32(x & 0xfffffffful);
	if (low_ffs)
		return low_ffs;

	hi_ffs = hwloc_ffs32(x >> 32);
	if (hi_ffs)
		return hi_ffs + 32;

	return 0;
#else
	return hwloc_ffs32(x);
#endif
}
#endif

/**
 * fls helpers.
 */
#ifdef __GNUC_____

#  if (__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4))
#    define hwloc_flsl(x) (x ? 8*sizeof(long) - __builtin_clzl(x) : 0)
#  else
#    define hwloc_fls(x) (x ? 8*sizeof(int) - __builtin_clz(x) : 0)
#    define HWLOC_NEED_FLSL
#  endif

#elif defined(HWLOC_HAVE_FLSL)

#  define hwloc_flsl(x) flsl(x)

#elif defined(HWLOC_HAVE_CLZL)

#  define hwloc_flsl(x) (x ? 8*sizeof(long) - clzl(x) : 0)

#elif defined(HWLOC_HAVE_FLS)

#  ifndef HWLOC_HAVE_DECL_FLS
extern int fsl(int);
#  endif

#  define hwloc_fls(x) fls(x)
#  define HWLOC_NEED_FLSL

#elif defined(HWLOC_HAVE_CLZ)

#  ifndef HWLOC_HAVE_DECL_CLZ
extern int clz(int);
#  endif

#  define hwloc_fls(x) (x ? 8*sizeof(int) - clz(x) : 0)
#  define HWLOC_NEED_FLSL

#else /* no fls implementation */

static __inline int hwloc_flsl(unsigned long x)
{
	int i = 0;

	if (!x)
		return 0;

	i = 1;
#if HWLOC_BITS_PER_LONG >= 64
	if ((x & 0xffffffff00000000ul)) {
		x >>= 32;
		i += 32;
	}
#endif
	if ((x & 0xffff0000u)) {
		x >>= 16;
		i += 16;
	}
	if ((x & 0xff00)) {
		x >>= 8;
		i += 8;
	}
	if ((x & 0xf0)) {
		x >>= 4;
		i += 4;
	}
	if ((x & 0xc)) {
		x >>= 2;
		i += 2;
	}
	if ((x & 0x2)) {
		x >>= 1;
		i += 1;
	}

	return i;
}

#endif

#ifdef HWLOC_NEED_FLSL

/* We only have an int fls(int) implementation, build a long one.  */

/* First make it 32 bits if it was only 16.  */
static __inline int hwloc_fls32(unsigned long x)
{
#if HWLOC_BITS_PER_INT == 16
	int low_fls, hi_fls;

	hi_fls = hwloc_fls(x >> 16);
	if (hi_fls)
		return hi_fls + 16;

	low_fls = hwloc_fls(x & 0xfffful);
	if (low_fls)
		return low_fls;

	return 0;
#else
	return hwloc_fls(x);
#endif
}

/* Then make it 64 bit if longs are.  */
static __inline int hwloc_flsl(unsigned long x)
{
#if HWLOC_BITS_PER_LONG == 64
	int low_fls, hi_fls;

	hi_fls = hwloc_fls32(x >> 32);
	if (hi_fls)
		return hi_fls + 32;

	low_fls = hwloc_fls32(x & 0xfffffffful);
	if (low_fls)
		return low_fls;

	return 0;
#else
	return hwloc_fls32(x);
#endif
}
#endif

static __inline int hwloc_weight_long(unsigned long w)
{
#if HWLOC_BITS_PER_LONG == 32
#if (__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__) >= 4)
	return __builtin_popcount(w);
#else
	unsigned int res = (w & 0x55555555) + ((w >> 1) & 0x55555555);
	res = (res & 0x33333333) + ((res >> 2) & 0x33333333);
	res = (res & 0x0F0F0F0F) + ((res >> 4) & 0x0F0F0F0F);
	res = (res & 0x00FF00FF) + ((res >> 8) & 0x00FF00FF);
	return (res & 0x0000FFFF) + ((res >> 16) & 0x0000FFFF);
#endif
#else /* HWLOC_BITS_PER_LONG == 32 */
#if (__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__) >= 4)
	return __builtin_popcountll(w);
#else
	unsigned long res;
	res = (w & 0x5555555555555555ul) + ((w >> 1) & 0x5555555555555555ul);
	res = (res & 0x3333333333333333ul) + ((res >> 2) & 0x3333333333333333ul);
	res = (res & 0x0F0F0F0F0F0F0F0Ful) + ((res >> 4) & 0x0F0F0F0F0F0F0F0Ful);
	res = (res & 0x00FF00FF00FF00FFul) + ((res >> 8) & 0x00FF00FF00FF00FFul);
	res = (res & 0x0000FFFF0000FFFFul) + ((res >> 16) & 0x0000FFFF0000FFFFul);
	return (res & 0x00000000FFFFFFFFul) + ((res >> 32) & 0x00000000FFFFFFFFul);
#endif
#endif /* HWLOC_BITS_PER_LONG == 64 */
}


#endif /* HWLOC_PRIVATE_CPUSET_H */

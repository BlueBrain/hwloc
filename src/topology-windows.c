/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

/* To try to get all declarations duplicated below.  */
#define _WIN32_WINNT 0x0601

#include <private/config.h>
#include <hwloc.h>
#include <private/private.h>
#include <private/debug.h>

#include <windows.h>

#ifndef HAVE_KAFFINITY
typedef ULONG_PTR KAFFINITY, *PKAFFINITY;
#endif

#ifndef HAVE_PROCESSOR_CACHE_TYPE
typedef enum _PROCESSOR_CACHE_TYPE {
  CacheUnified,
  CacheInstruction,
  CacheData,
  CacheTrace
} PROCESSOR_CACHE_TYPE;
#endif

#ifndef CACHE_FULLY_ASSOCIATIVE
#define CACHE_FULLY_ASSOCIATIVE 0xFF
#endif

#ifndef HAVE_CACHE_DESCRIPTOR
typedef struct _CACHE_DESCRIPTOR {
  BYTE Level;
  BYTE Associativity;
  WORD LineSize;
  DWORD Size; /* in bytes */
  PROCESSOR_CACHE_TYPE Type;
} CACHE_DESCRIPTOR, *PCACHE_DESCRIPTOR;
#endif

#ifndef HAVE_LOGICAL_PROCESSOR_RELATIONSHIP
typedef enum _LOGICAL_PROCESSOR_RELATIONSHIP {
  RelationProcessorCore,
  RelationNumaNode,
  RelationCache,
  RelationProcessorPackage,
  RelationGroup,
  RelationAll = 0xffff,
} LOGICAL_PROCESSOR_RELATIONSHIP;
#endif

#ifndef HAVE_SYSTEM_LOGICAL_PROCESSOR_INFORMATION
typedef struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION {
  ULONG_PTR ProcessorMask;
  LOGICAL_PROCESSOR_RELATIONSHIP Relationship;
  _ANONYMOUS_UNION
  union {
    struct {
      BYTE flags;
    } ProcessorCore;
    struct {
      DWORD NodeNumber;
    } NumaNode;
    CACHE_DESCRIPTOR Cache;
    ULONGLONG Reserved[2];
  } DUMMYUNIONNAME;
} SYSTEM_LOGICAL_PROCESSOR_INFORMATION, *PSYSTEM_LOGICAL_PROCESSOR_INFORMATION;
#endif

/* Extended interface, for group support */

#ifndef HAVE_GROUP_AFFINITY
typedef struct _GROUP_AFFINITY {
  KAFFINITY Mask;
  WORD Group;
  WORD Reserved[3];
} GROUP_AFFINITY, *PGROUP_AFFINITY;
#endif

#ifndef HAVE_PROCESSOR_RELATIONSHIP
typedef struct _PROCESSOR_RELATIONSHIP {
  BYTE Flags;
  ULONGLONG Reserved[2];
  GROUP_AFFINITY GroupMask;
} PROCESSOR_RELATIONSHIP, *PPROCESSOR_RELATIONSHIP;
#endif

#ifndef HAVE_NUMA_NODE_RELATIONSHIP
typedef struct _NUMA_NODE_RELATIONSHIP {
  DWORD NodeNumber;
  ULONGLONG Reserved[2];
  GROUP_AFFINITY GroupMask;
} NUMA_NODE_RELATIONSHIP, *PNUMA_NODE_RELATIONSHIP;
#endif

#ifndef HAVE_CACHE_RELATIONSHIP
typedef struct _CACHE_RELATIONSHIP {
  BYTE Level;
  BYTE Associativity;
  WORD LineSize;
  DWORD CacheSize;
  PROCESSOR_CACHE_TYPE Type;
  ULONGLONG Reserved[2];
  GROUP_AFFINITY GroupMask;
} CACHE_RELATIONSHIP, *PCACHE_RELATIONSHIP;
#endif

#ifndef HAVE_PROCESSOR_GROUP_INFO
typedef struct _PROCESSOR_GROUP_INFO {
  BYTE MaximumProcessorCount;
  BYTE ActiveProcessorCount;
  KAFFINITY ActiveProcessorMask;
  ULONGLONG Reserved[4];
} PROCESSOR_GROUP_INFO, *PPROCESSOR_GROUP_INFO;
#endif

#ifndef HAVE_GROUP_RELATIONSHIP
typedef struct _GROUP_RELATIONSHIP {
  WORD MaximumGroupCount;
  WORD ActiveGroupCount;
  ULONGLONG Reserved[2];
  PROCESSOR_GROUP_INFO GroupInfo[ANYSIZE_ARRAY];
} GROUP_RELATIONSHIP, *PGROUP_RELATIONSHIP;
#endif

#ifndef HAVE_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX
typedef struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX {
  LOGICAL_PROCESSOR_RELATIONSHIP Relationship;
  DWORD Size;
  _ANONYMOUS_UNION
  union {
    PROCESSOR_RELATIONSHIP Processor;
    NUMA_NODE_RELATIONSHIP NumaNode;
    CACHE_RELATIONSHIP Cache;
    GROUP_RELATIONSHIP Group;
    /* Odd: no member to tell the cpu mask of the package... */
  } DUMMYUNIONNAME;
} SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, *PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX;
#endif

/* TODO: SetThreadIdealProcessor */

static int
hwloc_win_set_thread_cpubind(hwloc_topology_t topology __hwloc_attribute_unused, hwloc_thread_t thread, hwloc_const_cpuset_t hwloc_set, int policy __hwloc_attribute_unused)
{
  /* TODO: groups SetThreadGroupAffinity */
  /* The resulting binding is always strict */
  DWORD mask = hwloc_cpuset_to_ulong(hwloc_set);
  if (!SetThreadAffinityMask(thread, mask))
    return -1;
  return 0;
}

static int
hwloc_win_set_thisthread_cpubind(hwloc_topology_t topology, hwloc_const_cpuset_t hwloc_set, int policy)
{
  return hwloc_win_set_thread_cpubind(topology, GetCurrentThread(), hwloc_set, policy);
}

static int
hwloc_win_set_proc_cpubind(hwloc_topology_t topology __hwloc_attribute_unused, hwloc_pid_t proc, hwloc_const_cpuset_t hwloc_set, int policy __hwloc_attribute_unused)
{
  /* TODO: groups */
  /* The resulting binding is always strict */
  DWORD mask = hwloc_cpuset_to_ulong(hwloc_set);
  if (!SetProcessAffinityMask(proc, mask))
    return -1;
  return 0;
}

static hwloc_cpuset_t
hwloc_win_get_proc_cpubind(hwloc_topology_t topology __hwloc_attribute_unused, hwloc_pid_t proc, int policy __hwloc_attribute_unused)
{
  hwloc_cpuset_t ret;
  DWORD proc_mask, sys_mask;
  /* TODO: groups */
  if (!GetProcessAffinityMask(proc, &proc_mask, &sys_mask))
    return NULL;
  ret = hwloc_cpuset_alloc();
  hwloc_cpuset_from_ulong(ret, proc_mask);
  return ret;
}

static int
hwloc_win_set_thisproc_cpubind(hwloc_topology_t topology, hwloc_const_cpuset_t hwloc_set, int policy)
{
  return hwloc_win_set_proc_cpubind(topology, GetCurrentProcess(), hwloc_set, policy);
}

static hwloc_cpuset_t
hwloc_win_get_thisproc_cpubind(hwloc_topology_t topology, int policy)
{
  return hwloc_win_get_proc_cpubind(topology, GetCurrentProcess(), policy);
}

void
hwloc_look_windows(struct hwloc_topology *topology)
{
  BOOL WINAPI (*GetLogicalProcessorInformationProc)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer, PDWORD ReturnLength);
  BOOL WINAPI (*GetLogicalProcessorInformationExProc)(LOGICAL_PROCESSOR_RELATIONSHIP relationship, PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Buffer, PDWORD ReturnLength);
  BOOL WINAPI (*GetNumaAvailableMemoryNodeProc)(UCHAR Node, PULONGLONG AvailableBytes);
  DWORD length;

  HMODULE kernel32;

  kernel32 = LoadLibrary("kernel32.dll");
  if (kernel32) {
    GetLogicalProcessorInformationProc = GetProcAddress(kernel32, "GetLogicalProcessorInformation");
    GetNumaAvailableMemoryNodeProc = GetProcAddress(kernel32, "GetNumaAvailableMemoryNode");

    if (GetLogicalProcessorInformationProc) {
      PSYSTEM_LOGICAL_PROCESSOR_INFORMATION procInfo;
      unsigned id;
      unsigned i;
      struct hwloc_obj *obj;
      hwloc_obj_type_t type;

      length = 0;
      procInfo = NULL;

      while (1) {
	if (GetLogicalProcessorInformationProc(procInfo, &length))
	  break;
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	  return;
	free(procInfo);
	procInfo = malloc(length);
      }

      for (i = 0; i < length / sizeof(*procInfo); i++) {

        /* Ignore non-data caches */
	if (procInfo[i].Relationship == RelationCache
		&& procInfo[i].Cache.Type != CacheUnified
		&& procInfo[i].Cache.Type != CacheData)
	  continue;

	id = -1;
	switch (procInfo[i].Relationship) {
	  case RelationNumaNode:
	    type = HWLOC_OBJ_NODE;
	    id = procInfo[i].NumaNode.NodeNumber;
	    break;
	  case RelationProcessorPackage:
	    type = HWLOC_OBJ_SOCKET;
	    break;
	  case RelationCache:
	    type = HWLOC_OBJ_CACHE;
	    break;
	  case RelationProcessorCore:
	    type = HWLOC_OBJ_CORE;
	    break;
	  case RelationGroup:
	  default:
	    type = HWLOC_OBJ_MISC;
	    break;
	}

	obj = hwloc_alloc_setup_object(type, id);
        obj->cpuset = hwloc_cpuset_alloc();
	hwloc_debug("%s#%u mask %lx\n", hwloc_obj_type_string(type), id, procInfo[i].ProcessorMask);
	hwloc_cpuset_from_ulong(obj->cpuset, procInfo[i].ProcessorMask);

	switch (type) {
	  case HWLOC_OBJ_NODE:
	    {
	      ULONGLONG avail;
	      if (GetNumaAvailableMemoryNodeProc && GetNumaAvailableMemoryNodeProc(id, &avail))
		obj->memory.local_memory = avail;
	      obj->memory.page_types_len = 1;
	      obj->memory.page_types = malloc(sizeof(*obj->memory.page_types));
	      memset(obj->memory.page_types, 0, sizeof(*obj->memory.page_types));
#ifdef HAVE__SC_LARGE_PAGESIZE
	      obj->memory.page_types[0].size = sysconf(_SC_LARGE_PAGESIZE);
#endif
	      break;
	    }
	  case HWLOC_OBJ_CACHE:
	    obj->attr->cache.size = procInfo[i].Cache.Size;
	    obj->attr->cache.depth = procInfo[i].Cache.Level;
	    break;
	  case HWLOC_OBJ_MISC:
	    obj->attr->misc.depth = procInfo[i].Relationship == RelationGroup;
	    break;
	  default:
	    break;
	}
	hwloc_insert_object_by_cpuset(topology, obj);
      }

      free(procInfo);
    }

    GetLogicalProcessorInformationExProc = GetProcAddress(kernel32, "GetLogicalProcessorInformationEx");

    /* Disabled for now as it wasn't tested at all.  */
    if (0 && GetLogicalProcessorInformationExProc) {
      PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX procInfoTotal, procInfo;

      signed id;
      struct hwloc_obj *obj;
      hwloc_obj_type_t type;
      KAFFINITY mask;
      WORD group;

      fprintf(stderr,"Note: GetLogicalProcessorInformationEx was never tested yet!\n");

      length = 0;
      procInfoTotal = NULL;

      while (1) {
	if (GetLogicalProcessorInformationExProc(RelationAll, procInfoTotal, &length))
	  break;
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	  return;
	free(procInfoTotal);
	procInfo = malloc(length);
      }

      for (procInfo = procInfoTotal;
	   (void*) procInfo < (void*) ((unsigned long) procInfoTotal + length);
	   procInfo = (void*) ((unsigned long) procInfo + procInfo->Size)) {

        /* Ignore non-data caches */
	if (procInfo->Relationship == RelationCache &&
		  (procInfo->Cache.Type == CacheUnified
		|| procInfo->Cache.Type == CacheData))
	  continue;

	id = -1;
	switch (procInfo->Relationship) {
	  case RelationNumaNode:
	    type = HWLOC_OBJ_NODE;
	    mask = procInfo->NumaNode.GroupMask.Mask;
	    group = procInfo->NumaNode.GroupMask.Group;
	    id = procInfo->NumaNode.NodeNumber;
	    break;
	  case RelationProcessorPackage:
	    type = HWLOC_OBJ_SOCKET;
	    mask = procInfo->Processor.GroupMask.Mask;
	    group = procInfo->Processor.GroupMask.Group;
	    break;
	  case RelationCache:
	    type = HWLOC_OBJ_CACHE;
	    mask = procInfo->Cache.GroupMask.Mask;
	    group = procInfo->Cache.GroupMask.Group;
	    break;
	  case RelationProcessorCore:
	    type = HWLOC_OBJ_CORE;
	    mask = procInfo->Processor.GroupMask.Mask;
	    group = procInfo->Processor.GroupMask.Group;
	    break;
	  case RelationGroup:
	    /* So strange an interface... */
	    for (id = 0; id < procInfo->Group.ActiveGroupCount; id++) {
	      obj = hwloc_alloc_setup_object(HWLOC_OBJ_MISC, id);
	      obj->cpuset = hwloc_cpuset_alloc();
	      mask = procInfo->Group.GroupInfo[id].ActiveProcessorMask;
	      hwloc_debug("group %d mask %lx\n", id, mask);
	      hwloc_cpuset_from_ith_ulong(obj->cpuset, id, mask);
	      hwloc_insert_object_by_cpuset(topology, obj);
	    }
	    continue;
	  default:
	    /* Don't know how to get the mask.  */
	    continue;
	}

	obj = hwloc_alloc_setup_object(type, id);
        obj->cpuset = hwloc_cpuset_alloc();
	hwloc_debug("%s#%d mask %d:%lx\n", hwloc_obj_type_string(type), id, group, mask);
	hwloc_cpuset_from_ith_ulong(obj->cpuset, group, mask);

	switch (type) {
	  case HWLOC_OBJ_NODE:
	    obj->memory.local_memory = 0; /* TODO GetNumaAvailableMemoryNodeEx  */
	    obj->memory.page_types_len = 1;
	    obj->memory.page_types = malloc(sizeof(*obj->memory.page_types));
	    memset(obj->memory.page_types, 0, sizeof(*obj->memory.page_types));
#ifdef HAVE__SC_LARGE_PAGESIZE
	    obj->memory.page_types[0].size = sysconf(_SC_LARGE_PAGESIZE);
#endif
	    break;
	  case HWLOC_OBJ_CACHE:
	    obj->attr->cache.size = procInfo->Cache.CacheSize;
	    obj->attr->cache.depth = procInfo->Cache.Level;
	    break;
	  default:
	    break;
	}
	hwloc_insert_object_by_cpuset(topology, obj);
      }
      free(procInfoTotal);
    }
  }

  /* add PROC objects */
  hwloc_setup_proc_level(topology, hwloc_fallback_nbprocessors(topology));
}

void
hwloc_set_windows_hooks(struct hwloc_topology *topology)
{
  topology->set_proc_cpubind = hwloc_win_set_proc_cpubind;
  topology->get_proc_cpubind = hwloc_win_get_proc_cpubind;
  topology->set_thread_cpubind = hwloc_win_set_thread_cpubind;
  topology->set_thisproc_cpubind = hwloc_win_set_thisproc_cpubind;
  topology->get_thisproc_cpubind = hwloc_win_get_thisproc_cpubind;
  topology->set_thisthread_cpubind = hwloc_win_set_thisthread_cpubind;
}

/* TODO memory binding: VirtualAllocExNuma */

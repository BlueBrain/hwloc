/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * Copyright © 2009 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/config.h>
#include <hwloc.h>
#include <private/private.h>
#include <private/debug.h>

int
hwloc_get_type_depth (struct hwloc_topology *topology, hwloc_obj_type_t type)
{
  return topology->type_depth[type];
}

hwloc_obj_type_t
hwloc_get_depth_type (hwloc_topology_t topology, unsigned depth)
{
  if (depth >= topology->nb_levels)
    return (hwloc_obj_type_t) -1;
  return topology->levels[depth][0]->type;
}

unsigned
hwloc_get_nbobjs_by_depth (struct hwloc_topology *topology, unsigned depth)
{
  if (depth >= topology->nb_levels)
    return 0;
  return topology->level_nbobjects[depth];
}

struct hwloc_obj *
hwloc_get_obj_by_depth (struct hwloc_topology *topology, unsigned depth, unsigned idx)
{
  if (depth >= topology->nb_levels)
    return NULL;
  if (idx >= topology->level_nbobjects[depth])
    return NULL;
  return topology->levels[depth][idx];
}

struct hwloc_obj *
hwloc_get_next_iodevice(struct hwloc_topology *topology, struct hwloc_obj *prev)
{
  if (prev) {
    if (prev->type != HWLOC_OBJ_PCI_DEVICE)
      return NULL;
    return prev->next_cousin;
  } else {
    return topology->first_device;
  }
}

unsigned hwloc_get_closest_objs (struct hwloc_topology *topology, struct hwloc_obj *src, struct hwloc_obj **objs, unsigned max)
{
  struct hwloc_obj *parent, *nextparent, **src_objs;
  int i,src_nbobjects;
  unsigned stored = 0;

  src_nbobjects = topology->level_nbobjects[src->depth];
  src_objs = topology->levels[src->depth];

  parent = src;
  while (stored < max) {
    while (1) {
      nextparent = parent->father;
      if (!nextparent)
	goto out;
      if (!hwloc_cpuset_isequal(parent->cpuset, nextparent->cpuset))
	break;
      parent = nextparent;
    }

    /* traverse src's objects and find those that are in nextparent and were not in parent */
    for(i=0; i<src_nbobjects; i++) {
      if (hwloc_cpuset_isincluded(src_objs[i]->cpuset, nextparent->cpuset)
	  && !hwloc_cpuset_isincluded(src_objs[i]->cpuset, parent->cpuset)) {
	objs[stored++] = src_objs[i];
	if (stored == max)
	  goto out;
      }
    }
    parent = nextparent;
  }

 out:
  return stored;
}

static int
hwloc__get_largest_objs_inside_cpuset (struct hwloc_obj *current, hwloc_const_cpuset_t set,
				       struct hwloc_obj ***res, int *max)
{
  int gotten = 0;
  unsigned i;

  /* the caller must ensure this */
  if (*max <= 0)
    return 0;

  if (hwloc_cpuset_isequal(current->cpuset, set)) {
    **res = current;
    (*res)++;
    (*max)--;
    return 1;
  }

  for (i=0; i<current->arity; i++) {
    hwloc_cpuset_t subset = hwloc_cpuset_dup(set);
    int ret;

    /* split out the cpuset part corresponding to this child and see if there's anything to do */
    hwloc_cpuset_andset(subset, current->children[i]->cpuset);
    if (hwloc_cpuset_iszero(subset)) {
      hwloc_cpuset_free(subset);
      continue;
    }

    ret = hwloc__get_largest_objs_inside_cpuset (current->children[i], subset, res, max);
    gotten += ret;
    hwloc_cpuset_free(subset);

    /* if no more room to store remaining objects, return what we got so far */
    if (!*max)
      break;
  }

  return gotten;
}

int
hwloc_get_largest_objs_inside_cpuset (struct hwloc_topology *topology, hwloc_const_cpuset_t set,
				      struct hwloc_obj **objs, int max)
{
  struct hwloc_obj *current = topology->levels[0][0];

  if (!hwloc_cpuset_isincluded(set, current->cpuset))
    return -1;

  if (max <= 0)
    return 0;

  return hwloc__get_largest_objs_inside_cpuset (current, set, &objs, &max);
}

const char *
hwloc_obj_type_string (hwloc_obj_type_t obj)
{
  switch (obj)
    {
    case HWLOC_OBJ_SYSTEM: return "System";
    case HWLOC_OBJ_MACHINE: return "Machine";
    case HWLOC_OBJ_MISC: return "Misc";
    case HWLOC_OBJ_NODE: return "NUMANode";
    case HWLOC_OBJ_SOCKET: return "Socket";
    case HWLOC_OBJ_CACHE: return "Cache";
    case HWLOC_OBJ_CORE: return "Core";
    case HWLOC_OBJ_BRIDGE: return "Bridge";
    case HWLOC_OBJ_PCI_DEVICE: return "PCIDev";
    case HWLOC_OBJ_PROC: return "Proc";
    default: return "Unknown";
    }
}

hwloc_obj_type_t
hwloc_obj_type_of_string (const char * string)
{
  if (!strcasecmp(string, "System")) return HWLOC_OBJ_SYSTEM;
  if (!strcasecmp(string, "Machine")) return HWLOC_OBJ_MACHINE;
  if (!strcasecmp(string, "Misc")) return HWLOC_OBJ_MISC;
  if (!strcasecmp(string, "NUMANode") || !strcasecmp(string, "Node")) return HWLOC_OBJ_NODE;
  if (!strcasecmp(string, "Socket")) return HWLOC_OBJ_SOCKET;
  if (!strcasecmp(string, "Cache")) return HWLOC_OBJ_CACHE;
  if (!strcasecmp(string, "Core")) return HWLOC_OBJ_CORE;
  if (!strcasecmp(string, "Proc")) return HWLOC_OBJ_PROC;
  if (!strcasecmp(string, "Bridge")) return HWLOC_OBJ_BRIDGE;
  if (!strcasecmp(string, "PCIDev")) return HWLOC_OBJ_PCI_DEVICE;
  return (hwloc_obj_type_t) -1;
}

static const char *
hwloc_pci_class_string(unsigned short class_id)
{
  switch ((class_id & 0xff00) >> 8) {
    case 0x00:
      switch (class_id) {
	case 0x0001: return "VGA";
      }
      return "PCI";
    case 0x01:
      switch (class_id) {
	case 0x0100: return "SCSI";
	case 0x0101: return "IDE";
	case 0x0102: return "Flop";
	case 0x0103: return "IPI";
	case 0x0104: return "RAID";
	case 0x0105: return "ATA";
	case 0x0106: return "SATA";
	case 0x0107: return "SAS";
      }
      return "Stor";
    case 0x02:
      switch (class_id) {
	case 0x0200: return "Ether";
	case 0x0201: return "TokRn";
	case 0x0202: return "FDDI";
	case 0x0203: return "ATM";
	case 0x0204: return "ISDN";
	case 0x0205: return "WrdFip";
	case 0x0206: return "PICMG";
      }
      return "Net";
    case 0x03:
      switch (class_id) {
	case 0x0300: return "VGA";
	case 0x0301: return "XGA";
	case 0x0302: return "3D";
      }
      return "Disp";
    case 0x04:
      switch (class_id) {
	case 0x0400: return "Video";
	case 0x0401: return "Audio";
	case 0x0402: return "Phone";
	case 0x0403: return "Auddv";
      }
      return "MM";
    case 0x05:
      switch (class_id) {
	case 0x0500: return "RAM";
	case 0x0501: return "Flash";
      }
      return "Mem";
    case 0x06:
      switch (class_id) {
	case 0x0600: return "Host";
	case 0x0601: return "ISA";
	case 0x0602: return "EISA";
	case 0x0603: return "MC";
	case 0x0604: return "PCI_B";
	case 0x0605: return "PCMCIA";
	case 0x0606: return "Nubus";
	case 0x0607: return "CardBus";
	case 0x0608: return "RACEway";
	case 0x0609: return "PCI_SB";
	case 0x060a: return "IB_B";
      }
      return "Bridg";
    case 0x07:
      switch (class_id) {
	case 0x0700: return "Ser";
	case 0x0701: return "Para";
	case 0x0702: return "MSer";
	case 0x0703: return "Modm";
	case 0x0704: return "GPIB";
	case 0x0705: return "SmrtCrd";
      }
      return "Comm";
    case 0x08:
      switch (class_id) {
	case 0x0800: return "PIC";
	case 0x0801: return "DMA";
	case 0x0802: return "Time";
	case 0x0803: return "RTC";
	case 0x0804: return "HtPl";
	case 0x0805: return "SD-HtPl";
      }
      return "Syst";
    case 0x09:
      switch (class_id) {
	case 0x0900: return "Kbd";
	case 0x0901: return "Pen";
	case 0x0902: return "Mouse";
	case 0x0903: return "Scan";
	case 0x0904: return "Game";
      }
      return "In";
    case 0x0a:
      return "Dock";
    case 0x0b:
      switch (class_id) {
	case 0x0b00: return "386";
	case 0x0b01: return "486";
	case 0x0b02: return "Pent";
	case 0x0b10: return "Alpha";
	case 0x0b20: return "PPC";
	case 0x0b30: return "MIPS";
	case 0x0b40: return "CoProc";
      }
      return "Proc";
    case 0x0c:
      switch (class_id) {
	case 0x0c00: return "Firw";
	case 0x0c01: return "ACCES";
	case 0x0c02: return "SSA";
	case 0x0c03: return "USB";
	case 0x0c04: return "Fiber";
	case 0x0c05: return "SMBus";
	case 0x0c06: return "IB";
	case 0x0c07: return "IPMI";
	case 0x0c08: return "SERCOS";
	case 0x0c09: return "CANBUS";
      }
      return "Ser";
    case 0x0d:
      switch (class_id) {
	case 0x0d00: return "IRDA";
	case 0x0d01: return "IR";
	case 0x0d10: return "RF";
	case 0x0d11: return "Blueth";
	case 0x0d12: return "BroadB";
	case 0x0d20: return "802.1a";
	case 0x0d21: return "802.1b";
      }
      return "Wifi";
    case 0x0e:
      switch (class_id) {
	case 0x0e00: return "I2O";
      }
      return "Intll";
    case 0x0f:
      switch (class_id) {
	case 0x0f00: return "S-TV";
	case 0x0f01: return "S-Aud";
	case 0x0f02: return "S-Voice";
	case 0x0f03: return "S-Data";
      }
      return "Satel";
    case 0x10:
      return "Crypt";
    case 0x11:
      return "Signl";
    case 0xff:
      return "Oth";
  }
  return "PCI";
}

#define hwloc_memory_size_printf_value(_size, _verbose) \
  (_size) < (10*1024) || _verbose ? (_size) : (_size) < (10*1024*1024) ? (((_size)>>9)+1)>>1 : (((_size)>>19)+1)>>1
#define hwloc_memory_size_printf_unit(_size, _verbose) \
  (_size) < (10*1024) || _verbose ? "KB" : (_size) < (10*1024*1024) ? "MB" : "GB"

int
hwloc_obj_type_snprintf(char * __hwloc_restrict string, size_t size, hwloc_obj_t obj, int verbose)
{
  hwloc_obj_type_t type = obj->type;
  switch (type) {
  case HWLOC_OBJ_SYSTEM:
  case HWLOC_OBJ_MACHINE:
  case HWLOC_OBJ_NODE:
  case HWLOC_OBJ_SOCKET:
  case HWLOC_OBJ_CORE:
    return hwloc_snprintf(string, size, "%s", hwloc_obj_type_string(type));
  case HWLOC_OBJ_PROC:
    return hwloc_snprintf(string, size, "%s", verbose ? hwloc_obj_type_string(type) : "P");
  case HWLOC_OBJ_CACHE:
    return hwloc_snprintf(string, size, "L%u%s", obj->attr->cache.depth, verbose ? hwloc_obj_type_string(type): "");
  case HWLOC_OBJ_MISC:
	  /* TODO: more pretty presentation? */
    return hwloc_snprintf(string, size, "%s%u", hwloc_obj_type_string(type), obj->attr->misc.depth);
  default:
    *string = '\0';
    return 0;
  }
}

int
hwloc_obj_attr_snprintf(char * __hwloc_restrict string, size_t size, hwloc_obj_t obj, const char * separator, int verbose)
{
  switch (obj->type) {
  case HWLOC_OBJ_SYSTEM:
    if (verbose)
      return hwloc_snprintf(string, size, "%lu%s%sHP=%lu*%lukB%s%s%s%s",
			    hwloc_memory_size_printf_value(obj->attr->system.memory_kB, verbose),
			    hwloc_memory_size_printf_unit(obj->attr->system.memory_kB, verbose),
			    separator,
			    obj->attr->system.huge_page_free, obj->attr->system.huge_page_size_kB,
			    separator,
			    obj->attr->system.dmi_board_vendor?obj->attr->system.dmi_board_vendor:"",
			    separator,
			    obj->attr->system.dmi_board_name?obj->attr->system.dmi_board_name:"");
    else
      return hwloc_snprintf(string, size, "%lu%s",
			    hwloc_memory_size_printf_value(obj->attr->system.memory_kB, verbose),
			    hwloc_memory_size_printf_unit(obj->attr->system.memory_kB, verbose));
  case HWLOC_OBJ_MACHINE:
    if (verbose)
      return hwloc_snprintf(string, size, "%lu%s%sHP=%lu*%lukB%s%s%s%s",
			    hwloc_memory_size_printf_value(obj->attr->machine.memory_kB, verbose),
			    hwloc_memory_size_printf_unit(obj->attr->machine.memory_kB, verbose),
			    separator,
			    obj->attr->machine.huge_page_free, obj->attr->machine.huge_page_size_kB,
			    separator,
			    obj->attr->machine.dmi_board_vendor?obj->attr->machine.dmi_board_vendor:"",
			    separator,
			    obj->attr->machine.dmi_board_name?obj->attr->machine.dmi_board_name:"");
    else
      return hwloc_snprintf(string, size, "%lu%s",
			    hwloc_memory_size_printf_value(obj->attr->machine.memory_kB, verbose),
			    hwloc_memory_size_printf_unit(obj->attr->machine.memory_kB, verbose));
  case HWLOC_OBJ_NODE:
    return hwloc_snprintf(string, size, "%lu%s",
			  hwloc_memory_size_printf_value(obj->attr->node.memory_kB, verbose),
			  hwloc_memory_size_printf_unit(obj->attr->node.memory_kB, verbose));
  case HWLOC_OBJ_CACHE:
    return hwloc_snprintf(string, size, "%lu%s",
			  hwloc_memory_size_printf_value(obj->attr->node.memory_kB, verbose),
			  hwloc_memory_size_printf_unit(obj->attr->node.memory_kB, verbose));
  default:
    *string = '\0';
    return 0;
  }
}


int
hwloc_obj_snprintf(char *string, size_t size,
    struct hwloc_topology *topology __hwloc_attribute_unused, struct hwloc_obj *l, const char *_indexprefix, int verbose)
{
  hwloc_obj_type_t type = l->type;
  const char *indexprefix = _indexprefix ? _indexprefix : "#";
  char os_index[12] = "";

  if (l->os_index != (unsigned) -1) {
      snprintf(os_index, 12, "%s%u", indexprefix, l->os_index);
  }

  switch (type) {
  case HWLOC_OBJ_SOCKET:
  case HWLOC_OBJ_CORE:
    return hwloc_snprintf(string, size, "%s%s", hwloc_obj_type_string(type), os_index);
  case HWLOC_OBJ_MISC:
	  /* TODO: more pretty presentation? */
    return hwloc_snprintf(string, size, "%s%u%s", hwloc_obj_type_string(type), l->attr->misc.depth, os_index);
  case HWLOC_OBJ_PROC:
    return hwloc_snprintf(string, size, "P%s", os_index);
  case HWLOC_OBJ_SYSTEM:
    if (verbose)
      return hwloc_snprintf(string, size, "%s(%lu%s HP=%lu*%lukB %s %s)", hwloc_obj_type_string(type),
		      hwloc_memory_size_printf_value(l->attr->system.memory_kB, verbose),
		      hwloc_memory_size_printf_unit(l->attr->system.memory_kB, verbose),
		      l->attr->system.huge_page_free, l->attr->system.huge_page_size_kB,
		      l->attr->system.dmi_board_vendor?l->attr->system.dmi_board_vendor:"",
		      l->attr->system.dmi_board_name?l->attr->system.dmi_board_name:"");
    else
      return hwloc_snprintf(string, size, "%s(%lu%s)", hwloc_obj_type_string(type),
		      hwloc_memory_size_printf_value(l->attr->system.memory_kB, verbose),
		      hwloc_memory_size_printf_unit(l->attr->system.memory_kB, verbose));
  case HWLOC_OBJ_MACHINE:
    if (verbose)
      return hwloc_snprintf(string, size, "%s(%lu%s HP=%lu*%lukB %s %s)", hwloc_obj_type_string(type),
		      hwloc_memory_size_printf_value(l->attr->machine.memory_kB, verbose),
		      hwloc_memory_size_printf_unit(l->attr->machine.memory_kB, verbose),
		      l->attr->machine.huge_page_free, l->attr->machine.huge_page_size_kB,
		      l->attr->machine.dmi_board_vendor?l->attr->machine.dmi_board_vendor:"",
		      l->attr->machine.dmi_board_name?l->attr->machine.dmi_board_name:"");
    else
      return hwloc_snprintf(string, size, "%s%s(%lu%s)", hwloc_obj_type_string(type), os_index,
		      hwloc_memory_size_printf_value(l->attr->machine.memory_kB, verbose),
		      hwloc_memory_size_printf_unit(l->attr->machine.memory_kB, verbose));
  case HWLOC_OBJ_NODE:
    return hwloc_snprintf(string, size, "%s%s(%lu%s)",
		    verbose ? hwloc_obj_type_string(type) : "Node", os_index,
		    hwloc_memory_size_printf_value(l->attr->node.memory_kB, verbose),
		    hwloc_memory_size_printf_unit(l->attr->node.memory_kB, verbose));
  case HWLOC_OBJ_CACHE:
    return hwloc_snprintf(string, size, "L%u%s%s(%lu%s)", l->attr->cache.depth,
		      verbose ? hwloc_obj_type_string(type) : "", os_index,
		    hwloc_memory_size_printf_value(l->attr->node.memory_kB, verbose),
		    hwloc_memory_size_printf_unit(l->attr->node.memory_kB, verbose));
  case HWLOC_OBJ_BRIDGE:
    if (verbose) {
      char up[32], down[32];
      /* upstream is PCI or HOST */
      if (l->attr->bridge.upstream_type == HWLOC_OBJ_BRIDGE_PCI)
	snprintf(up, sizeof(up), "PCI%04x:%02x:%02x.%01x",
		 l->attr->pcidev.domain, l->attr->pcidev.bus, l->attr->pcidev.dev, l->attr->pcidev.func);
      else /* HWLOC_OBJ_BRIDGE_HOST */
	snprintf(up, sizeof(up), "Host");
      /* downstream is_PCI */
      snprintf(down, sizeof(down), "PCI%04x:[%02x-%02x]",
	       l->attr->bridge.downstream.pci.domain, l->attr->bridge.downstream.pci.secondary_bus, l->attr->bridge.downstream.pci.subordinate_bus);
      return snprintf(string, size, "Bridge %s->%s", up, down);
    } else {
      if (l->attr->bridge.upstream_type == HWLOC_OBJ_BRIDGE_PCI)
        return snprintf(string, size, "PCI %04x:%04x", l->attr->pcidev.vendor_id, l->attr->pcidev.device_id);
      else if (l->attr->bridge.upstream_type == HWLOC_OBJ_BRIDGE_HOST)
	return snprintf(string, size, "HostBridge");
    }
  case HWLOC_OBJ_PCI_DEVICE:
    if (verbose)
      return snprintf(string, size, "%s%04x:%02x:%02x.%01x(%04x:%04x,class=%04x(%s))", hwloc_obj_type_string(type),
		      l->attr->pcidev.domain, l->attr->pcidev.bus, l->attr->pcidev.dev, l->attr->pcidev.func,
		      l->attr->pcidev.device_id, l->attr->pcidev.vendor_id, l->attr->pcidev.class_id, hwloc_pci_class_string(l->attr->pcidev.class_id));
    else {
      return snprintf(string, size, "%s %04x:%04x", hwloc_pci_class_string(l->attr->pcidev.class_id), l->attr->pcidev.vendor_id, l->attr->pcidev.device_id);
    }
  default:
    *string = '\0';
    return 0;
  }
}

int hwloc_obj_cpuset_snprintf(char *str, size_t size, size_t nobj, struct hwloc_obj * const *objs)
{
  hwloc_cpuset_t set = hwloc_cpuset_alloc();
  int res;
  unsigned i;

  hwloc_cpuset_zero(set);
  for(i=0; i<nobj; i++)
    hwloc_cpuset_orset(set, objs[i]->cpuset);

  res = hwloc_cpuset_snprintf(str, size, set);
  hwloc_cpuset_free(set);
  return res;
}

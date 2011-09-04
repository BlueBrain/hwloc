/*
 * Copyright © 2011 INRIA.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>
#include <hwloc.h>
#include <private/misc.h>

/******************************
 ****** Escaping strings ******
 ******************************/

static char *
hwloc__json_escape_string(const char *src)
{
  int fulllen, sublen;
  char *escaped, *dst;

  fulllen = strlen(src);

  sublen = strcspn(src, "\"/\\\b\f\n\r\t");
  if (sublen == fulllen)
    return NULL; /* nothing to escape */

  escaped = malloc(fulllen*2+1);
  dst = escaped;

  memcpy(dst, src, sublen);
  src += sublen;
  dst += sublen;

  while (*src) {
    *(dst++) = '\\';
    switch (*src) {
    case '\"': *dst = '\"'; break;
    case '/':  *dst = '/';  break;
    case '\\': *dst = '\\'; break;
    case '\b': *dst = 'b';  break;
    case '\f': *dst = 'f';  break;
    case '\n': *dst = 'n';  break;
    case '\r': *dst = 'r';  break;
    case '\t': *dst = 't';  break;
    default: break;
    }
    dst++; src++;

    sublen = strcspn(src, "\"/\\\b\f\n\r\t");
    memcpy(dst, src, sublen);
    src += sublen;
    dst += sublen;
  }

  *dst = 0;
  return escaped;
}

/*******************************
 ********* JSON backend ********
 *******************************/

int
hwloc_backend_json_init(struct hwloc_topology *topology, const char *jsonbuffer, int buflen)
{
  topology->backend_params.json.buffer = malloc(buflen+1);
  memcpy(topology->backend_params.json.buffer, jsonbuffer, buflen);
  topology->backend_params.json.buffer[buflen] = '\0';
  topology->is_thissystem = 0;
  topology->backend_type = HWLOC_BACKEND_JSON;
  return 0;
}

void
hwloc_backend_json_exit(struct hwloc_topology *topology)
{
  assert(topology->backend_type == HWLOC_BACKEND_JSON);
  free(topology->backend_params.json.buffer);
  topology->backend_type = HWLOC_BACKEND_NONE;
}

/******************************
 ********* JSON import ********
 ******************************/

static char * hwloc__json_import_object(struct hwloc_topology *topology, struct hwloc_obj *obj, char *buffer);

/* skip spaces until the next interesting char */
static char *
hwloc__json_import_ignore_spaces(char *buffer)
{
  return buffer + strspn(buffer, " \t\n");
}

/* skip spaces and possibly one comma (end of line) until the next interesting char */
static char *
hwloc__json_import_ignore_spaces_and_comma(char *buffer)
{
  buffer = hwloc__json_import_ignore_spaces(buffer);
  if (*buffer == ',')
    buffer = hwloc__json_import_ignore_spaces(buffer+1);
  return buffer;
}

/* return the beginning of the next field and put a pointer to its contents in next */
static char *
hwloc__json_import_next_field(char *buffer, char **next)
{
  char *end;
  char *contents;

  buffer = hwloc__json_import_ignore_spaces(buffer);
  if (*buffer != '\"')
    return NULL;
  buffer++;

  end = strchr(buffer, '\"');
  if (!end)
    return NULL;
  *end = '\0'; /* split the name from the contents */

  contents = strchr(end+1, ':');
  if (!contents)
    return NULL;
  *next = hwloc__json_import_ignore_spaces(contents+1);

  return buffer;
}

/* given a buffer starting right after the colon (between field name and value),
 * returns the value, and a pointer to the next interesting char after the value.
 */
static char *
hwloc__json_import_field_value(char *buffer, char **next)
{
  if (*buffer == '[') {
    /* array value, let the caller handle it */
    *next = NULL;
    return buffer;

  } else if (buffer[0] == '\"') {
    /* string value, unescape it */
    size_t len, escaped;
    char *end;
    len = 0; escaped = 0;
    while (buffer[1+len+escaped] != '\"') {
      if (buffer[1+len+escaped] == '\\') {
	switch (buffer[1+len+escaped+1]) {
	case '\"': buffer[1+len] = '\"'; break;
	case '/':  buffer[1+len] = '/'; break;
	case '\\': buffer[1+len] = '\\'; break;
	case 'b':  buffer[1+len] = '\b'; break;
	case 'f':  buffer[1+len] = '\f'; break;
	case 'n':  buffer[1+len] = '\n'; break;
	case 'r':  buffer[1+len] = '\r'; break;
	case 't':  buffer[1+len] = '\t'; break;
        default: break;
	}
	escaped++;
      } else {
	buffer[1+len] = buffer[1+len+escaped];
      }
      len++;
    }
    buffer[1+len] = '\0';
    end = &buffer[1+len+escaped+1]; /* skip the ending " */
    *next = hwloc__json_import_ignore_spaces_and_comma(end);
    return buffer+1;

  } else {
    /* integer value */
    char *end = buffer + strspn(buffer, "01234567890-");
    *next = hwloc__json_import_ignore_spaces_and_comma(end);
    return buffer;
  }
}

/* given a buffer that starts right after the beginning '[' of the page_types array,
 * gather all page_types entries and return the next interesting char after the ending ']'.
 */
static char *
hwloc__json_import_page_types(struct hwloc_obj *obj, char *buffer)
{
  while (*buffer == '{') {
    unsigned index = obj->memory.page_types_len++;
    obj->memory.page_types = realloc(obj->memory.page_types, obj->memory.page_types_len*sizeof(*obj->memory.page_types));

    buffer = hwloc__json_import_ignore_spaces(buffer+1);
    while (*buffer != '}') {
      char *contents, *name, *value;
      name = hwloc__json_import_next_field(buffer, &contents);
      if (!name || !contents) {
	fprintf(stderr, "failed to read next field name\n");
	return NULL;
      }
      value = hwloc__json_import_field_value(contents, &buffer);
      if (!value) {
	fprintf(stderr, "failed to read field %s value\n", name);
	return NULL;
      }

      if (!strcmp(name, "size")) {
	obj->memory.page_types[index].size = strtoull(value, NULL, 10);
      } else if (!strcmp(name, "count")) {
	obj->memory.page_types[index].count = strtoull(value, NULL, 10);
      } else {
	fprintf(stderr, "ignoring page_type attribute %s\n", name);
	return NULL;
      }
    }
    buffer = hwloc__json_import_ignore_spaces_and_comma(buffer+1);
  }

  if (*buffer != ']') {
    fprintf(stderr, "ignoring page_type array not ending with ]\n");
    return NULL;
  }
  return hwloc__json_import_ignore_spaces_and_comma(buffer+1);
}

/* given a buffer that starts right after the beginning '[' of the infos array,
 * gather all infos entries and return the next interesting char after the ending ']'.
 */
static char *
hwloc__json_import_infos(struct hwloc_obj *obj, char *buffer)
{
  while (*buffer == '{') {
    unsigned index = obj->infos_count++;
    obj->infos = realloc(obj->infos, obj->infos_count*sizeof(*obj->infos));

    buffer = hwloc__json_import_ignore_spaces(buffer+1);
    while (*buffer != '}') {
      char *contents, *name, *value;
      name = hwloc__json_import_next_field(buffer, &contents);
      if (!name || !contents) {
	fprintf(stderr, "failed to read next field name\n");
	return NULL;
      }
      value = hwloc__json_import_field_value(contents, &buffer);
      if (!value) {
	fprintf(stderr, "failed to read field %s value\n", name);
	return NULL;
      }

      if (!strcmp(name, "name")) {
	obj->infos[index].name = strdup(value);
      } else if (!strcmp(name, "value")) {
	obj->infos[index].value = strdup(value);
      } else {
	fprintf(stderr, "ignoring infos attribute %s\n", name);
	return NULL;
      }
    }
    buffer = hwloc__json_import_ignore_spaces_and_comma(buffer+1);
  }

  if (*buffer != ']') {
    fprintf(stderr, "ignoring info array not ending with ]\n");
    return NULL;
  }
  return hwloc__json_import_ignore_spaces_and_comma(buffer+1);
}

/* given a buffer that starts right after the beginning '[' of the distances matrix array,
 * gather all matrix values and return the next interesting char after the ending ']'.
 */
static char *
hwloc__json_import_distances_matrix(float *matrix, unsigned nbobjs, char *buffer)
{
  unsigned index = 0;
  while (*buffer == '\"') {
    if (index >= nbobjs*nbobjs) {
      fprintf(stderr, "ignoring latency matrix with more than %u values\n", nbobjs*nbobjs);
      return NULL;
    }

    buffer = hwloc__json_import_ignore_spaces(buffer+1);
    matrix[index] = atof(buffer);
    index++;
    buffer = strchr(buffer, '\"');
    if (!buffer)
      return NULL;
    buffer = hwloc__json_import_ignore_spaces_and_comma(buffer+1);
  }

  if (*buffer != ']') {
    fprintf(stderr, "ignoring distances array not ending with ]\n");
    return NULL;
  }
  return hwloc__json_import_ignore_spaces_and_comma(buffer+1);
}

/* given a buffer that starts right after the beginning '[' of the distances array,
 * gather all distances entries and return the next interesting char after the ending ']'.
 */
static char *
hwloc__json_import_distances(struct hwloc_topology *topology __hwloc_attribute_unused, struct hwloc_obj *obj, char *buffer)
{
  unsigned i;

  while (*buffer == '{') {
    unsigned nbobjs = 0;
    unsigned index = obj->distances_count++;
    obj->distances = realloc(obj->distances, obj->distances_count*sizeof(*obj->distances));
    obj->distances[index] = malloc(sizeof(**obj->distances));

    buffer = hwloc__json_import_ignore_spaces(buffer+1);
    while (*buffer != '}') {
      char *contents, *name, *value;
      name = hwloc__json_import_next_field(buffer, &contents);
      if (!name || !contents) {
	fprintf(stderr, "failed to read next field name\n");
	return NULL;
      }

      if (*contents == '[') {
	/* sub array */
	buffer = hwloc__json_import_ignore_spaces(contents+1);
	if (!strcmp(name, "latency")) {
	  float *matrix = obj->distances[index]->latency = malloc(nbobjs*nbobjs*sizeof(float));
	  float latmax = 0;
	  buffer = hwloc__json_import_distances_matrix(matrix, nbobjs, buffer);
	  for(i=0; i<nbobjs*nbobjs; i++)
	    if (matrix[index] > latmax)
	      latmax = matrix[index];
	   obj->distances[index]->latency_max = latmax;
	} else {
	  fprintf(stderr, "ignoring unknown distances array %s\n", name);
	  return NULL;
	}
	if (!buffer)
	  return NULL;
	continue;
      }

      value = hwloc__json_import_field_value(contents, &buffer);
      if (!value) {
	fprintf(stderr, "failed to read field %s value\n", name);
	return NULL;
      }

      if (!strcmp(name, "nbobjs")) {
	obj->distances[index]->nbobjs = nbobjs = strtoul(value, NULL, 10);
      } else if (!strcmp(name, "relative_depth")) {
	obj->distances[index]->relative_depth = strtoul(value, NULL, 10);
      } else if (!strcmp(name, "latency_base")) {
	obj->distances[index]->latency_base = atof(value);
      } else {
	fprintf(stderr, "ignoring distances attribute %s\n", name);
	return NULL;
      }
    }
    buffer = hwloc__json_import_ignore_spaces_and_comma(buffer+1);
  }

  if (*buffer != ']') {
    fprintf(stderr, "ignoring distances array not ending with ]\n");
    return NULL;
  }
  return hwloc__json_import_ignore_spaces_and_comma(buffer+1);
}

/* given a buffer that starts right after the beginning '[' of the children array,
 * gather all children objects and return the next interesting char after the ending ']'.
 */
static char *
hwloc__json_import_children(struct hwloc_topology *topology, struct hwloc_obj *obj, char *buffer)
{
  while (*buffer == '{') {
    hwloc_obj_t childobj = hwloc_alloc_setup_object(HWLOC_OBJ_TYPE_MAX, -1);
    hwloc_insert_object_by_parent(topology, obj, childobj);
    buffer = hwloc__json_import_ignore_spaces(buffer+1);
    buffer = hwloc__json_import_object(topology, childobj, buffer);
    if (!buffer)
      return NULL;
  }

  if (*buffer != ']') {
    fprintf(stderr, "ignoring children array not ending with ]\n");
    return NULL;
  }
  return hwloc__json_import_ignore_spaces_and_comma(buffer+1);
}

/* given a buffer that starts right after the beginning '{' of an object,
 * gather all object information and return the next interesting char after the ending '}'.
 */
static char *
hwloc__json_import_object(struct hwloc_topology *topology, struct hwloc_obj *obj, char *buffer)
{
  while (*buffer != '}') {
    char *contents, *name, *value;
    name = hwloc__json_import_next_field(buffer, &contents);
    if (!name || !contents) {
      fprintf(stderr, "failed to read next field name\n");
      return NULL;
    }

    if (*contents == '[') {
      /* sub array */
      buffer = hwloc__json_import_ignore_spaces(contents+1);
      if (!strcmp(name, "children")) {
	buffer = hwloc__json_import_children(topology, obj, buffer);
      } else if (!strcmp(name, "page_type")) {
	buffer = hwloc__json_import_page_types(obj, buffer);
      } else if (!strcmp(name, "info")) {
	buffer = hwloc__json_import_infos(obj, buffer);
      } else if (!strcmp(name, "distances")) {
	buffer = hwloc__json_import_distances(topology, obj, buffer);
      } else {
	fprintf(stderr, "ignoring unknown array %s\n", name);
	return NULL;
      }
      if (!buffer)
	return NULL;
      continue;
    }

    /* normal field, gets its value */
    value = hwloc__json_import_field_value(contents, &buffer);
    if (!value) {
      fprintf(stderr, "failed to read field %s value\n", name);
      return NULL;
    }

    if (!strcmp(name, "type")) {
      obj->type = hwloc_obj_type_of_string(value);
    } else if (!strcmp(name, "os_level")) {
      obj->os_level = strtoul(value, NULL, 10);
    } else if (!strcmp(name, "os_index")) {
      obj->os_index = strtoul(value, NULL, 10);
    }

    else if (!strcmp(name, "cpuset")) {
      obj->cpuset = hwloc_bitmap_alloc();
      hwloc_bitmap_sscanf(obj->cpuset, value);
    } else if (!strcmp(name, "complete_cpuset")) {
      obj->complete_cpuset = hwloc_bitmap_alloc();
      hwloc_bitmap_sscanf(obj->complete_cpuset, value);
    } else if (!strcmp(name, "allowed_cpuset")) {
      obj->allowed_cpuset = hwloc_bitmap_alloc();
      hwloc_bitmap_sscanf(obj->allowed_cpuset, value);
    } else if (!strcmp(name, "online_cpuset")) {
      obj->online_cpuset = hwloc_bitmap_alloc();
      hwloc_bitmap_sscanf(obj->online_cpuset, value);
    } else if (!strcmp(name, "nodeset")) {
      obj->nodeset = hwloc_bitmap_alloc();
      hwloc_bitmap_sscanf(obj->nodeset, value);
    } else if (!strcmp(name, "complete_nodeset")) {
      obj->complete_nodeset = hwloc_bitmap_alloc();
      hwloc_bitmap_sscanf(obj->complete_nodeset, value);
    } else if (!strcmp(name, "allowed_nodeset")) {
      obj->allowed_nodeset = hwloc_bitmap_alloc();
      hwloc_bitmap_sscanf(obj->allowed_nodeset, value);
    }

    else if (!strcmp(name, "name"))
      obj->name = strdup(value);

    else if (!strcmp(name, "cache_size")) {
      unsigned long long lvalue = strtoull(value, NULL, 10);
      if (obj->type == HWLOC_OBJ_CACHE)
	obj->attr->cache.size = lvalue;
      else
	fprintf(stderr, "ignoring cache_size attribute for non-cache object type\n");
    }
    else if (!strcmp(name, "cache_linesize")) {
      unsigned long lvalue = strtoul(value, NULL, 10);
      if (obj->type == HWLOC_OBJ_CACHE)
	obj->attr->cache.linesize = lvalue;
      else
	fprintf(stderr, "ignoring cache_linesize attribute for non-cache object type\n");
    }
    else if (!strcmp(name, "cache_associativity")) {
      unsigned long lvalue = strtoul(value, NULL, 10);
      if (obj->type == HWLOC_OBJ_CACHE)
	obj->attr->cache.associativity = lvalue;
      else
	fprintf(stderr, "ignoring cache_associativity attribute for non-cache object type\n");
    }

    else if (!strcmp(name, "local_memory"))
      obj->memory.local_memory = strtoull(value, NULL, 10);

    else if (!strcmp(name, "depth")) {
      unsigned long lvalue = strtoul(value, NULL, 10);
      switch (obj->type) {
      case HWLOC_OBJ_CACHE:
	obj->attr->cache.depth = lvalue;
	break;
      case HWLOC_OBJ_GROUP:
	obj->attr->group.depth = lvalue;
	break;
      case HWLOC_OBJ_BRIDGE:
	obj->attr->bridge.depth = lvalue;
	break;
      default:
	fprintf(stderr, "ignoring depth attribute for object type without depth\n");
	break;
      }
    }

    else if (!strcmp(name, "pci_busid")) {
      switch (obj->type) {
      case HWLOC_OBJ_PCI_DEVICE:
      case HWLOC_OBJ_BRIDGE: {
	unsigned domain, bus, dev, func;
	if (sscanf(value, "%04x:%02x:%02x.%01x",
		   &domain, &bus, &dev, &func) != 4) {
	  fprintf(stderr, "ignoring invalid pci_busid format string %s\n", value);
	} else {
	  obj->attr->pcidev.domain = domain;
	  obj->attr->pcidev.bus = bus;
	  obj->attr->pcidev.dev = dev;
	  obj->attr->pcidev.func = func;
	}
	break;
      }
      default:
	fprintf(stderr, "ignoring pci_busid attribute for non-PCI object\n");
	break;
      }
    }

    else if (!strcmp(name, "pci_type")) {
      switch (obj->type) {
      case HWLOC_OBJ_PCI_DEVICE:
      case HWLOC_OBJ_BRIDGE: {
	unsigned classid, vendor, device, subvendor, subdevice, revision;
	if (sscanf(value, "%04x [%04x:%04x] [%04x:%04x] %02x",
		   &classid, &vendor, &device, &subvendor, &subdevice, &revision) != 6) {
	  fprintf(stderr, "ignoring invalid pci_type format string %s\n", value);
	} else {
	  obj->attr->pcidev.class_id = classid;
	  obj->attr->pcidev.vendor_id = vendor;
	  obj->attr->pcidev.device_id = device;
	  obj->attr->pcidev.subvendor_id = subvendor;
	  obj->attr->pcidev.subdevice_id = subdevice;
	  obj->attr->pcidev.revision = revision;
	}
	break;
      }
      default:
	fprintf(stderr, "ignoring pci_type attribute for non-PCI object\n");
	break;
      }
    }

    else if (!strcmp(name, "pci_link_speed")) {
      switch (obj->type) {
      case HWLOC_OBJ_PCI_DEVICE:
      case HWLOC_OBJ_BRIDGE: {
        obj->attr->pcidev.linkspeed = atof(value);
	break;
      }
      default:
	fprintf(stderr, "ignoring pci_link_speed attribute for non-PCI object\n");
	break;
      }
    }

    else if (!strcmp(name, "bridge_type")) {
      switch (obj->type) {
      case HWLOC_OBJ_BRIDGE: {
	unsigned upstream_type, downstream_type;
	if (sscanf(value, "%u-%u", &upstream_type, &downstream_type) != 2)
	  fprintf(stderr, "ignoring invalid bridge_type format string %s\n", value);
	else {
	  obj->attr->bridge.upstream_type = upstream_type;
	  obj->attr->bridge.downstream_type = downstream_type;
	}
	break;
      }
      default:
	fprintf(stderr, "ignoring bridge_type attribute for non-bridge object\n");
	break;
      }
    }

    else if (!strcmp(name, "bridge_pci")) {
      switch (obj->type) {
      case HWLOC_OBJ_BRIDGE: {
	unsigned domain, secbus, subbus;
	if (sscanf(value, "%04x:[%02x-%02x]",
		   &domain, &secbus, &subbus) != 3) {
	  fprintf(stderr, "ignoring invalid bridge_pci format string %s\n", value);
	} else {
	  obj->attr->bridge.downstream.pci.domain = domain;
	  obj->attr->bridge.downstream.pci.secondary_bus = secbus;
	  obj->attr->bridge.downstream.pci.subordinate_bus = subbus;
	}
	break;
      }
      default:
	fprintf(stderr, "ignoring bridge_pci attribute for non-bridge object\n");
	break;
      }
    }

    else if (!strcmp(name, "osdev_type")) {
      switch (obj->type) {
      case HWLOC_OBJ_OS_DEVICE: {
	unsigned osdev_type;
	if (sscanf(value, "%u", &osdev_type) != 1)
	  fprintf(stderr, "ignoring invalid osdev_type format string %s\n", value);
 	else
	  obj->attr->osdev.type = osdev_type;
	break;
      }
      default:
	fprintf(stderr, "ignoring osdev_type attribute for non-osdev object\n");
	break;
      }
    }

    else {
      fprintf(stderr, "ignoring unknown field %s\n", name);
      return NULL;
    }
  }

  /* returns after '}' */
  return hwloc__json_import_ignore_spaces_and_comma(buffer+1);
}

void
hwloc_look_json(struct hwloc_topology *topology)
{
  char *begin, *contents, *end;

  topology->support.discovery->pu = 1;

  if (*topology->backend_params.json.buffer != '{')
    return;

  begin = hwloc__json_import_next_field(topology->backend_params.json.buffer+1, &contents);
  if (!begin)
    return;
  if (strcmp(begin, "topology") || *contents != '{')
    return;

  end = hwloc__json_import_object(topology, hwloc_get_root_obj(topology), contents+1);
  if (!end) {
    /* parsing went bad, the topology may not be in a very good shape (worse than with XML?) */
  }

  /* keep the "Backend" information intact */
  /* we could add "BackendSource=JSON" to notify that JSON was used between the actual backend and here */

  /* if we added some distances, we must check them, and make them groupable */
  /* FIXME: hwloc_xml__handle_distances(topology);*/
}

/*******************************
 ********* JSON export *********
 *******************************/

#define CHECK_SNPRINTF_RETURN(res, buf, len, ret)	\
  if (res < 0)						\
    return -1;						\
  ret += res;						\
  if (res >= (int) len)					\
    res = len>0 ? len - 1 : 0;				\
  buf += res;						\
  len -= res;

#define HWLOC_JSON_EXPORT_INDENT 4

/* exporting a static string */
#define hwloc__json_export__line(buf, len, indent, line, comma) \
  hwloc_snprintf(buf, len, "%*s%s%s\n", HWLOC_JSON_EXPORT_INDENT*(indent), " ", line, (comma) ? "," : "")
#define hwloc__json_export_line(buf, len, indent, line) \
  hwloc__json_export__line(buf, len, indent, line, 1)
#define hwloc__json_export_line_nocomma(buf, len, indent, line) \
  hwloc__json_export__line(buf, len, indent, line, 0)

/* exporting an array value */
#define hwloc__json_export__value(buf, len, indent, format, value, comma) \
  hwloc_snprintf(buf, len, "%*s" format "%s\n", HWLOC_JSON_EXPORT_INDENT*(indent), " ", value, (comma) ? "," : "")

/* exporting an attribute */
#define hwloc__json_export__attr(buf, len, indent, name, format, value, comma) \
  hwloc_snprintf(buf, len, "%*s\"%s\":" format "%s\n", HWLOC_JSON_EXPORT_INDENT*(indent), " ", name, value, (comma) ? "," : "")
#define hwloc__json_export_attr(buf, len, indent, name, format, value) \
  hwloc__json_export__attr(buf, len, indent, name, format, value, 1)
#define hwloc__json_export_attr_nocomma(buf, len, indent, name, format, value) \
  hwloc__json_export__attr(buf, len, indent, name, format, value, 0)

/* exporting an unsafe-string attribute */
static int
hwloc__json_export__attr_unsafestring(char *buf, size_t len, unsigned indent, const char *name, const char *value, int comma)
{
  char *escaped = hwloc__json_escape_string(value);
  if (escaped) {
    int ret = hwloc__json_export__attr(buf, len, indent, name, "\"%s\"", escaped, comma);
    free(escaped);
    return ret;
  } else {
    return hwloc__json_export__attr(buf, len, indent, name, "\"%s\"", value, comma);
  }
}
#define hwloc__json_export_attr_unsafestring(buf, len, indent, name, value) \
  hwloc__json_export__attr_unsafestring(buf, len, indent, name, value, 1)
#define hwloc__json_export_attr_unsafestring_nocomma(buf, len, indent, name, value) \
  hwloc__json_export__attr_unsafestring(buf, len, indent, name, value, 0)

/* exporting a bitmap attribute (always with a comma) */
static int
hwloc__json_export_attr_bitmap(char *buf, size_t len, unsigned indent, const char *name, hwloc_const_bitmap_t bitmap)
{
  char *bitmapstring;
  int ret;
  if (!bitmap)
    return 0;
  hwloc_bitmap_asprintf(&bitmapstring, bitmap);
  ret = hwloc__json_export_attr(buf, len, indent, name, "\"%s\"", bitmapstring);
  free(bitmapstring);
  return ret;
}


static int
hwloc__json_export_object (char *buf, size_t len, unsigned indent, hwloc_obj_t obj)
{
  size_t origlen = len;
  char tmp[255];
  unsigned i;
  int ret = 0, res;

  res = hwloc__json_export_attr(buf, len, indent, "type", "\"%s\"", hwloc_obj_type_string(obj->type));
  CHECK_SNPRINTF_RETURN(res, buf, len, ret);

  if (obj->os_index != (unsigned) -1) {
    res = hwloc__json_export_attr(buf, len, indent, "os_index", "%u", obj->os_index);
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
  }
  if (obj->os_level != -1) {
    res = hwloc__json_export_attr(buf, len, indent, "os_level", "%u", obj->os_level);
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
  }

  res = hwloc__json_export_attr_bitmap(buf, len, indent, "cpuset", obj->cpuset);
  CHECK_SNPRINTF_RETURN(res, buf, len, ret);
  res = hwloc__json_export_attr_bitmap(buf, len, indent, "complete_cpuset", obj->complete_cpuset);
  CHECK_SNPRINTF_RETURN(res, buf, len, ret);
  res = hwloc__json_export_attr_bitmap(buf, len, indent, "online_cpuset", obj->online_cpuset);
  CHECK_SNPRINTF_RETURN(res, buf, len, ret);
  res = hwloc__json_export_attr_bitmap(buf, len, indent, "allowed_cpuset", obj->allowed_cpuset);
  CHECK_SNPRINTF_RETURN(res, buf, len, ret);
  res = hwloc__json_export_attr_bitmap(buf, len, indent, "nodeset", obj->nodeset);
  CHECK_SNPRINTF_RETURN(res, buf, len, ret);
  res = hwloc__json_export_attr_bitmap(buf, len, indent, "complete_nodeset", obj->complete_nodeset);
  CHECK_SNPRINTF_RETURN(res, buf, len, ret);
  res = hwloc__json_export_attr_bitmap(buf, len, indent, "allowed_nodeset", obj->allowed_nodeset);
  CHECK_SNPRINTF_RETURN(res, buf, len, ret);

  if (obj->name) {
    res = hwloc__json_export_attr_unsafestring(buf, len, indent, "name", obj->name);
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
  }

  switch (obj->type) {
  case HWLOC_OBJ_CACHE:
    res = hwloc__json_export_attr(buf, len, indent, "cache_size", "%llu", (unsigned long long) obj->attr->cache.size);
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
    res = hwloc__json_export_attr(buf, len, indent, "depth", "%u", obj->attr->cache.depth);
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
    res = hwloc__json_export_attr(buf, len, indent, "cache_linesize", "%u", obj->attr->cache.linesize);
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
    res = hwloc__json_export_attr(buf, len, indent, "cache_associativity", "%d", obj->attr->cache.associativity);
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
    break;
  case HWLOC_OBJ_GROUP:
    res = hwloc__json_export_attr(buf, len, indent, "depth", "%u", obj->attr->group.depth);
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
    break;
  case HWLOC_OBJ_BRIDGE:
    sprintf(tmp, "%u-%u", obj->attr->bridge.upstream_type, obj->attr->bridge.downstream_type);
    res = hwloc__json_export_attr(buf, len, indent, "bridge_type", "\"%s\"", tmp);
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
    res = hwloc__json_export_attr(buf, len, indent, "depth", "%u", obj->attr->bridge.depth);
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
    if (obj->attr->bridge.downstream_type == HWLOC_OBJ_BRIDGE_PCI) {
      sprintf(tmp, "%04x:[%02x-%02x]",
	      (unsigned) obj->attr->bridge.downstream.pci.domain,
	      (unsigned) obj->attr->bridge.downstream.pci.secondary_bus,
	      (unsigned) obj->attr->bridge.downstream.pci.subordinate_bus);
      res = hwloc__json_export_attr(buf, len, indent, "bridge_pci", "\"%s\"", tmp);
      CHECK_SNPRINTF_RETURN(res, buf, len, ret);
    }
    if (obj->attr->bridge.upstream_type != HWLOC_OBJ_BRIDGE_PCI)
      break;
    /* fallthrough */
  case HWLOC_OBJ_PCI_DEVICE:
    sprintf(tmp, "%04x:%02x:%02x.%01x",
	    (unsigned) obj->attr->pcidev.domain,
	    (unsigned) obj->attr->pcidev.bus,
	    (unsigned) obj->attr->pcidev.dev,
	    (unsigned) obj->attr->pcidev.func);
    res = hwloc__json_export_attr(buf, len, indent, "pci_busid", "\"%s\"", tmp);
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
    sprintf(tmp, "%04x [%04x:%04x] [%04x:%04x] %02x",
	    (unsigned) obj->attr->pcidev.class_id,
	    (unsigned) obj->attr->pcidev.vendor_id, (unsigned) obj->attr->pcidev.device_id,
	    (unsigned) obj->attr->pcidev.subvendor_id, (unsigned) obj->attr->pcidev.subdevice_id,
	    (unsigned) obj->attr->pcidev.revision);
    res = hwloc__json_export_attr(buf, len, indent, "pci_type", "\"%s\"", tmp);
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
    res = hwloc__json_export_attr(buf, len, indent, "pci_link_speed", "\"%f\"", obj->attr->pcidev.linkspeed);
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
    break;
  case HWLOC_OBJ_OS_DEVICE:
    res = hwloc__json_export_attr(buf, len, indent, "osdev_type", "%u", obj->attr->osdev.type);
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
    break;
  default:
    break;
  }

  if (obj->memory.local_memory) {
    res = hwloc__json_export_attr(buf, len, indent, "local_memory", "%llu", (unsigned long long) obj->memory.local_memory);
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
  }
  if (obj->memory.page_types_len) {
    res = hwloc__json_export_line_nocomma(buf, len, indent, "\"page_type\":[");
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
    for(i=0; i<obj->memory.page_types_len; i++) {
      res = hwloc__json_export_line_nocomma(buf, len, indent+1, "{");
      CHECK_SNPRINTF_RETURN(res, buf, len, ret);
      res = hwloc__json_export_attr(buf, len, indent+2, "size", "%llu", (unsigned long long) obj->memory.page_types[i].size);
      CHECK_SNPRINTF_RETURN(res, buf, len, ret);
      res = hwloc__json_export_attr_nocomma(buf, len, indent+2, "count", "%llu", (unsigned long long) obj->memory.page_types[i].count);
      CHECK_SNPRINTF_RETURN(res, buf, len, ret);
      res = hwloc__json_export__line(buf, len, indent+1, "}", i != obj->memory.page_types_len-1);
      CHECK_SNPRINTF_RETURN(res, buf, len, ret);
    }
    res = hwloc__json_export_line(buf, len, indent, "]");
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
  }

  if (obj->infos_count) {
    res = hwloc__json_export_line_nocomma(buf, len, indent, "\"info\":[");
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
    for(i=0; i<obj->infos_count; i++) {
      res = hwloc__json_export_line_nocomma(buf, len, indent+1, "{");
      CHECK_SNPRINTF_RETURN(res, buf, len, ret);
      res = hwloc__json_export_attr_unsafestring(buf, len, indent+2, "name", obj->infos[i].name);
      CHECK_SNPRINTF_RETURN(res, buf, len, ret);
      res = hwloc__json_export_attr_unsafestring_nocomma(buf, len, indent+2, "value", obj->infos[i].value);
      CHECK_SNPRINTF_RETURN(res, buf, len, ret);
      res = hwloc__json_export__line(buf, len, indent+1, "}", i != obj->infos_count-1);
      CHECK_SNPRINTF_RETURN(res, buf, len, ret);
    }
    res = hwloc__json_export_line(buf, len, indent, "]");
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
  }

  if (obj->distances_count) {
    res = hwloc__json_export_line_nocomma(buf, len, indent, "\"distances\":[");
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
    for(i=0; i<obj->distances_count; i++) {
      unsigned nbobjs = obj->distances[i]->nbobjs;
      unsigned j;
      res = hwloc__json_export_line_nocomma(buf, len, indent+1, "{");
      CHECK_SNPRINTF_RETURN(res, buf, len, ret);
      res = hwloc__json_export_attr(buf, len, indent+2, "nbobjs", "%u", nbobjs);
      CHECK_SNPRINTF_RETURN(res, buf, len, ret);
      res = hwloc__json_export_attr(buf, len, indent+2, "relative_depth", "%u", obj->distances[i]->relative_depth);
      CHECK_SNPRINTF_RETURN(res, buf, len, ret);
      res = hwloc__json_export_attr(buf, len, indent+2, "latency_base", "\"%f\"", obj->distances[i]->latency_base);
      CHECK_SNPRINTF_RETURN(res, buf, len, ret);
      res = hwloc__json_export_line_nocomma(buf, len, indent+2, "\"latency\":[");
      CHECK_SNPRINTF_RETURN(res, buf, len, ret);
      for(j=0; j<nbobjs*nbobjs; j++) {
	res = hwloc__json_export__value(buf, len, indent+3, "\"%f\"", obj->distances[i]->latency[j], j != nbobjs*nbobjs-1);
	CHECK_SNPRINTF_RETURN(res, buf, len, ret);
      }
      res = hwloc__json_export__line(buf, len, indent+2, "]" ,0);
      CHECK_SNPRINTF_RETURN(res, buf, len, ret);
      res = hwloc__json_export__line(buf, len, indent+1, "}", i != obj->distances_count-1);
      CHECK_SNPRINTF_RETURN(res, buf, len, ret);
    }
    res = hwloc__json_export_line(buf, len, indent, "]");
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
  }

  if (obj->arity) {
    res = hwloc__json_export__line(buf, len, indent, "\"children\":[", 0);
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
    for (i=0; i<obj->arity; i++) {
      res = hwloc__json_export__line(buf, len, indent+1, "{", 0);
      CHECK_SNPRINTF_RETURN(res, buf, len, ret);
      res = hwloc__json_export_object (buf, len, indent+2, obj->children[i]);
      CHECK_SNPRINTF_RETURN(res, buf, len, ret);
      res = hwloc__json_export__line(buf, len, indent+1, "}", i != obj->arity-1);
      CHECK_SNPRINTF_RETURN(res, buf, len, ret);
    }
    res = hwloc__json_export_line(buf, len, indent, "]");
    CHECK_SNPRINTF_RETURN(res, buf, len, ret);
  }

  /* remove the last comma before closing the object (if we actually wrote something) */
  if (origlen>0 && ret>0) {
    assert(buf[-1] == '\n');
    assert(buf[-2] == ',');
    buf[-2] = '\n';
    buf[-1] = '\0';
    ret--;
  }

  return ret;
}

static int
hwloc__json_export_topology (char *buf, size_t len, hwloc_topology_t topology)
{
  int ret = 0, res;

  res = hwloc_snprintf(buf, len, "{\n");
  CHECK_SNPRINTF_RETURN(res, buf, len, ret);
  res = hwloc_snprintf(buf, len, "%*s\"topology\":{\n", HWLOC_JSON_EXPORT_INDENT, " ");
  CHECK_SNPRINTF_RETURN(res, buf, len, ret);
  res = hwloc__json_export_object(buf, len, 2, hwloc_get_root_obj(topology));
  CHECK_SNPRINTF_RETURN(res, buf, len, ret);
  res = hwloc_snprintf(buf, len, "%*s}\n", HWLOC_JSON_EXPORT_INDENT, " ");
  CHECK_SNPRINTF_RETURN(res, buf, len, ret);
  res = hwloc_snprintf(buf, len, "}\n");
  CHECK_SNPRINTF_RETURN(res, buf, len, ret);

  return ret;
}

void hwloc_topology_export_jsonbuffer(hwloc_topology_t topology, char **jsonbuffer, int *buflen)
{
  int len = hwloc__json_export_topology(NULL, 0, topology);
  char *buf = malloc(len+1);
  hwloc__json_export_topology(buf, len+1, topology);
  *jsonbuffer = buf;
  *buflen = len+1;
}

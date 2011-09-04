/*
 * Copyright © 2011 INRIA.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>

#include <hwloc.h>
#include <string.h>

#include "lstopo.h"

void output_json(hwloc_topology_t topology, const char *filename, int logical __hwloc_attribute_unused, int legend __hwloc_attribute_unused, int verbose_mode __hwloc_attribute_unused)
{
  char *buffer;
  int len;
  FILE *output = open_file(filename, "w");
  if (!output) {
    fprintf(stderr, "Failed to open %s for writing (%s)\n", filename, strerror(errno));
    return;
  }

  hwloc_topology_export_jsonbuffer(topology, &buffer, &len);
  fprintf(output, "%s", buffer);
  free(buffer);

  if (output != stdout)
    fclose(output);
}

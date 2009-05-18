/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <topology.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static void usage(void)
{
  fprintf(stderr, "Usage: topomask [depth:index] ...\n");
  fprintf(stderr, "  <depth> may be machine, node, die, core, proc or a numeric depth\n");
  fprintf(stderr, "  <index> may be:\n");
  fprintf(stderr, "   X\tone object with index X\n");
  fprintf(stderr, "   X-Y\tall objects with index between X and Y\n");
  fprintf(stderr, "   X-\tall objects with index at least X\n");
  fprintf(stderr, "   X:N\tN objects starting with index X, possibly wrapping-around the end of the level\n");
}

static int append_object(topo_topology_t topology, struct topo_topology_info *topoinfo,
			 topo_cpuset_t *set, const char *string, const char *sep,
			 int verbose)
{
  topo_obj_t obj;
  unsigned depth, width;
  char *sep2;
  unsigned first, wrap, amount;
  unsigned i,j;

  if (!strncmp(string, "machine", 1))
    depth = topo_get_type_depth(topology, TOPO_OBJ_MACHINE);
  else if (!strncmp(string, "node", 1))
    depth = topo_get_type_depth(topology, TOPO_OBJ_NODE);
  else if (!strncmp(string, "die", 1))
    depth = topo_get_type_depth(topology, TOPO_OBJ_DIE);
  else if (!strncmp(string, "core", 1))
    depth = topo_get_type_depth(topology, TOPO_OBJ_CORE);
  else if (!strncmp(string, "proc", 1))
    depth = topo_get_type_depth(topology, TOPO_OBJ_PROC);
  else
    depth = atoi(string);

  if (depth >= topoinfo->depth) {
    if (verbose)
      fprintf(stderr, "ignoring invalid depth %d\n", depth);
    return -1;
  }
  width = topo_get_depth_nbitems(topology, depth);

  first = atoi(sep+1);
  amount = 1;
  wrap = 0;

  sep2 = strchr(sep+1, '-');
  if (sep2) {
    if (*(sep2+1) == '\0')
      amount = width-first;
    else
      amount = atoi(sep2+1)-first+1;
  } else {
    sep2 = strchr(sep+1, ':');
    if (sep2) {
      amount = atoi(sep2+1);
      wrap = 1;
    }
  }

  for(i=first, j=0; j<amount; i++, j++) {
    if (wrap && i==width)
      i = 0;

    obj = topo_get_object(topology, depth, i);
    if (obj) {
      topo_cpuset_orset(set, &obj->cpuset);
    }
    if (verbose) {
      if (obj)
	printf("object (%d,%d) found\n", depth, i);
      else
	printf("object (%d,%d) does not exist\n", depth, i);
    }
  }

  return 0;
}

int main(int argc, char *argv[])
{
  topo_topology_t topology;
  struct topo_topology_info topoinfo;
  topo_cpuset_t set;
  int verbose = 0;
  char string[TOPO_CPUSET_STRING_LENGTH+1];

  topo_cpuset_zero(&set);

  topo_topology_init(&topology);
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

  while (argc >= 2) {
    char *colon;

    if (*argv[1] == '-') {
      if (!strcmp(argv[1], "-v")) {
        verbose = 1;
        goto next;
      }
      usage();
      return EXIT_FAILURE;
    }

    colon = strchr(argv[1], ':');
    if (!colon) {
      usage();
      return EXIT_FAILURE;
    }

    append_object(topology, &topoinfo, &set, argv[1], colon, verbose);

 next:
    argc--;
    argv++;
  }

  topo_cpuset_snprintf(string, sizeof(string), &set);
  printf("%s\n", string);

  topo_topology_destroy(topology);

  return EXIT_SUCCESS;
}

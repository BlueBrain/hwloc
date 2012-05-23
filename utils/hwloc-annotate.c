/*
 * Copyright © 2012 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/private.h>
#include <hwloc-calc.h>
#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void usage(const char *callname __hwloc_attribute_unused, FILE *where)
{
	fprintf(where, "Usage: hwloc-annotate <input.xml> <output.xml> <location> <annotation>\n");
	fprintf(where, "  <annotation> may be:\n");
	fprintf(where, "    info <name> <value>\n");
	fprintf(where, "    valarray <name> <nb> <index1>=<value1> <index2>=<value2> [...]\n");
	fprintf(where, "  <location> may be:\n");
	fprintf(where, "    all, root, <type>:<logicalindex>\n");
}

static char *infoname = NULL, *infovalue = NULL;

static char *valarrayname = NULL;
static unsigned valarraynb;
static float *valarrayvalues = NULL;
static unsigned *valarrayidx = NULL;

static void apply(hwloc_obj_t obj)
{
	if (infoname)
		hwloc_obj_add_info(obj, infoname, infovalue);
	if (valarrayname)
		hwloc_obj_add_valarray(obj, valarrayname, valarraynb, valarrayvalues, valarrayidx);
}

static void apply_recursive(hwloc_obj_t obj)
{
	unsigned i;
	for(i=0; i<obj->arity; i++)
		apply_recursive(obj->children[i]);
	apply(obj);
}

int main(int argc, char *argv[])
{
	hwloc_topology_t topology;
	char *callname, *input, *output, *location;
	unsigned i;
	int err;

	putenv("HWLOC_XML_VERBOSE=1");

	callname = argv[0];
	argc--;
	argv++;

	/* FIXME: option for clearing */

	if (argc < 3) {
		usage(callname, stderr);
		exit(EXIT_FAILURE);
	}
	input = argv[0];
	output = argv[1];
	location = argv[2];
	argc -= 3;
	argv += 3;

	if (argc < 1) {
		usage(callname, stderr);
		exit(EXIT_FAILURE);
	}
	if (!strcmp(argv[0], "info")) {
		/* FIXME: handle "clear" ? */
		if (argc < 3) {
			usage(callname, stderr);
			exit(EXIT_FAILURE);
		}
		infoname = argv[1];
		infovalue = argv[2];

	} else if (!strcmp(argv[0], "valarray")) {
		/* FIXME: handle "clear" ? */
		if (argc < 3) {
			usage(callname, stderr);
			exit(EXIT_FAILURE);
		}
		valarrayname = argv[1];
		valarraynb = atoi(argv[2]);
		argc -= 3;
		argv += 3;

		if (argc < (int) valarraynb || !valarraynb) {
			usage(callname, stderr);
			exit(EXIT_FAILURE);
		}

		valarrayvalues = malloc(valarraynb * sizeof(*valarrayvalues));
		valarrayidx = malloc(valarraynb * sizeof(*valarrayidx));

		for(i=0; i<valarraynb; i++) {
			valarrayidx[i] = 0;
			valarrayvalues[i] = 0.0;
			sscanf(argv[i], "%u=%f", &valarrayidx[i], &valarrayvalues[i]);
		}

	} else {
		fprintf(stderr, "Unrecognized annotation type: %s\n", argv[0]);
		usage(callname, stderr);
		exit(EXIT_FAILURE);
	}

	hwloc_topology_init(&topology);
	err = hwloc_topology_set_xml(topology, input);
	if (err < 0)
		goto out;
	hwloc_topology_load(topology);

	if (!strcmp(location, "all")) {
		apply_recursive(hwloc_get_root_obj(topology));
	} else if (!strcmp(location, "root")) {
		apply(hwloc_get_root_obj(topology));
	} else {
		unsigned i;
		hwloc_obj_t obj;
		hwloc_obj_type_t type;
		size_t typelen;
		int depth;
		char *sep;
		typelen = strspn(location, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
		if (!typelen || location[typelen] != ':') {
			/* FIXME: warn */
			goto out;
		}
		sep = &location[typelen];
		depth = hwloc_calc_parse_depth_prefix(topology, hwloc_topology_get_depth(topology),
						      location, typelen, &type, 0);
		if (depth < 0) {
			/* FIXME: warn */
			goto out;
		}
		if (!strcmp(sep+1, "all")) {
			for(i=0; i<hwloc_get_nbobjs_by_depth(topology, depth); i++) {
				obj = hwloc_get_obj_by_depth(topology, depth, i);
				assert(obj);
				apply(obj);
			}
		} else {
			i = atoi(sep+1);
			obj = hwloc_get_obj_by_depth(topology, depth, i);
			if (!obj) {
				/* FIXME: warn */
				goto out;
			}
			apply(obj);
		}
	}

	err = hwloc_topology_export_xml(topology, output);
	if (err < 0)
		goto out;

	hwloc_topology_destroy(topology);
	exit(EXIT_SUCCESS);

out:
	hwloc_topology_destroy(topology);
	exit(EXIT_FAILURE);
}

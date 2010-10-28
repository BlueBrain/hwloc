/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * Copyright © 2009 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/private.h>
#include <hwloc-calc.h>
#include <hwloc.h>

#include <unistd.h>
#include <errno.h>

static void usage(FILE *where)
{
  fprintf(where, "Usage: hwloc-bind [options] <location> -- command ...\n");
  fprintf(where, " <location> may be a space-separated list of cpusets or objects\n");
  fprintf(where, "            as supported by the hwloc-calc utility.\n");
  fprintf(where, "Options:\n");
  fprintf(where, "  --cpubind      Use following arguments for cpu binding (default)\n");
  fprintf(where, "  --membind      Use following arguments for memory binding\n");
  fprintf(where, "  --mempolicy <default|firsttouch|bind|interleave|replicate|nexttouch>\n"
		 "                 Change the memory binding policy (default is bind)\n");
  fprintf(where, "  -l --logical   Take logical object indexes (default)\n");
  fprintf(where, "  -p --physical  Take physical object indexes\n");
  fprintf(where, "  --single       Bind on a single CPU to prevent migration\n");
  fprintf(where, "  --strict       Require strict binding\n");
  fprintf(where, "  --get          Retrieve current process binding\n");
  fprintf(where, "  --pid <pid>    Operate on process <pid>\n");
  fprintf(where, "  --taskset      Manipulate taskset-specific cpuset strings\n");
  fprintf(where, "  -v             Show verbose messages\n");
  fprintf(where, "  --version      Report version and exit\n");
}

int main(int argc, char *argv[])
{
  hwloc_topology_t topology;
  unsigned depth;
  hwloc_bitmap_t cpubind_set, membind_set;
  int cpubind = 1; /* membind if 0 */
  int get_binding = 0;
  int single = 0;
  int verbose = 0;
  int logical = 1;
  int taskset = 0;
  int cpubind_flags = 0;
  hwloc_membind_policy_t membind_policy = HWLOC_MEMBIND_BIND;
  int membind_flags = 0;
  int opt;
  int ret;
  hwloc_pid_t pid = 0;
  char **orig_argv = argv;

  cpubind_set = hwloc_bitmap_alloc();
  membind_set = hwloc_bitmap_alloc();

  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);
  depth = hwloc_topology_get_depth(topology);

  /* skip argv[0], handle options */
  argv++;
  argc--;

  while (argc >= 1) {
    if (!strcmp(argv[0], "--")) {
      argc--;
      argv++;
      break;
    }

    opt = 0;

    if (*argv[0] == '-') {
      if (!strcmp(argv[0], "-v")) {
	verbose = 1;
	goto next;
      }
      else if (!strcmp(argv[0], "--help")) {
	usage(stdout);
	return EXIT_SUCCESS;
      }
      else if (!strcmp(argv[0], "--single")) {
	single = 1;
	goto next;
      }
      else if (!strcmp(argv[0], "--strict")) {
	cpubind_flags |= HWLOC_CPUBIND_STRICT;
	membind_flags |= HWLOC_MEMBIND_STRICT;
	goto next;
      }
      else if (!strcmp(argv[0], "--pid")) {
        if (argc < 2) {
          usage (stderr);
          exit(EXIT_FAILURE);
        }
        pid = atoi(argv[1]);
        opt = 1;
        goto next;
      }
      else if (!strcmp (argv[0], "--version")) {
          printf("%s %s\n", orig_argv[0], VERSION);
          exit(EXIT_SUCCESS);
      }
      if (!strcmp(argv[0], "-l") || !strcmp(argv[0], "--logical")) {
        logical = 1;
        goto next;
      }
      if (!strcmp(argv[0], "-p") || !strcmp(argv[0], "--physical")) {
        logical = 0;
        goto next;
      }
      if (!strcmp(argv[0], "--taskset")) {
        taskset = 1;
        goto next;
      }
      else if (!strcmp (argv[0], "--get")) {
	get_binding = 1;
	goto next;
      }
      else if (!strcmp (argv[0], "--cpubind")) {
	  cpubind = 1;
	  goto next;
      }
      else if (!strcmp (argv[0], "--membind")) {
	  cpubind = 0;
	  goto next;
      }
      else if (!strcmp (argv[0], "--mempolicy")) {
	if (!strncmp(argv[1], "default", 2))
	  membind_policy = HWLOC_MEMBIND_DEFAULT;
	else if (!strncmp(argv[1], "firsttouch", 2))
	  membind_policy = HWLOC_MEMBIND_FIRSTTOUCH;
	else if (!strncmp(argv[1], "bind", 2))
	  membind_policy = HWLOC_MEMBIND_BIND;
	else if (!strncmp(argv[1], "interleace", 2))
	  membind_policy = HWLOC_MEMBIND_INTERLEAVE;
	else if (!strncmp(argv[1], "replicate", 2))
	  membind_policy = HWLOC_MEMBIND_REPLICATE;
	else if (!strncmp(argv[1], "nexttouch", 2))
	  membind_policy = HWLOC_MEMBIND_NEXTTOUCH;
	else {
	  fprintf(stderr, "Unrecognized memory binding policy %s\n", argv[1]);
          usage (stderr);
          exit(EXIT_FAILURE);
	}
	opt = 1;
	goto next;
      }

      fprintf (stderr, "Unrecognized option: %s\n", argv[0]);
      usage(stderr);
      return EXIT_FAILURE;
    }

    ret = hwloc_mask_process_arg(topology, depth, argv[0], logical,
				 cpubind ? cpubind_set : membind_set,
				 taskset, verbose);
    if (ret < 0) {
      if (verbose)
	fprintf(stderr, "assuming the command starts at %s\n", argv[0]);
      break;
    }

  next:
    argc -= opt+1;
    argv += opt+1;
  }

  if (get_binding) {
    char *s, *policystr = NULL;
    int err;
    if (cpubind) {
      if (pid)
	err = hwloc_get_proc_cpubind(topology, pid, cpubind_set, 0);
      else
	err = hwloc_get_cpubind(topology, cpubind_set, 0);
      if (err) {
	const char *errmsg = strerror(errno);
	fprintf(stderr, "hwloc_get_cpubind failed (errno %d %s)\n", errno, errmsg);
	return EXIT_FAILURE;
      }
      if (taskset)
	hwloc_bitmap_taskset_asprintf(&s, cpubind_set);
      else
	hwloc_bitmap_asprintf(&s, cpubind_set);
    } else {
      hwloc_membind_policy_t policy;
      if (pid)
	err = hwloc_get_proc_membind(topology, pid, membind_set, &policy, 0);
      else
	err = hwloc_get_membind(topology, membind_set, &policy, 0);
      if (err) {
	const char *errmsg = strerror(errno);
	fprintf(stderr, "hwloc_get_membind failed (errno %d %s)\n", errno, errmsg);
	return EXIT_FAILURE;
      }
      if (taskset)
	hwloc_bitmap_taskset_asprintf(&s, membind_set);
      else
	hwloc_bitmap_asprintf(&s, membind_set);
      switch (policy) {
      case HWLOC_MEMBIND_DEFAULT: policystr = "default"; break;
      case HWLOC_MEMBIND_FIRSTTOUCH: policystr = "firsttouch"; break;
      case HWLOC_MEMBIND_BIND: policystr = "bind"; break;
      case HWLOC_MEMBIND_INTERLEAVE: policystr = "interleave"; break;
      case HWLOC_MEMBIND_REPLICATE: policystr = "replicate"; break;
      case HWLOC_MEMBIND_NEXTTOUCH: policystr = "nexttouch"; break;
      default: fprintf(stderr, "unknown memory policy %d\n", policy); assert(0); break;
      }
    }
    if (policystr)
      printf("%s (%s)\n", s, policystr);
    else
      printf("%s\n", s);
    free(s);
    return EXIT_SUCCESS;
  }

  if (!hwloc_bitmap_iszero(cpubind_set)) {
    if (verbose) {
      char *s;
      hwloc_bitmap_asprintf(&s, cpubind_set);
      fprintf(stderr, "binding on cpu set %s\n", s);
      free(s);
    }
    if (single)
      hwloc_bitmap_singlify(cpubind_set);
    if (pid)
      ret = hwloc_set_proc_cpubind(topology, pid, cpubind_set, cpubind_flags);
    else
      ret = hwloc_set_cpubind(topology, cpubind_set, cpubind_flags);
    if (ret) {
      int bind_errno = errno;
      const char *errmsg = strerror(bind_errno);
      char *s;
      hwloc_bitmap_asprintf(&s, cpubind_set);
      fprintf(stderr, "hwloc_set_cpubind %s failed (errno %d %s)\n", s, bind_errno, errmsg);
      free(s);
    }
  }

  if (!hwloc_bitmap_iszero(membind_set)) {
    if (verbose) {
      char *s;
      hwloc_bitmap_asprintf(&s, membind_set);
      fprintf(stderr, "binding on memory set %s\n", s);
      free(s);
    }
    if (single)
      hwloc_bitmap_singlify(membind_set);
    if (pid)
      ret = hwloc_set_proc_membind(topology, pid, membind_set, membind_policy, membind_flags);
    else
      ret = hwloc_set_membind(topology, membind_set, membind_policy, membind_flags);
    if (ret) {
      int bind_errno = errno;
      const char *errmsg = strerror(bind_errno);
      char *s;
      hwloc_bitmap_asprintf(&s, membind_set);
      fprintf(stderr, "hwloc_set_membind %s failed (errno %d %s)\n", s, bind_errno, errmsg);
      free(s);
    }
  }

  hwloc_bitmap_free(cpubind_set);
  hwloc_bitmap_free(membind_set);

  hwloc_topology_destroy(topology);

  if (pid)
    return EXIT_SUCCESS;

  if (0 == argc) {
    fprintf(stderr, "%s: nothing to do!\n", orig_argv[0]);
    return EXIT_FAILURE;
  }

  ret = execvp(argv[0], argv);
  if (ret) {
      fprintf(stderr, "%s: Failed to launch executable \"%s\"\n", 
              orig_argv[0], argv[0]);
      perror("execvp");
  }
  return EXIT_FAILURE;
}

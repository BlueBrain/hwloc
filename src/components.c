/*
 * Copyright © 2009-2012 Inria.  All rights reserved.
 * Copyright © 2012 Université Bordeau 1
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>
#include <hwloc.h>
#include <private/private.h>
#include <private/xml.h>

/* list of all registered core components, sorted by priority, higher priority first.
 * noos is last because its priority is 0.
 * others' priority is 10.
 */
static struct hwloc_core_component * hwloc_core_components = NULL;

static unsigned hwloc_components_users = 0; /* first one initializes, last ones destroys */

static int hwloc_components_verbose = 0;
#ifdef HWLOC_HAVE_PLUGINS
static int hwloc_plugins_verbose = 0;
#endif

#ifdef HWLOC_WIN_SYS
/* Basic mutex on top of InterlockedCompareExchange() on windows,
 * Far from perfect, but easy to maintain, and way enough given that this code will never be needed for real. */
#include <windows.h>
static LONG hwloc_components_mutex = 0;
#define HWLOC_COMPONENTS_LOCK() do {						\
  while (InterlockedCompareExchange(&hwloc_components_mutex, 1, 0) != 0)	\
    SwitchToThread();								\
} while (0)
#define HWLOC_COMPONENTS_UNLOCK() do {						\
  assert(hwloc_components_mutex == 1);						\
  hwloc_components_mutex = 0;							\
} while (0)

#elif defined HWLOC_HAVE_PTHREAD_MUTEX
/* pthread mutex if available (except on windows) */
#include <pthread.h>
static pthread_mutex_t hwloc_components_mutex = PTHREAD_MUTEX_INITIALIZER;
#define HWLOC_COMPONENTS_LOCK() pthread_mutex_lock(&hwloc_components_mutex)
#define HWLOC_COMPONENTS_UNLOCK() pthread_mutex_unlock(&hwloc_components_mutex)

#else /* HWLOC_WIN_SYS || HWLOC_HAVE_PTHREAD_MUTEX */
#error No mutex implementation available
#endif


#ifdef HWLOC_HAVE_PLUGINS

#include <ltdl.h>

/* array of pointers to dynamically loaded plugins */
static struct hwloc__plugin_desc {
  char *name;
  struct hwloc_component *component;
  lt_dlhandle handle;
  struct hwloc__plugin_desc *next;
} *hwloc_plugins = NULL;

static int
hwloc__dlforeach_cb(const char *filename, void *_data __hwloc_attribute_unused)
{
  const char *basename;
  lt_dlhandle handle;
  char *componentsymbolname = NULL;
  struct hwloc_component *component;
  struct hwloc__plugin_desc *desc;

  if (hwloc_plugins_verbose)
    fprintf(stderr, "Plugin dlforeach found `%s'\n", filename);

  basename = strrchr(filename, '/');
  if (!basename)
    basename = filename;
  else
    basename++;

  /* dlopen and get the component structure */
  handle = lt_dlopenext(filename);
  if (!handle) {
    if (hwloc_plugins_verbose)
      fprintf(stderr, "Failed to load plugin: %s\n", lt_dlerror());
    goto out;
  }
  componentsymbolname = malloc(6+strlen(basename)+10+1);
  sprintf(componentsymbolname, "%s_component", basename);
  component = lt_dlsym(handle, componentsymbolname);
  if (!component) {
    if (hwloc_plugins_verbose)
      fprintf(stderr, "Failed to find component symbol `%s'\n",
	      componentsymbolname);
    goto out_with_handle;
  }
  if (component->abi != HWLOC_COMPONENT_ABI) {
    if (hwloc_plugins_verbose)
      fprintf(stderr, "Plugin symbol ABI %u instead of %u\n",
	      component->abi, HWLOC_COMPONENT_ABI);
    goto out_with_handle;
  }
  if (hwloc_plugins_verbose)
    fprintf(stderr, "Plugin contains expected symbol `%s'\n",
	    componentsymbolname);
  free(componentsymbolname);
  componentsymbolname = NULL;

  if (HWLOC_COMPONENT_TYPE_CORE == component->type) {
    if (strncmp(basename, "hwloc_", 6)) {
      if (hwloc_plugins_verbose)
	fprintf(stderr, "Plugin name `%s' doesn't match its type CORE\n", basename);
      goto out_with_handle;
    }
  } else if (HWLOC_COMPONENT_TYPE_XML == component->type) {
    if (strncmp(basename, "hwloc_xml_", 10)) {
      if (hwloc_plugins_verbose)
	fprintf(stderr, "Plugin name `%s' doesn't match its type XML\n", basename);
      goto out_with_handle;
    }
  } else {
    if (hwloc_plugins_verbose)
      fprintf(stderr, "Plugin name `%s' has invalid type %u\n",
	      basename, (unsigned) component->type);
    goto out_with_handle;
  }

  /* allocate a plugin_desc and queue it */
  desc = malloc(sizeof(*desc));
  if (!desc)
    goto out_with_handle;
  desc->name = strdup(basename);
  desc->component = component;
  desc->handle = handle;
  if (hwloc_plugins_verbose)
    fprintf(stderr, "Plugin descriptor `%s' ready\n", basename);

  desc->next = hwloc_plugins;
  hwloc_plugins = desc;
  if (hwloc_plugins_verbose)
    fprintf(stderr, "Plugin descriptor `%s' queued\n", basename);
  return 0;

 out_with_handle:
  lt_dlclose(handle);
  free(componentsymbolname); /* NULL if already freed */
 out:
  return 0;
}

static void
hwloc_plugins_exit(void)
{
  struct hwloc__plugin_desc *desc, *next;

  if (hwloc_plugins_verbose)
    fprintf(stderr, "Closing all plugins\n");

  desc = hwloc_plugins;
  while (desc) {
    next = desc->next;
    lt_dlclose(desc->handle);
    free(desc->name);
    free(desc);
    desc = next;
  }
  hwloc_plugins = NULL;

  lt_dlexit();
}

static int
hwloc_plugins_init(void)
{
  char *verboseenv;
  char *path = HWLOC_PLUGINS_DIR;
  char *env;
  int err;

  verboseenv = getenv("HWLOC_PLUGINS_VERBOSE");
  hwloc_plugins_verbose = verboseenv ? atoi(verboseenv) : 0;

  err = lt_dlinit();
  if (err)
    goto out;

  env = getenv("HWLOC_PLUGINS_PATH");
  if (env)
    path = env;

  hwloc_plugins = NULL;

  if (hwloc_plugins_verbose)
    fprintf(stderr, "Starting plugin dlforeach in %s\n", path);
  err = lt_dlforeachfile(path, hwloc__dlforeach_cb, NULL);
  if (err)
    goto out_with_init;

  return 0;

 out_with_init:
  hwloc_plugins_exit();
 out:
  return -1;
}

#endif /* HWLOC_HAVE_PLUGINS */

static const char *
hwloc_core_component_type_string(hwloc_core_component_type_t type)
{
  switch (type) {
  case HWLOC_CORE_COMPONENT_TYPE_CPU: return "cpu";
  case HWLOC_CORE_COMPONENT_TYPE_GLOBAL: return "global";
  case HWLOC_CORE_COMPONENT_TYPE_ADDITIONAL: return "additional";
  default: return "Unknown";
  }
}

static int
hwloc_core_component_register(struct hwloc_core_component *component)
{
  struct hwloc_core_component **prev;

  prev = &hwloc_core_components;
  while (NULL != *prev) {
    if (!strcmp((*prev)->name, component->name)) {
      if (hwloc_components_verbose)
	fprintf(stderr, "Multiple `%s' components, only registering the first one\n",
		component->name);
      return -1;
    }
    prev = &((*prev)->next);
  }
  if (hwloc_components_verbose)
    fprintf(stderr, "Registered %s component `%s' with priority %u\n",
	    hwloc_core_component_type_string(component->type), component->name, component->priority);

  prev = &hwloc_core_components;
  while (NULL != *prev) {
    if ((*prev)->priority < component->priority)
      break;
    prev = &((*prev)->next);
  }
  component->next = *prev;
  *prev = component;
  return 0;
}

#include <static-components.h>

void
hwloc_components_init(struct hwloc_topology *topology __hwloc_attribute_unused)
{
#ifdef HWLOC_HAVE_PLUGINS
  struct hwloc__plugin_desc *desc;
#endif
  char *verboseenv;
  unsigned i;

  HWLOC_COMPONENTS_LOCK();
  assert((unsigned) -1 != hwloc_components_users);
  if (0 != hwloc_components_users++) {
    HWLOC_COMPONENTS_UNLOCK();
    goto ok;
  }

  verboseenv = getenv("HWLOC_COMPONENTS_VERBOSE");
  hwloc_components_verbose = verboseenv ? atoi(verboseenv) : 0;

#ifdef HWLOC_HAVE_PLUGINS
  hwloc_plugins_init();
#endif

  /* hwloc_static_components is created by configure in static-components.h */
  for(i=0; NULL != hwloc_static_components[i]; i++)
    if (HWLOC_COMPONENT_TYPE_CORE == hwloc_static_components[i]->type)
      hwloc_core_component_register(hwloc_static_components[i]->data);
    else if (HWLOC_COMPONENT_TYPE_XML == hwloc_static_components[i]->type)
      hwloc_xml_callbacks_register(hwloc_static_components[i]->data);
    else
      assert(0);

  /* dynamic plugins */
#ifdef HWLOC_HAVE_PLUGINS
  for(desc = hwloc_plugins; NULL != desc; desc = desc->next)
    if (HWLOC_COMPONENT_TYPE_CORE == desc->component->type)
      hwloc_core_component_register(desc->component->data);
    else if (HWLOC_COMPONENT_TYPE_XML == desc->component->type)
      hwloc_xml_callbacks_register(desc->component->data);
    else
      assert(0);
#endif

  HWLOC_COMPONENTS_UNLOCK();

 ok:
  topology->backends = NULL;
}

static struct hwloc_core_component *
hwloc_core_component_find(int type /* hwloc_component_type_t or -1 if any */,
			  const char *name /* name of NULL if any */)
{
  struct hwloc_core_component *comp = hwloc_core_components;
  while (NULL != comp) {
    if ((-1 == type || type == (int) comp->type)
       && (NULL == name || !strcmp(name, comp->name)))
      return comp;
    comp = comp->next;
  }
  return NULL;
}

/* used by set_xml(), set_synthetic(), ... environment variables, ... to force the first backend */
int
hwloc_core_component_force_enable(struct hwloc_topology *topology,
				  int envvar_forced,
				  int type, const char *name,
				  const void *data1, const void *data2, const void *data3)
{
  struct hwloc_core_component *comp;
  struct hwloc_backend *backend;

  comp = hwloc_core_component_find(type, name);
  if (!comp) {
    errno = ENOSYS;
    return -1;
  }

  backend = comp->instantiate(topology, comp, data1, data2, data3);
  if (backend) {
    backend->envvar_forced = envvar_forced;
    if (topology->backends)
      hwloc_backends_reset(topology);
    return hwloc_backend_enable(topology, backend);
  } else
    return -1;
}

static int
hwloc_core_component_try_enable(struct hwloc_topology *topology,
				struct hwloc_core_component *comp,
				const char *comparg,
				unsigned *excludes,
				int envvar_forced,
				int verbose_errors)
{
  struct hwloc_backend *backend;
  int err;

  if ((*excludes) & comp->type) {
    if (hwloc_components_verbose)
      fprintf(stderr, "Excluding %s component `%s', conflicts with excludes 0x%x\n",
	      hwloc_core_component_type_string(comp->type), comp->name, *excludes);
    return -1;
  }

  backend = comp->instantiate(topology, comp, comparg, NULL, NULL);
  if (!backend) {
    if (verbose_errors)
      fprintf(stderr, "Failed to instantiate component `%s'\n", comp->name);
    return -1;
  }

  backend->envvar_forced = envvar_forced;
  err = hwloc_backend_enable(topology, backend);
  if (err < 0)
    return -1;

  *excludes |= comp->excludes;

  return 0;
}

void
hwloc_core_components_enable_others(struct hwloc_topology *topology)
{
  struct hwloc_core_component *comp;
  unsigned excludes = 0;
  int tryall = 1;
  char *env;

  /* we have either no backends, or a single one, use its exclude */
  if (topology->backends) {
    excludes = topology->backends->component->excludes;
    assert(!topology->backends->next);
  }

  env = getenv("HWLOC_COMPONENTS");
  if (env) {
    size_t s;

    while (*env) {
      s = strcspn(env, ",");
      if (s) {
	char *arg;
	char c;
	/* save the last char and replace with \0 */
	c = env[s];
	env[s] = '\0';

	if (!strcmp(env, "stop")) {
	  tryall = 0;
	  break;
	}

	arg = strchr(env, '=');
	if (arg) {
	  *arg = '\0';
	  arg++;
	}

	comp = hwloc_core_component_find(-1, env);
	if (comp) {
	  hwloc_core_component_try_enable(topology, comp, arg, &excludes, 1 /* envvar forced */, 1 /* envvar forced need warnings on conflicts */);
	} else {
	  fprintf(stderr, "Cannot find component `%s'\n", env);
	}

	/* restore last char */
	env[s] = c;
      }

      env += s;
      if (*env)
	/* Skip comma */
	env++;
    }
  }

  if (tryall) {
    comp = hwloc_core_components;
    while (NULL != comp) {
      hwloc_core_component_try_enable(topology, comp, NULL, &excludes, 0 /* defaults, not envvar forced */, 0 /* defaults don't need warnings on conflicts */);
      comp = comp->next;
    }
  }
}

void
hwloc_components_destroy_all(struct hwloc_topology *topology __hwloc_attribute_unused)
{
  HWLOC_COMPONENTS_LOCK();
  assert(0 != hwloc_components_users);
  if (0 != --hwloc_components_users) {
    HWLOC_COMPONENTS_UNLOCK();
    return;
  }

  /* no need to unlink/free the list of components, they'll be unloaded below */

  hwloc_core_components = NULL;
  hwloc_xml_callbacks_reset();

#ifdef HWLOC_HAVE_PLUGINS
  hwloc_plugins_exit();
#endif

  HWLOC_COMPONENTS_UNLOCK();
}

struct hwloc_backend *
hwloc_backend_alloc(struct hwloc_topology *topology,
		    struct hwloc_core_component *component)
{
  struct hwloc_backend * backend = malloc(sizeof(*backend));
  if (!backend) {
    errno = ENOMEM;
    return NULL;
  }
  backend->component = component;
  backend->topology = topology;
  backend->flags = 0;
  backend->discover = NULL;
  backend->get_obj_cpuset = NULL;
  backend->notify_new_object = NULL;
  backend->disable = NULL;
  backend->is_custom = 0;
  backend->is_thissystem = -1;
  backend->next = NULL;
  backend->envvar_forced = 0;
  return backend;
}

static void
hwloc_backend_disable(struct hwloc_backend *backend)
{
  if (backend->disable)
    backend->disable(backend);
  free(backend);
}

int
hwloc_backend_enable(struct hwloc_topology *topology, struct hwloc_backend *backend)
{
  struct hwloc_backend **pprev;

  /* make sure we didn't already enable this backend, we don't want duplicates */
  pprev = &topology->backends;
  while (NULL != *pprev) {
    if ((*pprev)->component == backend->component) {
      if (hwloc_components_verbose)
	fprintf(stderr, "Cannot enable %s component `%s' twice\n",
		hwloc_core_component_type_string(backend->component->type), backend->component->name);
      hwloc_backend_disable(backend);
      errno = EBUSY;
      return -1;
    }
    pprev = &((*pprev)->next);
  }

  if (hwloc_components_verbose)
    fprintf(stderr, "Enabling %s component `%s'\n",
	    hwloc_core_component_type_string(backend->component->type), backend->component->name);

  /* enqueue at the end */
  pprev = &topology->backends;
  while (NULL != *pprev)
    pprev = &((*pprev)->next);
  backend->next = *pprev;
  *pprev = backend;

  return 0;
}

void
hwloc_backends_is_thissystem(struct hwloc_topology *topology)
{
  struct hwloc_backend *backend;
  char *local_env;

  /* Apply is_thissystem topology flag before we enforce envvar backends.
   * If the application changed the backend with set_foo(),
   * it may use set_flags() update the is_thissystem flag here.
   * If it changes the backend with environment variables below,
   * it may use HWLOC_THISSYSTEM envvar below as well.
   */

  topology->is_thissystem = 1;

  /* apply thissystem from normally-given backends (envvar_forced=0, either set_foo() or defaults) */
  backend = topology->backends;
  while (backend != NULL) {
    if (backend->envvar_forced == 0 && backend->is_thissystem != -1) {
      assert(backend->is_thissystem == 0);
      topology->is_thissystem = 0;
    }
    backend = backend->next;
  }

  /* override set_foo() with flags */
  if (topology->flags & HWLOC_TOPOLOGY_FLAG_IS_THISSYSTEM)
    topology->is_thissystem = 1;

  /* now apply envvar-forced backend (envvar_forced=1) */
  backend = topology->backends;
  while (backend != NULL) {
    if (backend->envvar_forced == 1 && backend->is_thissystem != -1) {
      assert(backend->is_thissystem == 0);
      topology->is_thissystem = 0;
    }
    backend = backend->next;
  }

  /* override with envvar-given flag */
  local_env = getenv("HWLOC_THISSYSTEM");
  if (local_env)
    topology->is_thissystem = atoi(local_env);
}

int
hwloc_backends_get_obj_cpuset(struct hwloc_backend *caller, struct hwloc_obj *obj, hwloc_bitmap_t cpuset)
{
  struct hwloc_topology *topology = caller->topology;
  struct hwloc_backend *backend = topology->backends;
  /* use the first backend's get_obj_cpuset callback */
  while (backend != NULL) {
    if (backend->get_obj_cpuset)
      return backend->get_obj_cpuset(backend, caller, obj, cpuset);
    backend = backend->next;
  }
  return -1;
}

int
hwloc_backends_notify_new_object(struct hwloc_backend *caller, struct hwloc_obj *obj)
{
  struct hwloc_backend *backend;
  int res = 0;

  backend = caller->topology->backends;
  while (NULL != backend) {
    if (backend != caller && backend->notify_new_object)
      res += backend->notify_new_object(backend, caller, obj);
    backend = backend->next;
  }

  return res;
}

void
hwloc_backends_disable_all(struct hwloc_topology *topology)
{
  struct hwloc_backend *backend;

  while (NULL != (backend = topology->backends)) {
    struct hwloc_backend *next = backend->next;
    if (hwloc_components_verbose)
      fprintf(stderr, "Disabling %s component `%s'\n",
	      hwloc_core_component_type_string(backend->component->type), backend->component->name);
    hwloc_backend_disable(backend);
    topology->backends = next;
  }
  topology->backends = NULL;
}

void
hwloc_backends_reset(struct hwloc_topology *topology)
{
  hwloc_backends_disable_all(topology);
  if (topology->is_loaded) {
    hwloc_topology_clear(topology);
    hwloc_distances_destroy(topology);
    hwloc_topology_setup_defaults(topology);
    topology->is_loaded = 0;
  }
}

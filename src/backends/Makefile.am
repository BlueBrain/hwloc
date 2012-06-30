lib_LTLIBRARIES = libhwlocbackendsglobaltopologysynthetic.la

###################
# Always compiled #
###################

libhwlocbackendsglobaltopologysynthetic_la_SOURCES = hwloc_backends_global_topology-synthetic.c
libhwlocbackendsglobaltopologysynthetic_la_CPPFLAGS = $(HWLOC_CPPFLAGS) -fPIC
libhwlocbackendsglobaltopologysynthetic_la_CFLAGS = $(HWLOC_CFLAGS) -fPIC
libhwlocbackendsglobaltopologysynthetic_la_LDFLAGS = $(HWLOC_LDLAGS) -shared -fPIC -module -avoid-version

#################################
# Only compiled if HWLOC_HAVE_* #
#################################

if HWLOC_HAVE_LINUX
lib_LTLIBRARIES += libhwlocbackendsbasetopologylinux.la
libhwlocbackendsbasetopologylinux_la_SOURCES = hwloc_backends_base_topology-linux.c
libhwlocbackendsbasetopologylinux_la_CPPFLAGS = $(HWLOC_CPPFLAGS) -fPIC
libhwlocbackendsbasetopologylinux_la_CFLAGS = $(HWLOC_CFLAGS) -fPIC
libhwlocbackendsbasetopologylinux_la_LDFLAGS = $(HWLOC_LDLAGS) -shared -fPIC -module -avoid-version
endif HWLOC_HAVE_LINUX

if HWLOC_HAVE_LIBPCI
lib_LTLIBRARIES += libhwlocbackendsiotopologylibpci.la
libhwlocbackendsiotopologylibpci_la_SOURCES = hwloc_backends_io_topology-libpci.c
libhwlocbackendsiotopologylibpci_la_CPPFLAGS = $(HWLOC_CPPFLAGS) -fPIC
libhwlocbackendsiotopologylibpci_la_CFLAGS = $(HWLOC_CFLAGS) -fPIC
libhwlocbackendsiotopologylibpci_la_LDFLAGS = $(HWLOC_LDLAGS) -shared -fPIC -module -avoid-version
endif HWLOC_HAVE_LIBPCI

hwlocdir = $(libdir)/hwloc

hwloc_LTLIBRARIES = libhwlocbackendsglobaltopologysynthetic.la

###################
# Always compiled #
#                 #
# Global backends #
###################

# Synthetic backend #

libhwlocbackendsglobaltopologysynthetic_la_SOURCES = hwloc_backends_global_topology-synthetic.c
libhwlocbackendsglobaltopologysynthetic_la_CPPFLAGS = $(HWLOC_CPPFLAGS) -fPIC
libhwlocbackendsglobaltopologysynthetic_la_CFLAGS = $(HWLOC_CFLAGS) -fPIC
libhwlocbackendsglobaltopologysynthetic_la_LDFLAGS = $(HWLOC_LDLAGS) -shared -fPIC -module -avoid-version

#################################
# Only compiled if HWLOC_HAVE_* #
#################################

# Base backends #

if HWLOC_HAVE_LINUX
hwloc_LTLIBRARIES += libhwlocbackendsbasetopologylinux.la
libhwlocbackendsbasetopologylinux_la_SOURCES = hwloc_backends_base_topology-linux.c
libhwlocbackendsbasetopologylinux_la_CPPFLAGS = $(HWLOC_CPPFLAGS) -fPIC
libhwlocbackendsbasetopologylinux_la_CFLAGS = $(HWLOC_CFLAGS) -fPIC
libhwlocbackendsbasetopologylinux_la_LDFLAGS = $(HWLOC_LDLAGS) -shared -fPIC -module -avoid-version
endif HWLOC_HAVE_LINUX

if HWLOC_HAVE_FREEBSD
hwloc_LTLIBRARIES += libhwlocbackendsbasetopologyfreebsd.la
libhwlocbackendsbasetopologyfreebsd_la_SOURCES = hwloc_backends_base_topology-freebsd.c
libhwlocbackendsbasetopologyfreebsd_la_CPPFLAGS = $(HWLOC_CPPFLAGS) -fPIC
libhwlocbackendsbasetopologyfreebsd_la_CFLAGS = $(HWLOC_CFLAGS) -fPIC
libhwlocbackendsbasetopologyfreebsd_la_LDFLAGS = $(HWLOC_LDLAGS) -shared -fPIC -module -avoid-version
endif HWLOC_HAVE_FREEBSD

if HWLOC_HAVE_SOLARIS
hwloc_LTLIBRARIES += libhwlocbackendsbasetopologysolaris.la
libhwlocbackendsbasetopologysolaris_la_SOURCES = hwloc_backends_base_topology-solaris.c hwloc_backends_base_topology-solaris-chiptype.c
libhwlocbackendsbasetopologysolaris_la_CPPFLAGS = $(HWLOC_CPPFLAGS) -fPIC
libhwlocbackendsbasetopologysolaris_la_CFLAGS = $(HWLOC_CFLAGS) -fPIC
libhwlocbackendsbasetopologysolaris_la_LDFLAGS = $(HWLOC_LDLAGS) -shared -fPIC -module -avoid-version
endif HWLOC_HAVE_SOLARIS

# IO backends #

if HWLOC_HAVE_LIBPCI
hwloc_LTLIBRARIES += libhwlocbackendsiotopologylibpci.la
libhwlocbackendsiotopologylibpci_la_SOURCES = hwloc_backends_io_topology-libpci.c
libhwlocbackendsiotopologylibpci_la_CPPFLAGS = $(HWLOC_CPPFLAGS) -fPIC
libhwlocbackendsiotopologylibpci_la_CFLAGS = $(HWLOC_CFLAGS) -fPIC
libhwlocbackendsiotopologylibpci_la_LDFLAGS = $(HWLOC_LDLAGS) -shared -fPIC -module -avoid-version
endif HWLOC_HAVE_LIBPCI

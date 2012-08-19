# Copyright © 2012 Inria.  All rights reserved.
# See COPYING in top-level directory.


# HWLOC_PREPARE_FILTER_COMPONENTS
#
# Given a list of <class>-<name>, define hwloc_<class>_<name>_component_maybeplugin=1.
#
# $1 = command-line given list of components to build as plugins
#
AC_DEFUN([HWLOC_PREPARE_FILTER_COMPONENTS], [
  for classname in `echo [$1] | sed -e 's/,/ /g'` ; do
    class=`echo $classname | cut -d- -f1`
    name=`echo $classname | cut -d- -f2`
    str="hwloc_${class}_${name}_component_wantplugin=1"
    eval $str
  done
])


# HWLOC_FILTER_COMPONENTS
#
# Given class $1, for each component in hwloc_<class>_components,
# check if hwloc_<class>_<name>_component_wantplugin=1 or enable_plugin=yes,
# and check if hwloc_<class>_<name>_component_maybeplugin=1.
# Add <name> to hwloc_[static|plugin]_<class>_components accordingly.
# And set hwloc_<class>_<name>_component=[static|plugin] accordingly.
#
# $1 = class name
#
AC_DEFUN([HWLOC_FILTER_COMPONENTS], [
for name in $hwloc_[$1]_components ; do
  str="maybeplugin=\$hwloc_[$1]_${name}_component_maybeplugin"
  eval $str
  str="wantplugin=\$hwloc_[$1]_${name}_component_wantplugin"
  eval $str
  if test x$hwloc_have_plugins = xyes && test x$maybeplugin = x1 && test x$wantplugin = x1 -o x$enable_plugins = xyes; then
    hwloc_plugin_[$1]_components="$hwloc_plugin_[$1]_components $name"
    str="hwloc_[$1]_${name}_component=plugin"
  else
    hwloc_static_[$1]_components="$hwloc_static_[$1]_components $name"
    str="hwloc_[$1]_${name}_component=static"
  fi
  eval $str
done
])


# HWLOC_LIST_STATIC_COMPONENTS
#
# Append to file $1 an array of component_register entries
# by listing components of class $2 given in $3.
#
# $1 = filename
# $2 = name of the component class
# $3 = list of component names in this class
#
AC_DEFUN([HWLOC_LIST_STATIC_COMPONENTS], [
cat <<EOF >>[$1]
static hwloc_component_init_fn_t hwloc_static_[$2]_components[[]] = {
EOF
for comp in [$3]; do
  echo "  hwloc_[$2]_${comp}_component_register," >>[$1]
done
cat <<EOF >>[$1]
  NULL
};
EOF
])

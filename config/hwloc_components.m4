# Copyright © 2012 Inria.  All rights reserved.
# See COPYING in top-level directory.


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

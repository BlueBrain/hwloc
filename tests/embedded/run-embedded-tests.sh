#!/bin/sh
#
# Copyright © 2010 Cisco Systems, Inc.  All rights reserved.
#
# Simple script to help test embedding:
#
#     ./run-embedded-tests.sh <tarball_name>
#

tarball=$1
if test "$tarball" = ""; then
    echo "Usage: $0 <tarball_name>"
    exit 1
elif test ! -r $tarball; then
    echo cannot read tarball: $tarball
    exit 1
fi

#---------------------------------------------------------------------

i=1
print() {
    echo === $i: $*
    i=`expr $i + 1`
}

#---------------------------------------------------------------------

try() {
    cmd=$*
    eval $cmd
    status=$?
    if test "$status" != "0"; then
        echo "Command failed (status $status): $cmd"
        exit 1
    fi
}

#---------------------------------------------------------------------

do_build() {
    print Running $1 configure...
    try $2/configure

    print Running make
    try make

    print Running make check
    try make check

    print Running make clean
    try make clean

    print Running make distclean
    try make distclean
}

#---------------------------------------------------------------------

# Get tarball name
print Got tarball: $tarball

# Get the version
ver=`echo $tarball | sed -e 's/^.*hwloc-//' -e 's/.tar.*$//'`
print Got version: $ver

# Extract
print Extracting tarball...
rm -rf hwloc-$ver
if test "`echo $tarball | grep .tar.bz2`" != ""; then
    try tar jxf $tarball
else
    try tar zxf $tarball
fi

print Removing old tree...
rm -rf hwloc-tree
mv hwloc-$ver hwloc-tree

# Autogen
print Running autogen...
try ./autogen.sh

# Do it normally (non-VPATH)
do_build non-VPATH .

# Do a VPATH in the same tree that we just setup
mkdir build
cd build
do_build VPATH ..

cd ..
rm -rf build

# Now whack the tree and do a clean VPATH
print Re-extracting tarball...
rm -rf hwloc-$ver
if test "`echo $tarball | grep .tar.bz2`" != ""; then
    try tar jxf $tarball
else
    try tar zxf $tarball
fi

print Removing old tree...
rm -rf hwloc-tree
mv hwloc-$ver hwloc-tree

# Autogen
print Running autogen...
try ./autogen.sh

# Run it again on a clean VPATH
mkdir build
cd build
do_build VPATH-clean ..

cd ..
rm -rf build

print All tests passed!

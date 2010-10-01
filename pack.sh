#!/bin/sh
#
# Usage: $(basename $0)
#
# Determine a revision or version string to use for the current source
# tree.

# First argument, if given, is the location of the project root.  Set
# here any environment variables for VCs that care about the project
# root.
export GIT_DIR="$(dirname $0)/.git"

# It's actually this simple.  We might want to keep the script around in
# case we want to re-introduce subversion compatibility.
#i needed to get rid of extra newline
VERSION=`git describe --tags` || exit 1
FILE="googleai-v${VERSION}.zip"

make clean
make -j 5 && zip "${FILE}" *.cc *.h || exit 1
make clean
mv "${FILE}" submits/

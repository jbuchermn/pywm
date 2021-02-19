#!/bin/sh
SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")
cd $SCRIPTPATH

meson build -Dwlroots:static=true || exit 1;
ninja -C build || exit 1;
python setup.py install --user || exit 1;

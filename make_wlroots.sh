#!/bin/sh
cp subprojects/wlroots.syms subprojects/wlroots/
cd subprojects/wlroots
meson build && sudo ninja -C build install
git status
git checkout -- .


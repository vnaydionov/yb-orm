#!/bin/sh

libtoolize --automake --copy --force &&
autoheader &&
rm -rf aclocal.m4 &&
aclocal &&
autoconf &&
automake --foreign --add-missing --copy


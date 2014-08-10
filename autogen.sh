#!/bin/sh

if hash libtoolize 2>&-
then
    libtoolize --automake --force
else
    glibtoolize --automake --force
fi
autoheader &&
rm -rf aclocal.m4 &&
aclocal -I . &&
autoconf &&
automake --foreign --add-missing --copy


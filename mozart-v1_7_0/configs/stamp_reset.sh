#!/bin/bash

echo Resetting date-stamps on configuration files ...

echo Step 1 of 5: configure.ac and configure.in
find . \( -name configure.ac -o -name configure.in \
          -o -name Makefile.am \) -print | xargs touch
sleep 1

echo Step 2 of 5: aclocal.m4
find . -name aclocal.m4 -print | xargs touch
sleep 1

echo Step 3 of 5: configure
find . -name configure -print | xargs touch
find . -name configure -print | xargs chmod +x
sleep 1

echo Step 4 of 5: config.h.in
find . -name config.h.in -print | xargs touch
sleep 1

echo Step 5 of 5: Makefile.in
find . -name Makefile.in -print | xargs touch

echo Date-stamps should now be in the proper sequence.

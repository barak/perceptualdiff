#!/bin/bash -eux

# Build Coverity intermediate files.

# This assumes Coverity has already been configured via
# "cov-configure --compiler g++-mp-4.8".

rm -rf perceptualdiff.tgz cov-int
cov-build --dir cov-int g++-mp-4.8 -std=c++0x -o perceptualdiff ./*.cpp $(pkg-config --cflags --libs gdk-pixbuf-2.0)
tar czvf perceptualdiff.tgz cov-int

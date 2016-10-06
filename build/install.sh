#!/usr/bin/env bash
cd ../OpenBLAS
make -j8
cd ../mxnet/amalgamation
make -j8
cd ../../build
cmake .
make -j8

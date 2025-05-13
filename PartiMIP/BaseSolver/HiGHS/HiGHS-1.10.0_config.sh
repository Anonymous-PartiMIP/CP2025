#!/bin/bash

unzip HiGHS-1.10.0.zip
cd HiGHS-1.10.0
cmake -S. -B build -DCMAKE_INSTALL_PREFIX=./
cmake --build build --parallel
cd build
make install
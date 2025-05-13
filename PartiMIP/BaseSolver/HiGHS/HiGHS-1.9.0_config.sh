#!/bin/bash

unzip HiGHS-1.9.0.zip
cd HiGHS-1.9.0
cmake -S. -B build -DCMAKE_INSTALL_PREFIX=./ -DBUILD_SHARED_LIBS=OFF
cmake --build build --parallel
cd build
make install
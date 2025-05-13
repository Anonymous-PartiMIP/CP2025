#! /bin/bash

echo "Downloading MIPLIB 2017 benchmark..."
mkdir -p mps
cd mps
wget https://miplib.zib.de/downloads/benchmark.zip
unzip benchmark.zip
#!/bin/bash

bash PartiMIP-HiGHS-8.sh

echo "Waiting for 10 minutes..."
sleep 10m

bash PartiMIP-HiGHS-16.sh

echo "Waiting for 10 minutes..."
sleep 10m

bash PartiMIP-HiGHS-32.sh

echo "Waiting for 10 minutes..."
sleep 10m

bash PartiMIP-HiGHS-64.sh

echo "Waiting for 10 minutes..."
sleep 10m

bash PartiMIP-HiGHS-128.sh

echo "Finished!"

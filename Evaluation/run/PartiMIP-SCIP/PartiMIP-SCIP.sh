#!/bin/bash

bash PartiMIP-SCIP-8.sh

echo "Waiting for 10 minutes..."
sleep 10m

bash PartiMIP-SCIP-16.sh

echo "Waiting for 10 minutes..."
sleep 10m

bash PartiMIP-SCIP-32.sh

echo "Waiting for 10 minutes..."
sleep 10m

bash PartiMIP-SCIP-64.sh

echo "Waiting for 10 minutes..."
sleep 10m

bash PartiMIP-SCIP-128.sh

echo "Finished!"

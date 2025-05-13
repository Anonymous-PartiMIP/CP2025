#!/bin/bash

bash highs-1.sh

echo "Waiting for 10 minutes..."
sleep 10m

bash highs-8.sh

echo "Waiting for 10 minutes..."
sleep 10m

bash highs-16.sh

echo "Waiting for 10 minutes..."
sleep 10m

bash highs-32.sh

echo "Waiting for 10 minutes..."
sleep 10m

bash highs-64.sh

echo "Waiting for 10 minutes..."
sleep 10m

bash highs-128.sh

echo "Finished!"
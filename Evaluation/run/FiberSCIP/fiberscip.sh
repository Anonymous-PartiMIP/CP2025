#!/bin/bash

bash fiberscip-8.sh

echo "Waiting for 20 minutes..."
sleep 20m

bash fiberscip-16.sh

echo "Waiting for 20 minutes..."
sleep 20m

bash fiberscip-32.sh

echo "Waiting for 20 minutes..."
sleep 20m

bash fiberscip-64.sh

echo "Waiting for 20 minutes..."
sleep 20m

bash fiberscip-128.sh

echo "Finished!"

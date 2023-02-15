#!/bin/env bash
# Down the bridges
sudo ip link set br-left down
sudo ip link set br-right down

# Remove the taps from the bridges
sudo ip link set tap-left nomaster
sudo ip link set tap-right nomaster

# Delete the bridges
sudo ip link del br-left
sudo ip link del br-right

# Delete the taps
sudo ip link del tap-left
sudo ip link del tap-right

# Stop all running container
docker compose -f scenarios/tap-csma-scenario.yaml down

# Verify if still any container existed
docker container ls -a

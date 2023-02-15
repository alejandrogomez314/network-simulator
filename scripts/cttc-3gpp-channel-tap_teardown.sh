#!/usr/bin/env bash

# Down the bridges
sudo ip link set br-left down
sudo ip link set br-right down
sudo ip link set br-server down

# Remove the taps from the bridges
sudo ip link set tap-left nomaster
sudo ip link set tap-right nomaster
sudo ip link set tap-server nomaster

# Delete the bridges
sudo ip link del br-left
sudo ip link del br-right
sudo ip link del br-server

# Delete the taps
sudo ip link del tap-left
sudo ip link del tap-right
sudo ip link del tap-server

# Stop all running container
docker compose -f scenarios/cttc-3gpp-channel-tap.yaml down

ip link

# Verify if still any container existed
docker container ls -a

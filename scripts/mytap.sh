#!/bin/env bash

# Exit immediately if a commands exits with non-zero status
set -e

# Add bridges
echo "Add bridges..."
sudo ip link add name bridge type bridge
echo "Done."

# Add tap devices
echo "Add tap devices..."
sudo ip tuntap add mytap mode tap

sudo ifconfig mytap 0.0.0.0 promisc up
echo "Done."

# Attach tap devices to bridges and activate
echo "Attach taps to bridges..."
sudo ip link set mytap master bridge
sudo ip link set bridge up
echo "Done."

# disallow bridge traffic to go through ip tables chain
# See: https://unix.stackexchange.com/questions/499756/how-does-iptable-work-with-linux-bridge
# and: https://wiki.libvirt.org/page/Net.bridge.bridge-nf-call_and_sysctl.conf
pushd /proc/sys/net/bridge
for f in bridge-nf-*; do echo 0 > $f; done
popd

# Create the network namespace runtime folder if not exists
sudo mkdir -p /var/run/netns

export pid=0

docker compose -f scenarios/cttc-3gpp-channel-example.yaml up -d

# Run container orchestrator
pid=$(docker inspect --format '{{ .State.Pid }}' node)

# Soft-link the network namespace created by container into the linux namespace runtime
sudo ln -s /proc/$pid/ns/net /var/run/netns/$pid

# Create Veth pair to attach to bridge
echo "Create Veth pairs..."
sudo ip link add internal type veth peer name external
sudo ip link set internal master bridge
sudo ip link set internal up

# Configure the container-side pair with an interface and address
echo "Configure Veth pairs"
sudo ip link set external netns $pid
sudo ip netns exec $pid ip link set dev external name eth0
sudo ip netns exec $pid ip link set eth0 address 12:34:88:5D:61:BD
sudo ip netns exec $pid ip link set eth0 up
sudo ip netns exec $pid ip addr add 10.1.1.1 dev eth0
sudo ip netns exec $pid ip route add 7.0.0.1 dev eth0
sudo ip netns exec $pid ip route add 10.1.1.2 dev eth0 

echo "Done."
echo "### Setup complete. Ready to start simulation ###"

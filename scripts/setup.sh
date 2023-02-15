#!/bin/env bash

# Exit immediately if a commands exits with non-zero status
set -e

# Add bridges
echo "Add bridges..."
sudo ip link add name br-left type bridge
sudo ip link add name br-right type bridge
echo "Done."

# Add tap devices
echo "Add tap devices..."
sudo ip tuntap add tap-left mode tap
sudo ip tuntap add tap-right mode tap

sudo ifconfig tap-left 0.0.0.0 promisc up
sudo ifconfig tap-right 0.0.0.0 promisc up
echo "Done."

# Attach tap devices to bridges and activate
echo "Attach taps to bridges..."
sudo ip link set tap-left master br-left
sudo ip link set br-left up
sudo ip link set tap-right master br-right
sudo ip link set br-right up
echo "Done."

# disallow bridge traffic to go through ip tables chain
# See: https://unix.stackexchange.com/questions/499756/how-does-iptable-work-with-linux-bridge
# and: https://wiki.libvirt.org/page/Net.bridge.bridge-nf-call_and_sysctl.conf
pushd /proc/sys/net/bridge
for f in bridge-nf-*; do echo 0 > $f; done
popd

# Create the network namespace runtime folder if not exists
sudo mkdir -p /var/run/netns

export pid_left=0
export pid_right=0

# Run container orchestrator
echo "Start containers..."
docker compose -f scenarios/tap-csma-scenario.yaml up -d
echo "Containers started."
pid_left=$(docker inspect --format '{{ .State.Pid }}' left)
pid_right=$(docker inspect --format '{{ .State.Pid }}' right)

# Soft-link the network namespace created by container into the linux namespace runtime
sudo ln -s /proc/$pid_left/ns/net /var/run/netns/$pid_left
sudo ln -s /proc/$pid_right/ns/net /var/run/netns/$pid_right

# Create Veth pair to attach to bridge
echo "Create Veth pairs..."
sudo ip link add internal-left type veth peer name external-left
sudo ip link set internal-left master br-left
sudo ip link set internal-left up

sudo ip link add internal-right type veth peer name external-right
sudo ip link set internal-right master br-right
sudo ip link set internal-right up

# Configure the container-side pair with an interface and address
sudo ip link set external-left netns $pid_left
sudo ip netns exec $pid_left ip link set dev external-left name eth0
sudo ip netns exec $pid_left ip link set eth0 address 12:34:88:5D:61:BD
sudo ip netns exec $pid_left ip link set eth0 up
sudo ip netns exec $pid_left ip addr add 10.0.0.1/16 dev eth0

sudo ip link set external-right netns $pid_right
sudo ip netns exec $pid_right ip link set dev external-right name eth0
sudo ip netns exec $pid_right ip link set eth0 address 5A:34:88:5D:61:BD
sudo ip netns exec $pid_right ip link set eth0 up
sudo ip netns exec $pid_right ip addr add 10.0.0.2/16 dev eth0
echo "Done."
echo "### Setup complete. Ready to start simulation ###"

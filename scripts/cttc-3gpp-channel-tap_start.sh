#!/usr/bin/env bash

# Exit immediately if a commands exits with non-zero status
set -e

# Add bridges
echo "Add bridges..."
sudo ip link add name br-left type bridge
sudo ip link add name br-right type bridge
sudo ip link add name br-server type bridge
echo "Done."

# Add tap devices
echo "Add tap devices..."
sudo ip tuntap add tap-left mode tap
sudo ip tuntap add tap-right mode tap
sudo ip tuntap add tap-server mode tap

sudo ifconfig tap-left 0.0.0.0 promisc up
sudo ifconfig tap-right 0.0.0.0 promisc up
sudo ifconfig tap-server 0.0.0.0 promisc up
echo "Done."

# Attach tap devices to bridges and activate
echo "Attach taps to bridges..."
sudo ip link set tap-left master br-left
sudo ip link set br-left up
sudo ip link set tap-right master br-right
sudo ip link set br-right up
sudo ip link set tap-server master br-server
sudo ip link set br-server up
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
export pid_server=0

# Run container orchestrator
echo "Start containers..."
docker compose -f scenarios/cttc-3gpp-channel-tap.yaml up --detach
echo "Containers started."
pid_left=$(docker inspect --format '{{ .State.Pid }}' left)
pid_right=$(docker inspect --format '{{ .State.Pid }}' right)
pid_server=$(docker inspect --format '{{ .State.Pid }}' server)

# Soft-link the network namespace created by container into the linux namespace runtime
sudo ln -s /proc/$pid_left/ns/net /var/run/netns/$pid_left
sudo ln -s /proc/$pid_right/ns/net /var/run/netns/$pid_right
sudo ln -s /proc/$pid_server/ns/net /var/run/netns/$pid_server

# Create Veth pair to attach to bridge
echo "Create Veth pairs..."
sudo ip link add internal-left type veth peer name external-left
sudo ip link set internal-left master br-left
sudo ip link set internal-left up

sudo ip link add internal-right type veth peer name external-right
sudo ip link set internal-right master br-right
sudo ip link set internal-right up

sudo ip link add internal-server type veth peer name external-server
sudo ip link set internal-server master br-server
sudo ip link set internal-server up

# Configure the container-side pair with an interface and address
sudo ip link set external-left netns $pid_left
sudo ip netns exec $pid_left ip link set dev external-left name eth0
sudo ip netns exec $pid_left ip link set eth0 address 12:34:88:5D:61:BD
sudo ip netns exec $pid_left ip link set eth0 up
sudo ip netns exec $pid_left ip addr add 10.1.1.1/16 dev eth0

sudo ip link set external-right netns $pid_right
sudo ip netns exec $pid_right ip link set dev external-right name eth0
sudo ip netns exec $pid_right ip link set eth0 address 5A:34:88:5D:61:BC
sudo ip netns exec $pid_right ip link set eth0 up
sudo ip netns exec $pid_right ip addr add 10.1.1.2/16 dev eth0

sudo ip link set external-server netns $pid_server
sudo ip netns exec $pid_server ip link set dev external-server name eth0
sudo ip netns exec $pid_server ip link set eth0 address 5A:34:88:5D:61:BA
sudo ip netns exec $pid_server ip link set eth0 up
sudo ip netns exec $pid_server ip addr add 10.1.1.3/16 dev eth0

echo "Done."
echo "### Setup complete. Starting simulation... ###"
docker exec ns-3 ./ns3 run scratch/cttc-3gpp-channel-scratch.cc > /tmp/ns3.log &
echo "Simulation running..."
sleep 5
echo "Starting server..."
docker exec server go run . > /tmp/server.log &
sleep 5
echo "Pinging server from UE0"
docker exec left curl -X POST 10.1.1.3:8080 -d '{"activity":{"description":"get resource","time":"2021-12-24T12:42:31Z","device":"iphone","node":"healthy"}}' > /tmp/node1.log
docker exec left curl -X GET 10.1.1.3:8080 -d '{"id":0}' >> /tmp/node1.log
echo "Pinging server from UE1"
docker exec right curl -X POST 10.1.1.3:8080 -d '{"activity":{"description":"get resource","time":"2021-12-24T12:43:31Z","device":"PC","node":"bad"}}' > /tmp/node2.log
docker exec left curl -X GET 10.1.1.3:8080 -d '{"id":1}' >> /tmp/node2.log
echo "Waiting for experiment to finish..."
sleep 15
echo "Experiment completed."
echo "Prepairing ns3 log results..."
docker exec ns-3 sh -c "tar -cf /tmp/results.tar *.pcap"
docker cp ns-3:/tmp/results.tar "/tmp/results.tar"
date=$(date +"%d%m%Y")
n=1
while [[ -d "results/${date}-${n}" ]] ; do
    n=$(($n+1))
done
mkdir "results/${date}-${n}"
tar -xf "/tmp/results.tar" -C "results/${date}-${n}"
rm "/tmp/results.tar"
mv /tmp/server.log results/${date}-${n}
mv /tmp/ns3.log results/${date}-${n}
mv /tmp/node1.log results/${date}-${n}
mv /tmp/node2.log results/${date}-${n}
#rm /tmp/server.log
#rm /tmp/node1.log
#rm /tmp/node2/log
echo "Results available at results/${date}-${n}"
echo "done."

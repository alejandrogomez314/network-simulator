FROM ubuntu:18.04

RUN apt-get update && apt-get install -y \
	iputils-ping 	\
	iproute2     	\
	netcat		\
  traceroute \
  curl \
	&& rm -rf /var/lib/apt/lists/*

CMD ["/bin/bash"]


#ns3 network simulator code
#Copyright 2023 Carnegie Mellon University.
#NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT INFRINGEMENT.
#Released under a MIT (SEI)-style license, please see license.txt or contact permission@sei.cmu.edu for full terms.
#[DISTRIBUTION STATEMENT A] This material has been approved for public release and unlimited distribution.  Please see Copyright notice for non-US Government use and distribution.
#This Software includes and/or makes use of the following Third-Party Software subject to its own license:
#1. ns-3 (https://www.nsnam.org/about/) Copyright 2011 nsnam.
#DM23-0109
#

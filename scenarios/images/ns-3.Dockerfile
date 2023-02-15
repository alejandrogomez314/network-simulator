FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive 

# Install dependencies
RUN apt update \
    && apt install git g++ python3 cmake make tar wget libc6-dev sqlite sqlite3 libsqlite3-dev -y \
    && rm -rf /var/lib/apt/lists/*

# Install ns-3
RUN cd /usr/local && \
    wget https://www.nsnam.org/release/ns-allinone-3.37.tar.bz2 && \
    tar xjf ns-allinone-3.37.tar.bz2

# Build ns-3
WORKDIR /usr/local/ns-allinone-3.37/ns-3.37

RUN ./ns3 configure --enable-examples --enable-tests \
    && ./ns3 build

# Install 5G Lena project and rebuild
RUN cd contrib && \
    git clone https://gitlab.com/cttc-lena/nr.git && \
    cd nr && \
    git checkout 5g-lena-v2.3.y \
    && cd ../../  \
    && ./ns3 configure --enable-examples --enable-tests \
    && ./ns3 build

# Test installation is successful
RUN ./ns3 run test.py


#ns3 network simulator code
#Copyright 2023 Carnegie Mellon University.
#NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT INFRINGEMENT.
#Released under a MIT (SEI)-style license, please see license.txt or contact permission@sei.cmu.edu for full terms.
#[DISTRIBUTION STATEMENT A] This material has been approved for public release and unlimited distribution.  Please see Copyright notice for non-US Government use and distribution.
#This Software includes and/or makes use of the following Third-Party Software subject to its own license:
#1. ns-3 (https://www.nsnam.org/about/) Copyright 2011 nsnam.
#DM23-0109
#

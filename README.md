# Network Simulator
This is the source repository for creating the experiments that will be run on ns-3. The developer codes an ns-3 simulation, creates a docker-compose file to setup the necessary containers and volumes, and executes using a script that both sets up the required linux networking interfaces on host and connects them to the running containers and simulation.

## Quickstart
Ensure you have docker compose installed in your system.
Run `docker compose -f scenarios/hello-world-scenario.yaml up`

To run other scenarios, such as the tap-csma-scenario, you must instead use a setup script such as `/scripts/setup.sh`. To teardown everything run the corresponding teardown script `/scripts/teardown.sh`.

## Project structure
### images
Contains the docker images used to create user-end devices, master nodes, ns-3 simulation, and any other kind of node that a simulation requires. Images are currently stored in the local workstation and need to be uploaded to the image registry so others can use.

### scenarios 
These are docker compose files that set up containers and volumes for a specific experiment, or "scenario." This allows for rapid deployment and teardowns across different workstations.

### src
Contains the ns-3 development code that creates the simulation. Note that the development file and docker compose file names coincide for readability. ALL ns-3 code should reside here, since the docker compose scenario files use this location as a mount for setting up the ns-3 simulation. Internally for ns-3, code is mounted on `<ns-3 installation>/scratch/`, which is an ns-3 specific location for development.

### scripts
Contains the scripts that actually run a scenario. Scripts set up host networking interfaces, start docker compose scenarios and connect these interfaces to the newly created containers. Scripts also exist to quickly teardown all devices and containers.

## Development
ns-3 development files are available in `src` folder. They are mounted as a volume when `docker compose` is called for the appropiate scenario. **Only perform development on this folder**.

ns3 network simulator code
Copyright 2023 Carnegie Mellon University.
NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT INFRINGEMENT.
Released under a MIT (SEI)-style license, please see license.txt or contact permission@sei.cmu.edu for full terms.
[DISTRIBUTION STATEMENT A] This material has been approved for public release and unlimited distribution.  Please see Copyright notice for non-US Government use and distribution.
This Software includes and/or makes use of the following Third-Party Software subject to its own license:
1. ns-3 (https://www.nsnam.org/about/) Copyright 2011 nsnam.
DM23-0109


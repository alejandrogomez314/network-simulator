#include <chrono>

#include "ns3/core-module.h"
#include "ns3/config-store-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/tap-bridge-module.h"

// nr stuff
#include "ns3/mobility-module.h"
#include "ns3/nr-module.h"
#include "ns3/antenna-module.h"
#include "ns3/point-to-point-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("5gEmu");

void
Log (std::string msg)
{
  std::cout << msg << std::endl;
}

void
LogNodes (NodeContainer nodes)
{
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  for (auto j = nodes.Begin (); j != nodes.End (); ++j)
    {
      Ptr<Node> object = *j;
      // get node name
      std::string nodeName = Names::FindName (object);
      if (nodeName.empty ())
        {
          nodeName = std::to_string (object->GetId ());
        }
      NS_LOG_UNCOND (nodeName);
      // log all ips for that node with interface index
      NS_LOG_UNCOND ("\tInterfaces: ");
      Ptr<Ipv4> node_ipv4 = object->GetObject<Ipv4> ();
      for (uint32_t i = 0; i < node_ipv4->GetNInterfaces (); i++)
        {
          Ipv4Address ipv4Address = node_ipv4->GetAddress (i, 0).GetLocal ();
          if (ipv4Address == Ipv4Address::GetLoopback ())
            {
              NS_LOG_UNCOND ("\t\t" << i << ": " << ipv4Address << " (Loopback)");
            }
          else
            {
              NS_LOG_UNCOND ("\t\t" << i << ": " << ipv4Address);
            }
        }
      // print all routes for that node
      Ptr<Ipv4StaticRouting> staticRouting = ipv4RoutingHelper.GetStaticRouting (node_ipv4);
      uint32_t nRoutes = staticRouting->GetNRoutes ();
      NS_LOG_UNCOND ("\tRoutes: ");
      for (uint32_t j = 0; j < nRoutes; j++)
        {
          Ipv4RoutingTableEntry route = staticRouting->GetRoute (j);
          NS_LOG_UNCOND ("\t\t" << route.GetDest () << " (" << route.GetDestNetworkMask ()
                                << ") --> " << route.GetGateway () << " (interface "
                                << route.GetInterface () << ")");
        }
    }
  NS_LOG_UNCOND ("---");
}

int
main (int argc, char *argv[])
{
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  NS_LOG_INFO ("Create nodes");
  NodeContainer nodes;
  nodes.Create (4);
  NodeContainer ghostNodes;
  ghostNodes.Add (nodes.Get (0));
  ghostNodes.Add (nodes.Get (1));
  Names::Add ("GhostNode0", ghostNodes.Get (0));
  Names::Add ("GhostNode1", ghostNodes.Get (1));
  NodeContainer ueNodes;
  ueNodes.Add (nodes.Get (2));
  ueNodes.Add (nodes.Get (3));
  Names::Add ("UeNode0", ueNodes.Get (0));
  Names::Add ("UeNode1", ueNodes.Get (1));

  NS_LOG_INFO ("Create NR network");
  double frequency = 28e9;
  double bandwidth = 100e6;
  double txPower = 40;
  enum BandwidthPartInfo::Scenario scenarioEnum = BandwidthPartInfo::RMa;
  double hBS = 35;
  double hUT = 1.5;
  double speed = 1;
  NodeContainer enbNodes;
  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (999999999));

  enbNodes.Create (1);
  Names::Add ("EnbNode0", enbNodes.Get (0));

  NS_LOG_DEBUG ("position the base station and UEs");
  // position the base stations
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  enbPositionAlloc->Add (Vector (0.0, 0.0, hBS));
  enbPositionAlloc->Add (Vector (0.0, 80.0, hBS));
  MobilityHelper enbmobility;
  enbmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbmobility.SetPositionAllocator (enbPositionAlloc);
  enbmobility.Install (enbNodes);

  // position the mobile terminals and enable the mobility
  MobilityHelper uemobility;
  uemobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  uemobility.Install (ueNodes);

  ueNodes.Get (0)->GetObject<MobilityModel> ()->SetPosition (
      Vector (90, 15, hUT)); // (x, y, z) in m
  ueNodes.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (
      Vector (0, speed, 0)); // move UE1 along the y axis

  ueNodes.Get (1)->GetObject<MobilityModel> ()->SetPosition (
      Vector (30, 50.0, hUT)); // (x, y, z) in m
  ueNodes.Get (1)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (
      Vector (-speed, 0, 0)); // move UE2 along the x axis

  NS_LOG_DEBUG ("create nr sim helpers");
  Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper> ();
  Ptr<IdealBeamformingHelper> beamHelper = CreateObject<IdealBeamformingHelper> ();
  Ptr<NrHelper> nrHelper = CreateObject<NrHelper> ();
  nrHelper->SetBeamformingHelper (beamHelper);
  nrHelper->SetEpcHelper (epcHelper);

  NS_LOG_DEBUG (
      "spectrum configuration, we create a single operational band and configure the scenario");
  BandwidthPartInfoPtrVector allBwps;
  CcBwpCreator ccBwpCreator;
  const uint8_t numCcPerBand = 1;

  /* Create the configuration for the CcBwpHelper. SimpleOperationBandConf creates
   * a single BWP per CC and a single BWP in CC.
   *
   * Hence, the configured spectrum is:
   *
   * |---------------Band---------------|
   * |---------------CC-----------------|
   * |---------------BWP----------------|
   */
  CcBwpCreator::SimpleOperationBandConf bandConf (frequency, bandwidth, numCcPerBand, scenarioEnum);
  OperationBandInfo band = ccBwpCreator.CreateOperationBandContiguousCc (bandConf);
  NS_LOG_DEBUG ("Initialize channel and pathloss, plus other things inside band.");
  nrHelper->InitializeOperationBand (&band);
  allBwps = CcBwpCreator::GetAllBwps ({band});

  NS_LOG_DEBUG ("Configure ideal beamforming method");
  beamHelper->SetAttribute ("BeamformingMethod", TypeIdValue (DirectPathBeamforming::GetTypeId ()));

  NS_LOG_DEBUG ("configure scheduler");
  nrHelper->SetSchedulerTypeId (NrMacSchedulerTdmaRR::GetTypeId ());

  NS_LOG_DEBUG ("Antennas for the UEs");
  nrHelper->SetUeAntennaAttribute ("NumRows", UintegerValue (2));
  nrHelper->SetUeAntennaAttribute ("NumColumns", UintegerValue (4));
  nrHelper->SetUeAntennaAttribute ("AntennaElement",
                                   PointerValue (CreateObject<IsotropicAntennaModel> ()));

  NS_LOG_DEBUG ("Antennas for the gNbs");
  nrHelper->SetGnbAntennaAttribute ("NumRows", UintegerValue (8));
  nrHelper->SetGnbAntennaAttribute ("NumColumns", UintegerValue (8));
  nrHelper->SetGnbAntennaAttribute ("AntennaElement",
                                    PointerValue (CreateObject<IsotropicAntennaModel> ()));

  NS_LOG_DEBUG ("install nr net devices");
  NetDeviceContainer enbNetDev = nrHelper->InstallGnbDevice (enbNodes, allBwps);
  NetDeviceContainer ueNetDev = nrHelper->InstallUeDevice (ueNodes, allBwps);

  nrHelper->GetGnbPhy (enbNetDev.Get (0), 0)->SetTxPower (txPower);

  for (auto it = enbNetDev.Begin (); it != enbNetDev.End (); ++it)
    {
      DynamicCast<NrGnbNetDevice> (*it)->UpdateConfig ();
    }

  for (auto it = ueNetDev.Begin (); it != ueNetDev.End (); ++it)
    {
      DynamicCast<NrUeNetDevice> (*it)->UpdateConfig ();
    }

  // get SGW/PGW and create a single RemoteHost
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (2);
  Names::Add ("RemoteHost", remoteHostContainer.Get (0));
  Names::Add ("RemoteHostGhost", remoteHostContainer.Get (1));
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internetStackHelper;
  internetStackHelper.Install (remoteHostContainer);

  // connect a remoteHost to pgw. Setup routing too
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (2500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer p2pInetDevs = p2ph.Install (pgw, remoteHost);

  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "/8");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (p2pInetDevs);
  Ipv4StaticRoutingHelper ipv4RoutingHelper;

  Ptr<Ipv4StaticRouting> staticRoutingRemoteHost =
      ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  staticRoutingRemoteHost->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("/8"), 1);

  internetStackHelper.Install (ueNodes);

  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueNetDev));

  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting =
          ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // attach UEs to the closest eNB
  nrHelper->AttachToClosestEnb (ueNetDev, enbNetDev);

  NS_LOG_INFO ("Add ghost ues");
  CsmaHelper csmaHelper;
  csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (5000000));

  NodeContainer csmaNodes;
  csmaNodes.Add (ghostNodes.Get (0));
  csmaNodes.Add (ghostNodes.Get (1));
  csmaNodes.Add (remoteHostContainer.Get(0));
  csmaNodes.Add (remoteHostContainer.Get(1));
  csmaNodes.Add (ueNodes.Get (0));
  csmaNodes.Add (ueNodes.Get (1));
  NetDeviceContainer csmaDevices = csmaHelper.Install (csmaNodes);

  internetStackHelper.Install (ghostNodes);
  // check if ue nodes have internet installed
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      if (!ueNode->GetObject<Ipv4> ())
        {
          internetStackHelper.Install (ueNode);
        }
    }

  NS_LOG_INFO ("Assign IP Addresses");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "/24");
  Ipv4InterfaceContainer csmaInterfaces = ipv4.Assign (csmaDevices);

  NS_LOG_INFO ("Static routing");

Ptr<Ipv4StaticRouting> staticRoutingUeNode0 =
    ipv4RoutingHelper.GetStaticRouting (ueNodes.Get (0)->GetObject<Ipv4> ());
staticRoutingUeNode0->AddHostRouteTo(Ipv4Address ("10.1.1.1"), Ipv4Address ("7.0.0.2"), 1);

  Ptr<Ipv4StaticRouting> staticRoutingUeNode1 =
      ipv4RoutingHelper.GetStaticRouting(ueNodes.Get(0)->GetObject<Ipv4> ());
  staticRoutingUeNode1->AddHostRouteTo(Ipv4Address ("10.1.1.2"), Ipv4Address ("7.0.0.3"), 1); // @TODO: change from ip to the right ip address


  NS_LOG_INFO ("Create tap device");
  TapBridgeHelper tapBridge;
  tapBridge.SetAttribute ("Mode", StringValue ("UseBridge"));
  tapBridge.SetAttribute ("DeviceName", StringValue ("tap-left"));
  tapBridge.Install (ghostNodes.Get (0), csmaDevices.Get (0));
  
  tapBridge.SetAttribute ("DeviceName", StringValue ("tap-right"));
  tapBridge.Install (ghostNodes.Get (1), csmaDevices.Get (1));

  tapBridge.SetAttribute ("DeviceName", StringValue ("tap-server"));
  tapBridge.Install (remoteHostContainer.Get (1), csmaDevices.Get (2));

  //internetStackHelper.EnablePcapIpv4 ("prefix", NodeContainer::GetGlobal ());

  // get all nodes
  NodeContainer allNodes = NodeContainer::GetGlobal ();

  // log nodes
  //Simulator::Schedule (Seconds (0.5), &LogNodes, NodeContainer::GetGlobal ());

  ApplicationContainer apps;

//  Simulator::Schedule (Seconds (1.0), &Log, "\nPing from UE0 to RemoteHost");
//  Ptr<Ipv4> remoteHostIpv4 = remoteHost->GetObject<Ipv4> ();
//  Ipv4Address remoteHostAddr = remoteHostIpv4->GetAddress (1, 0).GetLocal ();
//  V4PingHelper ping0 (remoteHostAddr);
//  ping0.SetAttribute ("Verbose", BooleanValue (true));
//  apps = ping0.Install (ueNodes.Get (0));
//  apps.Start (Seconds (1.0));
//  apps.Stop (Seconds (4.0));
//
//  Simulator::Schedule (Seconds (5.0), &Log, "\nPing from UE1 to RemoteHost");
//  V4PingHelper ping1 (remoteHostAddr);
//  ping1.SetAttribute ("Verbose", BooleanValue (true));
//  apps = ping1.Install (ueNodes.Get (1));
//  apps.Start (Seconds (5.0));
//  apps.Stop (Seconds (8.0));
//
//  Simulator::Schedule (Seconds (9.0), &Log, "\nPing from UE0 to UE1");
//  V4PingHelper ping2 (ueNodes.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());
//  ping2.SetAttribute ("Verbose", BooleanValue (true));
//  apps = ping2.Install (ueNodes.Get (0));
//  apps.Start (Seconds (9.0));
//  apps.Stop (Seconds (12.0));
//

  NS_LOG_INFO ("Run Simulation.");
  csmaHelper.EnablePcapAll ("5gEmu", true);

  auto start = std::chrono::high_resolution_clock::now ();

  Simulator::Stop (Seconds (30.));
  Simulator::Run ();

  // real time vs simulation time
  auto end = std::chrono::high_resolution_clock::now ();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds> (end - start);
  NS_LOG_INFO ("Real time: " << elapsed.count () << " ms");
  NS_LOG_INFO ("Simulation time: " << (Simulator::Now ()).GetMilliSeconds () << " ms");
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
/*
 * ns3 network simulator code
 * Copyright 2023 Carnegie Mellon University.
 * NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT INFRINGEMENT.
 * Released under a MIT (SEI)-style license, please see license.txt or contact permission@sei.cmu.edu for full terms.
 * [DISTRIBUTION STATEMENT A] This material has been approved for public release and unlimited distribution.  Please see Copyright notice for non-US Government use and distribution.
 * This Software includes and/or makes use of the following Third-Party Software subject to its own license:
 * 1. ns-3 (https://www.nsnam.org/about/) Copyright 2011 nsnam.
 * DM23-0109
 */

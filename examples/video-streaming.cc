/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors : Alessandro Russo <russo@disi.unitn.it>
 *
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/video-push-module.h"
#include "ns3/csma-module.h"
#include "ns3/inet-socket-address.h"
#include "ns3/video-helper.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("StreamingFirstExample");

int
main (int argc, char *argv[])
{
	SeedManager::SetSeed(1234);
	bool verbose = true;
	uint32_t node = 2;

	CommandLine cmd;
	cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", node);
	cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

	cmd.Parse (argc,argv);

	if (verbose)
	{
	LogComponentEnable ("StreamingFirstExample",  LogLevel( LOG_LEVEL_ALL | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
	LogComponentEnable ("VideoPushApplication",  LogLevel( LOG_LEVEL_ALL | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("UdpSocketImpl",  LogLevel( LOG_LEVEL_ALL | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("UdpL4Protocol",  LogLevel( LOG_LEVEL_ALL | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("Ipv4EndPointDemux",  LogLevel( LOG_LEVEL_ALL | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("Ipv4L3Protocol",  LogLevel( LOG_LEVEL_ALL | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("Ipv4Interface",  LogLevel( LOG_LEVEL_ALL | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("ArpL3Protocol",  LogLevel( LOG_LEVEL_ALL | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("Socket",  LogLevel( LOG_LEVEL_ALL | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("WifiNetDevice", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("AdhocWifiMac", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("DcaTxop", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("DcaManager", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("WifiMacQueue", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("DcfManager", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("Socket", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("Node", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("MacLow", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("MacRxMiddle", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("YansWifiPhy", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("InterferenceHelper", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("YansWifiChannel", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("Packet", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//	LogComponentEnable ("DefaultSimulatorImpl", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
	}

	NodeContainer nodes;
	nodes.Create(2);

	WifiHelper wifi = WifiHelper::Default ();
//	wifi.SetStandard (WIFI_PHY_STANDARD_80211g);

	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
	wifiMac.SetType ("ns3::AdhocWifiMac");

	YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();

	YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();;
	phy.SetChannel (wifiChannel.Create ());
	NqosWifiMacHelper mac = wifiMac;

	MobilityHelper mobility;
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
	  "MinX", DoubleValue (0.0),
	  "MinY", DoubleValue (0.0),
	  "DeltaX", DoubleValue (10),
	  "DeltaY", DoubleValue (10),
	  "GridWidth", UintegerValue (2),
	  "LayoutType", StringValue ("RowFirst"));

	mobility.Install(nodes);

	NetDeviceContainer nodesDevices = wifi.Install(phy, mac, nodes);

//	Ipv4StaticRoutingHelper staticRouting;
//	Ipv4ListRoutingHelper listRouters;
//	listRouters.Add (staticRouting, 0);
//
	InternetStackHelper internetRouters;
//	internetRouters.SetRoutingHelper (listRouters);
	internetRouters.Install (nodes);

	Ipv4AddressHelper address;
	address.SetBase ("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer addresses = address.Assign (nodesDevices);

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	VideoHelper video = VideoHelper ("ns3::UdpSocketFactory", InetSocketAddress (addresses.GetAddress (1), 9999));
	video.SetAttribute ("OffTime", RandomVariableValue (ConstantVariable (10.0)));
	video.SetAttribute ("OnTime", RandomVariableValue (ConstantVariable (1.0)));
	video.SetAttribute ("DataRate", StringValue ("10kb/s"));
	video.SetAttribute ("PacketSize", UintegerValue (1200));
	video.SetAttribute ("PeerType", EnumValue (SOURCE));
	video.SetAttribute ("Local", AddressValue (addresses.GetAddress(0)));
	video.SetAttribute ("PeerPolicy", EnumValue (PS_RANDOM));
	video.SetAttribute ("ChunkPolicy", EnumValue (CS_LATEST));

	ApplicationContainer apps1 = video.Install (nodes.Get(0));
	apps1.Start (Seconds (1.0));
	apps1.Stop (Seconds (10.0));

	video = VideoHelper ("ns3::UdpSocketFactory", InetSocketAddress (addresses.GetAddress (0), 9999));
	video.SetAttribute ("OffTime", RandomVariableValue (ConstantVariable (10.0)));
	video.SetAttribute ("OnTime", RandomVariableValue (ConstantVariable (1.0)));

	ApplicationContainer apps2 = video.Install (nodes.Get(1));
	apps2.Start (Seconds (1.0));
	apps2.Stop (Seconds (10.0));

	Simulator::Stop (Seconds (10.0));

	Packet::EnablePrinting ();

	NS_LOG_INFO ("Run Simulation.");
	Simulator::Run ();
	Simulator::Destroy ();
	NS_LOG_INFO ("Done.");
	return 0;
}

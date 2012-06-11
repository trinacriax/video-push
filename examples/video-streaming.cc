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

#include <iostream>
#include <fstream>
#include <cassert>
#include <string>
#include <sstream>

#include "ns3/wifi-net-device.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/adhoc-wifi-mac.h"
#include "ns3/yans-wifi-phy.h"
#include "ns3/arf-wifi-manager.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/error-rate-model.h"
#include "ns3/yans-error-rate-model.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/video-helper.h"
#include "ns3/video-push-module.h"
#include "ns3/wifi-module.h"
#include "ns3/aodv-helper.h"
#include "ns3/string.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("VideoStreaming");

int main(int argc, char **argv) {
	/// Number of nodes
	uint32_t size = 20;
	/// Simulation time, seconds
	double totalTime = 50;
	uint32_t run = 1;
	uint32_t seed = 3945244811;
	/// LogDistance exponent
	double log_n = 2.0;
	/// LogDistance reference loss path
	double log_r = 30;
	/// Verbose
	uint32_t verbose = 0;
	/// Grid xmax
	double xmax = 100;
	double ymax = 100;
	double pulltime = 1;
	uint32_t pullmax = 1;
	bool pullactive = true;

	CommandLine cmd;
	cmd.AddValue("size", "Number of nodes.", size);
	cmd.AddValue("time", "Simulation time, s.", totalTime);
	cmd.AddValue("run", "Run Identifier", run);
	cmd.AddValue("xmax", "Grid X max", xmax);
	cmd.AddValue("ymax", "Grid Y max", ymax);
	cmd.AddValue("pulltime", "Time between pull in sec.", pulltime);
	cmd.AddValue("pullmax", "Max number of pull allowed per chunk", pullmax);
	cmd.AddValue("pullactive", "Pull activation allowed", pullactive);
	cmd.AddValue("v", "Verbose", verbose);
	cmd.Parse(argc, argv);

	SeedManager::SetRun (run);
	SeedManager::SetSeed (seed);
	Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue("2200"));
	Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("1500"));
	Config::SetDefault("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue(log_r));
	Config::SetDefault("ns3::LogDistancePropagationLossModel::Exponent", DoubleValue(log_n));
	Config::SetDefault("ns3::VideoPushApplication::PullTime", TimeValue(Seconds(pulltime)));
	Config::SetDefault("ns3::VideoPushApplication::PullMax", UintegerValue(pullmax));
	Config::SetDefault("ns3::VideoPushApplication::PullActive", BooleanValue(pullactive));

	if(verbose==1){
		LogComponentEnable("VideoStreaming", LogLevel (LOG_LEVEL_ALL | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
		LogComponentEnable("VideoPushApplication", LogLevel (LOG_LEVEL_ALL |LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("MacLow", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//		LogComponentEnable("UdpSocketImpl", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//		LogComponentEnable("Socket", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//		LogComponentEnable("DefaultSimulatorImpl", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//		LogComponentEnable("YansWifiChannel",  LogLevel (LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("YansWifiPhy",  LogLevel (LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("ChunkBuffer", LogLevel (LOG_LEVEL_ALL |LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("Ipv4L3Protocol", LogLevel (LOG_LEVEL_ALL |LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
	}

	/// Video start
	double sourceStart = ceil(totalTime*.30);
	/// Video stop
	double sourceStop = ceil(totalTime*.80);
	/// Client start
	double clientStart = ceil(totalTime*.20);;
	/// Client stop
	double clientStop = ceil(totalTime*.95);

	NodeContainer fake;
	fake.Create(1);
	NodeContainer nodes;
	nodes.Create(size);

	WifiHelper wifi = WifiHelper::Default();
	wifi.SetStandard(WIFI_PHY_STANDARD_80211b);
	wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager"
			,"DataMode", StringValue("DsssRate11Mbps")
			,"ControlMode", StringValue ("DsssRate11Mbps")
			,"NonUnicastMode", StringValue ("DsssRate11Mbps")
			);
	YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
	wifiPhy.SetChannel(wifiChannel.Create());

	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();
	wifiMac.SetType("ns3::AdhocWifiMac");

	NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodes);

	MobilityHelper mobility;
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.SetPositionAllocator("ns3::RandomBoxPositionAllocator",
					"X",RandomVariableValue(UniformVariable(0,xmax)),
					"Y",RandomVariableValue(UniformVariable(0,ymax)),
					"Z",RandomVariableValue(ConstantVariable(0)));
	mobility.Install(nodes);

	AodvHelper aodv;
	InternetStackHelper stack;
	stack.SetRoutingHelper(aodv);
	stack.Install(nodes);

	Ipv4AddressHelper address;
	address.SetBase("10.0.0.0", "255.0.0.0");

	Ipv4InterfaceContainer interfaces;
	interfaces = address.Assign(devices);

	//Source streaming rate
	uint64_t stream = 1000000;
	Ipv4Address subnet ("10.255.255.255");
	NS_LOG_INFO ("Create Source");
	InetSocketAddress dst = InetSocketAddress (subnet, PUSH_PORT);
	Config::SetDefault ("ns3::UdpSocket::IpMulticastTtl", UintegerValue (1));
	VideoHelper video = VideoHelper ("ns3::UdpSocketFactory", dst);
	video.SetAttribute ("DataRate", DataRateValue (DataRate (stream)));
	video.SetAttribute ("PacketSize", UintegerValue (1200));
	video.SetAttribute ("PeerType", EnumValue (SOURCE));
	video.SetAttribute ("Local", AddressValue (interfaces.GetAddress(0)));
	video.SetAttribute ("PeerPolicy", EnumValue (PS_RANDOM));
	video.SetAttribute ("ChunkPolicy", EnumValue (CS_LATEST));

	ApplicationContainer apps = video.Install (nodes.Get (0));
	apps.Start (Seconds (sourceStart));
	apps.Stop (Seconds (sourceStop));

	for(uint32_t n = 1; n < nodes.GetN() ; n++){
		InetSocketAddress dstC = InetSocketAddress (subnet, PUSH_PORT);
		Config::SetDefault ("ns3::UdpSocket::IpMulticastTtl", UintegerValue (1));
		VideoHelper videoC = VideoHelper ("ns3::UdpSocketFactory", dstC);
		videoC.SetAttribute ("PeerType", EnumValue (PEER));
		videoC.SetAttribute ("LocalPort", UintegerValue (PUSH_PORT));
		videoC.SetAttribute ("Local", AddressValue(interfaces.GetAddress(n)));
		videoC.SetAttribute ("PeerPolicy", EnumValue (PS_RANDOM));
		videoC.SetAttribute ("ChunkPolicy", EnumValue (CS_LATEST));

		ApplicationContainer appC = videoC.Install (nodes.Get(n));
		appC.Start (Seconds (clientStart));
		appC.Stop (Seconds (clientStop));
	}

	std::cout << "Starting simulation for " << totalTime << " s ...\n";
	Simulator::Stop(Seconds(totalTime));
	Simulator::Run();
	Simulator::Destroy();
	return 0;
}

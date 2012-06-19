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
#include "ns3/mbn-aodv-module.h"
#include "ns3/string.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("VideoStreaming");

/// Verbose
uint32_t verbose = 0;

struct mycontext{
	uint32_t id;
	std::string callback;
};

struct mycontext GetContextInfo (std::string context){
	struct mycontext mcontext;
	int p2 = context.find("/");
	int p1 = context.find("/",p2+1);
	p2 = context.find("/",p1+1);
	mcontext.id = atoi(context.substr(p1+1, (p2-p1)-1).c_str());
//	std::cout<<"P1 = "<<p1<< " P2 = "<< p2 << " ID: " <<mcontext.id;
	p1 = context.find_last_of("/");
	p2 = context.length() - p2;
	mcontext.callback = context.substr(p1+1, p2).c_str();
//	std::cout<<"; P1 = "<<p1<< " P2 = "<< p2<< " CALL: "<< mcontext.callback<<"\n";
	return mcontext;
}

static void PhyTxDrop (Ptr<const Packet> p)
{
std::cout << Simulator::Now() <<" Phy Drop Packet "<< p->GetSize() << " bytes " << std::endl;
}

void
DevTxTrace (std::string context, Ptr<const Packet> p)
{
	struct mycontext mc = GetContextInfo (context);
	std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " "<< p->GetUid() << std::endl;
}
void
DevRxTrace (std::string context, Ptr<const Packet> p)
{
	struct mycontext mc = GetContextInfo (context);
	std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " " << p->GetUid() << std::endl;
}
void
PhyRxOkTrace (std::string context, Ptr<const Packet> packet, double
snr, WifiMode mode, enum WifiPreamble preamble)
{
	struct mycontext mc = GetContextInfo (context);
	std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " "<<  " PHYRXOK mode=" << mode << " snr=" << snr << " " <<
*packet << std::endl;
}

void
PhyRxErrorTrace (std::string context, Ptr<const Packet> packet, double
snr)
{
	struct mycontext mc = GetContextInfo (context);
	std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " "<<  " PHYRXERROR snr=" << snr << " " << *packet <<
std::endl;
}
void
PhyTxTrace (std::string context, Ptr<const Packet> packet, WifiMode
mode, WifiPreamble preamble, uint8_t txPower)
{
	struct mycontext mc = GetContextInfo (context);
	std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " "<<  " PHYTX mode=" << mode << " " << *packet <<
std::endl;
}

void
PhyStateTrace (std::string context, Time start, Time duration, enum
WifiPhy::State state)
{
	struct mycontext mc = GetContextInfo (context);
	std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " "<<  " state=" << state << " start=" << start << " duration=" << duration << std::endl;
}

static void ArpDiscard (std::string context, Ptr<const Packet> p)
{
	struct mycontext mc = GetContextInfo (context);
	std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " "<< " Arp discards packet "<< p->GetUid() << " of "<<p->GetSize() << " bytes " << std::endl;
}

int main(int argc, char **argv) {
	/// Number of nodes
	uint32_t size = 100;
	/// Simulation time in seconds
	double totalTime = 50;
	uint32_t run = 1;
	// Simulation seed
	uint32_t seed = 3945244811;
	/// Grid xmax
	double xmax = 80;
	/// Grid ymax
	double ymax = 80;
	// Period between pull
	double pulltime = 150;//100ms
	// max number of pull to retrieve a chunk
	uint32_t pullmax = 1;
	// Time in seconds between hellos
	double hellotime = 4;
	// max number of hello loss before removing a neighbor
	uint32_t helloloss = 1;
	// Activate pull as recovery mechanism
	bool pullactive = true;
	// Unicast routing protocol to use
	uint32_t routing = 0;
	// reference loss
	double PLref = 30.0;
	// loss exponent
	double PLexp = 3.5;
	// Tx power start
	double TxStart = 18.0;
	// Tx power end
	double TxEnd = 18.0;
	// Tx power levels
	uint32_t TxLevels = 1;
	// Energy detection threshold
	double EnergyDet= -95.0;
	// CCA mode 1
	double CCAMode1 = -62.0;


	CommandLine cmd;
	cmd.AddValue("size", "Number of nodes.", size);
	cmd.AddValue("time", "Simulation time, s.", totalTime);
	cmd.AddValue("run", "Run Identifier", run);
	cmd.AddValue("xmax", "Grid X max", xmax);
	cmd.AddValue("ymax", "Grid Y max", ymax);
	cmd.AddValue("routing", "Unicast Routing Protocol (1 - AODV, 2 - MBN) ", routing);
	cmd.AddValue("hellotime", "Hello time", hellotime);
	cmd.AddValue("helloloss", "Max number of hello loss to be removed from neighborhood", helloloss);
	cmd.AddValue("pulltime", "Time between pull in sec. (e.g., 0.100 sec = 100ms)", pulltime);
	cmd.AddValue("pullmax", "Max number of pull allowed per chunk", pullmax);
	cmd.AddValue("pullactive", "Pull activation allowed", pullactive);
	cmd.AddValue ("PLref", "Reference path loss dB.", PLref);
	cmd.AddValue ("PLexp", "Path loss exponent.", PLexp);
	cmd.AddValue ("TxStart", "Transmission power start dBm.", TxStart);
	cmd.AddValue ("TxEnd", "Transmission power end dBm.", TxEnd);
	cmd.AddValue ("TxLevels", "Transmission power levels.", TxLevels);
	cmd.AddValue ("EnergyDet", "Energy detection threshold dBm.", EnergyDet);
	cmd.AddValue ("CCAMode1", "CCA mode 1 threshold dBm.", CCAMode1);
	cmd.AddValue("v", "Verbose", verbose);
	cmd.Parse(argc, argv);

	SeedManager::SetRun (run);
	SeedManager::SetSeed (seed);
	Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue("2200"));
	Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("1600"));
	Config::SetDefault("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue(PLref));
	Config::SetDefault("ns3::LogDistancePropagationLossModel::Exponent", DoubleValue(PLexp));
	Config::SetDefault("ns3::VideoPushApplication::PullTime", TimeValue(Time::FromDouble(pulltime,Time::MS)));
	Config::SetDefault("ns3::VideoPushApplication::HelloTime", TimeValue(Seconds(hellotime)));
	Config::SetDefault("ns3::VideoPushApplication::PullMax", UintegerValue(pullmax));
	Config::SetDefault("ns3::VideoPushApplication::HelloLoss", UintegerValue(helloloss));
	Config::SetDefault("ns3::VideoPushApplication::PullActive", BooleanValue(pullactive));
	Config::SetDefault("ns3::VideoPushApplication::Source", Ipv4AddressValue(Ipv4Address("10.0.0.1")));
	Config::SetDefault("ns3::YansWifiPhy::TxGain",DoubleValue(0.0));
	Config::SetDefault("ns3::YansWifiPhy::RxGain",DoubleValue(0.0));
	Config::SetDefault("ns3::YansWifiPhy::TxPowerStart",DoubleValue(TxStart));
	Config::SetDefault("ns3::YansWifiPhy::TxPowerEnd",DoubleValue(TxEnd));
	Config::SetDefault("ns3::YansWifiPhy::TxPowerLevels",UintegerValue(TxLevels));
	Config::SetDefault("ns3::YansWifiPhy::EnergyDetectionThreshold",DoubleValue(EnergyDet));///17.3.10.1 Receiver minimum input sensitivity
	Config::SetDefault("ns3::YansWifiPhy::CcaMode1Threshold",DoubleValue(CCAMode1));///17.3.10.5 CCA sensitivity

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
		Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::YansWifiPhy/PhyTxDrop",MakeCallback (&PhyTxDrop));
		Config::Connect ("/NodeList/*/DeviceList/*/Mac/MacTx", MakeCallback (&DevTxTrace));
		Config::Connect ("/NodeList/*/DeviceList/*/Mac/MacRx",	MakeCallback (&DevRxTrace));
		Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/RxOk",	MakeCallback (&PhyRxOkTrace));
		Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/RxError",	MakeCallback (&PhyRxErrorTrace));
		Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/Tx",	MakeCallback (&PhyTxTrace));
		Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/State", MakeCallback (&PhyStateTrace));
		Config::Connect ("/NodeList/*/$ns3::ArpL3Protocol/Drop", MakeCallback (&ArpDiscard));
	}

	/// Video start
	double sourceStart = ceil(totalTime*.10);
	/// Video stop
	double sourceStop = ceil(totalTime*.80);
	/// Client start
	double clientStart = ceil(totalTime*.05);;
	/// Client stop
	double clientStop = ceil(totalTime*.95);

	NodeContainer fake;
	fake.Create(1);
	NodeContainer nodes;
	nodes.Create(size);

	WifiHelper wifi = WifiHelper::Default();
//	wifi.SetStandard(WIFI_PHY_STANDARD_80211b);
//	wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager"
//			,"DataMode", StringValue("DsssRate11Mbps")
//			,"ControlMode", StringValue ("DsssRate5_5Mbps")
//			,"NonUnicastMode", StringValue ("DsssRate5_5Mbps")
//			);
	wifi.SetStandard(WIFI_PHY_STANDARD_80211g);
	wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager"
			,"DataMode", StringValue("ErpOfdmRate54Mbps")
			,"ControlMode", StringValue("ErpOfdmRate6Mbps")
			,"NonUnicastMode", StringValue("ErpOfdmRate6Mbps")
			);
//	wifi.SetRemoteStationManager ("ns3::AarfWifiManager"
//			,"NonUnicastMode", StringValue("ErpOfdmRate6Mbps")
//			);
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

	InternetStackHelper stack;
	MbnAodvHelper mbnaodv;
	AodvHelper aodv;
	switch (routing)
	{
		case 1:
		{
			stack.SetRoutingHelper(aodv);
			break;
		}
		case 2:
		{
			stack.SetRoutingHelper(mbnaodv);
			break;
		}
		default:
		{
			break;
		}
	}
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
	video.SetAttribute ("PacketSize", UintegerValue (2304));
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

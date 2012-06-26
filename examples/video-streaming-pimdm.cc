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

#include "ns3/pimdm-helper.h"
#include "ns3/igmpx-helper.h"
#include "ns3/aodv-helper.h"
#include "ns3/mbn-aodv-helper.h"
#include "ns3/video-helper.h"

#include "ns3/wifi-module.h"
#include "ns3/wifi-net-device.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/adhoc-wifi-mac.h"
#include "ns3/yans-wifi-phy.h"
//#include "ns3/arf-wifi-manager.h"
#include "ns3/csma-helper.h"
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
#include "ns3/video-push-module.h"
#include "ns3/mbn-aodv-module.h"
#include "ns3/string.h"
#include "ns3/video-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("VideoStreamingPIMDM");

/// Verbose
static bool g_verbose = false;


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

static void GenericPacketTrace (std::string context, Ptr<const Packet> p)
{
	struct mycontext mc = GetContextInfo (context);
//	controls
	std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " " << p->GetSize() << " UID "<< p->GetUid() <<  std::endl;
}
/*
static void AppTx (Ptr<const Packet> p)
{
std::cout << Simulator::Now() <<" Sending Packet "<< p->GetSize() << " bytes " << std::endl;
}
*/
static void PhyTxDrop (Ptr<const Packet> p)
{
std::cout << Simulator::Now() <<" Phy Drop Packet "<< p->GetSize() << " bytes " << std::endl;
}

//static void TableChanged (std::string context, uint32_t size)
//{
//	struct mycontext mc = GetContextInfo (context);
//	std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " "<< size  << " entries " << std::endl;
//}

void
DevTxTrace (std::string context, Ptr<const Packet> p)
{
 if (g_verbose)
   {
		struct mycontext mc = GetContextInfo (context);
		std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " "<< p->GetUid() << std::endl;
   }
}
void
DevRxTrace (std::string context, Ptr<const Packet> p)
{
 if (g_verbose)
   {
		struct mycontext mc = GetContextInfo (context);
		std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " " << p->GetUid() << std::endl;
   }
}
void
PhyRxOkTrace (std::string context, Ptr<const Packet> packet, double
snr, WifiMode mode, enum WifiPreamble preamble)
{
 if (g_verbose)
   {

		struct mycontext mc = GetContextInfo (context);
		std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " "<<  " PHYRXOK mode=" << mode << " snr=" << snr << " " <<
*packet << std::endl;
   }
}
void
PhyRxErrorTrace (std::string context, Ptr<const Packet> packet, double
snr)
{
 if (g_verbose)
   {
		struct mycontext mc = GetContextInfo (context);
		std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " "<<  " PHYRXERROR snr=" << snr << " " << *packet <<
std::endl;
   }
}
void
PhyTxTrace (std::string context, Ptr<const Packet> packet, WifiMode
mode, WifiPreamble preamble, uint8_t txPower)
{
 if (g_verbose)
   {
		struct mycontext mc = GetContextInfo (context);
		std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " "<<  " PHYTX mode=" << mode << " " << *packet <<
std::endl;
   }
}

void
PhyStateTrace (std::string context, Time start, Time duration, enum
WifiPhy::State state)
{
 if (g_verbose)
   {
		struct mycontext mc = GetContextInfo (context);
		std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " "<<  " state=" << state << " start=" << start << " duration=" << duration << std::endl;
   }
}

static void ArpDiscard (std::string context, Ptr<const Packet> p)
{
	struct mycontext mc = GetContextInfo (context);
	std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " "<< " Arp discards packet "<< p->GetUid() << " of "<<p->GetSize() << " bytes " << std::endl;
}

int main(int argc, char **argv) {
	//Seed Run
	uint32_t run = 1;
	// Simulation seed
	uint32_t seed = 3945244811;
	// Verbose
	uint32_t verbose = 0;
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

	/// Number of router nodes
	uint32_t sizeRouter = 4;
	/// Number of client nodes
	uint32_t sizeClient = 1;
	/// Number of source nodes
	uint32_t sizeSource = 1;
	/// Grid xmax
	double xmax = 80;
	/// Grid ymax
	double ymax = 80;
	// Unicast routing protocol to use
	uint32_t routing = 0;

	//Source streaming rate
	uint64_t stream = 1000000;
	/// Simulation time, seconds
	double totalTime = 50;

	// Period between pull
	double pulltime = 150;//in ms
	// max number of pull to retrieve a chunk
	uint32_t pullmax = 1;
	// Time in seconds between hellos
	double hellotime = 4;
	// max number of hello loss before removing a neighbor
	uint32_t helloloss = 1;
	// Activate pull as recovery mechanism
	bool pullactive = false;

	CommandLine cmd;
	cmd.AddValue ("sizeRouter", "Number of router nodes.", sizeRouter);
	cmd.AddValue ("sizeClient", "Number of clients.", sizeClient);
	cmd.AddValue ("sizeSource", "Number of sources.", sizeSource);
	cmd.AddValue ("time", "Simulation time, s.", totalTime);
	cmd.AddValue ("run", "Run Identifier", run);
	cmd.AddValue ("PLref", "Reference path loss dB.", PLref);
	cmd.AddValue ("PLexp", "Path loss exponent.", PLexp);
	cmd.AddValue ("TxStart", "Transmission power start dBm.", TxStart);
	cmd.AddValue ("TxEnd", "Transmission power end dBm.", TxEnd);
	cmd.AddValue ("TxLevels", "Transmission power levels.", TxLevels);
	cmd.AddValue ("EnergyDet", "Energy detection threshold dBm.", EnergyDet);
	cmd.AddValue ("CCAMode1", "CCA mode 1 threshold dBm.", CCAMode1);
	cmd.AddValue ("xmax", "Grid X max", xmax);
	cmd.AddValue ("ymax", "Grid Y max", ymax);
	cmd.AddValue ("routing", "Unicast Routing Protocol (1 - AODV, 2 - MBN) ", routing);
	cmd.AddValue ("hellotime", "Hello time", hellotime);
	cmd.AddValue ("helloloss", "Max number of hello loss to be removed from neighborhood", helloloss);
	cmd.AddValue ("pullactive", "Pull activation allowed", pullactive);
	cmd.AddValue ("pullmax", "Max number of pull allowed per chunk", pullmax);
	cmd.AddValue ("pulltime", "Time between pull in sec. (e.g., 0.100 sec = 100ms)", pulltime);
	cmd.AddValue ("stream", "Source streaming rate in bps", stream);
	cmd.AddValue ("v", "Verbose", verbose);
	cmd.Parse(argc, argv);

	NS_LOG_DEBUG("Seed " << seed << " run "<< run << " sizeRouter "<< sizeRouter << " sizeClient " << sizeClient <<
			" sizeSource " << sizeSource << " routing " << routing <<
//			" range " << range << " cols " << cols <<
			" hellotime " << hellotime << " helloloss " << helloloss <<
			" pullactive " << pullactive << " pulltime " << pulltime << " pullmax " << pullmax <<
			" time " << totalTime << " PLref " << PLref << " PLexp " << PLexp << " TxStart " << TxStart <<
			" TxEnd " << TxEnd << " TxLevels " << TxLevels << " EnergyDet " << EnergyDet << " CCAMode1 "<< CCAMode1 <<
			" Stream "<< stream << "\n");

	g_verbose = (verbose==1);
	SeedManager::SetRun (run);
	SeedManager::SetSeed (seed);
	Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue("2346"));
	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("2346"));
	Config::SetDefault ("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue(PLref));
	Config::SetDefault ("ns3::LogDistancePropagationLossModel::Exponent", DoubleValue(PLexp));
	Config::SetDefault ("ns3::VideoPushApplication::PullActive", BooleanValue(pullactive));
	Config::SetDefault ("ns3::VideoPushApplication::PullTime", TimeValue(Time::FromDouble(pulltime,Time::MS)));
	Config::SetDefault ("ns3::VideoPushApplication::HelloTime", TimeValue(Time::FromDouble(hellotime,Time::S)));
	Config::SetDefault ("ns3::VideoPushApplication::PullMax", UintegerValue(pullmax));
	Config::SetDefault ("ns3::VideoPushApplication::HelloLoss", UintegerValue(helloloss));
	Config::SetDefault ("ns3::VideoPushApplication::Source", Ipv4AddressValue(Ipv4Address("10.0.0.1")));
	Config::SetDefault ("ns3::Ipv4L3Protocol::DefaultTtl", UintegerValue (1)); //avoid to forward broadcast packets
	Config::SetDefault ("ns3::Ipv4::IpForward", BooleanValue (false));
	Config::SetDefault ("ns3::YansWifiPhy::TxGain",DoubleValue(0.0));
	Config::SetDefault ("ns3::YansWifiPhy::RxGain",DoubleValue(0.0));
	Config::SetDefault ("ns3::YansWifiPhy::TxPowerStart",DoubleValue(TxStart));
	Config::SetDefault ("ns3::YansWifiPhy::TxPowerEnd",DoubleValue(TxEnd));
	Config::SetDefault ("ns3::YansWifiPhy::TxPowerLevels",UintegerValue(TxLevels));
	Config::SetDefault ("ns3::YansWifiPhy::EnergyDetectionThreshold",DoubleValue(EnergyDet));///17.3.10.1 Receiver minimum input sensitivity
	Config::SetDefault ("ns3::YansWifiPhy::CcaMode1Threshold",DoubleValue(CCAMode1));///17.3.10.5 CCA sensitivity

	if(verbose==1){
		LogComponentEnable("VideoStreamingPIMDM", LogLevel (LOG_LEVEL_ALL | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
		LogComponentEnable("VideoPushApplication", LogLevel (LOG_LEVEL_ALL |LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("CsmaChannel", LogLevel (LOG_LEVEL_ALL |LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("CsmaNetDevice", LogLevel (LOG_LEVEL_ALL |LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("AodvRoutingProtocol", LogLevel (LOG_LEVEL_ALL |LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
		LogComponentEnable("PIMDMMulticastRouting", LogLevel(LOG_INFO| LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("IGMPXRoutingProtocol", LogLevel(LOG_LEVEL_ALL| LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("UdpSocketImpl", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//		LogComponentEnable("Ipv4L3Protocol", LogLevel (LOG_LEVEL_ALL |LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("Ipv4RawSocketImpl", LogLevel (LOG_LEVEL_ALL |LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("Socket", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//		LogComponentEnable("MacLow", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//		LogComponentEnable("YansWifiChannel",  LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//		LogComponentEnable("YansWifiPhy",  LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//		LogComponentEnable("ArpL3Protocol", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
//		LogComponentEnable("Node", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));

//		LogComponentEnable("DefaultSimulatorImpl", LogLevel( LOG_LEVEL_ALL | LOG_DEBUG | LOG_LOGIC | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
		Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::YansWifiPhy/PhyTxDrop",MakeCallback (&PhyTxDrop));
		Config::Connect ("/NodeList/*/DeviceList/*/Mac/MacTx", MakeCallback (&DevTxTrace));
		Config::Connect ("/NodeList/*/DeviceList/*/Mac/MacRx",	MakeCallback (&DevRxTrace));
		Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/RxOk",	MakeCallback (&PhyRxOkTrace));
		Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/RxError",	MakeCallback (&PhyRxErrorTrace));
		Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/Tx",	MakeCallback (&PhyTxTrace));
		Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/State", MakeCallback (&PhyStateTrace));
		Config::Connect ("/NodeList/*/$ns3::ArpL3Protocol/Drop", MakeCallback (&ArpDiscard));
		Config::Connect ("/NodeList/*/$ns3::aodv::RoutingProtocol/ControlMessageTrafficSent", MakeCallback (&GenericPacketTrace));
		Config::Connect ("/NodeList/*/$ns3::aodv::RoutingProtocol/ControlMessageTrafficReceived", MakeCallback (&GenericPacketTrace));
	}

	Gnuplot g_Topology = Gnuplot("Topology.png");
	g_Topology.SetLegend("PosX","PosY");
	g_Topology.AppendExtra("set key out top center horizontal");
	Gnuplot2dDataset s_Topology ("Sources");
	s_Topology.SetStyle (Gnuplot2dDataset::POINTS);
	s_Topology.SetExtra("pointtype 2 pointsize 2");
	Gnuplot2dDataset r_Topology ("PIM-Routers");
	r_Topology.SetStyle (Gnuplot2dDataset::POINTS);
	r_Topology.SetExtra("pointtype 4 pointsize 2");
	Gnuplot2dDataset c_Topology ("PIM-Clients");
	c_Topology.SetExtra("pointtype 6 pointsize 2");
	c_Topology.SetStyle (Gnuplot2dDataset::POINTS);
	Gnuplot2dDataset a_Topology ("Nodes");
	a_Topology.SetExtra("pointtype 1 pointsize 2");
	a_Topology.SetStyle (Gnuplot2dDataset::POINTS);

	/// Video start
	double sourceStart = ceil(totalTime*.10);
	/// Video stop
	double sourceStop = ceil(totalTime*.80);
	/// Client start
	double clientStart = ceil(totalTime*.05);;
	/// Client stop
	double clientStop = ceil(totalTime*.95);

	NodeContainer source;
	source.Create(sizeSource);

	NodeContainer routers;
	routers.Create (sizeRouter);

	NodeContainer clients;
	clients.Create (sizeClient);
	NodeContainer allNodes;
	allNodes.Add(source);
	allNodes.Add(routers);
	allNodes.Add(clients);

	NS_LOG_INFO("Build WiFi level.");
	WifiHelper wifi = WifiHelper::Default();
//	wifi.SetStandard(WIFI_PHY_STANDARD_80211b);
//	wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager"
//			,"DataMode", StringValue("DsssRate11Mbps")
//			,"ControlMode", StringValue ("DsssRate5_5Mbps")
//			,"NonUnicastMode", StringValue ("DsssRate5_5Mbps")
//			);
	wifi.SetStandard(WIFI_PHY_STANDARD_80211g);
	wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager"
			,"DataMode", StringValue("ErpOfdmRate12Mbps")
			,"ControlMode", StringValue("ErpOfdmRate6Mbps")
			,"NonUnicastMode", StringValue("ErpOfdmRate6Mbps")
			);
//	wifi.SetRemoteStationManager ("ns3::AarfWifiManager"
//			,"NonUnicastMode", StringValue("ErpOfdmRate6Mbps")
//			);
	YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
	Ptr<ErrorRateModel> error = CreateObject<YansErrorRateModel> ();
	wifiPhy.SetErrorRateModel("ns3::NistErrorRateModel");
	wifiPhy.SetChannel(wifiChannel.Create());

	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();
	wifiMac.SetType("ns3::AdhocWifiMac");

	/* Source Node to Gateway */
	CsmaHelper csma; //Wired
	csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate (100000000)));
	csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

	NetDeviceContainer routersNetDev = wifi.Install(wifiPhy, wifiMac, routers);
	NetDeviceContainer clientsNetDev = wifi.Install(wifiPhy, wifiMac, clients);;

	NodeContainer s0r0;
	s0r0.Add(source.Get(0));
	s0r0.Add(routers.Get(0));
	NetDeviceContainer ds0dr0 = csma.Install (s0r0);

	// INSTALL INTERNET STACK
	AodvHelper aodvStack;
	MbnAodvHelper mbnStack;
	PimDmHelper pimdmStack;
	IgmpxHelper igmpxStack;
	NS_LOG_INFO ("Enabling Routing.");

	/* ROUTERS */
	Ipv4StaticRoutingHelper staticRouting;
	Ipv4ListRoutingHelper listRouters;
	listRouters.Add (staticRouting, 0);
	listRouters.Add (igmpxStack, 1);
	listRouters.Add (pimdmStack, 11);

	switch (routing)
	{
		case 1:
		{
//			Config::SetDefault ("ns3::aodv::RoutingProtocol::EnableHello", BooleanValue(false));
//			Config::SetDefault ("ns3::aodv::RoutingProtocol::EnableBroadcast", BooleanValue(false));
			Config::SetDefault ("ns3::aodv::RoutingProtocol::HelloInterval", TimeValue(Seconds(2)));
			listRouters.Add (aodvStack, 10);
			break;
		}
		case 2:
		{
//			Config::SetDefault ("ns3::mbn::RoutingProtocol::EnableHello", BooleanValue(false));
//			Config::SetDefault ("ns3::mbn::RoutingProtocol::EnableBroadcast", BooleanValue(false));
			/// Short Timer
			uint32_t short_t = 2, long_t = 6, rule1 = 1, rule2 = 1;
			Config::SetDefault ("ns3::mbn::RoutingProtocol::HelloInterval", TimeValue(Seconds(short_t)));
			Config::SetDefault ("ns3::mbn::RoutingProtocol::ShortInterval", TimeValue(Seconds(short_t)));
			Config::SetDefault ("ns3::mbn::RoutingProtocol::LongInterval", TimeValue(Seconds(long_t)));
			Config::SetDefault ("ns3::mbn::RoutingProtocol::Rule1", BooleanValue(rule1==1));
			Config::SetDefault ("ns3::mbn::RoutingProtocol::Rule2", BooleanValue(rule2==1));
			Config::SetDefault ("ns3::mbn::RoutingProtocol::localWeightFunction", EnumValue(mbn::W_NODE_DEGREE));
			Config::SetDefault ("ns3::mbn::RoutingProtocol::AllowedHelloLoss", UintegerValue(1));
			listRouters.Add (mbnStack, 10);
			break;
		}
		default:
		{
			break;
		}
	}

	InternetStackHelper internetRouters;
	internetRouters.SetRoutingHelper (listRouters);
	internetRouters.Install (routers);

	/* CLIENTS */
	Ipv4ListRoutingHelper listClients;
	listClients.Add (staticRouting, 0);
	listClients.Add (igmpxStack, 1);

	InternetStackHelper internetClients;
	internetClients.SetRoutingHelper (listClients);
	internetClients.Install (clients);

	Ipv4ListRoutingHelper listSource;
	switch (routing)
		{
			case 1:
			{
	//			Config::SetDefault ("ns3::aodv::RoutingProtocol::EnableHello", BooleanValue(false));
				Config::SetDefault ("ns3::aodv::RoutingProtocol::EnableBroadcast", BooleanValue(false));
				Config::SetDefault ("ns3::aodv::RoutingProtocol::HelloInterval", TimeValue(Seconds(2)));
				listSource.Add (aodvStack, 10);
				break;
			}
			case 2:
			{
	//			Config::SetDefault ("ns3::mbn::RoutingProtocol::EnableHello", BooleanValue(false));
				Config::SetDefault ("ns3::mbn::RoutingProtocol::EnableBroadcast", BooleanValue(false));
				/// Short Timer
				uint32_t short_t = 2, long_t = 6, rule1 = 1, rule2 = 1;
				Config::SetDefault ("ns3::mbn::RoutingProtocol::HelloInterval", TimeValue(Seconds(short_t)));
				Config::SetDefault ("ns3::mbn::RoutingProtocol::ShortInterval", TimeValue(Seconds(short_t)));
				Config::SetDefault ("ns3::mbn::RoutingProtocol::LongInterval", TimeValue(Seconds(long_t)));
				Config::SetDefault ("ns3::mbn::RoutingProtocol::Rule1", BooleanValue(rule1==1));
				Config::SetDefault ("ns3::mbn::RoutingProtocol::Rule2", BooleanValue(rule2==1));
				Config::SetDefault ("ns3::mbn::RoutingProtocol::localWeightFunction", EnumValue(mbn::W_NODE_DEGREE));
				Config::SetDefault ("ns3::mbn::RoutingProtocol::AllowedHelloLoss", UintegerValue(1));
				listSource.Add (mbnStack, 10);
				break;
			}
			default:
			{
				break;
			}
		}
	listSource.Add (staticRouting, 11);

	InternetStackHelper internetSource;
	internetSource.SetRoutingHelper (listSource);
	internetSource.Install (source);

	// Later, we add IP addresses.
	NS_LOG_INFO ("Assign IP Addresses.");

	Ipv4AddressHelper address;
	address.SetBase("10.0.0.0", "255.0.0.0");

	Ipv4InterfaceContainer ipRouter = address.Assign (routersNetDev);
	Ipv4InterfaceContainer ipClient = address.Assign (clientsNetDev);

	address.SetBase ("11.0.0.0", "255.0.0.0");
	Ipv4InterfaceContainer ipSource = address.Assign (ds0dr0);
	Ipv4Address multicastSource ("11.0.0.1");

	NS_LOG_INFO ("Configure multicasting.");

	Ipv4Address multicastGroup ("225.1.2.4");

	/* 1) Configure a (static) multicast route on ASNGW (multicastRouter) */
	Ptr<Node> multicastRouter = routers.Get (0); // The node in question
	Ptr<NetDevice> inputIf = routersNetDev.Get (0); // The input NetDevice

	Ipv4StaticRoutingHelper multicast;
	multicast.AddMulticastRoute (multicastRouter, multicastSource, multicastGroup, inputIf, routersNetDev.Get(0));

	/* 2) Set up a default multicast route on the sender n0 */
	Ptr<Node> sender = source.Get (0);
//	Ptr<NetDevice> senderIf = sourceNetDev.Get (0);
	Ptr<NetDevice> senderIf = ds0dr0.Get (0);
	multicast.SetDefaultMulticastRoute (sender, senderIf);

	std::stringstream ss;
	/* source, group, interface*/
	//ROUTERS
	ss<< multicastSource<< "," << multicastGroup;
	for (uint32_t n = 0;  n < routers.GetN() ; n++){
		std::stringstream command;//create a stringstream
		command<< "NodeList/" << routers.Get(n)->GetId() << "/$ns3::pimdm::MulticastRoutingProtocol/RegisterSG";
		Config::Set(command.str(), StringValue(ss.str()));
		command.str("");
		command<< "NodeList/" << routers.Get(n)->GetId() << "/$ns3::igmpx::IGMPXRoutingProtocol/PeerRole";
		Config::Set(command.str(), EnumValue(igmpx::ROUTER));
	}
	// CLIENTS
	ss.str("");
	ss<< multicastSource<< "," << multicastGroup << "," << "1";
	for (uint32_t n = 0;  n < clients.GetN() ; n++){//Clients are RN nodes
		std::stringstream command;
		command<< "/NodeList/" << clients.Get(n)->GetId()<<"/$ns3::igmpx::IGMPXRoutingProtocol/PeerRole";
		Config::Set(command.str(), EnumValue(igmpx::CLIENT));
		command.str("");
		command<< "/NodeList/" << clients.Get(n)->GetId()<<"/$ns3::igmpx::IGMPXRoutingProtocol/RegisterAsMember";
		Config::Set(command.str(), StringValue(ss.str()));
	}

//	Config::Connect ("/NodeList/*/$ns3::igmpx::IGMPXRoutingProtocol/RxIgmpxControl",MakeCallback (&GenericPacketTrace));
//	Config::Connect ("/NodeList/*/$ns3::igmpx::IGMPXRoutingProtocol/TxIgmpxControl",MakeCallback (&GenericPacketTrace));
//	Config::Connect ("/NodeList/*/$ns3::pimdm::MulticastRoutingProtocol/TxPimData",MakeCallback (&GenericPacketTrace));
//	Config::Connect ("/NodeList/*/$ns3::pimdm::MulticastRoutingProtocol/RxPimData",MakeCallback (&GenericPacketTrace));
//	Config::Connect ("/NodeList/*/$ns3::pimdm::MulticastRoutingProtocol/TxPimControl", MakeCallback (&GenericPacketTrace));
//	Config::Connect ("/NodeList/*/$ns3::pimdm::MulticastRoutingProtocol/RxPimControl", MakeCallback (&GenericPacketTrace));
//	Config::Connect ("/NodeList/*/$ns3::ArpL3Protocol/Drop", MakeCallback (&GenericPacketTrace));


	NS_LOG_INFO ("Create Source");
	InetSocketAddress dst = InetSocketAddress (multicastGroup, PUSH_PORT);
	Config::SetDefault ("ns3::UdpSocket::IpMulticastTtl", UintegerValue (1));
	VideoHelper video = VideoHelper ("ns3::UdpSocketFactory", dst);
	video.SetAttribute ("DataRate", DataRateValue (DataRate (stream)));
	video.SetAttribute ("PacketSize", UintegerValue (1200));
	video.SetAttribute ("PeerType", EnumValue (SOURCE));
	video.SetAttribute ("Local", AddressValue (multicastSource));
	video.SetAttribute ("PeerPolicy", EnumValue (PS_RANDOM));
	video.SetAttribute ("ChunkPolicy", EnumValue (CS_LATEST));

	ApplicationContainer apps = video.Install (source.Get (0));
	apps.Start (Seconds (sourceStart));
	apps.Stop (Seconds (sourceStop));

	Ptr<ListPositionAllocator> positionAllocS = CreateObject<ListPositionAllocator> ();
	positionAllocS->Add(Vector(-10.0, -10.0, 0.0));// Source
	MobilityHelper mobilityS;
	mobilityS.SetPositionAllocator(positionAllocS);
	mobilityS.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobilityS.Install(source);

	for(uint32_t n = 0; n < clients.GetN() ; n++){
		InetSocketAddress dstC = InetSocketAddress (multicastGroup, PUSH_PORT);
		Config::SetDefault ("ns3::UdpSocket::IpMulticastTtl", UintegerValue (1));
		VideoHelper videoC = VideoHelper ("ns3::UdpSocketFactory", dstC);
		videoC.SetAttribute ("PeerType", EnumValue (PEER));
		videoC.SetAttribute ("LocalPort", UintegerValue (PUSH_PORT));
		videoC.SetAttribute ("Local", AddressValue(ipClient.GetAddress(n)));
		videoC.SetAttribute ("PeerPolicy", EnumValue (PS_RANDOM));
		videoC.SetAttribute ("ChunkPolicy", EnumValue (CS_LATEST));

		ApplicationContainer appC = videoC.Install (clients.Get(n));
		appC.Start (Seconds (clientStart));
		appC.Stop (Seconds (clientStop));
	}

	MobilityHelper mobilityR;
	mobilityR.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobilityR.SetPositionAllocator("ns3::RandomBoxPositionAllocator",
					"X",RandomVariableValue(UniformVariable(0,xmax)),
					"Y",RandomVariableValue(UniformVariable(0,ymax)),
					"Z",RandomVariableValue(ConstantVariable(0)));
//	MobilityHelper mobilityR;
//	mobilityR.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
//	mobilityR.SetPositionAllocator ("ns3::GridPositionAllocator",
//	  "MinX", DoubleValue (0.0),
//	  "MinY", DoubleValue (0.0),
//	  "DeltaX", DoubleValue (range),
//	  "DeltaY", DoubleValue (range),
//	  "GridWidth", UintegerValue (cols),
//	  "LayoutType", StringValue ("RowFirst"));
	mobilityR.Install(routers);

	MobilityHelper mobilityC;
	mobilityC.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobilityC.SetPositionAllocator("ns3::RandomBoxPositionAllocator",
		"X",RandomVariableValue(UniformVariable(0,xmax)),
		"Y",RandomVariableValue(UniformVariable(0,ymax)),
		"Z",RandomVariableValue(ConstantVariable(0)));

	mobilityC.Install(clients);

	for(uint32_t i = 0; i < allNodes.GetN(); i++){
		  Ptr<MobilityModel> mobility = allNodes.Get(i)->GetObject<MobilityModel> ();
	      Vector pos = mobility->GetPosition (); // Get position
	      if (i<sizeSource)
	    	  s_Topology.Add (pos.x,pos.y);
	      else if (i < (sizeSource+sizeRouter))
	    	  r_Topology.Add (pos.x,pos.y);
	      else
	    	  c_Topology.Add (pos.x,pos.y);
	      a_Topology.Add (pos.x,pos.y);
	      NS_LOG_INFO("Position Node ["<<i<<"] = ("<< pos.x << ", " << pos.y<<", "<<pos.z<<")");
	}
	g_Topology.AddDataset (a_Topology);
	g_Topology.AddDataset (s_Topology);
	g_Topology.AddDataset (r_Topology);
	g_Topology.AddDataset (c_Topology);
	g_Topology.GenerateOutput (std::cout);
	std::cout << "Starting simulation for " << totalTime << " s ...\n";
	Simulator::Stop(Seconds(totalTime));
	Simulator::Run();
	NS_LOG_INFO ("Done.");

	Simulator::Destroy();
	return 0;
}

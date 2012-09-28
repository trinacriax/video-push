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
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("PushExample");

/// Verbose
uint32_t verbose = 0;
uint32_t arpSent = 0;
uint32_t videoBroadcast = 0;

std::vector<uint32_t> msgVideo;
uint32_t msgTxVideoT = 0;
uint32_t msgTxVideoP = 0;

std::vector<uint32_t> msgTxVideoControl;
uint32_t msgTxControlT = 0;
uint32_t msgControlP = 0;

std::vector<uint32_t> msgTxControlPull;
uint32_t msgTxControlL = 0;
uint32_t msgTxControlLP = 0;

std::vector<uint32_t> msgRxControlPull;
uint32_t msgRxControlL = 0;
uint32_t msgRxControlLP = 0;

std::vector<uint32_t> msgTxDataPull;
uint32_t msgTxDataL = 0;
uint32_t msgTxDataLP = 0;

std::vector<uint32_t> msgRxDataPull;
uint32_t msgRxDataL = 0;
uint32_t msgRxDataLP = 0;

std::vector<uint32_t> neighbors;
uint32_t neighborsT = 0;
uint32_t phyTxBegin = 0;
uint32_t phyTxBeginP = 0;
uint32_t phyTxEnd = 0;
uint32_t phyTxDrop = 0;
uint32_t macTx = 0;
uint32_t macTxP = 0;
uint32_t arpTx = 0;
uint32_t arpTxP = 0;

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

void
GenericPacketTrace (std::string context, Ptr<const Packet> p)
{
	struct mycontext mc = GetContextInfo (context);
//	controls
//	std::cout << Simulator::Now().GetSeconds() << " "<< mc.id << " <<Trace="<< mc.callback << ">> " << p->GetSize() << " Pid="<< p->GetUid() << " Psize="<<p->GetSize()<< std::endl;
	if ( mc.callback.compare("PhyTxBegin")==0 )
	{
		NS_LOG_INFO(mc.callback<<" ID="<<p->GetUid());
		phyTxBegin += p->GetSize();
		phyTxBeginP++;
	}
	else if ( mc.callback.compare("PhyTxEnd")==0 )
	{
		NS_LOG_INFO(mc.callback<<" ID="<<p->GetUid());
		phyTxEnd += p->GetSize();
	}
	else if ( mc.callback.compare("PhyTxDrop")==0)
	{
		NS_LOG_INFO(mc.callback<<" ID="<<p->GetUid());
		phyTxDrop += p->GetSize();
	}
	else if ( mc.callback.compare("MacTx")==0)
	{
		NS_LOG_INFO(mc.callback<<" ID="<<p->GetUid());
		macTx += p->GetSize();
		macTxP++;
	}
	else if ( mc.callback.compare("TxArp")==0)
	{
		NS_LOG_INFO(mc.callback<<" ID="<<p->GetUid());
		arpTx += p->GetSize();
		arpTxP++;
	}
}

void StatisticPhy ()
{
	std::cout << "PhyMessages\t" << Simulator::Now().GetSeconds()<< "\t" <<phyTxBegin<<"\t"<< phyTxBeginP<< "\n";
	phyTxBegin = phyTxBeginP = 0;
	phyTxEnd = 0;
	phyTxDrop = 0;
}

void StatisticMac ()
{
	std::cout << "MacMessages\t" << Simulator::Now().GetSeconds()<< "\t" <<macTx<<"\t"<< macTxP<< "\n";
	macTx = macTxP = 0;
}

void StatisticArp()
{
	std::cout << "ArpMessages\t" << Simulator::Now().GetSeconds()<< "\t" <<arpTx<<"\t"<< arpTxP<< "\n";
	arpTx = arpTxP = 0;
}

void
VideoTrafficSent (std::string context, Ptr<const Packet> p)
{
	struct mycontext mc = GetContextInfo (context);
	msgVideo[mc.id] += p->GetSize();
	msgTxVideoT += p->GetSize();
}

void StatisticVideo ()
{
	for (uint32_t i = 0; i < msgVideo.size(); i++)
	{
		std::cout << "VideoDataMessage Node\t" << i << "\t" << Simulator::Now().GetSeconds()<< "\t" << msgVideo[i] << "\n";
		msgVideo[i] = 0;
	}
	std::cout << "VideoDataMessages\t" << Simulator::Now().GetSeconds()<< "\t" << msgTxVideoT<< "\n";
	msgTxVideoT = 0;
}

void
TxDataPull (std::string context, Ptr<const Packet> p)
{
	struct mycontext mc = GetContextInfo (context);
	NS_LOG_INFO(mc.id<<" ID="<<p->GetUid());
	msgTxDataPull[mc.id] += p->GetSize();
	msgTxDataLP += p->GetSize();
	msgTxDataL++;
}

void StatisticTxDataPull ()
{
	for (uint32_t i = 0; i < msgTxDataPull.size(); i++)
	{
		std::cout << "TxDataPull Node\t" << i << "\t" << Simulator::Now().GetSeconds()<< "\t" << msgTxDataPull[i] << "\n";
		msgTxDataPull[i] = 0;
	}
	std::cout << "TxDataPulls\t" << Simulator::Now().GetSeconds()<< "\t" << msgTxDataLP<< "\t"<< msgTxDataL <<  "\n";
	msgTxDataLP = msgTxDataL = 0;
}

void
RxDataPull (std::string context, Ptr<const Packet> p, const Address & address)
{
	struct mycontext mc = GetContextInfo (context);
	NS_LOG_INFO(mc.id<<" ID="<<p->GetUid());
	msgRxDataPull[mc.id] += p->GetSize();
	msgRxDataLP += p->GetSize();
	msgRxDataL++;
}

void StatisticRxDataPull ()
{
	for (uint32_t i = 0; i < msgRxDataPull.size(); i++)
	{
		std::cout << "RxDataPull Node\t" << i << "\t" << Simulator::Now().GetSeconds()<< "\t" << msgRxDataPull[i] << "\n";
		msgRxDataPull[i] = 0;
	}
	std::cout << "RxDataPulls\t" << Simulator::Now().GetSeconds()<< "\t" << msgRxDataLP << "\t"<< msgRxDataL<< "\n";
	msgRxDataLP = msgRxDataL = 0;
}

void
VideoControlSent (std::string context, Ptr<const Packet> p)
{
	struct mycontext mc = GetContextInfo (context);
	msgTxVideoControl[mc.id] += (p->GetSize() + 20 + 8 );
	msgTxControlT += (p->GetSize() + 20 + 8 );
	msgControlP++;
}

void StatisticControl ()
{
	for (uint32_t i = 0; i < msgTxVideoControl.size(); i++)
	{
		std::cout << "VideoControlMessage Node\t" << i << "\t" << Simulator::Now().GetSeconds()<< "\t" << msgTxVideoControl[i] << "\n";
		msgTxVideoControl[i] = 0;
	}
	std::cout << "VideoControlMessages\t" << Simulator::Now().GetSeconds()<< "\t" << msgTxControlT<< "\t" << msgControlP << "\n";
	msgTxControlT = msgControlP = 0;
}

void
TxControlPull (std::string context, Ptr<const Packet> p)
{
	struct mycontext mc = GetContextInfo (context);
	NS_LOG_INFO(mc.id<<" ID="<<p->GetUid());
	msgTxControlPull[mc.id] += (p->GetSize() + 20 + 8 );
	msgTxControlL += (p->GetSize() + 20 + 8 );
	msgTxControlLP++;
}

void StatisticTxControlPull ()
{
	for (uint32_t i = 0; i < msgTxControlPull.size(); i++)
	{
		std::cout << "TxPullMessage Node\t" << i << "\t" << Simulator::Now().GetSeconds()<< "\t" << msgTxControlPull[i] << "\n";
		msgTxControlPull[i] = 0;
	}
	std::cout << "TxPullMessages\t" << Simulator::Now().GetSeconds()<< "\t" << msgTxControlL<< "\t" << msgTxControlLP << "\n";
	msgTxControlL = msgTxControlLP = 0;
}

void
RxControlPull (std::string context, Ptr<const Packet> p, const Address & address)
{
	struct mycontext mc = GetContextInfo (context);
	NS_LOG_INFO(mc.id<<" ID="<<p->GetUid());
	msgRxControlPull[mc.id] += (p->GetSize() + 20 + 8 );
	msgRxControlL += (p->GetSize() + 20 + 8 );
	msgRxControlLP++;
}

void StatisticRxControlPull ()
{
	for (uint32_t i = 0; i < msgRxControlPull.size(); i++)
	{
		std::cout << "RxPullMessage Node\t" << i << "\t" << Simulator::Now().GetSeconds()<< "\t" << msgRxControlPull[i] << "\n";
		msgRxControlPull[i] = 0;
	}
	std::cout << "RxPullMessages\t" << Simulator::Now().GetSeconds()<< "\t" << msgRxControlL<< "\t" << msgRxControlLP << "\n";
	msgRxControlL = msgRxControlLP = 0;
}

void
Neighbors (std::string context, const uint32_t n)
{
	struct mycontext mc = GetContextInfo (context);
	neighbors[mc.id] = n;
}

void StatisticNeighbors ()
{
	for (uint32_t i = 0; i < neighbors.size(); i++)
	{
		std::cout << "Neighbor Node\t" << i << "\t" << Simulator::Now().GetSeconds()<< "\t" << neighbors[i] << "\n";
		neighborsT += neighbors[i];
	}
	std::cout << "Neighbors\t" << Simulator::Now().GetSeconds()<< "\t" << (neighborsT/(1.0*neighbors.size())) << "\n";
	neighborsT = 0;
}

void
ResetValues ()
{
	StatisticControl ();
	StatisticTxControlPull ();
	StatisticRxControlPull ();
	StatisticVideo ();
	StatisticTxDataPull ();
	StatisticRxDataPull ();
	StatisticNeighbors ();
	StatisticPhy ();
	StatisticMac ();
	StatisticArp ();
	Simulator::Schedule (Seconds(1), &ResetValues);
}

int main(int argc, char **argv)
{
	/// Number of source nodes
	uint32_t sizeSource = 1;
	/// Number of router nodes
	uint32_t sizeRouter = 0;
	/// Number of client nodes
	uint32_t sizeClient = 5;
	/// Simulation time in seconds
	double totalTime = 160;
	// Simulation run
	uint32_t run = 1;
	// Simulation seed
	uint32_t seed = 3945244811;
	/// Grid xmax
	double xmax = 80;
	/// Grid ymax
	double ymax = 80;
	/// Radius range
	double radius = 120;
	// Streaming rate
	uint64_t stream = 1000000;
	// Packet Size
	uint64_t packetsize = 1500;
	// Period between pull
	double pulltime = 12;//in ms
	// max number of pull to retrieve a chunk
	uint32_t pullmax = 1;
	// pull ratio for activation
	double pullratiomin = .70;
	// pull ratio for de-activation
	double pullratiomax = .90;
	// max number of pull reply
	uint32_t pullmreply = 1;
	// Time in seconds between hellos
	uint32_t helloactive = 0;
	// Time in seconds between hellos
	double hellotime = 10;
	// max number of hello loss before removing a neighbor
	uint32_t helloloss = 1;
	// Activate pull as recovery mechanism
	bool pullactive = false;
	// Unicast routing protocol to use
	uint32_t routing = 0;
	// reference loss
	double log_r = 52.059796126;
	// loss exponent
	double log_n = 2.6;
	// Tx power start
	double TxStart = 20.0;
	// Tx power end
	double TxEnd = 20.0;
	// Tx power levels
	uint32_t TxLevels = 1;
	// Tx gain
	double TxGain = 1.0;
	// Rx gain
	double RxGain = 1.0;
	// Energy detection threshold
	double EnergyDet= -95.0;
	// CCA mode 1
	double CCAMode1 = -62.0;

	double nak_d1 = 80;
	double nak_d2 = 200;
	double nak_m0 = 1.0;
	double nak_m1 = 1.0;
	double nak_m2 = 1.0;

	double selectionWeight = 0.0;

	uint32_t flag = 0;

	CommandLine cmd;
	cmd.AddValue ("sizeSource", "Number of nodes.", sizeSource);
	cmd.AddValue ("sizeRouter", "Number of nodes.", sizeRouter);
	cmd.AddValue ("sizeClient", "Number of nodes.", sizeClient);
	cmd.AddValue ("time", "Simulation time, s.", totalTime);
	cmd.AddValue ("run", "Run Identifier", run);
	cmd.AddValue ("seed", "Seed ", seed);
	cmd.AddValue ("nakd1", "Nakagami distance 1", nak_d1);
	cmd.AddValue ("nakd2", "Nakagami distance 2", nak_d2);
	cmd.AddValue ("nakm0", "Nakagami exponent 0", nak_m0);
	cmd.AddValue ("nakm1", "Nakagami exponent 1", nak_m1);
	cmd.AddValue ("nakm2", "Nakagami exponent 2", nak_m2);
	cmd.AddValue ("logn", "Reference path loss dB.", log_r);
	cmd.AddValue ("logr", "Path loss exponent.", log_n);
	cmd.AddValue ("TxStart", "Transmission power start dBm.", TxStart);
	cmd.AddValue ("TxEnd", "Transmission power end dBm.", TxEnd);
	cmd.AddValue ("TxLevels", "Transmission power levels.", TxLevels);
	cmd.AddValue ("TxGain", "Transmission power gain dBm.", TxGain);
	cmd.AddValue ("RxGain", "Receiver power gain dBm.", RxGain);
	cmd.AddValue ("EnergyDet", "Energy detection threshold dBm.", EnergyDet);
	cmd.AddValue ("CCAMode1", "CCA mode 1 threshold dBm.", CCAMode1);
	cmd.AddValue ("xmax", "Grid X max", xmax);
	cmd.AddValue ("ymax", "Grid Y max", ymax);
	cmd.AddValue ("radius", "Radius range", radius);
	cmd.AddValue ("routing", "Unicast Routing Protocol (1 - AODV)", routing);
	cmd.AddValue ("stream", "Streaming ", stream);
	cmd.AddValue ("packetsize", "Packet Size", packetsize);
	cmd.AddValue ("hellotime", "Hello time", hellotime);
	cmd.AddValue ("helloloss", "Max number of hello loss to be removed from neighborhood", helloloss);
	cmd.AddValue ("helloactive", "Hello activation", helloactive);
	cmd.AddValue ("pullactive", "Pull activation allowed", pullactive);
	cmd.AddValue ("pullmax", "Max number of pull allowed per chunk", pullmax);
	cmd.AddValue ("pullratiomin", "Ratio to activate pull", pullratiomin);
	cmd.AddValue ("pullratiomax", "Ratio to de-activate pull", pullratiomax);
	cmd.AddValue ("pulltime", "Time between pull in ms (e.g., 100ms = 0.100s)", pulltime);
	cmd.AddValue ("pullmreply", "Max pull replies", pullmreply);
	cmd.AddValue ("selectionw", "Selection Weight [0-1]", selectionWeight);
	cmd.AddValue ("v", "Verbose", verbose);
	cmd.AddValue ("ff", "flag", flag);
	cmd.Parse(argc, argv);

	if (pullactive)
		NS_ASSERT (helloactive);

	SeedManager::SetRun (run);
	SeedManager::SetSeed (seed);
	Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue("2100"));
	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("2100"));
	Config::SetDefault ("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue(log_r));
	Config::SetDefault ("ns3::LogDistancePropagationLossModel::Exponent", DoubleValue(log_n));
	Config::SetDefault ("ns3::NakagamiPropagationLossModel::Distance1", DoubleValue(nak_d1));
	Config::SetDefault ("ns3::NakagamiPropagationLossModel::Distance2", DoubleValue(nak_d2));
	Config::SetDefault ("ns3::NakagamiPropagationLossModel::m0", DoubleValue(nak_m0));
	Config::SetDefault ("ns3::NakagamiPropagationLossModel::m1", DoubleValue(nak_m1));
	Config::SetDefault ("ns3::NakagamiPropagationLossModel::m2", DoubleValue(nak_m2));
	Config::SetDefault ("ns3::VideoPushApplication::DataRate", DataRateValue(stream));
	Config::SetDefault ("ns3::VideoPushApplication::PacketSize", UintegerValue(packetsize));
	Config::SetDefault ("ns3::VideoPushApplication::PullActive", BooleanValue(pullactive));
	Config::SetDefault ("ns3::VideoPushApplication::PullTime", TimeValue(Time::FromDouble(pulltime,Time::MS)));
	Config::SetDefault ("ns3::VideoPushApplication::PullMax", UintegerValue(pullmax));
	Config::SetDefault ("ns3::VideoPushApplication::PullRatioMin", DoubleValue(pullratiomin));
	Config::SetDefault ("ns3::VideoPushApplication::PullRatioMax", DoubleValue(pullratiomax));
	Config::SetDefault ("ns3::VideoPushApplication::HelloActive", UintegerValue(helloactive));
	Config::SetDefault ("ns3::VideoPushApplication::HelloTime", TimeValue(Time::FromDouble(hellotime,Time::S)));
	Config::SetDefault ("ns3::VideoPushApplication::HelloLoss", UintegerValue(helloloss));
	Config::SetDefault ("ns3::VideoPushApplication::Source", Ipv4AddressValue(Ipv4Address("10.0.0.1")));
	Config::SetDefault ("ns3::VideoPushApplication::Flag", UintegerValue(flag));
	Config::SetDefault ("ns3::VideoPushApplication::SelectionWeight", DoubleValue(selectionWeight));
	Config::SetDefault ("ns3::VideoPushApplication::MaxPullReply", UintegerValue(pullmreply));
	Config::SetDefault ("ns3::Ipv4L3Protocol::DefaultTtl", UintegerValue (1)); //avoid to forward broadcast packets
	Config::SetDefault ("ns3::Ipv4::IpForward", BooleanValue (false));
	Config::SetDefault ("ns3::YansWifiPhy::TxGain",DoubleValue(TxGain));
	Config::SetDefault ("ns3::YansWifiPhy::RxGain",DoubleValue(RxGain));
	Config::SetDefault ("ns3::YansWifiPhy::TxPowerStart",DoubleValue(TxStart));
	Config::SetDefault ("ns3::YansWifiPhy::TxPowerEnd",DoubleValue(TxEnd));
	Config::SetDefault ("ns3::YansWifiPhy::TxPowerLevels",UintegerValue(TxLevels));
	Config::SetDefault ("ns3::YansWifiPhy::EnergyDetectionThreshold",DoubleValue(EnergyDet));///17.3.10.1 Receiver minimum input sensitivity
	Config::SetDefault ("ns3::YansWifiPhy::CcaMode1Threshold",DoubleValue(CCAMode1));///17.3.10.5 CCA sensitivity

	if(verbose==1){
		LogComponentEnable("PushExample", LogLevel (LOG_LEVEL_ALL | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
		LogComponentEnable("VideoPushApplication", LogLevel (LOG_LEVEL_ALL | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("ChunkBuffer", LogLevel (LOG_LEVEL_ALL | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("AodvRoutingTable", LogLevel (LOG_LEVEL_ALL | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("AodvNeighbors", LogLevel (LOG_LEVEL_ALL | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("AodvRoutingProtocol", LogLevel (LOG_LEVEL_ALL | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("UdpSocketImpl", LogLevel (LOG_LEVEL_ALL | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("Ipv4L3Protocol", LogLevel (LOG_LEVEL_ALL | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("Socket", LogLevel (LOG_LEVEL_ALL | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("ArpL3Protocol", LogLevel (LOG_LEVEL_ALL | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("MacLow", LogLevel (LOG_LEVEL_ALL | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("YansWifiChannel",  LogLevel (LOG_LEVEL_ALL | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("YansWifiPhy",  LogLevel (LOG_LEVEL_ALL | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("DcfManager",  LogLevel (LOG_LEVEL_ALL | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("DcaTxop",  LogLevel (LOG_LEVEL_ALL | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("Node", LogLevel (LOG_LEVEL_ALL | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//		LogComponentEnable("DefaultSimulatorImpl", LogLevel (LOG_LEVEL_ALL | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
	}

	for (int c = 0; c < argc; c++)
		std::cout << argv[c] << " ";
	std::cout << "\n";

	std::cout << " --sizeSource=" << sizeSource
			<< " --sizeRouter=" << sizeRouter
			<< " --sizeClient=" << sizeClient
			<< " --time=" << totalTime
			<< " --run=" << run
			<< " --seed=" << seed
			<< " --nakd1=" << nak_d1
			<< " --nakd2=" << nak_d2
			<< " --nakm0=" << nak_m0
			<< " --nakm1=" << nak_m1
			<< " --nakm2=" << nak_m2
			<< " --logr=" << log_r
			<< " --logn=" << log_n
			<< " --TxStart=" << TxStart
			<< " --TxEnd=" << TxEnd
			<< " --TxLevels=" << TxLevels
			<< " --TxGain=" << TxGain
			<< " --RxGain=" << RxGain
			<< " --EnergyDet=" << EnergyDet
			<< " --CCAMode1=" << CCAMode1
			<< " --xmax=" << xmax
			<< " --ymax=" << ymax
			<< " --radius=" << radius
			<< " --routing=" << routing
			<< " --stream=" << stream
			<< " --packetsize=" << packetsize
			<< " --hellotime=" << hellotime
			<< " --helloloss=" << helloloss
			<< " --helloactive=" << helloactive
			<< " --pullactive=" << pullactive
			<< " --pullratiomin=" << pullratiomin
			<< " --pullratiomax=" << pullratiomax
			<< " --pullmax=" << pullmax
			<< " --pulltime=" << pulltime
			<< " --selectionw=" << selectionWeight
			<< " --v=" << verbose
			<< "\n";

	/// Video start
	double sourceStart = 30;
	/// Video stop
	double sourceStop = totalTime - 10;
	/// Client start
	double clientStart = 1;
	/// Client stop
	double clientStop = totalTime - 10;

	NodeContainer fake;
	fake.Create(1);
	NodeContainer source;
	source.Create (sizeSource);
	NodeContainer clients;
	clients.Create(sizeClient);
	NodeContainer all;
	all.Add (source);
	all.Add (clients);


	for (int k=0; k<sizeSource+sizeRouter+sizeClient; k++)
	{
	    msgTxVideoControl.push_back(0);
	    msgTxControlPull.push_back(0);
	    msgRxControlPull.push_back(0);
	    msgVideo.push_back(0);
	    msgTxDataPull.push_back(0);
	    msgRxDataPull.push_back(0);
	    neighbors.push_back(0);
	}

	NS_LOG_INFO ("Create WiFi channel");

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
			,"ControlMode", StringValue("ErpOfdmRate18Mbps")
			,"NonUnicastMode", StringValue("ErpOfdmRate18Mbps")
			);

//	wifi.SetRemoteStationManager ("ns3::AarfWifiManager"
//			,"NonUnicastMode", StringValue("ErpOfdmRate6Mbps")
//			);

	YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
	wifiChannel.AddPropagationLoss("ns3::NakagamiPropagationLossModel");

	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
	wifiPhy.SetChannel(wifiChannel.Create());

	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();
	wifiMac.SetType("ns3::AdhocWifiMac");

	NetDeviceContainer device;
	device.Add (wifi.Install (wifiPhy, wifiMac, source));
	device.Add (wifi.Install (wifiPhy, wifiMac, clients));

	InternetStackHelper stack;
	stack.Install(source);
	stack.Install(clients);
	Ipv4AddressHelper address;
	address.SetBase("10.0.0.0", "255.0.0.0");

	Ipv4InterfaceContainer interfaces;
	interfaces = address.Assign(device);

	NS_LOG_INFO ("Statically populate ARP cache!\n");
	Ptr<ArpCache> arp;
	for (uint32_t n = 0;  n < all.GetN() ; n++)
	{
		if(arp == NULL){
			arp = CreateObject<ArpCache> ();
			arp->SetAliveTimeout (Seconds (3600 * 24 * 365));
		}
		NS_LOG_INFO ("Populate node \n");
		Ptr<Ipv4L3Protocol> ip = all.Get(n)->GetObject<Ipv4L3Protocol> ();
		NS_ASSERT (ip !=0);
		ObjectVectorValue interfaces;
		ip->GetAttribute ("InterfaceList", interfaces);
		for(ObjectVectorValue::Iterator j = interfaces.Begin (); j != interfaces.End (); j++)
		{
			Ptr<Ipv4Interface> ipIface = j->second->GetObject<Ipv4Interface> ();
			NS_ASSERT (ipIface != 0);
			Ptr<NetDevice> device = ipIface->GetDevice ();
			NS_ASSERT (device != 0);
			Address hwAddr = Mac48Address::ConvertFrom (device->GetAddress ());
			NS_LOG_INFO ("Detected address " << hwAddr);
			for(uint32_t k = 0; k < ipIface->GetNAddresses (); k++)
			{
				Ipv4Address ipAddr = ipIface->GetAddress (k).GetLocal();
				if(ipAddr == Ipv4Address::GetLoopback ())
				  continue;
				ArpCache::Entry * entry = arp->Add (ipAddr);
				entry->MarkWaitReply (0);
				entry->MarkAlive (hwAddr);
			}
		}
	}
	for (uint32_t n = 0;  n < all.GetN() ; n++){
		NS_LOG_INFO ("Populate node "<<all.Get(n)->GetId());
		Ptr<Ipv4L3Protocol> ip = all.Get(n)->GetObject<Ipv4L3Protocol> ();
		NS_ASSERT (ip !=0);
		ObjectVectorValue interfaces;
		ip->GetAttribute ("InterfaceList", interfaces);
		for(ObjectVectorValue::Iterator j = interfaces.Begin (); j !=interfaces.End (); j++)
		{
			Ptr<Ipv4Interface> ipIface = j->second->GetObject<Ipv4Interface> ();
			ipIface->SetAttribute ("ArpCache", PointerValue (arp));
		}
	}

	Ptr<ListPositionAllocator> positionAllocS = CreateObject<ListPositionAllocator> ();
	positionAllocS->Add(Vector(0.0, 0.0, 0.0));// Source
	MobilityHelper mobilityS;
	mobilityS.SetPositionAllocator(positionAllocS);
	mobilityS.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobilityS.Install(source);

	Ptr<UniformRandomVariable> rhos = CreateObject<UniformRandomVariable> ();
	rhos->SetAttribute ("Min", DoubleValue (1));
	rhos->SetAttribute ("Max", DoubleValue (radius));

	MobilityHelper mobility;
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator",
					"X",DoubleValue(0.0),
					"Y",DoubleValue(0.0),
					"Rho", PointerValue (rhos)
					);
	mobility.Install(clients);

	//Source streaming rate
	Ipv4Address subnet ("10.255.255.255");
	NS_LOG_INFO ("Application: create source");
	for(uint32_t s = 0; s < source.GetN() ; s++){
		InetSocketAddress dst = InetSocketAddress (subnet, PUSH_PORT);
		Config::SetDefault ("ns3::UdpSocket::IpMulticastTtl", UintegerValue (1));
		VideoHelper video = VideoHelper ("ns3::UdpSocketFactory", dst);
		video.SetAttribute ("DataRate", DataRateValue (DataRate (stream)));
		video.SetAttribute ("PacketSize", UintegerValue (packetsize));
		video.SetAttribute ("PeerType", EnumValue (SOURCE));
		video.SetAttribute ("Local", AddressValue (interfaces.GetAddress(s)));
		video.SetAttribute ("PeerPolicy", EnumValue (PS_RANDOM));
		video.SetAttribute ("ChunkPolicy", EnumValue (CS_NEW_CHUNK));

		ApplicationContainer apps = video.Install (source.Get (s));
		apps.Start (Seconds (sourceStart));
		apps.Stop (Seconds (sourceStop));
	}

	for(uint32_t n = 0; n < clients.GetN() ; n++){
		InetSocketAddress dstC = InetSocketAddress (subnet, PUSH_PORT);
		Config::SetDefault ("ns3::UdpSocket::IpMulticastTtl", UintegerValue (1));
		VideoHelper videoC = VideoHelper ("ns3::UdpSocketFactory", dstC);
		videoC.SetAttribute ("DataRate", DataRateValue (DataRate (stream)));
		videoC.SetAttribute ("PacketSize", UintegerValue (packetsize));
		videoC.SetAttribute ("PeerType", EnumValue (PEER));
		videoC.SetAttribute ("LocalPort", UintegerValue (PUSH_PORT));
		videoC.SetAttribute ("Local", AddressValue(interfaces.GetAddress(source.GetN()+n)));
		videoC.SetAttribute ("PeerPolicy", EnumValue (PS_SINR));
		videoC.SetAttribute ("ChunkPolicy", EnumValue (CS_LEAST_MISSED));

		ApplicationContainer appC = videoC.Install (clients.Get(n));
		appC.Start (Seconds (clientStart));
		appC.Stop (Seconds (clientStop));
	}

//	if (verbose == 1)
	{
		Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx", MakeCallback (&GenericPacketTrace));
//		Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTxDrop", MakeCallback (&GenericPacketTrace));
		Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx", MakeCallback (&GenericPacketTrace));
//		Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRxDrop", MakeCallback (&GenericPacketTrace));
//
//		Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiPhy/PhyTxBegin",	MakeCallback (&GenericPacketTrace));
//		Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiPhy/PhyTxEnd",	MakeCallback (&GenericPacketTrace));
//		Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiPhy/PhyTxDrop",	MakeCallback (&GenericPacketTrace));
		Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxBegin",	MakeCallback (&GenericPacketTrace));
//		Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxEnd",	MakeCallback (&GenericPacketTrace));
//		Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop",	MakeCallback (&GenericPacketTrace));
		Config::Connect ("/NodeList/*/$ns3::ArpL3Protocol/TxArp", MakeCallback (&GenericPacketTrace));

		Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::VideoPushApplication/TxData", MakeCallback (&VideoTrafficSent));
		Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::VideoPushApplication/TxControl", MakeCallback (&VideoControlSent));
		Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::VideoPushApplication/TxPull", MakeCallback (&TxControlPull));
		Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::VideoPushApplication/RxPull", MakeCallback (&RxControlPull));
		Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::VideoPushApplication/TxDataPull", MakeCallback (&TxDataPull));
		Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::VideoPushApplication/RxDataPull", MakeCallback (&RxDataPull));
		Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::VideoPushApplication/NeighborTrace", MakeCallback (&Neighbors));
		Simulator::Schedule (Seconds(1), &ResetValues);
	}

	std::cout << "Starting simulation for " << totalTime << " s ...\n";

//	FlowMonitorHelper flowmon;
//	Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

	Simulator::Stop(Seconds(totalTime));
	Simulator::Run();

//	 monitor->CheckForLostPackets ();
//	  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
//	  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
//	  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
//	    {
//	      // first 2 FlowIds are for ECHO apps, we don't want to display them
//	      if (i->first > 2)
//	        {
//	          Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
//	          std::cout << "Flow " << i->first - 2 << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";           std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
//	          std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
//	          std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / 10.0 / 1024 / 1024  << " Mbps\n";
//	        }
//	    }
	std::cout<<"\n\n\n";

	Simulator::Destroy();
	return 0;
}

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/video-push-module.h"
#include "ns3/csma-module.h"
#include "ns3/inet-socket-address.h"
#include "ns3/video-helper.h"
#include "ns3/on-off-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("StreamingFirstExample");

int
main (int argc, char *argv[])
{
	SeedManager::SetSeed(1234);
	bool verbose = true;
	uint32_t nCsma = 2;

	CommandLine cmd;
	cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
	cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

	cmd.Parse (argc,argv);

	if (verbose)
	{
		LogComponentEnable ("StreamingFirstExample",  LogLevel( LOG_LEVEL_ALL | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
		LogComponentEnable ("VideoPushApplication",  LogLevel( LOG_LEVEL_ALL | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
		LogComponentEnable ("UdpSocketImpl",  LogLevel( LOG_LEVEL_ALL | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
		LogComponentEnable ("UdpL4Protocol",  LogLevel( LOG_LEVEL_ALL | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
		LogComponentEnable ("Ipv4EndPointDemux",  LogLevel( LOG_LEVEL_ALL | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
		LogComponentEnable ("Ipv4L3Protocol",  LogLevel( LOG_LEVEL_ALL | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
		LogComponentEnable ("Ipv4Interface",  LogLevel( LOG_LEVEL_ALL | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
		LogComponentEnable ("ArpL3Protocol",  LogLevel( LOG_LEVEL_ALL | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
		LogComponentEnable ("Socket",  LogLevel( LOG_LEVEL_ALL | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
		LogComponentEnable ("CsmaNetDevice",  LogLevel( LOG_LEVEL_ALL | LOG_PREFIX_FUNC | LOG_PREFIX_TIME));
	}

	PointToPointHelper pointToPoint;
	pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
	pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

	NodeContainer csmaNodes;
	csmaNodes.Create (nCsma);

	CsmaHelper csma;
	csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
	csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

	NetDeviceContainer csmaDevices;
	csmaDevices = csma.Install (csmaNodes);

	InternetStackHelper stack;
	stack.Install (csmaNodes);

	Ipv4AddressHelper address;
	address.SetBase ("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer addresses = address.Assign (csmaDevices);

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	InetSocketAddress dst1 = InetSocketAddress (addresses.GetAddress (0), 9999);
	VideoHelper video = VideoHelper ("ns3::UdpSocketFactory", dst1);
	video.SetAttribute ("OnTime", RandomVariableValue (ConstantVariable (10.0)));
	video.SetAttribute ("OffTime", RandomVariableValue (ConstantVariable (1.0)));
	video.SetAttribute ("DataRate", StringValue ("10kb/s"));
	video.SetAttribute ("PacketSize", UintegerValue (1200));

	ApplicationContainer apps1 = video.Install (csmaNodes.Get(1));
	apps1.Start (Seconds (1.0));
	apps1.Stop (Seconds (10.0));

	InetSocketAddress dst2 = InetSocketAddress (addresses.GetAddress (1));
	video = VideoHelper ("ns3::UdpSocketFactory", dst2);
	video.SetAttribute ("OnTime", RandomVariableValue (ConstantVariable (10.0)));
	video.SetAttribute ("OffTime", RandomVariableValue (ConstantVariable (1.0)));
	video.SetAttribute ("DataRate", StringValue ("10kb/s"));
	video.SetAttribute ("PacketSize", UintegerValue (1200));

	ApplicationContainer apps2 = video.Install (csmaNodes.Get(0));
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

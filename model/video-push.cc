/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 University of Trento, Italy
 * 					  University of California, Los Angeles, U.S.A.
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
 *
 * Authors: Alessandro Russo <russo@disi.unitn.it>
 *          University of Trento, Italy
 *          University of California, Los Angeles U.S.A.
 */

#define NS_LOG_APPEND_CONTEXT                                   \
  if (GetObject<Node> ()) { std::clog << "[node " << GetObject<Node> ()->GetId () << "] "; }


#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/address-utils.h"
#include "ns3/inet-socket-address.h"
#include "ns3/udp-socket.h"
#include <memory.h>
#include <math.h>
#include <stdio.h>

#include "video-push.h"
#include "ns3/pimdm-routing.h"

NS_LOG_COMPONENT_DEFINE ("VideoPushApplication");

static uint32_t m_latestChunkID;

using namespace std;

namespace ns3 {

#define DELAY_UNIT Time::US

NS_OBJECT_ENSURE_REGISTERED (VideoPushApplication);

TypeId
VideoPushApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VideoPushApplication")
    .SetParent<Application> ()
    .AddConstructor<VideoPushApplication> ()
    .AddAttribute ("Local", "Node main address",
                   AddressValue (),
                   MakeAddressAccessor (&VideoPushApplication::m_localAddress),
                   MakeAddressChecker ())
    .AddAttribute ("LocalPort", "Node main local port",
                   UintegerValue (PUSH_PORT),
                   MakeUintegerAccessor (&VideoPushApplication::m_localPort),
                   MakeUintegerChecker<uint16_t> (1))
    .AddAttribute ("PeerType", "Type of peer: source or peer.",
                   EnumValue(PEER),
                   MakeEnumAccessor(&VideoPushApplication::m_peerType),
                   MakeEnumChecker (PEER, "Regular Peer",
                		   	   	   SOURCE,"Source peer") )
    .AddAttribute ("DataRate", "The data rate in on state.",
                   DataRateValue (DataRate ("500kb/s")),
                   MakeDataRateAccessor (&VideoPushApplication::m_cbrRate),
                   MakeDataRateChecker ())
    .AddAttribute ("PacketSize", "The size of packets sent in on state",
                   UintegerValue (512),
                   MakeUintegerAccessor (&VideoPushApplication::m_pktSize),
                   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("Remote", "The address of the destination",
                   AddressValue (),
                   MakeAddressAccessor (&VideoPushApplication::m_peer),
                   MakeAddressChecker ())
    .AddAttribute ("MaxBytes",
                   "The total number of bytes to send. Once these bytes are sent, "
                   "no packet is sent again, even in on state. The value zero means "
                   "that there is no limit.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&VideoPushApplication::m_maxBytes),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Protocol", "The type of protocol to use.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&VideoPushApplication::m_tid),
                   MakeTypeIdChecker ())
    .AddAttribute ("PeerPolicy", "Peer selection algorithm.",
				   EnumValue(PS_RANDOM),
				   MakeEnumAccessor(&VideoPushApplication::m_peerSelection),
				   MakeEnumChecker (PS_RANDOM, "Random peer selection."))
	.AddAttribute ("ChunkPolicy", "Chunk selection algorithm.",
				   EnumValue(CS_LATEST),
				   MakeEnumAccessor(&VideoPushApplication::m_chunkSelection),
				   MakeEnumChecker (CS_LATEST, "Latest useful chunk"))
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                   MakeTraceSourceAccessor (&VideoPushApplication::m_txTrace))
	.AddTraceSource ("Rx", "A packet has been received",
				   MakeTraceSourceAccessor (&VideoPushApplication::m_rxTrace))
	.AddAttribute ("PullTime", "Time between two consecutive pulls.",
				 TimeValue (MilliSeconds (50)),
				 MakeTimeAccessor (&VideoPushApplication::SetPullTime,
								   &VideoPushApplication::GetPullTime),
				 MakeTimeChecker ())
	.AddAttribute ("HelloTime", "Hello Time.",
				 TimeValue (Seconds (12)),
				 MakeTimeAccessor (&VideoPushApplication::SetHelloTime,
								   &VideoPushApplication::GetHelloTime),
				 MakeTimeChecker ())
	.AddAttribute ("HelloNeighborsTime", "Hello Neighbors Time.",
				 TimeValue (Seconds (4)),
				 MakeTimeAccessor (&VideoPushApplication::SetHelloNeighborsTime,
								   &VideoPushApplication::GetHelloNeighborsTime),
				 MakeTimeChecker ())
	.AddAttribute ("PullMax", "Max number of pull.",
				   UintegerValue (1),
				   MakeUintegerAccessor (&VideoPushApplication::SetPullMax,
						   	   	   	   	 &VideoPushApplication::GetPullMax),
				   MakeUintegerChecker<uint32_t> (1))
	.AddAttribute ("PullActive", "Pull activation.",
				   BooleanValue (true),
				   MakeBooleanAccessor (&VideoPushApplication::SetPullActive,
										&VideoPushApplication::GetPullActive),
				   MakeBooleanChecker() )
	.AddAttribute ("PullWindow", "Pull window.",
				   UintegerValue (50),
				   MakeUintegerAccessor (&VideoPushApplication::SetPullWindow,
										 &VideoPushApplication::GetPullWindow),
				   MakeUintegerChecker<uint32_t> (50))
	.AddAttribute ("PullRatio", "Pull ratio.",
				   DoubleValue (0.80),
				   MakeDoubleAccessor (&VideoPushApplication::SetPullRatio,
									   &VideoPushApplication::GetPullRatio),
				   MakeDoubleChecker<double> (0.80, 1.0))
	.AddAttribute ("HelloLoss", "Number of allowed hello loss.",
				   UintegerValue (1),
				   MakeUintegerAccessor (&VideoPushApplication::SetHelloLoss,
										 &VideoPushApplication::GetHelloLoss),
				   MakeUintegerChecker<uint32_t> (1))
	.AddAttribute ("Source", "Source IP.",
				   Ipv4AddressValue (Ipv4Address::GetAny()),
				   MakeIpv4AddressAccessor (&VideoPushApplication::SetSource,
						   	   	   	   	    &VideoPushApplication::GetSource),
				   MakeIpv4AddressChecker())
	.AddAttribute ("HelloActive", "Hello activation.",
				   BooleanValue (true),
				   MakeBooleanAccessor (&VideoPushApplication::SetHelloActive,
										&VideoPushApplication::GetHelloActive),
				   MakeBooleanChecker() )
	.AddAttribute ("Flag", "Flag.",
				   UintegerValue (0),
				   MakeUintegerAccessor (&VideoPushApplication::m_flag),
				   MakeUintegerChecker<uint32_t> (0))
  ;
  return tid;
}


VideoPushApplication::VideoPushApplication ():
		m_totalRx(0), m_residualBits(0), m_lastStartTime(0), m_totBytes(0),
		m_connected(false), m_ipv4(0), m_socket(0),
		m_pullTimer (Timer::CANCEL_ON_DESTROY), m_pullMax (0), m_helloTimer (Timer::CANCEL_ON_DESTROY), m_helloNeighborsTimer (Timer::CANCEL_ON_DESTROY)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socketList.clear();
  m_duplicates.clear();
  m_sendEvent.Cancel();
  m_peerLoop.Cancel();
}

VideoPushApplication::~VideoPushApplication()
{
}

void
VideoPushApplication::SetMaxBytes (uint32_t maxBytes)
{
  NS_LOG_FUNCTION (this << maxBytes);
  m_maxBytes = maxBytes;
}

uint32_t VideoPushApplication::GetTotalRx () const
{
  return m_totalRx;
}

Ptr<Socket>
VideoPushApplication::GetTxSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

Ptr<Socket>
VideoPushApplication::GetListeningSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

std::list<Ptr<Socket> >
VideoPushApplication::GetAcceptedSockets (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socketList;
}

void
VideoPushApplication::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  std::map<uint32_t, ChunkVideo> tmp_buffer = m_chunks.GetChunkBuffer();
  uint32_t received = 1, receivedpull = 0, receivedpush = 0, missed = 0, duplicates = 0, chunkID = 0, current = 1, late = 0, split = 0, splitP = 0, splitL = 0;
  uint64_t delay = 0, delaylate = 0, delayavg = 0, delayavgpush = 0, delayavgpull = 0;
  uint64_t delaymax = (tmp_buffer.empty() ? 0: GetChunkDelay(tmp_buffer.begin()->first).GetMicroSeconds()), delaymin = (tmp_buffer.empty()?0:GetChunkDelay(tmp_buffer.begin()->first).GetMicroSeconds());
  Time delay_max, delay_min, delay_avg, delay_avg_push, delay_avg_pull;
  double miss = 0.0, rec = 0.0, dups = 0.0, sigma = 0.0, sigmaP = 0.0, sigmaL = 0.0, delayavgB = 0.0, delayavgP = 0.0, delayavgL = 0.0, dlate = 0.0 ;
  for(std::map<uint32_t, ChunkVideo>::iterator iter = tmp_buffer.begin(); iter != tmp_buffer.end() ; iter++){
	  chunkID = iter->first;
	  current = received + missed;
	  while (current < chunkID){
//		NS_LOG_DEBUG ("Missed chunk "<< received << "-"<<m_chunks.GetChunk(received));
		NS_ASSERT (!m_chunks.HasChunk(current));
		if (m_chunks.GetChunkState(current) == CHUNK_DELAYED)
		{
			delaylate += GetChunkDelay(current).GetMicroSeconds();
			late++;
		}
		missed++;
		current = received + missed;
	  }
	  duplicates+= GetDuplicate (current);
	  NS_ASSERT (m_chunks.HasChunk(current));
	  uint64_t chunk_timestamp = GetChunkDelay(current).GetMicroSeconds();
	  delaymax = (chunk_timestamp > delaymax)? chunk_timestamp : delaymax;
	  delaymin = (chunk_timestamp < delaymin)? chunk_timestamp : delaymin;
	  delayavg += chunk_timestamp;
	  switch (m_chunks.GetChunkState(current))
	  {
		  case CHUNK_RECEIVED_PUSH:
		  {
			  delayavgpush += chunk_timestamp;
			  receivedpush++;
			  break;
		  }
		  case CHUNK_RECEIVED_PULL:
		  {
			  delayavgpull += chunk_timestamp;
			  receivedpull++;
			  break;
		  }
		  default:
		  {
			  NS_ASSERT (false);
			  break;
		  }
	  }
	  if ( delayavg != 0 && received > 0 && received % 1000 == 0)
	  {
		  delayavgB += (delayavg/1000.0);
		  split++;
		  delayavg = 0;
	  }
	  if ( delayavgpush != 0 && receivedpush > 0 && receivedpush % 1000 == 0)
	  {
		  delayavgP += (delayavgpush/1000.0);
		  splitP++;
		  delayavgpush = 0;
	  }
	  if ( delayavgpull != 0 && receivedpull > 0 && receivedpull % 1000 == 0)
	  {
		  delayavgL += (delayavgpull/1000.0);
		  splitL++;
		  delayavgpull = 0;
	  }
//	  NS_LOG_DEBUG ("Node " << GetNode()->GetId() << " Dup("<<current<<")="<<GetDuplicate (current)<< " Delay("<<current<<")="<< chunk_timestamp
//			  <<" Max="<<delay_max.GetMicroSeconds()<< " Min="<<delay_min.GetMicroSeconds()<<" Avg="<< delay_avg.GetMicroSeconds()<<" Rec="<<received<<" Mis="<<missed);

	  received++;
  }
  received--;
  delayavgB = received == 0 ? 0 : (delayavgB/(1.0*split)) + (received%1000==0?0:((1.0*delayavg)/(received%1000)));
  delayavgP = receivedpush == 0 ? 0 : (splitP == 0 ? 0 : (delayavgP/(1.0*splitP))) + (receivedpush%1000==0 ? 0 : ((1.0*delayavgpush)/(receivedpush%1000)));
  delayavgL = receivedpull == 0 ? 0 : (splitL == 0 ? 0 : (delayavgL/(1.0*splitL))) + (receivedpull%1000==0 ? 0 : ((1.0*delayavgpull)/(receivedpull%1000)));
  delay_max = Time::FromInteger(delaymax, Time::US);
  delay_min = Time::FromInteger(delaymin, Time::US);
  current = received + missed;
  while ((current = received + missed) < m_latestChunkID) //OCIO QUI ANDAVA IN LOOP, devo distinguire received dai ricevuti e dai totali.
	  missed++;
  double actual = received-missed;
  actual = (actual <= 0?1:(actual));
//  double avg = delayavg/(1.0*actual);
  delay_avg = Time::FromDouble(delayavgB, Time::US);
  delay_avg_push = Time::FromDouble(delayavgP, Time::US);
  delay_avg_pull = Time::FromDouble(delayavgL, Time::US);
  for(std::map<uint32_t, ChunkVideo>::iterator iter = tmp_buffer.begin(); iter != tmp_buffer.end() ; iter++){
  	  double t_dev = ((GetChunkDelay(iter->second.c_id) - delay_avg).ToDouble(Time::US));
  	  sigma += pow(t_dev,2);
  	  switch (m_chunks.GetChunkState(iter->second.c_id))
  	  {
		  case CHUNK_RECEIVED_PUSH:
		  {
			  t_dev = ((GetChunkDelay(iter->second.c_id) - delay_avg_push).ToDouble(Time::US));
			  sigmaP += pow(t_dev,2);
			  break;
		  }
		  case CHUNK_RECEIVED_PULL:
		  {
			  t_dev = ((GetChunkDelay(iter->second.c_id) - delay_avg_pull).ToDouble(Time::US));
			  sigmaL += pow(t_dev,2);
			  break;
		  }
  	  }
  }
  sigma = sqrt(sigma/(1.0*(receivedpush+receivedpull)));
  sigmaP = sqrt(sigmaP/(1.0*receivedpush));
  sigmaL = sqrt(sigmaL/(1.0*receivedpull));
  if (received == 0)
  {
	  miss = 1;
	  rec = dups = 0;
	  delaylate = 0;
  }
  else
  {
	  miss = (missed/(1.0*m_latestChunkID));
	  rec = (received/(1.0*m_latestChunkID));
	  dups = (duplicates==0?0:duplicates/received);
	  dlate = (late==0?0: delaylate/(1.0*late));
  }
  NS_ASSERT (m_latestChunkID==received+missed);
  double tstudent = 1.96; // alpha = 0.025, degree of freedom = infinite
  double confidence = tstudent * (sigma/sqrt(received));
  double confidenceP = tstudent * (sigmaP/sqrt(receivedpush));
  double confidenceL = tstudent * (sigmaL/sqrt(receivedpull));
  if (receivedpush == 0)
  {
  	sigmaP = 0;
  	confidenceP = 0.0;
    delay_avg_push = MicroSeconds (0);
  }
  if (receivedpull == 0)
  {
	sigmaL = 0;
	confidenceL = 0.0;
	delay_avg_pull = MicroSeconds (0);
  }
  char buffer [1024];
  sprintf(buffer, "Chunks Node %d Rec %.5f Miss %.5f Dup %.5f K %d Max %ld us Min %ld us Avg %ld us sigma %.5f conf %.5f late %.5f RecP %d AvgP %ld us sigmaP %.5f confP %.5f RecL %d AvgL %ld us sigmaL %.5f confL %.5f\n",
		  	  	    m_node->GetId(), rec, miss, dups, received, delay_max.ToInteger(Time::US), delay_min.ToInteger(Time::US), delay_avg.ToInteger(Time::US), sigma, confidence, dlate,
		  receivedpush, delay_avg_push.ToInteger(Time::US), sigmaP, confidenceP, receivedpull, delay_avg_pull.ToInteger(Time::US), sigmaL, confidenceL);
  std::cout << buffer;
  m_socket = 0;
  m_socketList.clear();
  // chain up
  Application::DoDispose ();
}

uint32_t
VideoPushApplication::GetApplicationId (void) const
{
  Ptr<Node> node = GetNode ();
  for (uint32_t i = 0; i < node->GetNApplications (); ++i)
    {
      if (node->GetApplication (i) == this)
        {
          return i;
        }
    }
  NS_ASSERT_MSG (false, "forgot to add application to node");
  return 0; // quiet compiler
}

// Application Methods
void VideoPushApplication::StartApplication () // Called at time specified by Start
{
  NS_LOG_FUNCTION_NOARGS ();
  m_ipv4 = m_node->GetObject<Ipv4>();
  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      NS_ASSERT (m_socket != 0);
//      int32_t iface = 1;//TODO just one interface!
	  int status;
	  status = m_socket->Bind (InetSocketAddress(m_localPort));
	  NS_ASSERT (status != -1);
	  // NS_LOG_DEBUG("Push Socket "<< m_socket << " to "<<Ipv4Address::GetAny ()<<"::"<< m_localPort);
	  // Bind to any IP address so that packets can be received
      m_socket->SetAllowBroadcast (true);
      m_socket->SetRecvCallback (MakeCallback (&VideoPushApplication::HandleReceive, this));
      m_socket->SetAcceptCallback (
         MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
         MakeCallback (&VideoPushApplication::HandleAccept, this));
      m_socket->SetCloseCallbacks (
         MakeCallback (&VideoPushApplication::HandlePeerClose, this),
         MakeCallback (&VideoPushApplication::HandlePeerError, this));
      m_pullTimer.SetDelay (GetPullTime());
      m_pullTimer.SetFunction (&VideoPushApplication::PeerLoop, this);
      m_helloTimer.SetDelay (GetHelloTime());
      m_helloTimer.SetFunction (&VideoPushApplication::SendHello, this);
      m_helloNeighborsTimer.SetDelay (GetHelloNeighborsTime());
      m_helloNeighborsTimer.SetFunction (&VideoPushApplication::SendHelloNeighbors, this);
      Time start = Time::FromDouble (UniformVariable().GetValue (0, 5000), Time::MS);
      if (GetHelloActive())
      {
    	  Simulator::Schedule (start, &VideoPushApplication::SendHello, this);
    	  Simulator::Schedule (GetHelloNeighborsTime()+start, &VideoPushApplication::SendHelloNeighbors, this);
      }
      m_neighbors.SetExpire (Time::FromDouble (GetHelloNeighborsTime().GetSeconds() * (1 + GetHelloLoss()), Time::S));
      double inter_time = 1 / (m_cbrRate.GetBitRate()/(8.0*m_pktSize));
      m_pullSlot = Time::FromDouble(inter_time,Time::S);
    }
  // Insure no pending event
  CancelEvents ();
  StartSending ();
}

void VideoPushApplication::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION_NOARGS ();
  StopSending();
  while(!m_socketList.empty ()) //these are accepted sockets, close them
    {
      Ptr<Socket> acceptedSocket = m_socketList.front ();
      m_socketList.pop_front ();
      acceptedSocket->Close ();
    }
  CancelEvents ();
  if(m_socket)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
  else
    {
      NS_LOG_WARN ("VideoPush found null socket to close in StopApplication");
    }
}

Ptr<Ipv4Route>
VideoPushApplication::GetRoute(const Ipv4Address &local, const Ipv4Address &destination) {
	Ptr<Packet> receivedPacket = Create<Packet> (100);
	Ipv4Header hdr;
	hdr.SetDestination(destination);
	hdr.SetSource(local);
	Ptr<NetDevice> oif = 0;
	Socket::SocketErrno err = Socket::ERROR_NOROUTETOHOST;
	Ptr<Ipv4Route> route = m_ipv4->GetRoutingProtocol()->RouteOutput(receivedPacket, hdr, oif, err);
	return route;
}

Ipv4Address
VideoPushApplication::GetNextHop (const Ipv4Address &destination) {
	Ipv4Address local = Ipv4Address::ConvertFrom(m_localAddress);
	Ptr<Ipv4Route> route = GetRoute (local, destination);
	return (route == NULL ? m_gateway: route->GetGateway());
}

void VideoPushApplication::HandlePeerClose (Ptr<Socket> socket)
{
	// NS_LOG_DEBUG ("VideoPush, peerClose");
}

void VideoPushApplication::HandlePeerError (Ptr<Socket> socket)
{
	// NS_LOG_DEBUG ("VideoPush, peerError");
}

void VideoPushApplication::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  s->SetRecvCallback (MakeCallback (&VideoPushApplication::HandleReceive, this));
  m_socketList.push_back (s);
}

void VideoPushApplication::CancelEvents ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_sendEvent.IsRunning ())
    { // Cancel the pending send packet event
      // Calculate residual bits since last packet sent
      Time delta (Simulator::Now () - m_lastStartTime);
      int64x64_t bits = delta.To (Time::S) * m_cbrRate.GetBitRate ();
      m_residualBits += bits.GetHigh ();
    }
  Simulator::Cancel (m_sendEvent);
  Simulator::Cancel (m_peerLoop);
}

void VideoPushApplication::StartSending ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_lastStartTime = Simulator::Now ();
  switch (m_peerType)
  {
	case PEER:
	{
		break;
	}
	case SOURCE:
	{
		Simulator::ScheduleNow(&VideoPushApplication::PeerLoop, this);
		break;
	}
  }
}

void VideoPushApplication::StopSending ()
{
  NS_LOG_FUNCTION_NOARGS ();
  CancelEvents ();
}

void
VideoPushApplication::SetPullActive (bool pull)
{
	m_pullActive = pull;
}

bool
VideoPushApplication::GetPullActive () const
{
	return m_pullActive;
}

void
VideoPushApplication::SetHelloActive (bool hello)
{
	m_helloActive = hello;
}

bool
VideoPushApplication::GetHelloActive () const
{
	return m_helloActive;
}

void
VideoPushApplication::SetPullTime (Time pullt)
{
	m_pullTime = pullt;
}

Time
VideoPushApplication::GetPullTime () const
{
	return m_pullTime;
}

void
VideoPushApplication::SetHelloTime (Time hellot)
{
	m_helloTime = hellot;
}

Time
VideoPushApplication::GetHelloTime () const
{
	return m_helloTime;
}

void
VideoPushApplication::SetHelloNeighborsTime (Time hellot)
{
	m_helloNeighborsTime = hellot;
}

Time
VideoPushApplication::GetHelloNeighborsTime () const
{
	return m_helloNeighborsTime;
}

void
VideoPushApplication::SetPullMax (uint32_t max)
{
	m_pullMax = max;
}

uint32_t
VideoPushApplication::GetPullMax () const
{
	return m_pullMax;
}

void
VideoPushApplication::SetPullWindow (uint32_t window)
{
	m_pullWindow = window;
}

uint32_t
VideoPushApplication::GetPullWindow () const
{
	return m_pullWindow;
}

void
VideoPushApplication::SetPullRatio (double ratio)
{
	m_pullRatio = ratio;
}

double
VideoPushApplication::GetPullRatio () const
{
	return m_pullRatio;
}

void
VideoPushApplication::SetChunkDelay (uint32_t chunkid, Time delay)
{
	NS_ASSERT (chunkid>0);
	NS_ASSERT (m_chunks.HasChunk(chunkid)||m_chunks.GetChunkState(chunkid) == CHUNK_DELAYED);
	uint64_t udelay = delay.GetMicroSeconds();
	if (m_chunk_delay.find(chunkid) == m_chunk_delay.end())
		m_chunk_delay.insert(std::pair<uint32_t, uint64_t>(chunkid,udelay));
	else
		m_chunk_delay.find(chunkid)->second = udelay;
}

Time
VideoPushApplication::GetChunkDelay (uint32_t chunkid)
{
	NS_ASSERT (chunkid>0);
	NS_ASSERT (m_chunks.HasChunk(chunkid) || m_chunks.GetChunkState(chunkid) == CHUNK_DELAYED);
	NS_ASSERT (m_chunk_delay.find(chunkid) != m_chunk_delay.end());
	return Time::FromInteger(m_chunk_delay.find(chunkid)->second, Time::US);
}

void
VideoPushApplication::SetHelloLoss (uint32_t loss)
{
	m_helloLoss = loss;
}

uint32_t
VideoPushApplication::GetHelloLoss () const
{
	return m_helloLoss;
}


Ipv4Address
VideoPushApplication::GetLocalAddress ()
{
	return Ipv4Address::ConvertFrom(m_localAddress);
}

double
VideoPushApplication::GetReceived ()
{
	uint32_t last = m_chunks.GetLastChunk();
	uint32_t window = GetPullWindow();
	window = (last - window + 1) > 0 ? window : last;
	double ratio = 0.0;
	for (int i = last; i > (last - window + 1); i--)
	{
		ratio += (m_chunks.GetChunkState(i) == CHUNK_RECEIVED_PUSH || m_chunks.GetChunkState(i) == CHUNK_RECEIVED_PULL ? 1.0 : 0.0);
	}
	ratio = (ratio / window);
	return  ratio;
}

void
VideoPushApplication::PeerLoop ()
{
	NS_LOG_FUNCTION_NOARGS ();
	switch (m_peerType)
	{
		case PEER:
		{
			uint32_t missed = m_chunks.GetLeastMissed();
			uint32_t last = m_chunks.GetLastChunk();
			double ratio = GetReceived ();
			NS_LOG_INFO ("Node=" <<m_node->GetId()<< " IP=" << GetLocalAddress() << " Last="<<last<<" Missed="<< missed <<" ("<<(missed?GetPullRetry(missed):0)<<","<<GetPullMax()<<")"<<" TimerRunning="<<(m_pullTimer.IsRunning()?"Yes":"No"));
			NS_ASSERT (GetPullActive());
			NS_ASSERT (GetHelloActive());
			if (missed && !m_pullTimer.IsRunning())
			{
				m_pullTimer.Cancel();
				while (missed && GetPullRetry(missed) >= GetPullMax())
				{
					m_chunks.SetChunkState(missed, CHUNK_SKIPPED);
					uint32_t lastmissed = missed;
					missed = m_chunks.GetLeastMissed();
					NS_LOG_INFO ("Node=" <<m_node->GetId()<< " is marking chunk "<< lastmissed <<" as skipped ("<<(lastmissed?GetPullRetry(lastmissed):0)<<","
							<<GetPullMax()<<") New missed="<<missed);
				}
				ratio = GetReceived ();
				if (missed && ratio < GetPullRatio())
				{
					uint32_t size = m_neighbors.GetSize();
					m_neighbors.Purge();
					NS_LOG_INFO ("Node=" <<m_node->GetId()<< " purge neighbors, size="<<size<<"->"<<m_neighbors.GetSize());
					if (m_neighbors.GetSize() == 0)
					{
						NS_LOG_INFO ("Node=" <<m_node->GetId()<< " has no neighbors to pull chunk "<< missed);
					}
					AddPullRetry(missed);
					if (m_neighbors.GetSize() != 0 )
					{
						//TODO PULL WINDOW CHECK
						Ipv4Address target = PeerSelection (PS_RANDOM);
						NS_ASSERT (target != Ipv4Address::GetAny());
//						double delayv = rint(UniformVariable().GetValue (m_pullTime.GetMicroSeconds()*.01, m_pullTime.GetMicroSeconds()*.20));
//						double delayv = rint(UniformVariable().GetValue (m_pullSlot.GetMicroSeconds()*.01, m_pullSlot.GetMicroSeconds()*.40));
						double delayv = 0;
						Time delay = Time::FromDouble (delayv, Time::US);
						Simulator::Schedule (delay, &VideoPushApplication::SendPull, this, missed, target);
						m_pullTimer.Schedule();
					}
				}
			}
			break;
		}
		case SOURCE:
		{
		  if (m_maxBytes == 0 || m_totBytes < m_maxBytes)
			{
			  uint32_t bits = m_pktSize * 8 - m_residualBits;
			  NS_LOG_LOGIC ("bits = " << bits);
			  Time nextTime (Seconds (bits / static_cast<double>(m_cbrRate.GetBitRate ()))); // Time till next packet
			  m_sendEvent = Simulator::ScheduleNow (&VideoPushApplication::SendPacket, this);
			  m_peerLoop = Simulator::Schedule (nextTime, &VideoPushApplication::PeerLoop, this);
			}
		  else
			{ // All done, cancel any pending events
			  StopApplication ();
			}
		  break;
		}
		default:
		{
		  NS_LOG_ERROR("Condition not allowed");
		  break;
		}
	}
}

void
VideoPushApplication::HandleChunk (ChunkHeader::ChunkMessage &chunkheader, const Ipv4Address &sender)
{
	if (m_peerType == SOURCE)
	  return;
	ChunkVideo chunk = chunkheader.GetChunk();
	m_totalRx += chunk.GetSize () + chunk.GetAttributeSize();
	// Update Chunk Buffer START
	uint32_t last = m_chunks.GetLastChunk();
	uint32_t missed = m_chunks.GetLeastMissed();
	bool duplicated = false;
	bool toolate = false;
//#define MISS // INDUCING MISSING CHUNKS START
#ifdef MISS
	bool missed_chunk = UniformVariable().GetValue() < 0.05;
//	uint32_t chunktomiss = 10;//Only node 2 misses chunk #1
//	missed_chunk = (chunk.c_id>last && chunk.c_id%chunktomiss == 0);
	if(missed_chunk)
	{
	  NS_LOG_INFO ("Node " << GetLocalAddress() << " missed chunk " <<  chunk.c_id);
	  return;
	}
#endif
	// INDUCING MISSING CHUNKS END.
	toolate = m_chunks.GetChunkState(chunk.c_id) == CHUNK_SKIPPED;
	duplicated = !toolate && !m_chunks.AddChunk(chunk, CHUNK_RECEIVED_PUSH);
	if (toolate) // Chunk was pulled and received to late
	{
		m_chunks.SetChunkState(chunk.c_id, CHUNK_DELAYED);
		SetChunkDelay(chunk.c_id, (Simulator::Now() - Time::FromInteger(chunk.c_tstamp,Time::US)));
	}
	else if (duplicated) // Duplicated chunk
	{
	  AddDuplicate (chunk.c_id);
	  if(IsPending(chunk.c_id))
		  RemovePending(chunk.c_id);
	}
	else //chunk received correctly
	{
	  SetChunkDelay(chunk.c_id, (Simulator::Now() - Time::FromInteger(chunk.c_tstamp,Time::US)));
	}
	if (missed == chunk.c_id)
	{
		NS_LOG_INFO ("Node "<< GetLocalAddress() << " has received missed chunk "<< missed);
		m_pullTimer.Cancel();
		m_chunks.SetChunkState(chunk.c_id, CHUNK_RECEIVED_PULL);
	}
	missed = m_chunks.GetLeastMissed();
	//TODO PULL WINDOW CHECK
	if (missed && !m_pullTimer.IsRunning() && GetPullActive())
	{
	  Simulator::ScheduleNow (&VideoPushApplication::PeerLoop, this);
	}
	NS_LOG_INFO ("Node " << GetLocalAddress() << (duplicated?" RecDup ":(toolate?" RecLate":" Received "))
		  << chunk << "("<< GetChunkDelay(chunk.c_id).GetMicroSeconds()<< ")"<<" from "
		  << sender << " totalRx="<<m_totalRx<<" Timer "<< m_pullTimer.IsRunning()<<" Neighbors "<< m_neighbors.GetSize()<<" Missed="<<missed);
}

void
VideoPushApplication::HandlePull (ChunkHeader::PullMessage &pullheader, const Ipv4Address &sender)
{
	NS_ASSERT (GetPullActive());
	uint32_t chunkid = pullheader.GetChunk();
	bool hasChunk = m_chunks.HasChunk (chunkid);
	Time delay (0);
	if (hasChunk)
	{
//	  double delayv = rint(UniformVariable().GetValue (m_pullTime.GetMicroSeconds()*.01, m_pullTime.GetMicroSeconds()*.20));
//	  double delayv = rint(UniformVariable().GetValue (m_pullSlot.GetMicroSeconds()*.01, m_pullSlot.GetMicroSeconds()*.40));
	  double delayv = 0;
//	  NS_ASSERT_MSG (delayv > 1, "HandlePull: pulltime is 0");
	  delay = Time::FromDouble (delayv, Time::US);
	  Simulator::Schedule (delay, &VideoPushApplication::SendChunk, this, chunkid, sender);
	  AddPending(chunkid);
	}
	NS_LOG_INFO ("Node " << GetLocalAddress() << " Received pull for " <<  chunkid << (hasChunk?"(Y)":"(N)") <<" from " << sender << ", reply in "<<delay.GetSeconds());
}

void
VideoPushApplication::HandleHello (ChunkHeader::HelloMessage &helloheader, const Ipv4Address &sender)
{
	uint32_t n_last = helloheader.GetLastChunk();
	uint32_t n_chunks = helloheader.GetChunksReceived();
	Ipv4Address destination = helloheader.GetDestination();
	Ipv4Mask mask ("255.0.0.0");
	if (destination.IsSubnetDirectedBroadcast(mask))
	{
		NS_LOG_INFO ("Node " << GetLocalAddress() << " receives broadcast("<<destination<<") hello from " << sender << " #Chunks="<< n_chunks);
		Neighbor nt (sender, PUSH_PORT);
		if(!m_neighbors.IsNeighbor (nt) && m_flag > 0)
		{
//			double delayv = rint(UniformVariable().GetValue (1, m_helloNeighborsTime.GetMilliSeconds()));
//			NS_ASSERT_MSG (delayv > 0, "HandleHello pulltime is 0");
//			Time delay = Time::FromDouble (delayv, Time::MS);
			Simulator::ScheduleNow (&VideoPushApplication::SendHelloUnicast, this, sender);
//			NS_LOG_INFO ("Node " << GetLocalAddress() << " reply to " << sender << " in "<< delay.GetSeconds() << "sec");
		}
	}
	else if (destination.IsEqual(GetLocalAddress()))
	{
		Neighbor nt (sender, PUSH_PORT);
		if(!m_neighbors.IsNeighbor (nt))
		{
			m_neighbors.AddNeighbor (nt);
		}
		NeighborData* neighbor = m_neighbors.GetNeighbor(nt);
		neighbor->Update(n_last, n_chunks);
		uint32_t last = m_chunks.GetLastChunk();
		double ratio = m_chunks.GetBufferSize() / (1.0 * n_last);
		NS_LOG_INFO ("Node " << GetLocalAddress() << " receives hello from " << sender
				<< ", Chunks="<< n_chunks << "("<<m_chunks.GetBufferSize()<<")"
				<< ", Last="<<n_last<<"("<<last<<") "
				<< ", Ratio="<<ratio<<", #Neighbors="<<m_neighbors.GetSize());
		//TODO PULL WINDOW CHECK
//		if ( m_chunks.GetLastChunk() < n_last && !m_pullTimer.IsRunning() && GetPullActive())
//		{
//			NS_ASSERT (GetPullActive());
//			double delayv = rint(UniformVariable().GetValue (0, m_pullTime.GetMicroSeconds()*.30));
//			Time delay = Time::FromDouble(delayv, Time::US);
//			Simulator::Schedule (delay, &VideoPushApplication::SendPull, this, n_last, sender);
//		}
	}
	//TODO: If the source isn't a neighbor, subscribe to a node
}

void VideoPushApplication::HandleReceive (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;

  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        { //EOF
          break;
        }
      ns3::pimdm::RelayTag relayTag;
      bool rtag = packet->RemovePacketTag(relayTag);
      InetSocketAddress address = InetSocketAddress::ConvertFrom (from);
      Ipv4Address sourceAddr = address.GetIpv4 ();
      uint32_t port = address.GetPort();
      Ipv4Address gateway = GetNextHop(sourceAddr);
      Ipv4Mask mask ("255.0.0.0");
      Ipv4Address subnet = GetLocalAddress().GetSubnetDirectedBroadcast(mask);
      NS_LOG_DEBUG("Node " << GetLocalAddress() <<  " receives packet from "<< sourceAddr << " gw " << gateway<< " Tag ["<< relayTag.m_sender<<","<< relayTag.m_receiver<<"] :: "<<relayTag.m_receiver.IsBroadcast());
      if(rtag && subnet != relayTag.m_receiver)
      {
//    	  // NS_LOG_DEBUG("Discarded: not for clients "<<relayTag.m_receiver);
    	  break;
      }
      if(gateway!=relayTag.m_sender)
      {
//    	  NS_LOG_DEBUG("Duplicated packet Gateway "<<gateway<< " Sender " << relayTag.m_sender);
//		  break;
      }
      NS_ASSERT (sourceAddr != GetLocalAddress());
      if (InetSocketAddress::IsMatchingType (from))
        {
          ChunkHeader chunkH (MSG_CHUNK);
          packet->RemoveHeader(chunkH);
          switch (chunkH.GetType())
          {
          	  case MSG_CHUNK:
			  {
				  HandleChunk(chunkH.GetChunkMessage(), sourceAddr);
				  break;
			  }
			  case MSG_PULL:
			  {
				  NS_ASSERT (GetPullActive());
				  HandlePull(chunkH.GetPullMessage(), sourceAddr);
				  break;
			  }
			  case MSG_HELLO:
			  {
				  NS_ASSERT (GetHelloActive());
				  HandleHello(chunkH.GetHelloMessage(), sourceAddr);
				  break;
			  }
          }
        }
      m_rxTrace (packet, from);
    }
}

void
VideoPushApplication::AddPending (uint32_t chunkid)
{
	NS_ASSERT (chunkid >0);
	if (m_pendingPull.find(chunkid) == m_pendingPull.end())
		m_pendingPull.insert(std::pair<uint32_t,uint32_t>(chunkid,0));
	m_pendingPull.find(chunkid)->second+=1;
}

bool
VideoPushApplication::IsPending (uint32_t chunkid)
{
	NS_ASSERT (chunkid >0);
	if (m_pendingPull.find(chunkid) == m_pendingPull.end())
		return false;
	return (m_pendingPull.find(chunkid)->second > 0);
}

bool
VideoPushApplication::RemovePending (uint32_t chunkid)
{
	if (m_pendingPull.find(chunkid) == m_pendingPull.end())
		return false;
	m_pendingPull.erase(chunkid);
	NS_ASSERT (m_pendingPull.find(chunkid) == m_pendingPull.end());
	return true;
}

void
VideoPushApplication::SetSource (Ipv4Address source)
{
	NS_ASSERT (source != Ipv4Address() && source != Ipv4Address::GetAny());
	m_source = source;
}

Ipv4Address
VideoPushApplication::GetSource () const
{
	return m_source;
}

void
VideoPushApplication::AddPullRetry (uint32_t chunkid)
{
	NS_ASSERT (chunkid>0);
	if (m_pullRetries.find(chunkid) == m_pullRetries.end())
		m_pullRetries.insert(std::pair<uint32_t, uint32_t>(chunkid,0));
	m_pullRetries.find(chunkid)->second++;
}

uint32_t
VideoPushApplication::GetPullRetry (uint32_t chunkid)
{
	NS_ASSERT (chunkid>0);
	if (m_pullRetries.find(chunkid) == m_pullRetries.end())
			return 0;
	return m_pullRetries.find(chunkid)->second;
}

void
VideoPushApplication::AddDuplicate (uint32_t chunkid)
{
	NS_ASSERT (chunkid>0);
	if(m_duplicates.find(chunkid) == m_duplicates.end())
	{
	  std::pair<uint32_t, uint32_t> dup(chunkid,0);
	  m_duplicates.insert(dup);
	}
	m_duplicates.find(chunkid)->second++;
}

uint32_t
VideoPushApplication::GetDuplicate (uint32_t chunkid)
{
	NS_ASSERT (chunkid>0);
	if(m_duplicates.find(chunkid) == m_duplicates.end())
		return 0;
	return m_duplicates.find(chunkid)->second;
}


Ipv4Address
VideoPushApplication::PeerSelection (PeerPolicy policy)
{
	NS_LOG_FUNCTION (this);
	Ipv4Address target = m_neighbors.SelectNeighbor(policy).GetAddress();
	NS_ASSERT (target != Ipv4Address() && target != Ipv4Address::GetAny());
	return target;
}

ChunkVideo*
VideoPushApplication::ForgeChunk ()
{
	uint64_t tstamp = Simulator::Now().ToInteger(Time::US);
	if (m_chunks.GetBufferSize() == 0 )
		m_latestChunkID = 0;
	return new ChunkVideo (++m_latestChunkID, tstamp, m_pktSize, 0);
}

uint32_t
VideoPushApplication::ChunkSelection (ChunkPolicy policy){
	NS_LOG_FUNCTION(this<<policy);
	uint32_t chunkid = 0;
	switch (policy){
		case CS_NEW_CHUNK:
		{
			ChunkVideo *cv = ForgeChunk();
			chunkid = cv->c_id;
			if(!m_chunks.AddChunk(*cv, CHUNK_RECEIVED_PUSH))
			{
				AddDuplicate(cv->c_id);
				NS_ASSERT (true);
			}
			NS_ASSERT (m_duplicates.find(cv->c_id) == m_duplicates.end());
			break;
		}
		case CS_LEAST_USEFUL:
		{
			chunkid = m_chunks.GetLeastMissed();
			break;
		}
		default:
		{
		  NS_LOG_ERROR("Condition not allowed");
		  NS_ASSERT (true);
		  break;
		}
	}
	return chunkid;
}

void VideoPushApplication::SendPacket ()
{
	NS_LOG_FUNCTION_NOARGS ();
	switch (m_peerType)
	{
		case PEER:
		{
			break;
		}
		case SOURCE:
		{
			NS_ASSERT (m_sendEvent.IsExpired ());
			uint32_t new_chunk = ChunkSelection(CS_NEW_CHUNK);
			ChunkVideo *copy = m_chunks.GetChunk(new_chunk);
			ChunkHeader chunk (MSG_CHUNK);
			chunk.GetChunkMessage().SetChunk(*copy);
			Ptr<Packet> packet = Create<Packet> (m_pktSize);//AAA Here the data
			packet->AddHeader(chunk);
			uint32_t payload = copy->c_size+copy->c_attributes_size;//data and attributes already in chunk header;
			m_txTrace (packet);
			m_socket->SendTo(packet, 0, m_peer);
			m_totBytes += payload;
			m_lastStartTime = Simulator::Now ();
			m_residualBits = 0;
			NS_ASSERT (new_chunk == m_chunks.GetLastChunk());
			SetChunkDelay(new_chunk, Seconds(0));
			NS_LOG_LOGIC ("Node " << GetNode()->GetId() << " push packet " << *copy<< " Dup="<<GetDuplicate(copy->c_id)
					<< " Delay="<<GetChunkDelay(copy->c_id)<< " UID="<< packet->GetUid() << " Size="<< payload);
			break;
		}
		default:
		{
			NS_LOG_ERROR("Condition not allowed");
			break;
		}
	}
}

void
VideoPushApplication::SendPull (uint32_t chunkid, const Ipv4Address target)
{
	NS_LOG_FUNCTION (this<<chunkid);
	NS_ASSERT (chunkid>0);
	ChunkHeader pull (MSG_PULL);
	pull.GetPullMessage ().SetChunk (chunkid);
	Ptr<Packet> packet = Create<Packet> ();
	packet->AddHeader(pull);
	m_txTrace (packet);
	NS_LOG_INFO ("Node " << GetNode()->GetId() << " sends pull to "<< target << " for chunk "<< chunkid);
	m_socket->SendTo(packet, 0, InetSocketAddress (target, PUSH_PORT));
}

void VideoPushApplication::SendHello ()
{
	NS_LOG_FUNCTION (this);
	Ipv4Mask mask ("255.0.0.0");
	Ipv4Address subnet = GetLocalAddress().GetSubnetDirectedBroadcast(Ipv4Mask (mask));
	ChunkHeader hello (MSG_HELLO);
	hello.GetHelloMessage().SetLastChunk (m_chunks.GetLastChunk());
	hello.GetHelloMessage().SetChunksReceived (m_chunks.GetBufferSize());
	hello.GetHelloMessage().SetDestination (subnet);
	Ptr<Packet> packet = Create<Packet> ();
	packet->AddHeader(hello);
	m_txTrace (packet);
	NS_LOG_INFO ("Node " << GetLocalAddress()<< " sends hello to "<< subnet);
	NS_ASSERT (GetHelloActive());
	m_socket->SendTo(packet, 0, InetSocketAddress (subnet, PUSH_PORT));
	Time t = Time::FromDouble((0.01 * UniformVariable ().GetValue (0, 1000)), Time::MS);
	m_helloTimer.Schedule (m_helloTime - t);
}

void VideoPushApplication::SendHelloNeighbors ()
{
	NS_LOG_FUNCTION (this);
	uint32_t size = m_neighbors.GetSize();
	for (int i = 0; i < size && m_flag > 1 ;  i++)
	{
		Neighbor nt = m_neighbors.Get(i);
//		Time delay = Time::FromDouble (UniformVariable().GetValue(0, 200),Time::MS);
		Simulator::ScheduleNow (&VideoPushApplication::SendHelloUnicast, this, nt.GetAddress());
	}
	Time t = Time::FromDouble((0.01 * UniformVariable ().GetValue (0, 100)), Time::MS);
	m_helloNeighborsTimer.Schedule (m_helloNeighborsTime - t);
}

void VideoPushApplication::SendHelloUnicast (Ipv4Address &neighbor)
{
	NS_LOG_FUNCTION (this);
	ChunkHeader hello (MSG_HELLO);
	hello.GetHelloMessage().SetLastChunk (m_chunks.GetLastChunk());
	hello.GetHelloMessage().SetChunksReceived (m_chunks.GetBufferSize());
	hello.GetHelloMessage().SetDestination (neighbor);
	Ptr<Packet> packet = Create<Packet> ();
	packet->AddHeader(hello);
	m_txTrace (packet);
	NS_LOG_INFO ("Node " << GetLocalAddress()<< " sends hello directly to "<< neighbor);
	NS_ASSERT (GetHelloActive());
	m_socket->SendTo(packet, 0, InetSocketAddress (neighbor, PUSH_PORT));
}

void
VideoPushApplication::SendChunk (uint32_t chunkid, const Ipv4Address target)
{
	NS_LOG_FUNCTION (this<<chunkid<<target);
	NS_ASSERT (chunkid>0);
	NS_ASSERT (target != GetLocalAddress());
	NS_ASSERT (GetPullActive());
	NS_ASSERT (GetHelloActive());
	switch (m_peerType)
	{
		case PEER:
		{
			if (!IsPending(chunkid)){
				NS_LOG_DEBUG("Chunk "<< chunkid << " is not pending anymore");
				break;
			}
			ChunkHeader chunk (MSG_CHUNK);
			ChunkVideo *copy = m_chunks.GetChunk(chunkid);
			Ptr<Packet> packet = Create<Packet> (copy->GetSize());
			chunk.GetChunkMessage().SetChunk(*copy);
			packet->AddHeader(chunk);
			NS_LOG_LOGIC ("Node " << GetLocalAddress() << " replies pull to " << target << " for chunk [" << *copy<< "] Size " << packet->GetSize() << " UID "<< packet->GetUid());
			m_txTrace (packet);
			m_socket->SendTo (packet, 0, InetSocketAddress(target, PUSH_PORT));
			break;
		}
		case SOURCE:
		{
			if (!IsPending(chunkid)){
				NS_LOG_DEBUG("Chunk "<< chunkid << " is not pending anymore");
				break;
			}
			ChunkHeader chunk (MSG_CHUNK);
			ChunkVideo *copy = m_chunks.GetChunk(chunkid);
			Ptr<Packet> packet = Create<Packet> (copy->GetSize());
			chunk.GetChunkMessage().SetChunk(*copy);
			packet->AddHeader(chunk);
			NS_LOG_LOGIC ("Node " << GetLocalAddress() << " replies pull to " << target << " for chunk [" << *copy<< "] Size " << packet->GetSize() << " UID "<< packet->GetUid());
			m_txTrace (packet);
			m_socket->SendTo (packet, 0, InetSocketAddress(target, PUSH_PORT));
			break;
		}
		default:
		{
			NS_LOG_ERROR("Condition not allowed");
			break;
		}
	}
}

void VideoPushApplication::ConnectionSucceeded (Ptr<Socket>)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_connected = true;

}

void VideoPushApplication::ConnectionFailed (Ptr<Socket>)
{
  NS_LOG_FUNCTION_NOARGS ();
  cout << "VideoPush, Connection Failed" << endl;
}

void VideoPushApplication::SetGateway (const Ipv4Address &gateway)
{
	m_gateway = gateway;
}

} // Namespace ns3

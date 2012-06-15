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

uint32_t last_chunk;
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
				 TimeValue (Seconds (2)),
				 MakeTimeAccessor (&VideoPushApplication::SetPullTime,
								   &VideoPushApplication::GetPullTime),
				 MakeTimeChecker ())
	.AddAttribute ("HelloTime", "Hello Time.",
				 TimeValue (Seconds (4)),
				 MakeTimeAccessor (&VideoPushApplication::SetHelloTime,
								   &VideoPushApplication::GetHelloTime),
				 MakeTimeChecker ())
	.AddAttribute ("PullMax", "Max number of pull.",
				   UintegerValue (1),
				   MakeUintegerAccessor (&VideoPushApplication::SetPullMax,
						   	   	   	   	 &VideoPushApplication::GetPullMax),
				   MakeUintegerChecker<uint32_t> (1))
	.AddAttribute ("PullActive", "Pull activation.",
				   BooleanValue (1),
				   MakeBooleanAccessor (&VideoPushApplication::SetPullActive,
										&VideoPushApplication::GetPullActive),
				   MakeBooleanChecker() )
	.AddAttribute ("HelloLoss", "Number of allowed hello loss.",
				   UintegerValue (1),
				   MakeUintegerAccessor (&VideoPushApplication::SetHelloLoss,
										 &VideoPushApplication::GetHelloLoss),
				   MakeUintegerChecker<uint32_t> (1))
	.AddAttribute ("Source", "Source IP.",
				   Ipv4AddressValue (Ipv4Address::GetAny()),
				   MakeIpv4AddressAccessor (&VideoPushApplication::SetSource),
				   MakeIpv4AddressChecker())
  ;
  return tid;
}


VideoPushApplication::VideoPushApplication ():
		m_totalRx(0), m_residualBits(0), m_lastStartTime(0), m_totBytes(0),
		m_connected(false), m_ipv4(0), m_socket(0),
		m_pullTimer (Timer::CANCEL_ON_DESTROY), m_pullMax (0), m_helloTimer (Timer::CANCEL_ON_DESTROY)
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
//  NS_LOG_FUNCTION_NOARGS ();
  std::map<uint32_t, ChunkVideo> tmp_buffer = m_chunks.GetChunkBuffer();
  uint32_t received = 1, missed = 0, duplicates = 0, cnt = 0;
  Time delay_max, delay_min, delay_avg;
  double miss =0.0, rec = 0.0, dups = 0.0, sigma =0.0;
  for(std::map<uint32_t, ChunkVideo>::iterator iter = tmp_buffer.begin(); iter != tmp_buffer.end() ; iter++){
	  uint32_t cid = iter->first;
	  while (received < cid){
//		  NS_LOG_DEBUG ("Missed chunk "<< received << "-"<<m_chunks.GetChunk(received));
		missed++;
		received++;
	  }
	  duplicates+= GetDuplicate (cid);
	  NS_ASSERT(m_chunks.HasChunk(cid));
	  Time chunk_timestamp = GetChunkDelay(cid);
	  delay_max = (delay_max < chunk_timestamp)? chunk_timestamp : delay_max;
	  delay_min = delay_min==0? delay_max: (delay_min > chunk_timestamp)? chunk_timestamp : delay_min;
	  delay_avg += chunk_timestamp;
	  received = cid+1;
//	  NS_LOG_DEBUG ("Node " << GetNode()->GetId() << " Dup("<<cid<<")="<<GetDuplicate (cid)<< " Delay("<<cid<<")="<< chunk_timestamp.GetMicroSeconds()
//			  <<" Max="<<delay_max.GetMicroSeconds()<< " Min="<<delay_min.GetMicroSeconds()<<" Avg="<< delay_avg.GetMicroSeconds()<<" Last="<<last<<" Missed="<<missed);
  }
  while (received < m_latestChunkID)
	  missed++;
  double actual = received-missed-1;
  double avg = delay_avg.ToDouble(Time::US) / (actual <= 0?1:(actual));
  delay_avg = Time::FromDouble(avg, Time::US);
  for(std::map<uint32_t, ChunkVideo>::iterator iter = tmp_buffer.begin(); iter != tmp_buffer.end() ; iter++){
  	  double t_dev = ((GetChunkDelay(iter->second.c_id) - delay_avg).ToDouble(Time::US));
  	sigma += pow(t_dev,2);
  	  cnt++;
  }
  cnt--;
//  // NS_LOG_DEBUG("Computing std deviation over " <<cnt <<" samples");
  sigma = sqrt(sigma/(1.0*cnt));
  while(received>0 && received < last_chunk){
	  missed++;
	  received++;
  }
//  // NS_LOG_DEBUG("done " <<last<<","<<missed<<","<<duplicates);
  if (received == 0) {
	  miss = 1;
	  rec = dups = 0;
  }
  else
  {
	  miss = (missed/(1.0*received));
	  double diff = received-missed ;
	  rec = diff == 0? 0 : (diff/received);
	  dups = duplicates==0?0:duplicates/diff;
  }
  double tstudent = 1.96; // alpha = 0.025, degree of freedom = infinite
  double confidence = tstudent * (sigma/sqrt(received));
  char buffer [1024];
  sprintf(buffer, " Rec %.5f Miss %.5f Dup %.5f K %d Max %ld us Min %ld us Avg %ld us sigma %.5f conf %.5f",
		  	  	  	   rec, miss, dups, received, delay_max.ToInteger(Time::US), delay_min.ToInteger(Time::US), delay_avg.ToInteger(Time::US), sigma, confidence);
  std::cout << "Chunks Node " << m_node->GetId() << buffer << "\n";

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
      m_pullTimer.SetDelay(GetPullTime());
      m_pullTimer.SetFunction(&VideoPushApplication::PeerLoop, this);
      m_helloTimer.SetDelay(GetHelloTime());
      m_helloTimer.SetFunction(&VideoPushApplication::SendHello, this);
      Time start = Time::FromDouble (UniformVariable().GetValue (0, 2*GetHelloTime().GetMicroSeconds()), Time::US);
      Simulator::Schedule (start, &VideoPushApplication::SendHello, this);
      m_neighbors.SetExpire (Time::FromDouble (GetHelloTime().GetMicroSeconds() * (1 + GetHelloLoss()), Time::US));
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
VideoPushApplication::GetRoute(Ipv4Address local, Ipv4Address destination) {
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
VideoPushApplication::GetNextHop (Ipv4Address destination) {
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
  Simulator::ScheduleNow(&VideoPushApplication::PeerLoop, this);
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
VideoPushApplication::SetHelloTime (Time pullt)
{
	m_helloTime = pullt;
}

Time
VideoPushApplication::GetHelloTime () const
{
	return m_helloTime;
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
VideoPushApplication::SetChunkDelay (uint32_t chunkid, Time delay)
{
	NS_ASSERT (chunkid>0);
	NS_ASSERT (m_chunks.HasChunk(chunkid));
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
	NS_ASSERT (m_chunks.HasChunk(chunkid));
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

void VideoPushApplication::PeerLoop ()
{
	NS_LOG_FUNCTION_NOARGS ();
	switch (m_peerType)
	{
		case PEER:
		{
			uint32_t missed = m_chunks.GetLeastMissed();
			uint32_t last = m_chunks.GetLastChunk();
			NS_LOG_INFO ("Node=" <<m_node->GetId()<< " IP=" << GetLocalAddress() << " Last="<<last<<" Missed="<< missed <<" ("<<(missed?GetPullRetry(missed):0)<<","<<GetPullMax()<<")"<<" TimerRunning="<<(m_pullTimer.IsRunning()?"Yes":"No"));
			if (missed && !m_pullTimer.IsRunning())
			{
				m_pullTimer.Cancel();
				while (missed && GetPullRetry(missed) >= GetPullMax())
				{
					m_chunks.SetChunkState(missed, CHUNK_SKIPPED);
					NS_LOG_INFO ("Node=" <<m_node->GetId()<< " is marking chunk "<< missed <<" as skipped ("<<(missed?GetPullRetry(missed):0)<<","<<GetPullMax()<<")");
					missed = m_chunks.GetLeastMissed();
				}
				if (missed)
				{
					m_neighbors.Purge();
					if (m_neighbors.GetSize() == 0) break;
					AddPullRetry(missed);
					Ipv4Address target = PeerSelection (PS_RANDOM);
					NS_ASSERT (target != Ipv4Address::GetAny());
					SendPull (missed, target);
					m_pullTimer.Schedule();
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
VideoPushApplication::HandleChunk (ChunkHeader::ChunkMessage &chunkheader, Ipv4Address sender)
{
	ChunkVideo chunk = chunkheader.GetChunk();
	m_totalRx += chunk.GetSize () + chunk.GetAttributeSize();
	// Update Chunk Buffer START
	uint32_t last = m_chunks.GetLastChunk();
	uint32_t missed = m_chunks.GetLeastMissed();
	if (m_peerType == SOURCE)
	  return;
	bool duplicated = false;
#define MISS // INDUCING MISSING CHUNKS START
#ifdef MISS
	bool missed_chunk;
	uint32_t chunktomiss = 10;//Only node 2 misses chunk #1
	missed_chunk = (GetNode()->GetId() == 2 && chunk.c_id%chunktomiss == 0);
	if(missed_chunk)
	{
	  NS_LOG_INFO ("Node " << GetLocalAddress() << " missed chunk " <<  chunk.c_id);
	  return;
	}
#endif
	// INDUCING MISSING CHUNKS END.
	if (missed == chunk.c_id)
	{
		NS_LOG_INFO ("Node "<< GetLocalAddress() << " has received missed chunk "<< missed);
		m_pullTimer.Cancel();
	}
	duplicated = !m_chunks.AddChunk(chunk, CHUNK_RECEIVED_PUSH);
	if (duplicated)
	{
	  AddDuplicate (chunk.c_id);
	  duplicated = true;
	  if(IsPending(chunk.c_id))
		  RemovePending(chunk.c_id);
	}
	else
	{
	  SetChunkDelay(chunk.c_id, (Simulator::Now() - Time::FromInteger(chunk.c_tstamp,Time::US)));
	}
	last = m_chunks.GetLastChunk();
	missed = m_chunks.GetLeastMissed();
	if (missed && !m_pullTimer.IsRunning() && GetPullActive())
	  Simulator::ScheduleNow(&VideoPushApplication::PeerLoop, this);
	NS_LOG_INFO ("Node " << GetLocalAddress() << (duplicated?" RecDup ":" Received ")
		  << chunk << "("<< GetChunkDelay(chunk.c_id).GetMicroSeconds()<< ")"<<" from "
		  << sender << " totalRx="<<m_totalRx<<" Timer "<< m_pullTimer.IsRunning()<<" Neighbors "<< m_neighbors.GetSize());
}

void
VideoPushApplication::HandlePull (ChunkHeader::PullMessage &pullheader, Ipv4Address sender)
{
	uint32_t chunkid = pullheader.GetChunk();
	bool hasChunk = m_chunks.HasChunk (chunkid);
	NS_LOG_INFO ("Node " <<m_node->GetId()<< " IP " << GetLocalAddress()
			<< " Received Pull for [" <<  chunkid << "::"<< (hasChunk?"Yes":"No") <<"] from " << sender);
//	if (m_peerType != PEER) return; // source does not reply
	if (hasChunk)
	{
//	  double rangeM = rint(m_pullTime.GetMicroSeconds()*.010), rangem = rint (rangeM*.001);
	  double rangeM = rint(UniformVariable().GetValue (100,10000));
	  NS_ASSERT (rangeM > 1);
	  Time delay = Time::FromDouble (rangeM, Time::US);
	  Simulator::Schedule (delay, &VideoPushApplication::SendChunk, this, chunkid, sender);
	  AddPending(chunkid);
	}
}

void
VideoPushApplication::HandleHello (ChunkHeader::HelloMessage &helloheader, Ipv4Address sender)
{
	uint32_t last = helloheader.GetLastChunk();
	uint32_t chunks = helloheader.GetChunksReceived();
	NS_LOG_INFO ("Node " << GetLocalAddress() << " Received hello from "
			<< sender << " with "<< chunks << " chunks.");
	Neighbor nt (sender, PUSH_PORT);
	if(!m_neighbors.IsNeighbor (nt))
		m_neighbors.AddNeighbor (nt);
	NeighborData* neighbor = m_neighbors.GetNeighbor(nt);
	neighbor->Update(last, chunks);
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
      Ipv4Mask mask ("255.255.255.0");
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
				  HandlePull(chunkH.GetPullMessage(), sourceAddr);
				  break;
			  }
			  case MSG_HELLO:
			  {
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
	NS_ASSERT(chunkid>0);
	if (m_pullRetries.find(chunkid) == m_pullRetries.end())
		m_pullRetries.insert(std::pair<uint32_t, uint32_t>(chunkid,0));
	m_pullRetries.find(chunkid)->second++;
}

uint32_t
VideoPushApplication::GetPullRetry (uint32_t chunkid)
{
	NS_ASSERT(chunkid>0);
	if (m_pullRetries.find(chunkid) == m_pullRetries.end())
			return 0;
	return m_pullRetries.find(chunkid)->second;
}

void
VideoPushApplication::AddDuplicate (uint32_t chunkid)
{
	NS_ASSERT(chunkid>0);
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
	NS_ASSERT(chunkid>0);
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
VideoPushApplication::ChunkSelection (ChunkPolicy policy){
	NS_LOG_FUNCTION(this<<policy);
	ChunkVideo *copy;
	switch (policy){
		case CS_NEW_CHUNK:
		{
			uint64_t tstamp = Simulator::Now().ToInteger(Time::US);
			if (m_chunks.GetBufferSize() == 0 )
				m_latestChunkID = 1;
			ChunkVideo cv(m_latestChunkID,tstamp,m_pktSize,0);
			copy = cv.Copy();
			break;
		}
		case CS_LEAST_USEFUL:
		{

			break;
		}
		default:
		{
		  NS_LOG_ERROR("Condition not allowed");
		  NS_ASSERT(true);
		  break;
		}
	}
	return copy;
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
			ChunkVideo *copy = ChunkSelection(CS_NEW_CHUNK);
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
			last_chunk = (last_chunk < m_latestChunkID) ? m_latestChunkID : last_chunk;
			if(!m_chunks.AddChunk(*copy,CHUNK_RECEIVED_PUSH))
			{
				AddDuplicate(copy->c_id);
				NS_ASSERT (true);
			}
			NS_ASSERT (copy->c_id == m_chunks.GetLastChunk());
			SetChunkDelay(copy->c_id, Seconds(0));
			NS_LOG_LOGIC ("Node " << GetNode()->GetId() << " push packet " << *copy<< " Dup="<<GetDuplicate(copy->c_id)
					<< " Delay="<<GetChunkDelay(copy->c_id)<< " UID="<< packet->GetUid() << " Size="<< payload);
			NS_ASSERT (m_duplicates.find(copy->c_id) == m_duplicates.end());
			m_latestChunkID++;
			delete copy;
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
VideoPushApplication::SendPull (uint32_t chunkid, Ipv4Address target)
{
	NS_LOG_FUNCTION (this<<chunkid);
	NS_ASSERT(chunkid>0);
	ChunkHeader pull (MSG_PULL);
	pull.GetPullMessage ().SetChunk (chunkid);
	Ptr<Packet> packet = Create<Packet> ();
	packet->AddHeader(pull);
	m_txTrace (packet);
	NS_LOG_INFO ("Node " << GetNode()->GetId() << " sends PULL to "<< target << " for chunk "<< chunkid);
	m_socket->SendTo(packet, 0, InetSocketAddress (target, PUSH_PORT));
}

void VideoPushApplication::SendHello ()
{
	NS_LOG_FUNCTION (this);
	ChunkHeader hello (MSG_HELLO);
	hello.GetHelloMessage().SetLastChunk (m_chunks.GetLastChunk());
	hello.GetHelloMessage().SetChunksReceived (m_chunks.GetBufferSize());
	Ptr<Packet> packet = Create<Packet> ();
	packet->AddHeader(hello);
	m_txTrace (packet);
	Ipv4Address subnet = GetLocalAddress().GetSubnetDirectedBroadcast(Ipv4Mask ("255.0.0.0"));
	NS_LOG_INFO ("Node " << GetLocalAddress()<< " sends hello to "<< subnet);
	m_socket->SendTo(packet, 0, InetSocketAddress (subnet, PUSH_PORT));
	m_helloTimer.Schedule();
}

void
VideoPushApplication::SendChunk (uint32_t chunkid, Ipv4Address target)
{
	NS_LOG_FUNCTION (this<<chunkid<<target);
	NS_ASSERT(chunkid>0);
	NS_ASSERT(target != GetLocalAddress());
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
			NS_LOG_LOGIC ("Node " << GetLocalAddress() << " replies PULL to " << target << " for chunk [" << *copy<< "] Size " << packet->GetSize() << " UID "<< packet->GetUid());
			m_txTrace (packet);
			Ipv4Address subnet ("10.255.255.255");
			m_socket->SendTo (packet, 0, InetSocketAddress(subnet, PUSH_PORT));
			break;
		}
		case SOURCE:
		{
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

void VideoPushApplication::SetGateway (Ipv4Address gateway)
{
	m_gateway = gateway;
}

} // Namespace ns3

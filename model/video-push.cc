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

using namespace std;

namespace ns3 {

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
	.AddAttribute ("PullMax", "Max number of pull.",
				   UintegerValue (1),
				   MakeUintegerAccessor (&VideoPushApplication::SetPullMax,
						   	   	   	   	 &VideoPushApplication::GetPullMax),
				   MakeUintegerChecker<uint32_t> (1))
  ;
  return tid;
}


VideoPushApplication::VideoPushApplication ():
		m_totalRx(0), m_residualBits(0), m_lastStartTime(0), m_totBytes(0),
		m_connected(false), m_ipv4(0), m_latestChunkID(1), m_socket(0),
		m_pullTimer (Timer::CANCEL_ON_DESTROY), m_pullMax (0)
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
  uint32_t last = 1;
  uint32_t missed = 0;
  uint32_t duplicates = 0;
  Time delay_max;
  Time delay_min;
  Time delay_avg;
  double miss;
  double rec;
  double dups;
  double dev;
  uint32_t cnt;
  miss = rec = dups = 0;
  for(std::map<uint32_t, ChunkVideo>::iterator iter = tmp_buffer.begin(); iter != tmp_buffer.end() ; iter++){
	  uint32_t cid = iter->first;
	  while (last < cid){
//		// NS_LOG_DEBUG ("Missed chunk "<< last << "-"<<m_chunks.GetChunk(last));
		missed++;
		last++;
	  }
	  duplicates+=m_duplicates.find(cid)->second;
	  last = cid +1;
	  Time chunk_timestamp = Time::FromInteger(iter->second.c_tstamp,Time::US);
	  delay_max = (delay_max < chunk_timestamp)? chunk_timestamp : delay_max;
	  delay_min = delay_min==0? delay_max: (delay_min > chunk_timestamp)? chunk_timestamp : delay_min;
	  delay_avg += chunk_timestamp;
//	  // NS_LOG_DEBUG ("Time "<< chunk_delay <<" "<<delay_max<< " "<<delay_min<<" "<< delay_avg);
  }
  double actual = last-missed;
  delay_avg = Time::FromDouble(delay_avg.ToDouble(Time::US) / (actual <= 0?1:(actual)), Time::US);
  cnt = 0;
  dev = 0;
  for(std::map<uint32_t, ChunkVideo>::iterator iter = tmp_buffer.begin(); iter != tmp_buffer.end() ; iter++){
  	  Time chunk_delay = Time::FromInteger(iter->second.c_tstamp,Time::US);
  	  double t_dev = ((chunk_delay - delay_avg).ToDouble(Time::US));
  	  dev += pow(t_dev,2);
  	  cnt++;
  }
  cnt--;
//  // NS_LOG_DEBUG("Computing std deviation over " <<cnt <<" samples");
  dev = sqrt(dev/(1.0*cnt));
  while(last>0 && last < last_chunk){
	  missed++;
	  last++;
  }
//  // NS_LOG_DEBUG("done " <<last<<","<<missed<<","<<duplicates);
  if (last == 0) {
	  miss = 1;
	  rec = dups = 0;
  }
  else
  {
	  miss = (missed/(1.0*last));
	  double diff = last-missed ;
	  rec = diff == 0? 0 : (diff/last);
	  dups = duplicates==0?0:duplicates/diff;
  }
//  char ss[100];
//  sprintf(ss, " R %.2f M %.2f D %.2f T %d M %lu m %lu A %lu",rec,miss,dups,last,delay_max.ToInteger(Time::NS),delay_min.ToInteger(Time::NS),delay_avg.ToInteger(Time::NS));
  char dd[120];
  sprintf(dd, " Rec %.5f Miss %.5f Dup %.5f K %d Max %ld us Min %ld us Avg %ld us S %.5f",rec,miss,dups,last,delay_max.ToInteger(Time::US),delay_min.ToInteger(Time::US),delay_avg.ToInteger(Time::US), dev);
//  NS_LOG_INFO("Chunks Node " << m_node->GetId() << dd);
  std::cout << "Chunks Node " << m_node->GetId() << dd << "\n";

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
	  status = m_socket->Bind (InetSocketAddress(Ipv4Address::GetAny (), m_localPort));
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
//    // NS_LOG_DEBUG("Chunks: " << m_chunks.PrintBuffer());
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
VideoPushApplication::SetPullMax (uint32_t max)
{
	m_pullMax = max;
}

uint32_t
VideoPushApplication::GetPullMax () const
{
	return m_pullMax;
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
			NS_LOG_INFO ("Node=" <<m_node->GetId()<< " IP=" << GetLocalAddress() << " Last="<<last<<" Missed="<< missed <<"("<<(missed?GetPullRetry(missed):0)<<","<<GetPullMax()<<")"<<" TimerRunning="<<(m_pullTimer.IsRunning()?"Yes":"No"));
			if (missed && !m_pullTimer.IsRunning())
			{
				m_pullTimer.Cancel();
				AddPullRetry(missed);
				while (missed && GetPullRetry(missed) > GetPullMax())
				{
					m_chunks.SetChunkState(missed, CHUNK_SKIPPED);
					missed = m_chunks.GetLeastMissed();
				}
				if (missed)
				{
					SendPull (missed);
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
      Ipv4Address current = Ipv4Address::ConvertFrom(m_localAddress);
      InetSocketAddress inetAddr = InetSocketAddress::ConvertFrom (from);
      Ipv4Address sourceAddr = inetAddr.GetIpv4 ();
//      uint16_t sourcePort = inetAddr.GetPort();
      Ipv4Address gateway = GetNextHop(sourceAddr);
      Ipv4Mask mask ("255.255.255.0");
      current = current.GetSubnetDirectedBroadcast(mask);
      NS_LOG_DEBUG("Packet from "<< from << " Local "<< current << " gw " << gateway<< " Tag ["<< relayTag.m_sender<<","<< relayTag.m_receiver<<"] :: "<<relayTag.m_receiver.IsBroadcast());
      if(rtag && current != relayTag.m_receiver){
//    	  // NS_LOG_DEBUG("Discarded: not for clients "<<relayTag.m_receiver);
    	  break;
      }
      if(gateway!=relayTag.m_sender){
    	  NS_LOG_DEBUG("Duplicated packet Gateway "<<gateway<< " Sender " << relayTag.m_sender);
//		  break;
      }
      if (InetSocketAddress::IsMatchingType (from))
        {
          ChunkHeader chunkH (CHUNK);
          packet->RemoveHeader(chunkH);
          switch (chunkH.GetType())
          {
			  case CHUNK:{
				  ChunkVideo chunk = chunkH.GetChunkMessage().GetChunk();
				  m_totalRx += chunk.GetSize () + chunk.GetAttributeSize();
				  InetSocketAddress address = InetSocketAddress::ConvertFrom (from);
				  Time delay_0 = Time::FromInteger(chunk.c_tstamp,Time::US);
				  Time delay_1 = Simulator::Now();
				  delay_1 -= delay_0;
				  chunk.c_tstamp = delay_1.ToInteger(Time::US);
				  uint32_t port = address.GetPort();
				  Ipv4Address senderAddr = address.GetIpv4 ();
				  // Update Neighbors START
				  Neighbor *sender = new Neighbor(senderAddr, port);
				  if(!m_neighbors.IsNeighbor(*sender)){
					m_neighbors.AddNeighbor(*sender);
				  }
				  NeighborData *ndata = m_neighbors.GetNeighbor(*sender);
				  ndata->n_contact = Simulator::Now();
				  ndata->n_latestChunk = chunk.c_id;
				  // Update Neighbors END

				  // Update Chunk Buffer START
				  if(!m_chunks.AddChunk(chunk)){
					  if(m_duplicates.find(chunk.c_id)==m_duplicates.end()){
						  std::pair<uint32_t, uint32_t> dup(chunk.c_id,0);
						  m_duplicates.insert(dup);
					  }
					m_duplicates.find(chunk.c_id)->second++;
					NS_LOG_INFO ("Node " <<m_node->GetId()<< " IP " << Ipv4Address::ConvertFrom(m_localAddress) << " ReceivedDuplicate [" <<  chunk << "::"<< delay_1 <<"] from " << address.GetIpv4 () << " [" << relayTag.m_sender << "] UID "<< packet->GetUid() << " total Rx " << m_totalRx);
					// NS_LOG_DEBUG("Chunk " << chunk->c_id <<" already received " << m_duplicates.find(chunk->c_id)->second<<" times");
				  }
				  else
					  NS_LOG_INFO ("Node " <<m_node->GetId()<< " IP " << Ipv4Address::ConvertFrom(m_localAddress) << " Received [" <<  chunk << "::"<< delay_1 <<"] from " << address.GetIpv4 () << " [" << relayTag.m_sender << "] UID "<< packet->GetUid() << " total Rx " << m_totalRx);
				  // Update ChunkBuffer END
				  //cast address to void , to suppress 'address' set but not used
				  //compiler warning in optimized builds
				  (void) address;
				  delete sender;
				  //
				  break;
			  }
			  case PULL:
			  {

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
  Simulator::Cancel (m_sendTx);
}

// Event handlers
void VideoPushApplication::StartSending ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_lastStartTime = Simulator::Now ();
  ScheduleNextTx ();  // Schedule the send packet event
}

void VideoPushApplication::StopSending ()
{
  NS_LOG_FUNCTION_NOARGS ();
  CancelEvents ();

}

ChunkVideo*
VideoPushApplication::ChunkSelection (ChunkPolicy policy){
	NS_LOG_FUNCTION(this<<policy);
	ChunkVideo *copy;
	switch (policy){
		case CS_NEW_CHUNK:
		{
			uint64_t tstamp = Simulator::Now().ToInteger(Time::US);
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
		  break;
		}
	}
	return copy;
// Private helpers
void VideoPushApplication::ScheduleNextTx ()
{
  NS_LOG_FUNCTION_NOARGS ();
  if(m_peerType == PEER) return;
  if (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    {
      uint32_t bits = m_pktSize * 8 - m_residualBits;
      NS_LOG_LOGIC ("bits = " << bits);
      Time nextTime (Seconds (bits / static_cast<double>(m_cbrRate.GetBitRate ()))); // Time till next packet
      NS_LOG_LOGIC ("nextTime = " << nextTime);
      m_sendEvent = Simulator::ScheduleNow (&VideoPushApplication::PeerLoop, this);
      // NS_LOG_DEBUG("ScheduleNextTx Now");
      m_sendTx = Simulator::Schedule (nextTime, &VideoPushApplication::ScheduleNextTx, this);
    }
  else
    { // All done, cancel any pending events
      StopApplication ();
    }
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
			NS_LOG_LOGIC ("Push packet " << *copy<< " UID "<< packet->GetUid() << " Push Size "<< payload);
			m_txTrace (packet);
			m_socket->SendTo(packet, 0, m_peer);
			m_totBytes += payload;
			m_lastStartTime = Simulator::Now ();
			m_residualBits = 0;
			last_chunk = (last_chunk < m_latestChunkID) ? m_latestChunkID : last_chunk;
			copy->c_tstamp = Simulator::Now().ToInteger(Time::US) - copy->c_tstamp;
			if(!m_chunks.AddChunk(*copy,CHUNK_RECEIVED_PUSH)){
				NS_ASSERT (m_duplicates.find(copy->c_id) != m_duplicates.end());
				m_duplicates.insert(std::pair<uint32_t , uint32_t> (copy->c_id,0));
				SetChunkDelay(copy->c_id,Seconds(0));
			}
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

void VideoPushApplication::SendHello ()
{

}


{
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

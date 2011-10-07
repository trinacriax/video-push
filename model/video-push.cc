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

#include "video-push.h"

NS_LOG_COMPONENT_DEFINE ("VideoPushApplication");

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
                   UintegerValue (),
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
    .AddAttribute ("OnTime", "A RandomVariable used to pick the duration of the 'On' state.",
                   RandomVariableValue (ConstantVariable (1.0)),
                   MakeRandomVariableAccessor (&VideoPushApplication::m_onTime),
                   MakeRandomVariableChecker ())
    .AddAttribute ("OffTime", "A RandomVariable used to pick the duration of the 'Off' state.",
                   RandomVariableValue (ConstantVariable (1.0)),
                   MakeRandomVariableAccessor (&VideoPushApplication::m_offTime),
                   MakeRandomVariableChecker ())
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
				   EnumValue(RANDOM),
				   MakeEnumAccessor(&VideoPushApplication::m_peerSelection),
				   MakeEnumChecker (RANDOM, "Random peer selection."))
	.AddAttribute ("ChunkPolicy", "Chunk selection algorithm.",
				   EnumValue(LATEST),
				   MakeEnumAccessor(&VideoPushApplication::m_chunkSelection),
				   MakeEnumChecker (LATEST, "Latest useful chunk"))
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                   MakeTraceSourceAccessor (&VideoPushApplication::m_txTrace))
	.AddTraceSource ("Rx", "A packet has been received",
				   MakeTraceSourceAccessor (&VideoPushApplication::m_rxTrace))
  ;
  return tid;
}


VideoPushApplication::VideoPushApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;
  m_localPort = PUSH_PORT;
  m_ipv4 = GetObject<Ipv4>();
  m_connected = false;
  m_residualBits = 0;
  m_lastStartTime = Seconds (0);
  m_totBytes = 0;
  m_totalRx = 0;
  m_latestChunkID = 0;
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

  m_socket = 0;
  m_socketList.clear();
  // chain up
  Application::DoDispose ();
}

// Application Methods
void VideoPushApplication::StartApplication () // Called at time specified by Start
{
  NS_LOG_FUNCTION_NOARGS ();

  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      NS_ASSERT (m_socket != 0);
      int32_t iface = 1;//TODO just one interface!
	  int status;
	  status = m_socket->Bind (InetSocketAddress(Ipv4Address::GetAny (), m_localPort));
	  NS_ASSERT (status != -1);
	  // Bind to any IP address so that broadcasts can be received
      m_socket->SetAllowBroadcast (true);
      m_socket->SetRecvCallback (MakeCallback (&VideoPushApplication::HandleReceive, this));
      m_socket->SetAcceptCallback (
         MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
         MakeCallback (&VideoPushApplication::HandleAccept, this));
      m_socket->SetCloseCallbacks (
         MakeCallback (&VideoPushApplication::HandlePeerClose, this),
         MakeCallback (&VideoPushApplication::HandlePeerError, this));
    }
  // Insure no pending event
  CancelEvents ();
  ScheduleStartEvent ();
}

void VideoPushApplication::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION_NOARGS ();

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
    NS_LOG_DEBUG("Chunks: " << m_chunks.PrintBuffer());
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
      if (InetSocketAddress::IsMatchingType (from))
        {
          m_totalRx += packet->GetSize ();
          ChunkHeader chunkH;
          packet->RemoveHeader(chunkH);
          ChunkVideo *chunk = chunkH.GetChunk();
          NS_LOG_INFO("Received chunk "<<*chunk);
          InetSocketAddress address = InetSocketAddress::ConvertFrom (from);
          NS_LOG_INFO ("Node " <<m_node->GetId()<< " Ip " << Ipv4Address::ConvertFrom(m_localAddress) << " Received " << packet->GetSize () << " bytes from " <<
                       address.GetIpv4 () << " [" << address << "]"
                                   << " total Rx " << m_totalRx);
                                   Ptr<NetDevice> pt = socket->GetBoundNetDevice();
          uint32_t port = address.GetPort();
          Ipv4Address senderAddr = address.GetIpv4 ();
          // Update Neighbors START
          Neighbor *sender = new Neighbor(senderAddr, port);
          if(!m_neighbors.IsNeighbor(*sender)){
          	m_neighbors.AddNeighbor(*sender);
          }
          NeighborData *ndata = m_neighbors.GetNeighbor(*sender);
          ndata->n_contact = Simulator::Now();
          ndata->n_latestChunk = chunk->c_id;
		  // Update Neighbors END

		  // Update Chunk Buffer START
		  if(!m_chunks.AddChunk(*chunk)){
		  	NS_LOG_DEBUG("Chunk " << chunk->c_id <<" already received " << (chunk==m_chunks.GetChunk(chunk->c_id)));
		  }
		  // Update ChunkBuffer END

          //cast address to void , to suppress 'address' set but not used
          //compiler warning in optimized builds
          (void) address;
        }
      m_rxTrace (packet, from);
    }
}

void VideoPushApplication::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_INFO ("VideoPush, peerClose");
}

void VideoPushApplication::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_INFO ("VideoPush, peerError");
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
  Simulator::Cancel (m_startStopEvent);
}

// Event handlers
void VideoPushApplication::StartSending ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_lastStartTime = Simulator::Now ();
  ScheduleStopEvent ();
  ScheduleNextTx ();  // Schedule the send packet event
}

void VideoPushApplication::StopSending ()
{
  NS_LOG_FUNCTION_NOARGS ();
  CancelEvents ();

  ScheduleStartEvent ();
}

// Private helpers
void VideoPushApplication::ScheduleNextTx ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    {
      uint32_t bits = m_pktSize * 8 - m_residualBits;
      NS_LOG_LOGIC ("bits = " << bits);
      Time nextTime (Seconds (bits / static_cast<double>(m_cbrRate.GetBitRate ()))); // Time till next packet
      NS_LOG_LOGIC ("nextTime = " << nextTime);
      m_sendEvent = Simulator::Schedule (nextTime, &VideoPushApplication::PeerLoop, this);
    }
  else
    { // All done, cancel any pending events
      StopApplication ();
    }
}

void VideoPushApplication::ScheduleStartEvent ()
{  // Schedules the event to stop sending data (switch to "Off" state)
  NS_LOG_FUNCTION_NOARGS ();

  Time onInterval = Seconds (m_onTime.GetValue ());
  NS_LOG_LOGIC ("start at " << onInterval);
  m_startStopEvent = Simulator::Schedule (onInterval, &VideoPushApplication::StartSending, this);
}

void VideoPushApplication::ScheduleStopEvent ()
{  // Schedules the event to start sending data (switch to the "On" state)
  NS_LOG_FUNCTION_NOARGS ();

  Time offInterval = Seconds (m_offTime.GetValue ());
  NS_LOG_LOGIC ("stop at " << offInterval);
  m_startStopEvent = Simulator::Schedule (offInterval, &VideoPushApplication::StopSending, this);
}

void VideoPushApplication::PeerLoop ()
{
  NS_LOG_FUNCTION_NOARGS ();
  Time tx = Time::FromDouble(UniformVariable().GetValue(),Time::MS);
  if(m_peerType == SOURCE){
	  NS_LOG_DEBUG("PTx @ "<< tx.GetSeconds()<<"s");
	  Simulator::Schedule (tx, &VideoPushApplication::SendPacket, this);
	  tx = Time::FromDouble(tx.GetSeconds()*1.02,Time::S);
	  NS_LOG_DEBUG("NTx @ "<< tx.GetSeconds()<<"s");
	  Simulator::Schedule (tx, &VideoPushApplication::ScheduleNextTx, this);
  }
}

void VideoPushApplication::SendHello ()
{

}

ChunkVideo* VideoPushApplication::ChunkSelection(){
	uint64_t tstamp = Simulator::Now().GetMilliSeconds();
	ChunkVideo cv(m_latestChunkID,tstamp,m_pktSize,0);
	NS_LOG_DEBUG("Chunk "<< cv);
	ChunkVideo *copy = cv.Copy();
	return copy;
}

void VideoPushApplication::SendPacket ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (m_sendEvent.IsExpired ());
  ChunkVideo *copy = ChunkSelection();
  ChunkHeader chunk = ChunkHeader (*copy);
  uint32_t payload = 0; //copy->c_size+copy->c_attributes_size;//data and attributes already in chunk header;
  Ptr<Packet> packet = Create<Packet> (payload);
  packet->AddHeader(chunk);
  NS_LOG_LOGIC ("Push packet at " << Simulator::Now ()<< " UID "<< packet->GetUid() << " Push Size "<< packet->GetSize());
  m_txTrace (packet);
  m_socket->SendTo(packet, 0, m_peer);
  m_totBytes += m_pktSize;
  m_lastStartTime = Simulator::Now ();
  m_residualBits = 0;
}

void VideoPushApplication::ConnectionSucceeded (Ptr<Socket>)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_connected = true;
  ScheduleStartEvent ();
}

void VideoPushApplication::ConnectionFailed (Ptr<Socket>)
{
  NS_LOG_FUNCTION_NOARGS ();
  cout << "VideoPush, Connection Failed" << endl;
}

} // Namespace ns3

/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 University of Trento, Italy
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
 *
 */

#define NS_LOG_APPEND_CONTEXT                                   \
  if (GetObject<Node> ()) { std::clog << "[node " << GetObject<Node> ()->GetId () << "] "; }

#include "video-push.h"

#include <ns3/log.h>
#include <ns3/address.h>
#include <ns3/node.h>
#include <ns3/nstime.h>
#include <ns3/data-rate.h>
#include <ns3/random-variable.h>
#include <ns3/socket.h>
#include <ns3/simulator.h>
#include <ns3/socket-factory.h>
#include <ns3/packet.h>
#include <ns3/uinteger.h>
#include <ns3/double.h>
#include <ns3/pointer.h>
#include <ns3/enum.h>
#include <ns3/boolean.h>
#include <ns3/trace-source-accessor.h>
#include <ns3/udp-socket-factory.h>
#include <ns3/address-utils.h>
#include <ns3/inet-socket-address.h>
#include <ns3/udp-socket.h>
#include <ns3/snr-tag.h>
#include <memory.h>
#include <math.h>
#include <stdio.h>

NS_LOG_COMPONENT_DEFINE("VideoPushApplication");

static uint32_t m_latestChunkID;

namespace ns3
{

  NS_OBJECT_ENSURE_REGISTERED(VideoPushApplication);

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
                     MakeEnumChecker (PS_RANDOM, "Random peer selection.",
                                      PS_SINR, "SINR based selection.",
                                      PS_DELAY, "Delay based selection.",
                                      PS_ROUNDROBIN, "RoundRobin selection.",
                                      PS_BROADCAST, "Pull is sent in broadcast."))
      .AddAttribute ("ChunkPolicy", "Chunk selection algorithm.",
                     EnumValue(CS_LATEST),
                     MakeEnumAccessor(&VideoPushApplication::m_chunkSelection),
                     MakeEnumChecker (CS_LATEST, "Latest chunk",
                                      CS_LEAST_MISSED, "Least missed",
                                      CS_LATEST_MISSED, "Latest missed",
                                      CS_NEW_CHUNK, "New chunks"))
      .AddAttribute ("PullTime", "Time between two consecutive pulls.",
                     TimeValue (MilliSeconds (50)),
                     MakeTimeAccessor (&VideoPushApplication::SetPullTime,
                                       &VideoPushApplication::GetPullTime),
                     MakeTimeChecker ())
      .AddAttribute ("HelloTime", "Hello Time.",
                     TimeValue (Seconds (10)),
                     MakeTimeAccessor (&VideoPushApplication::SetHelloTime,
                                       &VideoPushApplication::GetHelloTime),
                     MakeTimeChecker ())
      .AddAttribute ("PullMax", "Max number of pull.",
                     UintegerValue (1),
                     MakeUintegerAccessor (&VideoPushApplication::SetPullMax,
                                           &VideoPushApplication::GetPullMax),
                     MakeUintegerChecker<uint32_t> (0))
      .AddAttribute ("PullActive", "Pull activation.",
                     BooleanValue (false),
                     MakeBooleanAccessor (&VideoPushApplication::SetPullActive,
                                          &VideoPushApplication::GetPullActive),
                     MakeBooleanChecker() )
      .AddAttribute ("PullWindow", "Pull window.",
                     UintegerValue (50),
                     MakeUintegerAccessor (&VideoPushApplication::SetPullWindow,
                                           &VideoPushApplication::GetPullWindow),
                     MakeUintegerChecker<uint32_t> (50))
      .AddAttribute ("PullRatioMin", "Min ratio to activate pull.",
                     DoubleValue (0.80),
                     MakeDoubleAccessor (&VideoPushApplication::SetPullRatioMin,
                                         &VideoPushApplication::GetPullRatioMin),
                     MakeDoubleChecker<double> (0.50, .90))
      .AddAttribute ("PullRatioMax", "Max ratio to stop pull.",
                     DoubleValue (0.90),
                     MakeDoubleAccessor (&VideoPushApplication::SetPullRatioMax,
                                         &VideoPushApplication::GetPullRatioMax),
                     MakeDoubleChecker<double> (0.80, 1.0))
      .AddAttribute ("HelloLoss", "Number of allowed hello loss.",
                     UintegerValue (1),
                     MakeUintegerAccessor (&VideoPushApplication::SetHelloLoss,
                                           &VideoPushApplication::GetHelloLoss),
                     MakeUintegerChecker<uint32_t> (0))
      .AddAttribute ("Source", "Source IP.",
                     Ipv4AddressValue (Ipv4Address::GetAny()),
                     MakeIpv4AddressAccessor (&VideoPushApplication::SetSource,
                                              &VideoPushApplication::GetSource),
                     MakeIpv4AddressChecker())
      .AddAttribute ("HelloActive", "Hello activation.",
                     UintegerValue (0),
                     MakeUintegerAccessor (&VideoPushApplication::SetHelloActive,
                                           &VideoPushApplication::GetHelloActive),
                     MakeUintegerChecker<uint32_t> (0, 1))
      .AddAttribute ("SelectionWeight", "Neighbor selection weight (p * W) + (1-p) * (%ChunkReceived).",
                     DoubleValue (0),
                     MakeDoubleAccessor (&VideoPushApplication::n_selectionWeight),
                     MakeDoubleChecker<double> (0))
      .AddAttribute ("MaxPullReply", "Max number of pull to reply.",
                     UintegerValue (1),
                     MakeUintegerAccessor (&VideoPushApplication::SetPullReplyMax,
                                           &VideoPushApplication::GetPullReplyMax),
                     MakeUintegerChecker<uint32_t> (0))
      .AddAttribute ("ChunkDelay", "Chunk Delay Trace",
                     PointerValue (),
                     MakePointerAccessor (&VideoPushApplication::m_delay),
                     MakePointerChecker<TimeMinMaxAvgTotalCalculator>())
      .AddTraceSource ("VideoTxData", "A data packet has been sent in push",
                       MakeTraceSourceAccessor (&VideoPushApplication::m_txDataTrace))
      .AddTraceSource ("VideoRxData", "A packet has been received in push",
                       MakeTraceSourceAccessor (&VideoPushApplication::m_rxDataTrace))
      .AddTraceSource ("VideoTxControl", "A new packet is created and is sent",
                       MakeTraceSourceAccessor (&VideoPushApplication::m_txControlTrace))
      .AddTraceSource ("VideoRxControl", "A packet has been received",
                       MakeTraceSourceAccessor (&VideoPushApplication::m_rxControlTrace))
      .AddTraceSource ("VideoTxPull", "A new pull has been sent",
                       MakeTraceSourceAccessor (&VideoPushApplication::m_txControlPullTrace))
      .AddTraceSource ("VideoRxPull", "A new pull has been received",
                       MakeTraceSourceAccessor (&VideoPushApplication::m_rxControlPullTrace))
      .AddTraceSource ("VideoTxDataPull", "A data packet has been sent in reply to a pull request",
                       MakeTraceSourceAccessor (&VideoPushApplication::m_txDataPullTrace))
      .AddTraceSource ("VideoRxDataPull", "A data packet has been received after a pull request",
                       MakeTraceSourceAccessor (&VideoPushApplication::m_rxDataPullTrace))
      .AddTraceSource ("VideoNeighborTrace", "Neighbors",
                       MakeTraceSourceAccessor (&VideoPushApplication::m_neighborsTrace))
      .AddTraceSource ("VideoPullStart", "Pull start",
                       MakeTraceSourceAccessor (&VideoPushApplication::m_pullStartTrace))
      .AddTraceSource ("VideoPullStop", "Pull stop",
                       MakeTraceSourceAccessor (&VideoPushApplication::m_pullStopTrace))
    ;
    return tid;
  }

  VideoPushApplication::VideoPushApplication () :
      m_socket(0), m_localAddress(Ipv4Address::GetAny()), m_localPort(0), m_peerType(PEER), m_ipv4(0),
      m_source(Ipv4Address::GetAny()), m_gateway(Ipv4Address::GetAny()), m_totalRx(0), m_connected(false), m_pktSize(0),
      m_residualBits(0), m_lastStartTime(0), m_maxBytes(0), m_totBytes(0), m_playoutWindow(0), m_chunkRatioMin(0),
      m_chunkRatioMax(0), m_pullActive(false), m_pullSlot(0), m_pullSlotStart(0), m_pullChunkMissed(0),
      m_pullReplyMax(0), m_pullReplyCurrent(0), m_pullReplyTimer(Timer::CANCEL_ON_DESTROY), m_pullTimeout(0),
      m_pullTimer(Timer::CANCEL_ON_DESTROY), m_pullRetriesMax(0), m_pullWBase(0), m_playout(Timer::CANCEL_ON_DESTROY),
      m_statisticsPullRequest(0), m_statisticsPullReceived(0), m_statisticsPullReply(0), m_statisticsPullHit(0),
      m_helloActive(0), m_helloTime(0), m_helloTimer(Timer::CANCEL_ON_DESTROY), m_helloLoss(0),
      m_peerSelection(PS_RANDOM), m_chunkSelection(CS_LATEST), n_selectionWeight(0), m_delay(0)

  {
    NS_LOG_FUNCTION_NOARGS ();
    m_socketList.clear();
    m_pullRetriesCurrent.clear();
    m_pullTimes.clear();
    m_pullPending.clear();
    m_duplicates.clear();
    m_chunk_delay.clear();
    m_neighbors.Clear();

    m_helloEvent.Cancel();
    m_loopEvent.Cancel();
    m_pullEvent.Cancel();
    m_chunkEvent.Cancel();
    m_pullSlotEvent.Cancel();
  }

  VideoPushApplication::~VideoPushApplication ()
  {
  }

  void
  VideoPushApplication::SetMaxBytes (uint32_t maxBytes)
  {
    NS_LOG_FUNCTION (this << maxBytes);
    m_maxBytes = maxBytes;
  }

  uint32_t
  VideoPushApplication::GetTotalRx () const
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
    StatisticChunk();
    m_socket = 0;
    m_socketList.clear();
    Application::DoDispose();
  }

  void
  VideoPushApplication::StatisticChunk (void)
  {
    NS_LOG_FUNCTION_NOARGS ();
    std::map<uint32_t, ChunkVideo> current_buffer = m_chunks.GetChunkBuffer();
    uint32_t received = 1, receivedpull = 0, receivedpush = 0, delayed = 0, missed = 0, duplicates = 0, chunkID = 0,
        current = 1, split = 0, splitP = 0, splitL = 0;
    uint64_t delaylate = 0, delayavg = 0, delayavgpush = 0, delayavgpull = 0;
    uint64_t delaymax = (current_buffer.empty() ? 0 : GetChunkDelay(current_buffer.begin()->first).GetMicroSeconds()),
        delaymin = (current_buffer.empty() ? 0 : GetChunkDelay(current_buffer.begin()->first).GetMicroSeconds());
    uint32_t missing[] =
      { 0, 0, 0, 0, 0, 0 }, hole = 0; // hole size = 1 2 3 4 5 >5
    Time delay_max, delay_min, delay_avg, delay_avg_push, delay_avg_pull;
    double miss = 0.0, rec = 0.0, dups = 0.0, sigma = 0.0, sigmaP = 0.0, sigmaL = 0.0, delayavgB = 0.0, delayavgP = 0.0,
        delayavgL = 0.0, dlate = 0.0;
    for (std::map<uint32_t, ChunkVideo>::iterator iter = current_buffer.begin(); iter != current_buffer.end(); iter++)
      {
        chunkID = iter->first;
        current = received + missed;
        while (current < chunkID)
          {
            NS_ASSERT(!m_chunks.HasChunk(current));
            missed++;
            hole++;
            current = received + missed;
          }
        if (hole != 0)
          {
            hole = hole > 5 ? 5 : hole;
            missing[hole - 1]++;
            hole = 0;
          }
        duplicates += GetDuplicate(current);
        NS_ASSERT(m_chunks.HasChunk(current));
        uint64_t chunk_timestamp = GetChunkDelay(current).GetMicroSeconds();
        delaymax = (chunk_timestamp > delaymax) ? chunk_timestamp : delaymax;
        delaymin = (chunk_timestamp < delaymin) ? chunk_timestamp : delaymin;
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
              delayed++;
              delaylate += GetChunkDelay(current).GetMicroSeconds();
              break;
            }
          }
        if (delayavg != 0 && received > 0 && received % 1000 == 0)
          {
            delayavgB += (delayavg / 1000.0);
            split++;
            delayavg = 0;
          }
        if (delayavgpush != 0 && receivedpush > 0 && receivedpush % 1000 == 0)
          {
            delayavgP += (delayavgpush / 1000.0);
            splitP++;
            delayavgpush = 0;
          }
        if (delayavgpull != 0 && receivedpull > 0 && receivedpull % 1000 == 0)
          {
            delayavgL += (delayavgpull / 1000.0);
            splitL++;
            delayavgpull = 0;
          }
//	  NS_LOG_DEBUG ("Node " << GetNode()->GetId() << " Dup("<<current<<")="<<GetDuplicate (current)<< " Delay("<<current<<")="<< chunk_timestamp
//            <<" Max="<<delay_max.GetMicroSeconds()<< " Min="<<delay_min.GetMicroSeconds()<<" Avg="<< delay_avg.GetMicroSeconds()<<" Rec="<<received<<" Mis="<<missed);
        received++;
      }
    received--;
    delayavgB = ((delayavgB / (1.0 * (split > 0 ? split : 1)))
        + ((1.0 * delayavg) / (received % 1000 == 0 ? 1 : received % 1000)))
        / (split > 0 && received % 1000 != 0 ? 2 : 1);
    delayavgP = ((delayavgP / (1.0 * (splitP > 0 ? splitP : 1)))
        + ((1.0 * delayavgpush) / (receivedpush % 1000 == 0 ? 1 : receivedpush % 1000)))
        / (splitP > 0 && receivedpush % 1000 != 0 ? 2 : 1);
    delayavgL = ((delayavgL / (1.0 * (splitL > 0 ? splitL : 1)))
        + ((1.0 * delayavgpull) / (receivedpull % 1000 == 0 ? 1 : receivedpull % 1000)))
        / (splitL > 0 && receivedpull % 1000 != 0 ? 2 : 1);
    delay_max = Time::FromInteger(delaymax, Time::US);
    delay_min = Time::FromInteger(delaymin, Time::US);
    current = received + missed;
    while ((current = received + missed) < m_latestChunkID)
      missed++;
    double actual = received - missed;
    actual = (actual <= 0 ? 1 : (actual));
    delay_avg = Time::FromDouble(delayavgB, Time::US);
    delay_avg_push = Time::FromDouble(delayavgP, Time::US);
    delay_avg_pull = Time::FromDouble(delayavgL, Time::US);
    for (std::map<uint32_t, ChunkVideo>::iterator iter = current_buffer.begin(); iter != current_buffer.end(); iter++)
      {
        double t_dev = ((GetChunkDelay(iter->second.c_id) - delay_avg).ToDouble(Time::US));
        sigma += pow(t_dev, 2);
        switch (m_chunks.GetChunkState(iter->second.c_id))
          {
          case CHUNK_RECEIVED_PUSH:
            {
              t_dev = ((GetChunkDelay(iter->second.c_id) - delay_avg_push).ToDouble(Time::US));
              sigmaP += pow(t_dev, 2);
              break;
            }
          case CHUNK_RECEIVED_PULL:
            {
              t_dev = ((GetChunkDelay(iter->second.c_id) - delay_avg_pull).ToDouble(Time::US));
              sigmaL += pow(t_dev, 2);
              break;
            }
          default:
            {
              break;
            }
          }
      }
    NS_ASSERT(received == (delayed+receivedpush+receivedpull));
    sigma = sqrt(sigma / (1.0 * (receivedpush + receivedpull)));
    sigmaP = sqrt(sigmaP / (1.0 * receivedpush));
    sigmaL = sqrt(sigmaL / (1.0 * receivedpull));
    if (received == 0)
      {
        miss = 1;
        sigma = rec = dups = 0;
        delaylate = 0;
      }
    else
      {
        miss = (missed / (1.0 * m_latestChunkID));
        rec = (received / (1.0 * m_latestChunkID));
        dups = (duplicates == 0 ? 0 : (1.0 * duplicates) / received);
        dlate = (delayed == 0 ? 0 : delaylate / (1.0 * delayed));
      }
    NS_ASSERT(m_latestChunkID==received+missed);
    double tstudent = 1.96; // alpha = 0.025, degree of freedom = infinite
    double confidence = (sigma == 0 ? 1 : tstudent * (sigma / sqrt(received)));
    double confidenceP = tstudent * (sigmaP / sqrt(receivedpush));
    double confidenceL = tstudent * (sigmaL / sqrt(receivedpull));
    if (receivedpush == 0)
      {
        sigmaP = 0;
        confidenceP = 0.0;
        delay_avg_push = MicroSeconds(0);
      }
    if (receivedpull == 0)
      {
        sigmaL = 0;
        confidenceL = 0.0;
        delay_avg_pull = MicroSeconds(0);
      }
    printf(
        "Chunks Node %d Rec %.5f Miss %.5f Dup %.5f K %d Max %ld us Min %ld us Avg %ld us sigma %.5f conf %.5f late %.5f RecP %d AvgP %ld us sigmaP %.5f confP %.5f RecL %d AvgL %ld us sigmaL %.5f confL %.5f PRec %d PRep %.4f PReq %d PHit %.4f H1 %d H2 %d H3 %d H4 %d H5 %d H6 %d\n",
        m_node->GetId(), rec, miss, dups, received, delay_max.ToInteger(Time::US), delay_min.ToInteger(Time::US),
        delay_avg.ToInteger(Time::US), sigma, confidence, dlate, receivedpush, delay_avg_push.ToInteger(Time::US),
        sigmaP, confidenceP, receivedpull, delay_avg_pull.ToInteger(Time::US), sigmaL, confidenceL,
        m_statisticsPullReceived,
        (m_statisticsPullReceived == 0 ? 0 : m_statisticsPullReply / (1.0 * m_statisticsPullReceived)),
        m_statisticsPullRequest,
        (m_statisticsPullRequest == 0 ? 0 : m_statisticsPullHit / (1.0 * m_statisticsPullRequest)), missing[0],
        missing[1], missing[2], missing[3], missing[4], missing[5]);
  }

  uint32_t
  VideoPushApplication::GetApplicationId (void) const
  {
    Ptr<Node> node = GetNode();
    for (uint32_t i = 0; i < node->GetNApplications(); ++i)
      {
        if (node->GetApplication(i) == this)
          {
            return i;
          }
      }
    NS_ASSERT_MSG(false, "forgot to add application to node");
    return 0;
  }

  void
  VideoPushApplication::StartApplication ()
  {
    NS_LOG_FUNCTION_NOARGS ();
    m_ipv4 = m_node->GetObject<Ipv4>();
    CancelEvents();
    if (!m_socket)
      {
        m_socket = Socket::CreateSocket(GetNode(), m_tid);
        NS_ASSERT(m_socket != 0);
        int status;
        status = m_socket->Bind(InetSocketAddress(m_localPort));
        NS_ASSERT(status != -1);
        // NS_LOG_DEBUG("Push Socket "<< m_socket << " to "<<Ipv4Address::GetAny ()<<"::"<< m_localPort);
        // Bind to any IP address so that packets can be received
        m_socket->SetAllowBroadcast(true);
        m_socket->SetRecvCallback(MakeCallback(&VideoPushApplication::HandleReceive, this));
        m_socket->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address &>(),
            MakeCallback(&VideoPushApplication::HandleAccept, this));
        m_socket->SetCloseCallbacks(MakeCallback(&VideoPushApplication::HandlePeerClose, this),
            MakeCallback(&VideoPushApplication::HandlePeerError, this));
        m_pullTimer.SetDelay(GetPullTime());
        m_pullTimer.SetFunction(&VideoPushApplication::PeerLoop, this);
        m_helloTimer.SetDelay(GetHelloTime());
        m_helloTimer.SetFunction(&VideoPushApplication::SendHello, this);
        Time start = Time::FromDouble(UniformVariable().GetValue(0, GetHelloTime().GetMilliSeconds() * .5), Time::MS);
        if (GetHelloActive())
          {
            m_helloEvent = Simulator::Schedule(start, &VideoPushApplication::SendHello, this);
          }
        m_neighbors.SetExpire(Time::FromDouble(GetHelloTime().GetSeconds() * (1.10 * (1.0 + GetHelloLoss())), Time::S));
        m_neighbors.SetSelectionWeight(n_selectionWeight);
        double inter_time = 1 / (m_cbrRate.GetBitRate() / (8.0 * m_pktSize));
        m_pullSlot = Time::FromDouble(inter_time, Time::S);
        m_playout.SetDelay(Time::FromDouble(inter_time, Time::S));
        m_playout.SetFunction(&VideoPushApplication::UpdatePullWBase, this);
        m_pullReplyTimer.SetDelay(m_pullSlot);
        m_pullReplyTimer.SetFunction(&VideoPushApplication::ResetPullReplyCurrent, this);
      }
    StartSending();
  }

  void
  VideoPushApplication::StopApplication ()
  {
    NS_LOG_FUNCTION_NOARGS ();
    StopSending();
    while (!m_socketList.empty())
      {
        Ptr<Socket> acceptedSocket = m_socketList.front();
        m_socketList.pop_front();
        acceptedSocket->Close();
      }
    CancelEvents();
    if (m_socket)
      {
        m_socket->Close();
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> >());
      }
    else
      {
        NS_LOG_WARN ("VideoPush found null socket to close in StopApplication");
      }
  }

  void
  VideoPushApplication::HandlePeerClose (Ptr<Socket> socket)
  {
    NS_LOG_DEBUG ("VideoPush, peerClose");
  }

  void
  VideoPushApplication::HandlePeerError (Ptr<Socket> socket)
  {
    NS_LOG_DEBUG ("VideoPush, peerError");
  }

  void
  VideoPushApplication::HandleAccept (Ptr<Socket> s, const Address& from)
  {
    NS_LOG_FUNCTION (this << s << from);
    s->SetRecvCallback(MakeCallback(&VideoPushApplication::HandleReceive, this));
    m_socketList.push_back(s);
  }

  void
  VideoPushApplication::CancelEvents ()
  {
    NS_LOG_FUNCTION_NOARGS ();

    if (m_chunkEvent.IsRunning())
      { // Cancel the pending send packet event
        // Calculate residual bits since last packet sent
        Time delta(Simulator::Now() - m_lastStartTime);
        int64x64_t bits = delta.To(Time::S) * m_cbrRate.GetBitRate();
        m_residualBits += bits.GetHigh();
      }
    Simulator::Cancel(m_helloEvent);
    Simulator::Cancel(m_loopEvent);
    Simulator::Cancel(m_pullEvent);
    Simulator::Cancel(m_pullSlotEvent);
    Simulator::Cancel(m_chunkEvent);
  }

  void
  VideoPushApplication::StartSending ()
  {
    NS_LOG_FUNCTION_NOARGS ();
    m_lastStartTime = Simulator::Now();
    switch (m_peerType)
      {
      case PEER:
        {
          break;
        }
      case SOURCE:
        {
          m_loopEvent = Simulator::ScheduleNow(&VideoPushApplication::PeerLoop, this);
          break;
        }
      }
  }

  void
  VideoPushApplication::StopSending ()
  {
    NS_LOG_FUNCTION_NOARGS ();
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

  uint32_t
  VideoPushApplication::GetPullReplyCurrent () const
  {
    return m_pullReplyCurrent;
  }

  void
  VideoPushApplication::SetPullReplyCurrent (uint32_t value)
  {
    m_pullReplyCurrent = value;
  }

  void
  VideoPushApplication::AddPullReplyCurrent ()
  {
    m_pullReplyCurrent++;
  }

  void
  VideoPushApplication::ResetPullReplyCurrent ()
  {
    SetPullReplyCurrent(0);
    if (m_pullReplyTimer.IsRunning())
      m_pullReplyTimer.Cancel();
    m_pullReplyTimer.Schedule();
  }

  uint32_t
  VideoPushApplication::GetPullReplyMax () const
  {
    return m_pullReplyMax;
  }

  void
  VideoPushApplication::SetPullReplyMax (uint32_t value)
  {
    m_pullReplyMax = value;
  }

  void
  VideoPushApplication::SetHelloActive (uint32_t hello)
  {
    m_helloActive = hello;
  }

  uint32_t
  VideoPushApplication::GetHelloActive () const
  {
    return m_helloActive;
  }

  void
  VideoPushApplication::SetPullTime (Time pullt)
  {
    NS_ASSERT(pullt.GetSeconds()> 0);
    m_pullTimeout = pullt;
  }

  Time
  VideoPushApplication::GetPullTime () const
  {
    return m_pullTimeout;
  }

  void
  VideoPushApplication::SetHelloTime (Time hellot)
  {
    NS_ASSERT(hellot.GetSeconds() > 0);
    m_helloTime = hellot;
  }

  Time
  VideoPushApplication::GetHelloTime () const
  {
    return m_helloTime;
  }

  void
  VideoPushApplication::SetPullMax (uint32_t max)
  {
    NS_ASSERT(max > 0);
    m_pullRetriesMax = max;
  }

  uint32_t
  VideoPushApplication::GetPullMax () const
  {
    return m_pullRetriesMax;
  }

  void
  VideoPushApplication::SetPullWindow (uint32_t window)
  {
    m_playoutWindow = window;
  }

  uint32_t
  VideoPushApplication::GetPullWindow () const
  {
    return m_playoutWindow;
  }

  void
  VideoPushApplication::UpdatePullWBase ()
  {
    m_pullWBase++;
    m_playout.Schedule();
  }

  void
  VideoPushApplication::SetPullWBase (uint32_t base)
  {
    NS_ASSERT(base >= 0);
    m_pullWBase = base;
  }

  uint32_t
  VideoPushApplication::GetPullWBase ()
  {
    return m_pullWBase;
  }

  void
  VideoPushApplication::SetPullRatioMin (double ratio)
  {
    m_chunkRatioMin = ratio;
  }

  double
  VideoPushApplication::GetPullRatioMin () const
  {
    return m_chunkRatioMin;
  }

  void
  VideoPushApplication::SetPullRatioMax (double ratio)
  {
    m_chunkRatioMax = ratio;
  }

  double
  VideoPushApplication::GetPullRatioMax () const
  {
    return m_chunkRatioMax;
  }

  bool
  VideoPushApplication::InPullRange ()
  {
    double low = GetReceived(CHUNK_RECEIVED_PUSH);
    bool active = (low >= GetPullRatioMin() && low <= GetPullRatioMax());
    if (active)
      m_pullStartTrace(low);
    else
      m_pullStopTrace(low);
    return active;
  }

  void
  VideoPushApplication::SetPullTimes (uint32_t chunkid)
  {
    NS_LOG_DEBUG("LOADING "<<chunkid);
    if (m_pullTimes.find(chunkid) == m_pullTimes.end())
      {
        m_pullTimes.insert(std::pair<uint32_t, Time> (chunkid, Simulator::Now()));
      }
  }

  void
  VideoPushApplication::SetPullTimes (uint32_t chunkid, Time time)
  {
    NS_LOG_DEBUG("LOADING "<<chunkid);
    if (m_pullTimes.find(chunkid) == m_pullTimes.end())
      {
        m_pullTimes.insert(std::pair<uint32_t, Time> (chunkid, Seconds(time)));
      }
  }

  Time
  VideoPushApplication::GetPullTimes (uint32_t chunkid)
  {
    NS_LOG_DEBUG("REMLOADING "<<chunkid);
    return (m_pullTimes.find(chunkid)!= m_pullTimes.end()?m_pullTimes.find(chunkid)->second:Seconds(0));
  }

  Time
  VideoPushApplication::RemPullTimes (uint32_t chunkid)
  {
    NS_LOG_DEBUG("REMLOADING "<<chunkid);
    Time p = (m_pullTimes.find(chunkid)!= m_pullTimes.end()?m_pullTimes.find(chunkid)->second:Seconds(0));
    if(m_pullTimes.find(chunkid)!= m_pullTimes.end())
      {
        m_pullTimes.erase(chunkid);
      }
    return p;
  }

  bool
  VideoPushApplication::Pulled (uint32_t chunkid)
  {
    NS_LOG_DEBUG("Pulled "<<chunkid);
    return (m_pullTimes.find(chunkid) != m_pullTimes.end());
  }

  void
  VideoPushApplication::SetChunkDelay (uint32_t chunkid, Time delay)
  {
    NS_ASSERT(chunkid>0);
    NS_ASSERT(m_chunks.GetChunkState(chunkid) != CHUNK_DELAYED);
    uint64_t udelay = delay.GetMicroSeconds();
    NS_ASSERT(delay.GetMicroSeconds() >= 0);
    NS_ASSERT(m_chunk_delay.find(chunkid) == m_chunk_delay.end());
    m_chunk_delay.insert(std::pair<uint32_t, uint64_t>(chunkid, udelay));
  }

  Time
  VideoPushApplication::GetChunkDelay (uint32_t chunkid)
  {
    NS_ASSERT(chunkid>0);
    NS_ASSERT(m_chunks.HasChunk(chunkid) || m_chunks.GetChunkState(chunkid) == CHUNK_DELAYED);
    NS_ASSERT(m_chunk_delay.find(chunkid) != m_chunk_delay.end());
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

  Time
  VideoPushApplication::GetPullSlot () const
  {
    return m_pullSlot;
  }

  Time
  VideoPushApplication::GetPullSlotStart () const
  {
    return m_pullSlotStart;
  }

  void
  VideoPushApplication::SetPullSlotStart (Time start)
  {
    m_pullSlotStart = start + LPULLGUARD;
    if (m_pullSlotEvent.IsRunning())
      m_pullSlotEvent.Cancel();
    /// Schedule the next pull start
    Time nextStart = start + GetPullSlot();
    Time delay = GetPullSlot() - RPULLGUARD;
    NS_LOG_DEBUG ("Now "<< Simulator::Now()<< " Start="<<m_pullSlotStart
        <<" End="<<GetPullSlotEnd()<< " Slot="<<GetPullSlot()
        <<" Delay="<<delay<<" Next="<<nextStart);
    m_pullSlotEvent = Simulator::Schedule(delay, &VideoPushApplication::SetPullSlotStart, this, nextStart);
  }

  Time
  VideoPushApplication::GetPullSlotEnd () const
  {
    return GetPullSlotStart() + GetPullSlot() - LPULLGUARD - RPULLGUARD;
  }

  double
  VideoPushApplication::PullSlot ()
  {
    NS_LOG_DEBUG ("Node=" <<m_node->GetId()<< " NextSlot="<<GetPullSlotEnd().GetSeconds());
    Time now = Simulator::Now();
    double r = 1.0;
    if (now >= GetPullSlotStart() && now <= GetPullSlotEnd())
      {
        NS_ASSERT(GetPullSlot () > RPULLGUARD + LPULLGUARD);
        double n = (now - GetPullSlotStart()).ToDouble(Time::US);
        double m = (GetPullSlot() - RPULLGUARD - LPULLGUARD).ToDouble(Time::US);
        r = n / m;
        NS_LOG_DEBUG ("Slot ratio " <<n<< "/"<< m<<"="<<r);
      }
    return r;
  }

  Ipv4Address
  VideoPushApplication::GetLocalAddress ()
  {
    return Ipv4Address::ConvertFrom(m_localAddress);
  }

  double
  VideoPushApplication::GetReceived (enum ChunkState state)
  {
    uint32_t last = m_chunks.GetLastChunk();
    uint32_t base = GetPullWBase();
    uint32_t window = GetPullWindow();
    window = last < window ? 1 : window;
    double ratio = 0.0;
    for (uint32_t i = base; base > 0 && i < (base + window); i++)
      {
        ratio += (m_chunks.GetChunkState(i) == state ? 1.0 : 0.0);
      }
    ratio = (ratio / window);
    return ratio;
  }

  void
  VideoPushApplication::SetChunkMissed (uint32_t chunkid)
  {
    NS_LOG_FUNCTION(this<<chunkid);
    NS_ASSERT(!chunkid||!m_chunks.HasChunk(chunkid));
    NS_ASSERT(!chunkid||m_chunks.GetChunkState(chunkid)!=CHUNK_DELAYED);
    NS_ASSERT(!chunkid||m_chunks.GetChunkState(chunkid)!=CHUNK_SKIPPED);
    NS_ASSERT(!chunkid||m_chunks.GetChunkState(chunkid)==CHUNK_MISSED);
    m_pullChunkMissed = chunkid;
  }

  uint32_t
  inline
  VideoPushApplication::GetChunkMissed () const
  {
    return m_pullChunkMissed;
  }

  void
  VideoPushApplication::PeerLoop ()
  {
    NS_LOG_FUNCTION (Simulator::Now());
    switch (m_peerType)
      {
      case PEER:
        {
          NS_LOG_DEBUG ("Node " <<m_node->GetId()<<" PULLSTART");
          NS_ASSERT(GetPullActive());
          NS_ASSERT(GetHelloActive());
          NS_ASSERT(!m_pullTimer.IsRunning());
          NS_ASSERT(!m_pullEvent.IsRunning());
          /* There is a missed chunk*/
          while (GetChunkMissed()
              && (GetPullRetryCurrent(GetChunkMissed()) >= GetPullMax() || GetChunkMissed() < GetPullWBase()))/* Mark chunks as skipped*/
            {
              uint32_t lastmissed = GetChunkMissed();
              NS_ASSERT(m_chunks.GetChunkState(lastmissed)==CHUNK_MISSED);
              m_chunks.SetChunkState(lastmissed, CHUNK_SKIPPED); // Mark as skipped
              NS_ASSERT(m_chunks.GetChunkState(lastmissed)==CHUNK_SKIPPED);
              RemPullTimes(lastmissed); // Remove the chunk form PullTimes
              SetPullTimes(lastmissed, Seconds(0));
              SetChunkMissed(ChunkSelection(m_chunkSelection)); // Update chunk missed
              NS_ASSERT (lastmissed != GetChunkMissed());
              NS_LOG_INFO ("Node " <<m_node->GetId()<< " is marking chunk "<< lastmissed
                  <<" as skipped ("<<(lastmissed?GetPullRetryCurrent(lastmissed):0)<<"/"<<GetPullMax()<<") New missed="<<GetChunkMissed ());
            }
          SetChunkMissed(ChunkSelection(m_chunkSelection));
          NS_LOG_INFO ("Node " << m_node->GetId() << " IP=" << GetLocalAddress()
              << " Ratio [" << GetReceived(CHUNK_RECEIVED_PUSH) << ":" << GetReceived(CHUNK_RECEIVED_PUSH) + GetReceived(CHUNK_RECEIVED_PULL) << "] ["<<GetPullRatioMin() << ":" << GetPullRatioMax() << "]" << " Total="<< m_chunks.GetSize()
              << " Last=" << m_chunks.GetLastChunk() << " Missed=" << GetChunkMissed() << " ("<<(GetChunkMissed()?GetPullRetryCurrent(GetChunkMissed()):0)<<","<<GetPullMax()<<")"
              << " Wmin=" << GetPullWBase() <<" Wmax="<< GetPullWindow()+GetPullWBase()
              << " Timer="<<(m_pullTimer.IsRunning()?"Yes":"No"));
          if (GetChunkMissed() && InPullRange())/*check whether the node is within Pull-allowed range*/
            {
              Neighbor target = PeerSelection(m_peerSelection);
              m_neighborsTrace(m_neighbors.GetSize());
              NS_ASSERT(!m_pullTimer.IsRunning());
              NS_ASSERT(!m_pullEvent.IsRunning());
              if (target.GetAddress() != Ipv4Address::GetAny())
                {
                  NS_ASSERT(m_neighbors.IsNeighbor(target));
                  Time delay = TransmissionDelay(100, 2000, Time::US); //[0-2000]us random
                  m_pullTimer.Schedule();
                  m_pullEvent = Simulator::Schedule(delay, &VideoPushApplication::SendPull, this, GetChunkMissed(),
                      target.GetAddress());
                  NS_LOG_INFO ("Node " <<m_node->GetId()<< " schedule pull to "<< target.GetAddress()
                      << " for chunk " << GetChunkMissed() <<" ("<< GetPullRetryCurrent(GetChunkMissed())<<") at "
                      << Simulator::Now()+delay << " timeout "<< (Simulator::Now()+m_pullTimer.GetDelay())
                      << " useful time "<< (m_pullTimer.GetDelay()-delay)
                      << " PullTimer "<< m_pullTimer.IsRunning());
                }
              else
                {
                  NS_LOG_DEBUG ("Node " <<m_node->GetId()<< " has no neighbors to pull chunk "<< GetChunkMissed());NS_LOG_DEBUG ("Node " <<m_node->GetId()<<" PULLEND");
                }
            }
          else
            {
              NS_LOG_DEBUG ("Node " <<m_node->GetId()<<" PULLEND "<< GetChunkMissed() << ", "<<InPullRange());
              NS_ASSERT(!m_pullTimer.IsRunning());
              NS_ASSERT(!m_pullEvent.IsRunning());
            }
          break;
        }
      case SOURCE:
        {
          if (m_maxBytes == 0 || m_totBytes < m_maxBytes)
            {
              uint32_t bits = m_pktSize * 8 - m_residualBits;
              NS_LOG_LOGIC ("bits = " << bits);
              Time nextTime(Seconds(bits / static_cast<double>(m_cbrRate.GetBitRate()))); // Time till next packet
              m_chunkEvent = Simulator::ScheduleNow(&VideoPushApplication::SendPacket, this);
              m_loopEvent = Simulator::Schedule(nextTime, &VideoPushApplication::PeerLoop, this);
            }
          else
            { // All done, cancel any pending events
              StopApplication();
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
    NS_ASSERT(m_peerType == PEER);
    ChunkVideo chunk = chunkheader.GetChunk();
    m_totalRx += chunk.GetSize() + chunk.GetAttributeSize();
    bool toolate = (m_chunks.GetChunkState(chunk.c_id) == CHUNK_SKIPPED || chunk.c_id < GetPullWBase()); // chunk has been expired
    bool duplicated = m_chunks.HasChunk(chunk.c_id);
    if (duplicated) // Duplicated chunk
      {
        StatisticAddDuplicateChunk(chunk.c_id);
      }
    else if (GetPullRetryCurrent(chunk.c_id) && toolate) // has been pulled and received too late
      {
        m_chunks.SetChunkState(chunk.c_id, CHUNK_DELAYED);
        NS_LOG_INFO ("Node "<< GetLocalAddress() << " has received too late missed chunk "<< chunk.c_id);NS_LOG_DEBUG ("Node " <<m_node->GetId()<<" PULLEND");
      }
    else
      {
        Time delay = Simulator::Now() - MicroSeconds(chunk.c_tstamp);
        SetChunkDelay(chunk.c_id, delay);
        if (GetPullRetryCurrent(chunk.c_id)) // has been pulled and received in time
          {
            m_chunks.AddChunk(chunk, CHUNK_RECEIVED_PULL);
            NS_ASSERT(sender != GetSource());
            NS_ASSERT(m_pullTimer.IsRunning());
            NS_ASSERT(!m_pullEvent.IsRunning());
            m_pullTimer.Cancel();
            StatisticAddPullHit();
            Time shift = (Simulator::Now() - GetPullTimes(chunk.c_id));
            NS_LOG_INFO ("Node "<< GetLocalAddress() << " has received missed chunk "<< chunk.c_id<< " after "
                << shift.GetSeconds()<< " ~ "<< (shift.GetSeconds()/(1.0*GetPullTime().GetSeconds())));
            /* TODO need min and max values to limit the timeout value.
             * Time pulltimeout = Time::FromDouble(0.5 * GetPullTime().ToDouble(Time::US) + 0.5 * shift.ToDouble(Time::US), Time::US);
             * NS_LOG_INFO ("Node updates its pull timeout from "<< GetPullTime().GetMilliSeconds() << " to "<< pulltimeout.GetMilliSeconds());
             * SetPullTime(pulltimeout);
             */
            SetPullTimes(chunk.c_id, shift);
            NS_LOG_DEBUG ("Node " <<m_node->GetId()<<" PULLEND");
          }
        else
          {
            SetPullSlotStart(Simulator::Now());
            ResetPullReplyCurrent();
            if (m_chunks.GetSize() == 1) // this is the first chunk
              {
                NS_ASSERT(!m_playout.IsRunning());
                double playtime = ( (8.0 * m_pktSize * GetPullWindow()) / m_cbrRate.GetBitRate() );
                m_playout.Schedule(Time::FromDouble(playtime, Time::S));
              }
            NS_ASSERT(sender == GetSource());
            m_chunks.AddChunk(chunk, CHUNK_RECEIVED_PUSH);
          }
      }
    SetChunkMissed(ChunkSelection(m_chunkSelection));
    NS_LOG_INFO ("Node " << GetLocalAddress() << (duplicated?" RecDup ":(toolate?" RecLate ":" Received ")) << chunk.c_id
        <<" from " << sender <<" Ratio ["<<GetReceived(CHUNK_RECEIVED_PUSH)<<":"<<GetReceived(CHUNK_RECEIVED_PULL)<<"]"<<" Wmin=" << GetPullWBase() <<" Wmax="<< GetPullWindow()+GetPullWBase()
        <<" PullTimer "<< m_pullTimer.IsRunning() << "(D="<<m_pullTimer.GetDelay()<<"/N=" << m_pullTimer.GetDelayLeft()<<")"
        <<" #Neighbors "<< m_neighbors.GetSize()
        <<" Missed="<<GetChunkMissed()<< " Chunks="<<m_chunks.GetBufferSize()
        <<" Slot="<<GetPullSlotStart().GetSeconds());
    if (GetPullActive() && GetChunkMissed() && !m_pullTimer.IsRunning() && !m_pullEvent.IsRunning() && InPullRange())
      {
        Time delay(0);
        if (GetPullSlotStart() > Simulator::Now())
          delay = GetPullSlotStart() - Simulator::Now();
        m_pullTimer.Schedule(delay);
        NS_LOG_INFO ("Node " << GetLocalAddress() << " will pull "<<GetChunkMissed()<< " at "<<(Simulator::Now()+delay));
      }
  }

  void
  VideoPushApplication::SendPull (uint32_t chunkid, const Ipv4Address target)
  {
    NS_LOG_FUNCTION (this<<chunkid);
    NS_ASSERT(chunkid>0);
    NS_ASSERT(GetPullActive());
    NS_ASSERT(m_chunks.GetLastChunk()>=GetPullWindow());
    if (PullSlot() < PullReqThr)/*Check whether the node is within a pull slot or not*/
      {
        ChunkHeader pull(MSG_PULL);
        pull.GetPullMessage().SetChunk(chunkid);
        Ptr<Packet> packet = Create<Packet>();
        packet->AddHeader(pull);
        NS_LOG_DEBUG ("Node " << GetNode()->GetId() << " sends pull to "<< target << " for chunk "<< chunkid<< " pid "<< packet->GetUid());
        NS_ASSERT(GetPullSlotStart() <= Simulator::Now() && (GetPullSlotStart() + m_pullSlot) > Simulator::Now());
        NS_ASSERT(Simulator::Now() >= GetPullSlotStart());
        NS_ASSERT(Simulator::Now() <= GetPullSlotEnd());
        AddPullRetryCurrent(chunkid);
        SetPullTimes(chunkid);
        StatisticAddPullRequest();
        //TODO CHECK too late chunks
        NS_ASSERT(chunkid <= (GetPullWBase()+GetPullWindow()));
        m_socket->SendTo(packet, 0, InetSocketAddress(target, PUSH_PORT));
        m_txControlPullTrace(packet);
      }
    else // out of threshold, cancel the PeerLoop
      {
        m_pullTimer.Cancel();
      }
  }

  void
  VideoPushApplication::HandlePull (ChunkHeader::PullMessage &pullheader, const Ipv4Address &sender)
  {
    switch (m_peerType)
      {
      case SOURCE:
        {
          break;
        }
      case PEER:
        {
          NS_ASSERT(GetPullActive());
          NS_ASSERT(m_statisticsPullReceived>=m_statisticsPullReply);
          uint32_t chunkid = pullheader.GetChunk();
          Time now = Simulator::Now();
          bool hasChunk = m_chunks.HasChunk(chunkid);
          Time delay = TransmissionDelay(100, 1500, Time::US);
          StatisticAddPullReceived();
          if (hasChunk && !m_chunkEvent.IsRunning() && GetPullReplyCurrent() <= GetPullReplyMax()
              && PullSlot() < PullRepThr)
            {
              NS_LOG_DEBUG(GetPullSlotStart().GetMicroSeconds()<<" < " << now.GetMicroSeconds() << " < " << GetPullSlotEnd().GetMicroSeconds() << " : "<< (GetPullSlotEnd()-Simulator::Now()).GetMicroSeconds());
              NS_ASSERT(now >= GetPullSlotStart());
              NS_ASSERT(now <= GetPullSlotEnd());
              m_chunkEvent = Simulator::Schedule(delay, &VideoPushApplication::SendChunk, this, chunkid, sender);
              NS_LOG_INFO ("Node " << GetLocalAddress() << " Received pull for " << chunkid << " from " << sender << ", reply in "<<delay.GetSeconds());
            }
          else
            NS_LOG_INFO ("Node " << GetLocalAddress() << " Received pull for " << chunkid << " from " << sender << " NO reply");
          break;
        }
      default:
        {
          NS_ASSERT_MSG(false, "State not valid");
          break;
        }
      }
  }

  void
  VideoPushApplication::SendChunk (uint32_t chunkid, const Ipv4Address target)
  {
    NS_LOG_FUNCTION (this<<chunkid<<target);
    NS_ASSERT(chunkid>0);
    NS_ASSERT(target != GetLocalAddress());
    NS_ASSERT(GetPullActive());
    NS_ASSERT(GetHelloActive());
    switch (m_peerType)
      {
      case PEER:
        {
          NS_ASSERT(!m_chunkEvent.IsRunning());
          ChunkHeader chunk(MSG_CHUNK);
          ChunkVideo *copy = m_chunks.GetChunk(chunkid);
          Ptr<Packet> packet = Create<Packet>(copy->GetSize());
          chunk.GetChunkMessage().SetChunk(*copy);
          packet->AddHeader(chunk);
          NS_LOG_LOGIC ("Node " << GetLocalAddress() << " replies pull to " << target << " for chunk [" << *copy<< "] Size " << packet->GetSize() << " UID "<< packet->GetUid());
          StatisticAddPullReply();
          AddPullReplyCurrent();
          m_txDataPullTrace(packet);
          m_socket->SendTo(packet, 0, InetSocketAddress(target, PUSH_PORT));
          break;
        }
      case SOURCE:
        {
          NS_ASSERT_MSG(false, "source cannot reply to pull");
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
  VideoPushApplication::HandleHello (ChunkHeader::HelloMessage &helloheader, const Ipv4Address &sender)
  {
    switch (m_peerType)
      {
      case SOURCE:
        {
          break;
        }
      case PEER:
        {
          uint32_t n_last = helloheader.GetLastChunk();
          uint32_t n_chunks = helloheader.GetChunksReceived();
          double n_ratio = (helloheader.GetChunksRatio() / 1000.0);
          Ipv4Mask mask("255.0.0.0");
          NS_LOG_DEBUG ("Node " << GetLocalAddress() << " receives broadcast hello from " << sender << " #Chunks="<< n_chunks << " Ratio="<< n_ratio);
          Neighbor nt(sender, PUSH_PORT);
          if (m_neighbors.IsNeighbor(nt))
            {
              m_neighbors.GetNeighbor(nt)->Update(n_last, n_chunks, n_ratio);
              m_neighbors.ClearNeighborhood();
            }
          break;
        }
      default:
        {
          NS_ASSERT_MSG(false, "no valid peer state");
          break;
        }
      }
  }

  void
  VideoPushApplication::HandleReceive (Ptr<Socket> socket)
  {
    switch (m_peerType)
      {
      case SOURCE:
        {
          break;
        }
      case PEER:
        {
          NS_LOG_FUNCTION (this << socket);
          Ptr<Packet> packet;

          Address from;
          while ((packet = socket->RecvFrom(from)))
            {
              if (packet->GetSize() == 0)
                {
                  break;
                }
              SnrTag ptag;
              bool packetTag = packet->RemovePacketTag(ptag);
              InetSocketAddress address = InetSocketAddress::ConvertFrom(from);
              Ipv4Address sourceAddr = address.GetIpv4();
              uint32_t port = address.GetPort();
              Ipv4Mask mask("255.0.0.0");
              Neighbor nt(sourceAddr, port);
              Ipv4Address subnet = GetLocalAddress().GetSubnetDirectedBroadcast(mask);
              NS_LOG_DEBUG("Node " << GetLocalAddress() << " receives packet from "<< sourceAddr
                  << " SINR "<< ptag.GetSinr());
              // Filter on associated access point.
//	      if(gateway!=relayTag.m_sender)
//	      {
//	    	  NS_LOG_DEBUG("Duplicated packet Gateway "<<gateway<< " Sender " << relayTag.m_sender);
//			  break;
//	      }
              NS_ASSERT(sourceAddr != GetLocalAddress());
              if (InetSocketAddress::IsMatchingType(from))
                {
                  ChunkHeader chunkH(MSG_CHUNK);
                  packet->RemoveHeader(chunkH);
                  switch (chunkH.GetType())
                    {
                    case MSG_CHUNK:
                      {
                        if (sourceAddr == GetSource())
                          {
                            m_rxDataTrace(packet, address);
                          }
                        else
                          {
                            m_rxDataPullTrace(packet, address);
                          }
                        HandleChunk(chunkH.GetChunkMessage(), sourceAddr);
                        break;
                      }
                    case MSG_PULL:
                      {
                        NS_ASSERT(GetPullActive());
                        m_rxControlPullTrace(packet, address);
                        HandlePull(chunkH.GetPullMessage(), sourceAddr);
                        break;
                      }
                    case MSG_HELLO:
                      {
                        NS_ASSERT(GetHelloActive());
                        if (packetTag)
                          {
                            double sinr = ptag.GetSinr();
                            double alpha = 1.0; //more weight to latest sample
                            if (!m_neighbors.IsNeighbor(nt))
                              m_neighbors.AddNeighbor(nt);
                            sinr = (alpha * sinr) + ((1 - alpha) * m_neighbors.GetNeighbor(nt)->GetSINR());
                            m_neighbors.GetNeighbor(nt)->SetSINR(sinr);
                          }
                        m_rxControlTrace(packet, address);
                        HandleHello(chunkH.GetHelloMessage(), sourceAddr);
                        break;
                      }
                    }
                }
            }
          break;
        }
      default:
        {
          NS_ASSERT_MSG(false, "Invalid peer type: "<<m_peerType);
          break;
        }
      }
  }

  void
  VideoPushApplication::StatisticAddPullRequest ()
  {
    m_statisticsPullRequest++;
  }

  void
  VideoPushApplication::StatisticAddPullHit ()
  {
    m_statisticsPullHit++;
  }

  void
  VideoPushApplication::StatisticAddPullReceived ()
  {
    m_statisticsPullReceived++;
  }

  void
  VideoPushApplication::StatisticAddPullReply ()
  {
    m_statisticsPullReply++;
  }

  void
  VideoPushApplication::AddPending (uint32_t chunkid)
  {
    NS_ASSERT(chunkid >0);
    if (m_pullPending.find(chunkid) == m_pullPending.end())
      m_pullPending.insert(std::pair<uint32_t, uint32_t>(chunkid, 0));
    m_pullPending.find(chunkid)->second += 1;
  }

  bool
  VideoPushApplication::IsPending (uint32_t chunkid)
  {
    NS_ASSERT(chunkid >0);
    if (m_pullPending.find(chunkid) == m_pullPending.end())
      return false;
    return (m_pullPending.find(chunkid)->second > 0);
  }

  bool
  VideoPushApplication::RemovePending (uint32_t chunkid)
  {
    if (m_pullPending.find(chunkid) == m_pullPending.end())
      return false;
    m_pullPending.erase(chunkid);
    NS_ASSERT(m_pullPending.find(chunkid) == m_pullPending.end());
    return true;
  }

  void
  VideoPushApplication::SetSource (Ipv4Address source)
  {
    NS_ASSERT(source != Ipv4Address());
    m_source = source;
  }

  Ipv4Address
  VideoPushApplication::GetSource () const
  {
    return m_source;
  }

  void
  VideoPushApplication::AddPullRetryCurrent (uint32_t chunkid)
  {
    NS_ASSERT(chunkid>0);
    if (m_pullRetriesCurrent.find(chunkid) == m_pullRetriesCurrent.end())
      m_pullRetriesCurrent.insert(std::pair<uint32_t, uint32_t>(chunkid, 0));
    m_pullRetriesCurrent.find(chunkid)->second++;
  }

  uint32_t
  VideoPushApplication::GetPullRetryCurrent (uint32_t chunkid)
  {
    NS_ASSERT(chunkid>0);
    if (m_pullRetriesCurrent.find(chunkid) == m_pullRetriesCurrent.end())
      return 0;
    return m_pullRetriesCurrent.find(chunkid)->second;
  }

  void
  VideoPushApplication::StatisticAddDuplicateChunk (uint32_t chunkid)
  {
    NS_ASSERT(chunkid>0);
    if (m_duplicates.find(chunkid) == m_duplicates.end())
      {
        std::pair<uint32_t, uint32_t> dup(chunkid, 0);
        m_duplicates.insert(dup);
      }
    m_duplicates.find(chunkid)->second++;
  }

  uint32_t
  VideoPushApplication::GetDuplicate (uint32_t chunkid)
  {
    NS_ASSERT(chunkid>0);
    if (m_duplicates.find(chunkid) == m_duplicates.end())
      return 0;
    return m_duplicates.find(chunkid)->second;
  }

  Neighbor
  VideoPushApplication::PeerSelection (PeerPolicy policy)
  {
    NS_LOG_FUNCTION (this);
    return m_neighbors.SelectNeighbor(policy);
  }

  ChunkVideo
  VideoPushApplication::ForgeChunk ()
  {
    uint64_t tstamp = Simulator::Now().ToInteger(Time::US);
    if (m_chunks.GetBufferSize() == 0)
      m_latestChunkID = 0;
    ChunkVideo cv(++m_latestChunkID, tstamp, m_pktSize, 0);
    return cv;
  }

  uint32_t
  VideoPushApplication::ChunkSelection (ChunkPolicy policy)
  {
    NS_LOG_FUNCTION(this<<policy);
    uint32_t chunkid = 0;
    switch (policy)
      {
      case CS_NEW_CHUNK:
        {
          ChunkVideo cv = ForgeChunk();
          chunkid = cv.c_id;
          bool addChunk = m_chunks.AddChunk(cv, CHUNK_RECEIVED_PUSH);
          NS_ASSERT(addChunk);
          NS_ASSERT(m_duplicates.find(cv.c_id) == m_duplicates.end());
          break;
        }
      case CS_LATEST_MISSED:
        {
          chunkid = m_chunks.GetLatestMissed(GetPullWBase(), GetPullWindow());
          NS_ASSERT(!chunkid||!m_chunks.HasChunk(chunkid));
          NS_ASSERT(!chunkid||m_chunks.GetChunkState(chunkid)==CHUNK_MISSED);
          NS_ASSERT(!chunkid||(chunkid>=GetPullWBase() && chunkid<=(GetPullWBase()+GetPullWindow())));
          break;
        }
      case CS_LEAST_MISSED:
        {
          chunkid = m_chunks.GetLeastMissed(GetPullWBase(), GetPullWindow());
          NS_ASSERT(!chunkid||!m_chunks.HasChunk(chunkid));
          NS_ASSERT(!chunkid||m_chunks.GetChunkState(chunkid)==CHUNK_MISSED);
          NS_ASSERT(!chunkid||(chunkid>=GetPullWBase() && chunkid<=(GetPullWBase()+GetPullWindow())));
          break;
        }
      case CS_LATEST:
        {
          chunkid = m_chunks.GetLastChunk();
          break;
        }
      default:
        {
          NS_LOG_ERROR("Condition not allowed");
          NS_ASSERT(true);
          break;
        }
      }
    return chunkid;
  }

  void
  VideoPushApplication::SendPacket ()
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
          NS_ASSERT(m_chunkEvent.IsExpired ());
          uint32_t new_chunk = ChunkSelection(CS_NEW_CHUNK);
          ChunkVideo *copy = m_chunks.GetChunk(new_chunk);
          ChunkHeader chunk(MSG_CHUNK);
          chunk.GetChunkMessage().SetChunk(*copy);
          Ptr<Packet> packet = Create<Packet>(m_pktSize); //TODO You can add here the real chunk data
          packet->AddHeader(chunk);
          uint32_t payload = copy->c_size + copy->c_attributes_size; //data and attributes already in chunk header;
          m_txDataTrace(packet);
          m_socket->SendTo(packet, 0, m_peer);
          m_totBytes += payload;
          m_lastStartTime = Simulator::Now();
          m_residualBits = 0;
          NS_ASSERT(new_chunk == m_chunks.GetLastChunk());
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
  VideoPushApplication::SendHello ()
  {
    switch (m_peerType)
      {
      case SOURCE:
        {
          break;
        }
      case PEER:
        {
          NS_LOG_FUNCTION (this);
          NS_ASSERT(GetHelloActive());
          Ipv4Mask mask("255.0.0.0");
          Ipv4Address subnet = GetLocalAddress().GetSubnetDirectedBroadcast(Ipv4Mask(mask));
          ChunkHeader hello(MSG_HELLO);
          hello.GetHelloMessage().SetLastChunk(m_chunks.GetLastChunk());
          double low = GetReceived(CHUNK_RECEIVED_PUSH);
          uint32_t ratio = ((low) == 0 ? 1 : (uint32_t) (floor(low * 1000)));
          hello.GetHelloMessage().SetChunksRatio(ratio);
          hello.GetHelloMessage().SetChunksReceived(m_chunks.GetBufferSize());
//          hello.GetHelloMessage().SetDestination(subnet);
//          hello.GetHelloMessage().SetNeighborhoodSize(m_neighbors.GetSize());
          Ptr<Packet> packet = Create<Packet>();
          packet->AddHeader(hello);
          m_txControlTrace(packet);
          NS_LOG_DEBUG ("Node " << GetLocalAddress()<< " sends hello to "<< subnet);
          m_socket->SendTo(packet, 0, InetSocketAddress(subnet, PUSH_PORT));
          m_helloTimer.Schedule();
          break;
        }
      default:
        {
          NS_ASSERT_MSG(false, "no valid peer state");
          break;
        }
      }
  }

//  void
//  VideoPushApplication::ConnectionSucceeded (Ptr<Socket>)
//  {
//    NS_LOG_FUNCTION_NOARGS ();
//    m_connected = true;
//
//  }

//  void
//  VideoPushApplication::ConnectionFailed (Ptr<Socket>)
//  {
//    NS_LOG_FUNCTION_NOARGS ();
//    NS_LOG_INFO("VideoPush, Connection Failed");
//  }

  void
  VideoPushApplication::SetGateway (const Ipv4Address &gateway)
  {
    m_gateway = gateway;
  }

  Time
  VideoPushApplication::TransmissionDelay (double l, double u, enum Time::Unit unit)
  {
    double delay = UniformVariable().GetValue(l, u);
    Time delayms = Time::FromDouble(delay, unit);
    return delayms;
  }
} // namespace ns3

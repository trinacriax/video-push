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

#ifndef __VIDEO_PUSH_H__
#define __VIDEO_PUSH_H__

#include "chunk-video.h"
#include "chunk-buffer.h"
#include "chunk-packet.h"
#include "neighbor-set.h"

#include <ns3/address.h>
#include <ns3/ipv4-address.h>
#include <ns3/application.h>
#include <ns3/event-id.h>
#include <ns3/ptr.h>
#include <ns3/data-rate.h>
#include <ns3/random-variable.h>
#include <ns3/traced-callback.h>
#include <ns3/ipv4.h>
#include <ns3/timer.h>
#include <ns3/stats-module.h>

namespace ns3
{

  class Address;
  class RandomVariable;
  class Socket;
  class NeighborsSet;

  enum PeerType
  {
    PEER, SOURCE
  };

  enum ChunkPolicy
  {
    CS_NEW_CHUNK, CS_LATEST, CS_LEAST_USEFUL, CS_LATEST_MISSED, CS_LEAST_MISSED
  };

  const uint32_t PUSH_PORT = 9999;
  const Time LPULLGUARD = MicroSeconds(500);
  const Time RPULLGUARD = MicroSeconds(500);
  const double PullReqThr = 0.90;
  const double PullRepThr = 0.95;

  /**
   * \brief A video application.
   * This module provides a simple push protocol for video streaming application.
   * User can enable the pull mechanism to retrieve chunks,
   * setting the proper scheduling algorithm for both peer and chunk selection.
   */
  class VideoPushApplication : public Application
  {

    public:
      static TypeId
      GetTypeId (void);

      /**
       *  Default constructor.
       */
      VideoPushApplication ();

      virtual
      ~VideoPushApplication ();

      /**
       *
       * \param gateway Access Point address.
       * Associated access point
       */
      void
      SetGateway (const Ipv4Address &gateway);

      /**
       * \param maxBytes the total number of bytes to send
       *
       * Set the total number of bytes to send. Once these bytes are sent, no packet
       * is sent again, even in on state. The value zero means that there is no
       * limit.
       */
      void
      SetMaxBytes (uint32_t maxBytes);

      /**
       * \return pointer to associated socket
       * Get associated socket.
       */
      Ptr<Socket>
      GetTxSocket (void) const;

      /**
       * \return the total bytes received in this sink app
       * Get the total bytes received in this application
       */
      uint32_t
      GetTotalRx () const;

      /**
       * \return pointer to listening socket
       * Get listening socket
       */
      Ptr<Socket>
      GetListeningSocket (void) const;

      /**
       * \return list of pointers to accepted sockets
       * Get list of pointers to accepted sockets
       */
      std::list<Ptr<Socket> >
      GetAcceptedSockets (void) const;

      /**
       * \param chunkid chunk identifier
       * Set the max pull retries for a given chunk identifier.
       */
      void
      SetPullMax (uint32_t max);

      /**
       * \return max number of pull retries
       * Get the max number of pull retries for a given chunk identifier.
       */
      uint32_t
      GetPullMax () const;

      /**
       * \param window size.
       * Set the pull window size.
       */
      void
      SetPullWindow (uint32_t window);

      /**
       * \return window size.
       * Get the pull window size.
       */
      uint32_t
      GetPullWindow () const;

      /**
       * \param ratio Chunk ratio within the pull window.
       * Set the minimum chunk ratio within the window to activate pull in the node.
       */
      void
      SetPullRatioMin (double ratio);

      /**
       * \return Minimum chunk ratio within the pull window.
       * Get the minimum chunk ratio within the window to activate pull in the node.
       */
      double
      GetPullRatioMin () const;

      /**
       * \param ratio Chunk ratio within the pull window.
       * Set the maximum chunk ratio within the window before deactivating the pull in the node.
       */
      void
      SetPullRatioMax (double ratio);

      /**
       * \return Maximum chunk ratio within the pull window.
       * Get the maximum chunk ratio within the window before deactivating the pull in the node.
       */
      double
      GetPullRatioMax () const;

      /**
       * \param pull True if the pull mechanism is enabled, false otherwise.
       * Set whether the pull mechanism is enabled or not.
       */
      void
      SetPullActive (bool pull);

      /**
       * \return True if the pull mechanism is enabled, false otherwise.
       * Get whether the pull mechanism is enabled or not.
       */
      bool
      GetPullActive () const;

      /**
       * \return Max number of replies.
       * Get the maximum current number of pull reply.
       */
      uint32_t
      GetPullReplyMax () const;

      /**
       * \param Max number of replies.
       * Set the maximum current number of pull reply.
       */
      void
      SetPullReplyMax (uint32_t value);

      /**
       * \param time Pull timeout as the maxium time to retrieve a chunk.
       * Set the pull timeout before issuing another pull message or skip the chunk.
       */
      void
      SetPullTime (Time time);

      /**
       * \return The pull timeout as the maxium time to retrieve a chunk.
       * Get the pull timeout before issuing another pull message or skip the chunk.
       */
      Time
      GetPullTime () const;

      /**
       * \return True if the hello messages are enabled, false otherwise.
       * Set whether hello messages are enabled or not.
       */
      void
      SetHelloActive (uint32_t hello);

      /**
       * \param True if the hello messages are enabled, false otherwise.
       * Get whether hello messages are enabled or not.
       */
      uint32_t
      GetHelloActive () const;

      /**
       * \param time Hello time.
       * Set the time period for hello messages.
       */
      void
      SetHelloTime (Time time);

      /**
       * \return Hello time.
       * Get the time period for hello messages.
       */
      Time
      GetHelloTime () const;

      /**
       * \param loss Maximum hello messages loss.
       * Set the maximum loss allowed in hello messages before deleting a neighbor.
       */
      void
      SetHelloLoss (uint32_t loss);

      /**
       * \return Maximum hello messages loss.
       * Get the maximum loss allowed in hello messages before deleting a neighbor.
       */
      uint32_t
      GetHelloLoss () const;

      /**
       * \param source Multicast source address.
       * Set the multicast source address for this application.
       */
      void
      SetSource (Ipv4Address source);

      /**
       * \return Multicast source address.
       * Get the multicast source address for this application.
       */
      Ipv4Address
      GetSource () const;

//      /**
//       * \param delay Pointer to tracker delay.
//       * Pointer to tracker delay to collect statistics.
//       */
//      void
//      SetDelayTracker (Ptr<TimeMinMaxAvgTotalCalculator> delay);

    protected:
      virtual void
      DoDispose (void);

    private:
      // inherited from Application base class.
      virtual void
      StartApplication (void);    // Called at time specified by Start

      // inherited from Application base class.
      virtual void
      StopApplication (void);     // Called at time specified by Stop

      /**
       * Cancel all pending events.
       */
      void
      CancelEvents ();

//      void
//      ConnectionSucceeded (Ptr<Socket>);

//      void
//      ConnectionFailed (Ptr<Socket>);

//      void
//      Ignore (Ptr<Socket>);

      /**
       * \param Socket pointer.
       * \param from Address.
       * Accepted socket on the address.
       */
      void
      HandleAccept (Ptr<Socket>, const Address& from);

      /**
       * \param socket pointer.
       * Socket closed successfully.
       */
      void
      HandlePeerClose (Ptr<Socket>);

      /**
       * \param socket pointer
       * Socket error in close.
       */
      void
      HandlePeerError (Ptr<Socket>);

      /**
       * \return Local address.
       * Get current node address.
       */
      Ipv4Address
      GetLocalAddress ();

      /**
       * \return Current application identifier.
       * Get current application identifier.
       */

      uint32_t
      GetApplicationId (void) const;

      /**
       * \return A new chunk video.
       * Forge a new video chunk.
       */
      ChunkVideo
      ForgeChunk ();

      /**
       * Peer Loop function.
       * Is the core function of the protocol where the source
       * sends new chunks and the peers execute the pull mechanism.
       */
      void
      PeerLoop ();

      /**
       * Activate the chunk sending.
       */
      void
      StartSending ();

      /**
       * Stop chunk sending.
       */
      void
      StopSending ();

      /**
       * Source forge and send a new chunk.
       */
      void
      SendPacket ();

      /**
       * \param chunkid chunk identifier.
       * \param target neighbor address.
       * Send a chunk to a given peer.
       */
      void
      SendChunk (uint32_t chunkid, const Ipv4Address target);

      /**
       *
       * \param chunkid chunk identifier.
       * \param target neighbor address
       * Send a pull message for a given chunk to a neighbor node.
       */
      void
      SendPull (uint32_t chunkid, const Ipv4Address target);

      /**
       * Send hello message.
       */
      void
      SendHello ();

      /**
       * \param Socket source.
       * Parse a packet received from a socket, delivering the packet to the proper handler.
       */

      void
      HandleReceive (Ptr<Socket>);

      /**
       * \param chunkheader Chunk header.
       * \param sender Sender node.
       * Parse a chunk received.
       */
      void
      HandleChunk (ChunkHeader::ChunkMessage &chunkheader, const Ipv4Address &sender);

      /**
       * \param pullheader Pull header.
       * \param sender Sender node.
       * Parse a pull message.
       */
      void
      HandlePull (ChunkHeader::PullMessage &pullheader, const Ipv4Address &sender);

      /**
       * \param helloheader Hello header.
       * \param sender Sender node.
       * Parse a hello message.
       */
      void
      HandleHello (ChunkHeader::HelloMessage &helloheader, const Ipv4Address &sender);

      /**
       * Add a pull request sent for statistics
       */
      void
      StatisticAddPullRequest ();

      /**
       * Add a pull received for statistics
       */
      void
      StatisticAddPullReceived ();

      /**
       * Add a pull reply for statistics
       */
      void
      StatisticAddPullReply ();

      /**
       * Add a successful pull for statistics (receive a positive reply)
       */
      void
      StatisticAddPullHit ();

      /**
       * Compute chunks statistics.
       */
      void
      StatisticChunk (void);

      /**
       * \param chunkid chunk identifier.
       * Add one duplicate for the given chunk.
       */
      void
      StatisticAddDuplicateChunk (uint32_t chunkid);

      /**
       * \param start Time to start sending pull messages.
       * Set the pull slot start, where the pull may occur.
       */
      void
      SetPullSlotStart (Time start);

      /**
       * \return Time to start sending pull messages.
       * Get the pull slot start, where the pull may occur.
       */
      Time
      GetPullSlotStart () const;

      /**
       * \return Time to end sending pull messages.
       * Get the pull slot end, after which no pull messages can be sent.
       */
      Time
      GetPullSlotEnd () const;

      /**
       * \return Time slot to send pull messages.
       * Get the pull slot time.
       */
      Time
      GetPullSlot () const;

      /**
       * \return percentage of pull slot used.
       * Get the fraction of pull slot already used.
       */
      double
      PullSlot ();

      /**
       * \param state Chunk state.
       * \return Fraction of chunks in pull window.
       * Fraction of chunks received with the pull window with the given state.
       */
      double
      GetReceived (enum ChunkState state);

      /**
       * \param chunkid chunk identifier.
       * Set the time a node started pulling a chunk.
       */
      void
      SetPullTimes (uint32_t chunkid);

      /**
       * \param chunkid chunk identifier.
       * \param time start time
       * Set the time a node started pulling a chunk.
       */
      void
      SetPullTimes (uint32_t chunkid, Time time);

      /**
       * \return Chunk identifier.
       * Get the time a node started pulling a chunk.
       */
      Time
      GetPullTimes (uint32_t chunkid);

      /**
       * \param chunkid chunk identifier.
       * \return Time pull start.
       * Get the time a node started pulling the given chunk,
       * removing the item from the list.
       */
      Time
      RemPullTimes (uint32_t chunkid);

      /**
       * \param chunkid chunk identifier.
       * \return True, the chunk has been pulled, false otherwise.
       * Check whether the chunk has been pulled or not.
       */
      bool
      Pulled (uint32_t chunkid);

      /**
       * \param chunkid chunk identifier
       * Set the current pull retries for a given chunk identifier.
       */

      void
      AddPullRetryCurrent (uint32_t chunkid);

      /**
       * \param chunkid chunk identifier
       * \return number of pull retries for the chunk.
       * Get the current pull retries for a given chunk identifier.
       */
      uint32_t
      GetPullRetryCurrent (uint32_t chunkid);

      /**
       * \return True if the chunk ratio is withing min and max ratio, false otherwise.
       * Check whether the node's chunk ratio is between the minimum and the maximum chunk ratio
       * in order to activate or not the pull mechanism.
       */
      bool
      InPullRange ();

      /**
       * \return Current number of replies.
       * Get the current number of pull reply.
       */
      uint32_t
      GetPullReplyCurrent () const;

      /**
       * \param value Number of pull replies.
       * Set the current number of pull reply.
       */
      void
      SetPullReplyCurrent (uint32_t value);

      /**
       * Add one to the current number of pull replies.
       */
      void
      AddPullReplyCurrent ();

      /**
       * Reset the current number of pull replies.
       */
      void
      ResetPullReplyCurrent ();

      /**
       * \return Pull window base chunk identifier.
       * Get the lowest chunk identifier in the pull window,
       * i.e., the first that will be consumed in the next cycle.
       */
      uint32_t
      GetPullWBase ();

      /**
       * \param Pull window base chunk identifier.
       * Set the lowest chunk identifier in the pull window.
       */
      void
      SetPullWBase (uint32_t base);

      /**
       * Advance the lowest chunk identifier in the pull window by one.
       */
      void
      UpdatePullWBase ();

      /**
       * \param chunkid chunk identifier.
       * Set the current chunk as pending in the transmission queue.
       */

      void
      AddPending (uint32_t chunkid);

      /**
       * \param chunkid chunk identifier.
       * \return True is the chunk is pending in the transmission queue, false otherwise.
       * Check whether the chunk is pending in the transmission queue or not.
       */

      bool
      IsPending (uint32_t chunkid);

      /**
       * \param chunkid chunk identifier.
       * \return True if the chunk was pending, false otherwise.
       * Remove the chunk as pending in the transmission queue.
       */
      bool
      RemovePending (uint32_t chunkid);

      /**
       * \param chunkid chunk identifier.
       * \param delay Chunk delay.
       * Set the chunk delay.
       */
      void
      SetChunkDelay (uint32_t chunkid, Time delay);

      /**
       * \param chunkid chunk identifier.
       * \return Chunk delay.
       * Get the chunk delay.
       */
      Time
      GetChunkDelay (uint32_t chunkid);

      /**
       * \param chunkid chunk identifier.
       * \return Number of duplicates.
       * Number of duplicates for a given chunk
       */
      uint32_t
      GetDuplicate (uint32_t chunkid);

      /**
       * \param chunkid chunk identifier.
       * Mark the chunk as missed
       */
      void
      SetChunkMissed (uint32_t chunkid);

      /**
       * \return Missed chunk identifier.
       * Current missed chunk identifier.
       */
      uint32_t
      GetChunkMissed () const;

      /**
       * \param policy Chunk selection policy.
       * \return Chunk identifier according to policy, 0 otherwise.
       * Piece selection algorithm.
       */
      uint32_t
      ChunkSelection (ChunkPolicy policy);

      /**
       * \param policy Peer selection policy.
       * \return Neighbor node according to policy, null otherwise.
       * Peer selection algorithm.
       */
      Neighbor
      PeerSelection (PeerPolicy policy);

      /**
       *
       * \param l Minimum value.
       * \param u Maximum value.
       * \param unit Time unit.
       * \return Time Random time between min and max with the given unit.
       * Random time within the given bounds.
       */
      Time
      TransmissionDelay (double l, double u, enum Time::Unit unit);

      Ptr<Socket> m_socket;                    /// Associated socket
      std::list<Ptr<Socket> > m_socketList;    /// Accepted sockets
      Address m_localAddress;                  /// Local address to bind to
      uint16_t m_localPort;                    /// Local port to bind to
      Address m_peer;                          /// Peer address
      PeerType m_peerType;                     /// Peer type
      TypeId m_tid;                            /// Application TID
      Ptr<Ipv4> m_ipv4;                        /// IPv4 entity
      Ipv4Address m_source;                    /// Source address (only one)
      Ipv4Address m_gateway;                   /// Gateway address (if set)

      // STREAMING AND CHUNKS
      uint32_t m_totalRx;        /// Total bytes received
      bool m_connected;          /// True if connected
      DataRate m_cbrRate;        /// Rate that data is generated
      uint32_t m_pktSize;        /// Size of packets
      uint32_t m_residualBits;   /// Number of generated, but not sent, bits
      Time m_lastStartTime;      /// Time last packet sent
      uint32_t m_maxBytes;       /// Limit total number of bytes sent
      uint32_t m_totBytes;       /// Total bytes sent so far
      uint32_t m_playoutWindow;  /// Playout window size
      double m_chunkRatioMin;    /// Chunks' ratio within the playout window - MIN
      double m_chunkRatioMax;    /// Chunks' ratio within the playout window - MAX

      // PULL CONTROL MESSAGES
      bool m_pullActive;                                 /// Activate or not the pull mechanism
      EventId m_pullEvent;                               /// Eventid of pending "pull tx " event
      Time m_pullSlot;                                   /// Pull slot duration for pull operations
      Time m_pullSlotStart;                              /// Current pull slot start
      EventId m_pullSlotEvent;                           /// Event ID to update the next pull slot start
      uint32_t m_pullChunkMissed;                        /// Chunk identifier of the current missed chunk
      uint32_t m_pullReplyMax;                           /// Max number of pull replies within a pull slot
      uint32_t m_pullReplyCurrent;                       /// Current number of pull replies in the current slot
      Timer m_pullReplyTimer;                            /// Timer to reset the pull replies for the next slot
      Time m_pullTimeout;                                /// Pull timeout time
      Timer m_pullTimer;                                 /// Pull timer to pull chunks
      uint32_t m_pullRetriesMax;                         /// Max number of pull attempts allowed per chunk
      uint32_t m_pullWBase;                              /// Pull window base chunk
      Timer m_playout;                                   /// Playout Timer
      std::map<uint32_t, uint32_t> m_pullRetriesCurrent; /// Count pull attempts to recover a chunk
      std::map<uint32_t, Time> m_pullTimes;              /// Collect the time to recover each chunk
      std::map<uint32_t, uint32_t> m_pullPending;        /// Collect pending pulls

      // STATISTICS ON PULL
      uint32_t m_statisticsPullRequest;  /// statistics on pull request sent (SENDER)
      uint32_t m_statisticsPullReceived; /// statistics on pull request received (RECEIVER)
      uint32_t m_statisticsPullReply;    /// statistics on pull reply sent (RECEIVER)
      uint32_t m_statisticsPullHit;      /// statistics on pull reply received (i.e., success pull) (SENDER)

      // HELLO CONTROL MESSAGES
      uint32_t m_helloActive;   /// Activate or not the hello mechanism
      EventId m_helloEvent;     /// Eventid of pending "hello packet" event
      Time m_helloTime;         /// Hello Time
      Timer m_helloTimer;       /// Timer to send hello messages
      uint32_t m_helloLoss;     /// Max number of hello loss before removing a node as neighbor

      // CHUNK CONTROL MESSAGES
      EventId m_chunkEvent;                       /// Eventid of pending "chunk tx" event
      EventId m_loopEvent;                        /// Eventid of pending "loop" event
      ChunkBuffer m_chunks;                       /// Node's chun buffer
      std::map<uint32_t, uint32_t> m_duplicates;  /// Collect the number of duplicated chunks
      std::map<uint32_t, uint64_t> m_chunk_delay; /// Collect the chunks' delay
      enum PeerPolicy m_peerSelection;            /// Peer selection algorithm
      enum ChunkPolicy m_chunkSelection;          /// Chunk selection algorithm

      // NEIGHBORHOOD PART
      NeighborsSet m_neighbors;   /// Local neighborhood
      double n_selectionWeight;   /// Neighborhood weight

      // TRACE CALLBACK
      TracedCallback<Ptr<const Packet> > m_txDataTrace;
      TracedCallback<Ptr<const Packet>, const Address &> m_rxDataTrace;
      TracedCallback<Ptr<const Packet> > m_txControlTrace;
      TracedCallback<Ptr<const Packet>, const Address &> m_rxControlTrace;
      TracedCallback<Ptr<const Packet> > m_txControlPullTrace;
      TracedCallback<Ptr<const Packet>, const Address &> m_rxControlPullTrace;
      TracedCallback<Ptr<const Packet> > m_txDataPullTrace;
      TracedCallback<Ptr<const Packet>, const Address &> m_rxDataPullTrace;
      TracedCallback<double> m_pullStartTrace;
      TracedCallback<double> m_pullStopTrace;
      TracedCallback<uint32_t> m_neighborsTrace;

      Ptr<TimeMinMaxAvgTotalCalculator> m_delay;
  };
} // namespace ns3
#endif /* VIDEO_PUSH_H_ */


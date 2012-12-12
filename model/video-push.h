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

#ifndef __VIDEO_PUSH_H__
#define __VIDEO_PUSH_H__

#include "chunk-video.h"
#include "chunk-buffer.h"
#include "chunk-packet.h"
#include "neighbor-set.h"

#include "ns3/address.h"
#include "ns3/ipv4-address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable.h"
#include "ns3/traced-callback.h"
#include "ns3/ipv4.h"
#include "ns3/timer.h"
#include "ns3/stats-module.h"

namespace ns3{

class Address;
class RandomVariable;
class Socket;
class NeighborsSet;

enum PeerType {
	PEER,
	SOURCE
};

enum ChunkPolicy {
	CS_NEW_CHUNK,
	//
	CS_LATEST, //latest chunk received
	CS_LEAST_USEFUL,
	CS_LATEST_MISSED,
	CS_LEAST_MISSED

};

static uint32_t	m_pullWBase;		// pull window
const uint32_t PUSH_PORT = 9999;
const uint32_t LPULLGUARD = 0;
const uint32_t RPULLGUARD = 0;

class VideoPushApplication : public Application
{

public:
	static TypeId GetTypeId (void);
	VideoPushApplication ();
	virtual ~VideoPushApplication ();
	void SetGateway (const Ipv4Address &gateway);

	/**
	* \param maxBytes the total number of bytes to send
	*
	* Set the total number of bytes to send. Once these bytes are sent, no packet
	* is sent again, even in on state. The value zero means that there is no
	* limit.
	*/
	void SetMaxBytes (uint32_t maxBytes);

	/**
	* \return pointer to associated socket
	*/
	Ptr<Socket> GetTxSocket (void) const;

	/**
	* \return the total bytes received in this sink app
	*/
	uint32_t GetTotalRx () const;

	/**
	* \return pointer to listening socket
	*/
	Ptr<Socket> GetListeningSocket (void) const;

	/**
	* \return list of pointers to accepted sockets
	*/
	std::list<Ptr<Socket> > GetAcceptedSockets (void) const;

	void AddPullRetry (uint32_t chunkid);
	uint32_t GetPullRetry (uint32_t chunkid);
	void SetPullMax (uint32_t max);
	uint32_t GetPullMax () const;
	void SetPullWindow (uint32_t window);
	uint32_t GetPullWindow () const;
	void SetPullRatioMin (double ratio);
	double GetPullRatioMin () const;
	void SetPullRatioMax (double ratio);
	double GetPullRatioMax () const;
	bool InPullRange ();

	/**
	 * Activate or deactivate the pull mechanism.
	 * */
	void SetPullActive (bool pull);

	/**
	 * Get the state of the pull mechanism, whether is active or not.
	 * */
	bool GetPullActive () const;
	uint32_t GetPullCReply () const;
	void SetPullCReply (uint32_t value);
	void ResetPullCReply ();
	uint32_t GetPullMReply () const;
	void SetPullMReply (uint32_t value);
	void SetHelloActive (uint32_t hello);
	uint32_t GetHelloActive () const;
	void SetChunkDelay (uint32_t chunkid, Time delay);
	Time GetChunkDelay (uint32_t chunkid);
	void SetPullTime (Time time);
	Time GetPullTime () const;
	void SetHelloTime (Time time);
	Time GetHelloTime () const;
	void SetHelloLoss (uint32_t loss);
	uint32_t GetHelloLoss () const;
	void SetSource (Ipv4Address source);
	Ipv4Address GetSource () const;
	Time TransmissionDelay (double l, double u, enum Time::Unit unit);

	void SetDelayTracker (Ptr<TimeMinMaxAvgTotalCalculator> delay);
protected:
	virtual void DoDispose (void);
private:
	// inherited from Application base class.
	virtual void StartApplication (void);    // Called at time specified by Start
	virtual void StopApplication (void);     // Called at time specified by Stop

	//helpers
	void CancelEvents ();

	void Construct (Ptr<Node> n,
					  const Address &remote,
					  std::string tid,
					  const RandomVariable& ontime,
					  const RandomVariable& offtime,
					  uint32_t size);

	uint32_t GetApplicationId (void) const;

	void PeerLoop();
	void AddDuplicate (uint32_t chunkid);
	uint32_t GetDuplicate (uint32_t chunkid);

	void StartSending ();
	void StopSending ();
	void SendPacket ();
	void SendChunk (uint32_t chunkid, const Ipv4Address target);
	void SendPull (uint32_t chunkid, const Ipv4Address target);
	void SendHello ();

	ChunkVideo ForgeChunk ();
	uint32_t ChunkSelection (ChunkPolicy policy);
	Neighbor PeerSelection (PeerPolicy policy);

	void AddPending (uint32_t chunkid);
	bool IsPending (uint32_t chunkid);
	bool RemovePending (uint32_t chunkid);
	double GetReceived ();
	void AddPullRequest ();
	void AddPullHit ();
	void AddPullReceived ();
	void AddPullReply ();

	void SetPullSlotStart (Time start);
	Time GetPullSlotStart () const;
	Time GetPullSlotEnd () const;
	Time GetPullSlot () const;
	double PullSlot ();

	void SetChunkMissed (uint32_t chunkid);
	uint32_t GetChunkMissed () const;

	void SetPullTimes (uint32_t chunkid);
	void SetPullTimes (uint32_t chunkid, Time time);
	Time GetPullTimes (uint32_t chunkid);
	Time RemPullTimes (uint32_t chunkid);
	bool Pulled (uint32_t chunkid);
	// Event handlers
	void HandleReceive (Ptr<Socket>);
	void HandleChunk (ChunkHeader::ChunkMessage &chunkheader, const Ipv4Address &sender);
	void HandlePull (ChunkHeader::PullMessage &pullheader, const Ipv4Address &sender);
	void HandleHello (ChunkHeader::HelloMessage &helloheader, const Ipv4Address &sender);
	void HandleAccept (Ptr<Socket>, const Address& from);
	void HandlePeerClose (Ptr<Socket>);
	void HandlePeerError (Ptr<Socket>);
	Ptr<Ipv4Route> GetRoute (const Ipv4Address &local, const Ipv4Address &destination);
	Ipv4Address GetNextHop (const Ipv4Address &destination);

	void ConnectionSucceeded (Ptr<Socket>);
	void ConnectionFailed (Ptr<Socket>);
	void Ignore (Ptr<Socket>);
	Ipv4Address GetLocalAddress ();
	void StatisticChunk (void);

	static void SetPullWBase (uint32_t base);
	static uint32_t GetPullWBase ();

	Ptr<Socket>     m_socket;				// Associated socket
	std::list<Ptr<Socket> > m_socketList;	// Accepted sockets
	Address     	m_localAddress;			// Local address to bind to
	uint16_t 		m_localPort;			// Local port to bind to
	Address 		m_peer;         		// Peer address
	PeerType		m_peerType;     		// Peer type
	TypeId          m_tid;					// Application TID
	Ptr<Ipv4> 		m_ipv4;					// IPv4 entity
	Ipv4Address		m_source;				// Source address (only one)
	Ipv4Address		m_gateway;				// Gateway address (if set)

	/// STREAMING AND CHUNKS
	uint32_t        m_totalRx;				// Total bytes received
	bool            m_connected;   			// True if connected
	DataRate        m_cbrRate;     			// Rate that data is generated
	uint32_t        m_pktSize;     			// Size of packets
	uint32_t        m_residualBits;			// Number of generated, but not sent, bits
	Time            m_lastStartTime;		// Time last packet sent
	uint32_t        m_maxBytes;    			// Limit total number of bytes sent
	uint32_t        m_totBytes;    			// Total bytes sent so far
	uint32_t 		m_playoutWindow;		// Playout window size
	double	 		m_chunkRatioMin;		// Chunks' ratio within the playout window - MIN
	double	 		m_chunkRatioMax;		// Chunks' ratio within the playout window - MAX


	/// PULL CONTROL MESSAGES
	bool			m_pullActive;			// Activate or not the pull mechanism
	EventId         m_pullEvent;			// Eventid of pending "pull tx " event
	Time			m_pullSlot;				// Pull slot duration for pull operations
	Time			m_pullSlotStart;		// Current pull slot start
	EventId 		m_pullSlotEvent;		// Event ID to update the next pull slot start
	uint32_t	 	m_pullChunkMissed;		// Chunk identifier of the current missed chunk
	uint32_t		m_pullReplyMax;			// Max number of pull replies within a pull slot
	uint32_t		m_pullReplyCurrent;		// Current number of pull replies in the current slot
	Timer			m_pullReplyTimer;		// Timer to reset the pull replies for the next slot
	Time 			m_pullTimeout;			// Pull timeout time
	Timer 			m_pullTimer;			// Pull timer to pull chunks
	uint32_t 		m_pullRetriesMax;		// Max number of pull attempts allowed per chunk
	std::map<uint32_t, uint32_t> m_pullRetriesCurrent;	// Count pull attempts to recover a chunk
	std::map<uint32_t, Time> m_pullTimes; 	// Collect the time to recover each chunk
	std::map<uint32_t, uint32_t> m_pullPending;	// Collect pending pulls


	/// STATISTICS ON PULL
	uint32_t	 	m_statisticsPullRequest;	// statistics on pull request sent (SENDER)
	uint32_t	 	m_statisticsPullReceived;	// statistics on pull request received (RECEIVER)
	uint32_t		m_statisticsPullReply;		// statistics on pull reply sent (RECEIVER)
	uint32_t 		m_statisticsPullHit;		// statistics on pull reply received (i.e., success pull) (SENDER)


	/// HELLO CONTROL MESSAGES
	uint32_t		m_helloActive;			// Activate or not the hello mechanism
	EventId         m_helloEvent;			// Eventid of pending "hello packet" event
	Time 			m_helloTime;			// Hello Time
	Timer 			m_helloTimer;			// Timer to send hello messages
	uint32_t		m_helloLoss;			// Max number of hello loss before removing a node as neighbor


	/// CHUNK CONTROL MESSAGES
	EventId         m_chunkEvent;			// Eventid of pending "chunk tx" event
	EventId         m_loopEvent;			// Eventid of pending "loop" event
	ChunkBuffer		m_chunks;				// Node's chun buffer
	std::map<uint32_t, uint32_t> m_duplicates; 	// Collect the number of duplicated chunks
	std::map<uint32_t, uint64_t> m_chunk_delay;	// Collect the chunks' delay
	enum PeerPolicy m_peerSelection; // Peer selection algorithm
	enum ChunkPolicy m_chunkSelection; // Chunk selection algorithm

	/// NEIGHBORHOOD PART
	NeighborsSet 	m_neighbors;			// Local neighborhood
	double			n_selectionWeight;


	/// TRACE CALLBACK
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
	//	uint32_t 		m_flag;					// TESTING PURPOSE

	Ptr<TimeMinMaxAvgTotalCalculator> m_delay;
};
}
#endif /* VIDEO_PUSH_H_ */


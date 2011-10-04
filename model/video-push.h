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

#define PUSH_PORT 9999
#include "chunk-video.h"
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

namespace ns3{

class Address;
class RandomVariable;
class Socket;

enum PeerType {PEER, SOURCE};

enum PeerPolicy {RANDOM};

enum ChunkPolicy {LATEST};

class VideoPushApplication : public Application
{

public:
	static TypeId GetTypeId (void);
	VideoPushApplication ();
	virtual ~VideoPushApplication ();

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

	void PeerLoop();

	void StartSending ();
	void StopSending ();
	void SendPacket ();
	void SendHello ();

	// Event handlers
	void HandleReceive (Ptr<Socket>);
	void HandleAccept (Ptr<Socket>, const Address& from);
	void HandlePeerClose (Ptr<Socket>);
	void HandlePeerError (Ptr<Socket>);

	void ScheduleNextTx ();
	void ScheduleStartEvent ();
	void ScheduleStopEvent ();
	void ConnectionSucceeded (Ptr<Socket>);
	void ConnectionFailed (Ptr<Socket>);
	void Ignore (Ptr<Socket>);

	Ptr<Socket>     m_socket;       // Associated socket
	std::list<Ptr<Socket> > m_socketList; //the accepted sockets
	Address     	m_localAddress;	// Local address to bind to
	uint16_t 		m_localPort;	// Local port to bind to

	uint32_t        m_totalRx;      // Total bytes received
	Address 		m_peer;         // Peer address
	bool            m_connected;    // True if connected
	RandomVariable  m_onTime;       // rng for On Time
	RandomVariable  m_offTime;      // rng for Off Time
	PeerType		m_peerType;     // Peer type
	DataRate        m_cbrRate;      // Rate that data is generated
	uint32_t        m_pktSize;      // Size of packets
	uint32_t        m_residualBits; // Number of generated, but not sent, bits
	Time            m_lastStartTime; // Time last packet sent
	uint32_t        m_maxBytes;     // Limit total number of bytes sent
	uint32_t        m_totBytes;     // Total bytes sent so far
	EventId         m_startStopEvent;     // Event id for next start or stop event
	EventId         m_sendEvent;    // Eventid of pending "send packet" event
	bool            m_sending;      // True if currently in sending state
	TypeId          m_tid;
	Ptr<Ipv4> 		m_ipv4;

	enum PeerPolicy m_peerSelection; // Peer selection algorithm
	enum ChunkPolicy m_chunkSelection; // Chunk selection algorithm
	TracedCallback<Ptr<const Packet> > m_txTrace;
	TracedCallback<Ptr<const Packet>, const Address &> m_rxTrace;
};

#endif /* VIDEO_PUSH_H_ */
}

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

#ifndef __NEIGHBORS_SET_H__
#define __NEIGHBORS_SET_H__

#include "neighbor.h"
#include "ns3/object.h"
#include "ns3/simulator.h"
#include "ns3/random-variable.h"
#include <map>

namespace ns3{

using namespace streaming;

enum PeerPolicy {
	PS_RANDOM,
	PS_DELAY,
	PS_RSSI,
	PS_ROUNDROBIN
};

enum PeerState {
	UNKNOWN, ACTIVE, INACTIVE
};

struct NeighborData{
	NeighborData () :
		n_contact (Simulator::Now()), n_state (ACTIVE),
		n_bufferSize (0), n_latestChunk(0), n_rssiPower (0)
	{}
	NeighborData (Time start, PeerState state, uint32_t size, uint32_t c_id, double rssi):
		n_contact (start), n_state (state), n_bufferSize (size), n_latestChunk(c_id), n_rssiPower (rssi)
	{}
	Time n_contact;
	//bitmap
	//bitmap last time
	//chunk buffer size;
	//bandwidth capacity
	enum PeerState n_state;
	uint32_t n_bufferSize;
	uint32_t n_latestChunk;
	double n_rssiPower;
	Time GetLastContact () const;
	void SetLastContact (Time contact);
	PeerState GetPeerState () const;
	void SetPeerState (PeerState state);
	uint32_t GetBufferSize () const;
	void SetBufferSize (uint32_t size);
	uint32_t GetLastChunk () const;
	void SetLastChunk (uint32_t last);
	double GetRssiPower () const;
	void SetRssiPower (double rssi);
	void Update (uint32_t size, uint32_t last);
	};

static inline std::ostream&
operator <<(std::ostream& outStream, const NeighborData& neighbor)
{
	return outStream << "State="<<neighbor.n_state << " Contact="<<neighbor.n_contact<< " Rssi="<<neighbor.n_rssiPower;
}

class NeighborsSet {

public:
	typedef std::pair<Neighbor, NeighborData> NeigborPair;
	NeighborsSet ();
	virtual ~NeighborsSet ();

public:
	NeighborData* GetNeighbor (Ipv4Address n_addr, uint32_t n_iface);
	NeighborData* GetNeighbor (Neighbor neighbor);
	Neighbor Get (uint32_t index);
	Neighbor SelectNeighbor (PeerPolicy policy);
	Neighbor SelectRandom ();
	void SortRssi ();
	Neighbor SelectRssi ();
	bool AddNeighbor (const Neighbor neighbor, NeighborData data);
	bool AddNeighbor (Neighbor neighbor);
	bool DelNeighbor (Ipv4Address n_addr, uint32_t n_iface);
	bool DelNeighbor (Neighbor neighbor);
	bool IsNeighbor (const Neighbor neighbor);
	size_t GetSize();
	void Purge ();
	void SetExpire (Time time);
	Time GetExpire () const;

protected:
//	std::map<Neighbor, NeighborData> m_neighbor_set;
	std::map<Neighbor, NeighborData> m_neighbor_set;
	Time m_expire;
	double *m_neighborRssi;
	std::vector<NeigborPair> m_neighborPairRssi;

	struct RssiCmp {
	    bool operator()(const NeigborPair &lhs, const NeigborPair &rhs) {
	        return lhs.second.GetRssiPower() > rhs.second.GetRssiPower();
	    }
	};

};
}
#endif

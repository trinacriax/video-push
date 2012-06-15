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
 *
 *
 */

#include "neighbor-set.h"
#include "ns3/simulator.h"

namespace ns3{

Time
NeighborData::GetLastContact () const
{
return n_contact;
}

void NeighborData::SetLastContact (Time contact)
{
	n_contact = contact;
}

PeerState
NeighborData::GetPeerState () const
{
	return n_state;
}

void
NeighborData::SetPeerState (PeerState state)
{
	n_state = state;
}

uint32_t
NeighborData::GetBufferSize () const
{
	return n_bufferSize;
}

void
NeighborData::SetBufferSize (uint32_t size)
{
	NS_ASSERT (size >= 0);
	n_bufferSize = size;
}

uint32_t
NeighborData::GetLastChunk () const
{
	return n_latestChunk;
}

void
NeighborData::SetLastChunk (uint32_t last)
{
	NS_ASSERT (last >= 0);
	n_latestChunk = last;
}

void
NeighborData::Update (uint32_t size, uint32_t last)
{
	NS_ASSERT (size >= 0);
	NS_ASSERT (last >= 0);
	SetLastContact (Simulator::Now());
	SetBufferSize (size);
	SetLastChunk (last);
}

Ipv4Address
Neighbor::GetAddress ()
{
return n_address;
}

uint32_t
Neighbor::GetPort ()
{
return n_port;
}


NeighborsSet::NeighborsSet () {
		m_neighbor_set.clear();
	}

NeighborsSet::~NeighborsSet () {
	m_neighbor_set.clear();
}

size_t
NeighborsSet::GetSize()
{
	return m_neighbor_set.size();
}

NeighborData*
NeighborsSet::GetNeighbor (Ipv4Address n_addr, uint32_t n_port){
	Neighbor *n = new Neighbor(n_addr, n_port);
	return GetNeighbor(*n);
}

NeighborData*
NeighborsSet::GetNeighbor (Neighbor neighbor){
	NeighborData *n_data = 0;
	if(IsNeighbor(neighbor))
		n_data = &(m_neighbor_set.find(neighbor)->second);
	return  n_data;
}

bool
NeighborsSet::IsNeighbor (const Neighbor neighbor){
	std::map<Neighbor, NeighborData>::iterator iter = m_neighbor_set.find(neighbor);
	return (iter != m_neighbor_set.end());
}

bool
NeighborsSet::AddNeighbor (Neighbor neighbor, NeighborData data){
	if(IsNeighbor(neighbor))
		return false;
	std::pair<std::map<Neighbor, NeighborData>::iterator,bool> test;
	test = m_neighbor_set.insert(std::pair<Neighbor,NeighborData> (neighbor,data));
	return test.second;
}

bool
NeighborsSet::AddNeighbor (Neighbor neighbor){
	if(IsNeighbor(neighbor))
		return false;
	std::pair<std::map<Neighbor, NeighborData>::iterator,bool> test;
	NeighborData data;
	test = m_neighbor_set.insert(std::pair<Neighbor,NeighborData> (neighbor,data));
	return test.second;
}

void
NeighborsSet::SetExpire (Time time)
{
	m_expire = time;
}

Time
NeighborsSet::GetExpire () const
{
	return m_expire;
}

bool
NeighborsSet::DelNeighbor (Ipv4Address n_addr, uint32_t n_port){
	Neighbor *n = new Neighbor(n_addr, n_port);
	return DelNeighbor(*n);
}

bool
NeighborsSet::DelNeighbor (Neighbor neighbor){
	if(!IsNeighbor(neighbor))
		return false;
	return (m_neighbor_set.erase(neighbor) == 1 );
}

Neighbor
NeighborsSet::SelectRandom ()
{
	Neighbor nt;
	if (GetSize() == 0) return nt;
	uint32_t index = UniformVariable().GetInteger(0, GetSize()-1);
	std::map<Neighbor, NeighborData>::iterator iter = m_neighbor_set.begin();
	for (; iter != m_neighbor_set.end() && index>0; iter++, index--);
//		std::cout << "I:"<<index<<" Peer "<< iter->first.n_address<<"\n";
//	std::cout << "Selected:"<<index<<" Peer "<< iter->first.n_address<<"\n";
	return iter->first;
}

Neighbor
NeighborsSet::SelectNeighbor (PeerPolicy policy)
{
	NS_ASSERT (policy>=PS_RANDOM && policy <= PS_ROUNDROBIN);
	Purge();
	Neighbor target;
	switch (policy)
	{
		case PS_RANDOM:
		{
			target = SelectRandom();
			break;
		}
		case PS_DELAY:
		{
			NS_ASSERT_MSG (false, "SelectNeighbor: Not yet implemented.");
			break;
		}
		case PS_ROUNDROBIN:
		{
			NS_ASSERT_MSG (false, "SelectNeighbor: Not yet implemented.");
			break;
		}
		default:
		{
			NS_ASSERT_MSG (false, "SelectNeighbor: invalid.");
			break;
		}
	}
	return target;
}

void
NeighborsSet::Purge ()
{
//	std::map<Neighbor, NeighborData> newset;
	for (std::map<Neighbor, NeighborData>::iterator iter = m_neighbor_set.begin();
		iter != m_neighbor_set.end(); iter++)
		{
//			std::cout << "Node "<< iter->first.n_address << " Last "<< iter->second.GetLastContact() << "\n";
			if (Simulator::Now() - iter->second.GetLastContact() > m_expire)
			{
				std::map<Neighbor, NeighborData>::iterator iter2 = iter;
				m_neighbor_set.erase (iter2);
//				newset.insert (*iter);
			}
		}
//	m_neighbor_set = newset;
}

}



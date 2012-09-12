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
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("NeighborSet");

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


double
NeighborData::GetRssiPower () const
{
	return n_rssiPower;
}

void
NeighborData::SetRssiPower (double rssi)
{
	n_rssiPower = rssi;
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
	m_neighborPairRssi.clear();
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

void
NeighborsSet::SetSelectionWeight (double weight)
{
	m_selectionWeight = weight;
}

double
NeighborsSet::GetSelectionWeight () const
{
	return m_selectionWeight;
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
NeighborsSet::Get (uint32_t index)
{
	Neighbor nt;
	if (GetSize() == 0) return nt;
	std::map<Neighbor, NeighborData>::iterator iter = m_neighbor_set.begin();
	for (; iter != m_neighbor_set.end() && index>0; iter++, index--);
//		std::cout << "I:"<<index<<" Peer "<< iter->first.n_address<<"\n";
//	std::cout << "Selected:"<<index<<" Peer "<< iter->first.n_address<<"\n";
	return iter->first;
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

void
NeighborsSet::SortRssi ()
{
//	for (std::map<Neighbor, NeighborData>::const_iterator iter = m_neighbor_set.begin(); iter != m_neighbor_set.end(); iter++)
//		NS_LOG_DEBUG ("Neighbor="<< iter->first <<" " << iter->second);
	size_t nsize = m_neighbor_set.size();
	m_neighborPairRssi.reserve (nsize);
	m_neighborPairRssi = std::vector<NeigborPair> (m_neighbor_set.begin(), m_neighbor_set.end());
	NS_ASSERT (m_neighborPairRssi.size() == nsize);
	std::sort (m_neighborPairRssi.begin(), m_neighborPairRssi.end(), RssiCmp());
//	for (std::vector<std::pair<Neighbor, NeighborData> >::const_iterator iter = m_neighborPairRssi.begin(); iter != m_neighborPairRssi.end(); iter++)
//	{
//			NS_LOG_DEBUG ("Neighbor="<< iter->first <<" Data=" << iter->second << " Size "<< m_neighborPairRssi.size());
//	}
    double weight[nsize];
    double weights = 0;
    double tot = 0;
    m_neighborRssi = new double[nsize];
    uint32_t i = 0;
    for (std::vector<std::pair<Neighbor, NeighborData> >::const_iterator iter = m_neighborPairRssi.begin(); iter != m_neighborPairRssi.end(); iter++, i++)
    {
    	weight[i] = iter->second.GetRssiPower();
//    	NS_LOG_DEBUG ("Neighbor "<< iter->first << " w=" << weight[i] << "/"<<weights);
    	weights += weight[i];
    }
    for (i = 0; i < nsize; i++)
    {
    	m_neighborRssi[i] = weight[i] / weights;
//    	NS_LOG_DEBUG ("Neighbor "<< i << " w=" << weight[i] << "/"<< weights << ": P=" << m_neighborRssi[i]);
    	tot += m_neighborRssi[i];
    }
//    i = 0;
//    for (std::vector<std::pair<Neighbor, NeighborData> >::const_iterator iter = m_neighborPairRssi.begin(); iter != m_neighborPairRssi.end(); iter++, i++)
//	{
//		NS_LOG_DEBUG ("Neighbor="<< iter->first <<" Data=" << iter->second << " Prob="<< (weight[i] / weights) );
//	}
//   	NS_LOG_DEBUG ("Neighbors P(all)=" << tot);
    NS_ASSERT ((1-tot)< pow10(-6));
}

Neighbor
NeighborsSet::SelectRssi ()
{
	Neighbor nt;
	NS_ASSERT (GetSize() > 0);
//	uint32_t index = UniformVariable().GetInteger(0, GetSize()-1);
//	std::map<Neighbor, NeighborData>::iterator iter = m_neighbor_set.begin();
//	for (; iter != m_neighbor_set.end() && index>0; iter++, index--);
//
//	return iter->first;
	size_t nsize = m_neighbor_set.size();
	if ((m_neighborPairRssi.size() == 0 && m_neighbor_set.size() > 0) || (m_neighborPairRssi.size() != nsize) )
		SortRssi ();
	double dice = UniformVariable().GetValue();
	uint32_t id = 0;
    while (dice > 0 && nt.GetAddress() == Ipv4Address(Ipv4Address::GetAny()) && id < nsize)
    {
    	NeighborData *nd = GetNeighbor (m_neighborPairRssi[id].first);
    	NS_ASSERT (nd);
    	double weight = nd->n_bufferSize/(nd->n_latestChunk*1.0);
//           if (debug >= 8) {
//               System.out.println("\t(" + id + ") Value " + value + ", Prob " + prob[id] + " (" + neighbors[id] + ")\n");
//           }
		dice -= (GetSelectionWeight() * m_neighborRssi[id]) + ((1-GetSelectionWeight()) * m_neighborRssi[id] * weight);
		if (dice <= 0) {
		   nt = m_neighborPairRssi[id].first;
		}
		id++;
	}
	if (nt.GetAddress() == Ipv4Address() && id >= nsize) {
	//           if (debug >= 10) {
	//               System.out.println("Out of Candidate ID range: " + id + " -- " + candidate);
	//           }
	   id = 0;
	   nt = m_neighborPairRssi[id].first;
	}
	//       if (debug >= 10) {
	//           System.out.println("Return Candidate ID " + id + " -- " + candidate);
	//       }
	return nt;
}


Neighbor
NeighborsSet::SelectNeighbor (PeerPolicy policy)
{
	NS_ASSERT (policy>=PS_RANDOM && policy <= PS_ROUNDROBIN);
	Purge();
	Neighbor target;
	if (!GetSize()) return target;
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
		case PS_RSSI:
		{
			target = SelectRssi();
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
		iter != m_neighbor_set.end();)
		{
//			std::cout << "Node "<< iter->first.n_address << " Last "<< iter->second.GetLastContact() << "\n";
			if (Simulator::Now() - iter->second.GetLastContact() > m_expire)
			{
//				std::map<Neighbor, NeighborData>::iterator iter2 = iter;
				m_neighbor_set.erase (iter++);
				m_neighborPairRssi.clear();
//				newset.insert (*iter);
			}
			else
				++iter;
		}
//	m_neighbor_set = newset;
}

}



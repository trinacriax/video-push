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

//#define NS_LOG_APPEND_CONTEXT                                   \
//  if (GetObject<Node> ()) { std::clog << "[node " << GetObject<Node> ()->GetId () << "] "; }

#include "neighbor-set.h"

namespace ns3{

NeighborsSet::NeighborsSet () {
		m_neighbor_set.clear();
	}

NeighborsSet::~NeighborsSet () {
	m_neighbor_set.clear();
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

bool
NeighborsSet::DelNeighbor (Ipv4Address n_addr, uint32_t n_port){
	Neighbor *n = new Neighbor(n_addr, n_port);
	return DelNeighbor(*n);
}

bool
NeighborsSet::DelNeighbor (Neighbor neighbor){
	if(!IsNeighbor(neighbor))
		return false;
	return (m_neighbor_set.erase(neighbor) ==1 );
}
}



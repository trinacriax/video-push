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

#include "neighbor-set.h"
#include <ns3/simulator.h>
#include <ns3/log.h>

NS_LOG_COMPONENT_DEFINE("NeighborSet");

namespace ns3
{

    Time
    NeighborData::GetLastContact () const
    {
      return n_contact;
    }

    void
    NeighborData::SetLastContact (Time contact)
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
      NS_ASSERT(state >= UNKNOWN && state <= INACTIVE);
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
      NS_ASSERT(size >= 0);
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
      NS_ASSERT(last >= 0);
      n_latestChunk = last;
    }

    void
    NeighborData::Update (uint32_t size, uint32_t last, double ratio)
    {
      NS_ASSERT(size >= 0);
      NS_ASSERT(last >= 0);
      NS_ASSERT(ratio >= 0 && ratio <=1);
      SetLastContact(Simulator::Now());
      SetBufferSize(size);
      SetLastChunk(last);
      SetChunkRatio(ratio);
    }

    double
    NeighborData::GetSINR () const
    {
      return n_sinr;
    }

    void
    NeighborData::SetSINR (double sinr)
    {
      n_sinr = sinr;
    }

    double
    NeighborData::GetChunkRatio () const
    {
      return n_chunksRatio;
    }

    void
    NeighborData::SetChunkRatio (double ratio)
    {
      NS_ASSERT(ratio >= 0 && ratio <=1);
      n_chunksRatio = ratio;
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

    NeighborsSet::NeighborsSet () :
        m_selectionWeight(0), m_neighborProbability(0), m_expire(0)
    {
      m_neighbor_set.clear();
      m_neighborProbVector.clear();
    }

    NeighborsSet::~NeighborsSet ()
    {
      m_neighbor_set.clear();
      m_neighborProbVector.clear();
    }

    bool
    NeighborsSet::AddNeighbor (Neighbor neighbor, NeighborData data)
    {
      if (IsNeighbor(neighbor))
        return false;
      std::pair<std::map<Neighbor, NeighborData>::iterator, bool> test;
      test = m_neighbor_set.insert(std::pair<Neighbor, NeighborData>(neighbor, data));
      ClearNeighborhood();
      return test.second;
    }

    bool
    NeighborsSet::AddNeighbor (Neighbor neighbor)
    {
      if (IsNeighbor(neighbor))
        return false;
      std::pair<std::map<Neighbor, NeighborData>::iterator, bool> test;
      NeighborData data;
      test = m_neighbor_set.insert(std::pair<Neighbor, NeighborData>(neighbor, data));
      ClearNeighborhood();
      return test.second;
    }

    bool
    NeighborsSet::DelNeighbor (Ipv4Address n_addr, uint32_t n_port)
    {
      Neighbor *n = new Neighbor(n_addr, n_port);
      return DelNeighbor(*n);
    }

    bool
    NeighborsSet::DelNeighbor (Neighbor neighbor)
    {
      return (!IsNeighbor(neighbor) && m_neighbor_set.erase(neighbor) == 1);
    }

    bool
    NeighborsSet::IsNeighbor (const Neighbor neighbor)
    {
      return (m_neighbor_set.find(neighbor) != m_neighbor_set.end());
    }

    size_t
    NeighborsSet::GetSize ()
    {
      return m_neighbor_set.size();
    }

    NeighborData*
    NeighborsSet::GetNeighbor (Ipv4Address n_addr, uint32_t n_port)
    {
      Neighbor *n = new Neighbor(n_addr, n_port);
      return GetNeighbor(*n);
    }

    NeighborData*
    NeighborsSet::GetNeighbor (Neighbor neighbor)
    {
      NeighborData *n_data = 0;
      if (IsNeighbor(neighbor))
        n_data = &(m_neighbor_set.find(neighbor)->second);
      return n_data;
    }

    Neighbor
    NeighborsSet::GetNeighbor (uint32_t index)
    {
      Neighbor nt;
      if (GetSize() == 0 || index >= GetSize())
        return nt;
      std::map<Neighbor, NeighborData>::iterator iter = m_neighbor_set.begin();
      for (; iter != m_neighbor_set.end() && index > 0; iter++, index--)
        ;
//      std::cout << "I:" << index << " Peer " << iter->first.n_address << "\n";
//      std::cout << "Selected:" << index << " Peer " << iter->first.n_address<<"\n";
      return iter->first;
    }

    void
    NeighborsSet::Purge ()
    {
      for (std::map<Neighbor, NeighborData>::iterator iter = m_neighbor_set.begin(); iter != m_neighbor_set.end();)
        {
          if (Simulator::Now() - iter->second.GetLastContact() > GetExpire())
            {
              m_neighbor_set.erase(iter++);
              ClearNeighborhood();
            }
          else
            ++iter;
        }
    }

    void
    NeighborsSet::SetSelectionWeight (double weight)
    {
      NS_ASSERT(weight>=0 && weight<=1);
      m_selectionWeight = weight;
    }

    double
    NeighborsSet::GetSelectionWeight () const
    {
      return m_selectionWeight;
    }

    void
    NeighborsSet::ClearNeighborhood ()
    {
      m_neighborProbVector.clear();
    }

    Neighbor
    NeighborsSet::SelectNeighbor (PeerPolicy policy)
    {
      NS_ASSERT(policy>=PS_RANDOM && policy <= PS_BROADCAST);
      Purge();
      Neighbor target;
      if (!GetSize())
        return target;
      switch (policy)
        {
        case PS_RANDOM:
          {
            target = SelectPeerByRandom();
            break;
          }
        case PS_DELAY:
          {
            NS_ASSERT_MSG(false, "SelectNeighbor: Not yet implemented.");
            break;
          }
        case PS_ROUNDROBIN:
          {
            NS_ASSERT_MSG(false, "SelectNeighbor: Not yet implemented.");
            break;
          }
        case PS_SINR:
          {
            target = SelectPeerBySINR();
            break;
          }
        case PS_BROADCAST:
          {
            target = Neighbor(Ipv4Address::GetBroadcast(),0);
            break;
          }
        default:
          {
            NS_ASSERT_MSG(false, "SelectNeighbor: invalid.");
            break;
          }
        }
      return target;
    }

    Neighbor
    NeighborsSet::SelectPeerByRandom ()
    {
      Neighbor nt;
      return (GetSize() == 0 ? nt : GetNeighbor (UniformVariable().GetInteger(0, GetSize() - 1)));
    }

    void
    NeighborsSet::SortNeighborhood (PeerPolicy policy)
    {
//      for (std::map<Neighbor, NeighborData>::const_iterator iter = m_neighbor_set.begin(); iter != m_neighbor_set.end();
//          iter++)
//        NS_LOG_DEBUG ("Neighbor="<< iter->first <<" " << iter->second);
      size_t nsize = m_neighbor_set.size();
      m_neighborProbVector.reserve(nsize);
      m_neighborProbVector = std::vector<NeigborPair>(m_neighbor_set.begin(), m_neighbor_set.end());
      NS_ASSERT(m_neighborProbVector.size() == nsize);
      std::sort(m_neighborProbVector.begin(), m_neighborProbVector.end(), SnrCmp());
      uint32_t limit = 10;
      nsize = (nsize < limit ? nsize : limit);
//      while (m_neighborProbVector.size() > nsize)
//        {
//          m_neighborProbVector.pop_back();
//        }
      m_neighbor_set = std::map<Neighbor, NeighborData>(m_neighborProbVector.begin(),
          m_neighborProbVector.begin() + nsize);
      NS_ASSERT(m_neighbor_set.size() == nsize);
//      for (std::vector<std::pair<Neighbor, NeighborData> >::const_iterator iter = m_neighborPairRssi.begin();
//          iter != m_neighborPairRssi.end(); iter++)
//        {
//          NS_LOG_DEBUG ("Neighbor="<< iter->first <<" Data=" << iter->second << " Size "<< m_neighborPairRssi.size());
//        }
      double weight1[nsize];
      double weights1 = 0;
      double weight2[nsize];
      double weights2 = 0;
      double tot = 0;
      m_neighborProbability = new double[nsize];
      uint32_t i = 0;
      for (std::vector<std::pair<Neighbor, NeighborData> >::const_iterator iter = m_neighborProbVector.begin();
          i < nsize && iter != m_neighborProbVector.end(); iter++, i++)
        {
          switch (policy)
            {
            case PS_SINR:
              {
                weight1[i] = iter->second.GetSINR();
                weight2[i] = iter->second.GetChunkRatio();
                break;
              }
            default:
              {
                NS_ASSERT_MSG(false, "INVALID PEER SELECTION");
                break;
              }
            }
          //      NS_LOG_DEBUG ("Neighbor "<< iter->first << " w=" << weight[i] << "/"<<weights);
          weights1 += weight1[i];
          weights2 += weight2[i];
        }
      double q = GetSelectionWeight();
      for (i = 0; i < nsize; i++)
        {
          double w1 = ((1.0 - q) * (weights1 == 0 ? 1.0 / nsize : weight1[i] / weights1));
          double w2 = (q * (weights2 == 0 ? 1.0 / nsize : weight2[i] / weights2));
          m_neighborProbability[i] = w1 + w2;
          NS_LOG_DEBUG ("Neighbor "<< i << " w1=" <<w1<< "("<< weight1[i] << "/"<< weights1 <<") w2=" << w2 <<"("<<weight2[i] << "/"<< weights2 << ") P=" << m_neighborProbability[i]);
          tot += m_neighborProbability[i];
        }
//      i = 0;
//      for (std::vector<std::pair<Neighbor, NeighborData> >::const_iterator iter = m_neighborPairRssi.begin();
//          iter != m_neighborPairRssi.end(); iter++, i++)
//        {
//          NS_LOG_DEBUG ("Neighbor="<< iter->first <<" Data=" << iter->second << " Prob="<< (weight[i] / weights) );
//        } NS_LOG_DEBUG ("Neighbors P(all)=" << tot);
      NS_ASSERT_MSG(abs(1-tot)< pow10(-6), "Error in computing probabilities");
    }

    Neighbor
    NeighborsSet::SelectPeerBySINR ()
    {
      Neighbor nt;
      if (m_neighbor_set.empty())
        {
          return nt;
        }
      NS_ASSERT(GetSize() > 0);
      size_t msize = m_neighbor_set.size();
      if (m_neighborProbVector.empty() && m_neighborProbVector.size() != msize)
        SortNeighborhood(PS_SINR);
      size_t nsize = m_neighbor_set.size();
      double random = UniformVariable().GetValue();
      uint32_t id = 0;
      while (random > 0 && nt.GetAddress() == Ipv4Address(Ipv4Address::GetAny()) && id < nsize)
        {
          NeighborData *nd = GetNeighbor(m_neighborProbVector[id].first);
          NS_ASSERT(nd);
          random -= m_neighborProbability[id];
          if (random <= 0)
            {
              nt = m_neighborProbVector[id].first;
            }
          id++;
        }
      if (nt.GetAddress() == Ipv4Address() && id >= nsize)
        {
          id = 0;
          nt = m_neighborProbVector[id].first;
        }
      return nt;
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
    NeighborsSet::Clear ()
    {
      m_neighbor_set.clear();
    }

} //namespace ns3


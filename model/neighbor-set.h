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
 */

#ifndef __NEIGHBORS_SET_H__
#define __NEIGHBORS_SET_H__

#include "neighbor.h"
#include <ns3/object.h>
#include <ns3/simulator.h>
#include <ns3/random-variable.h>
#include <map>

namespace ns3
{
using namespace streaming;

    enum PeerPolicy
    {
      PS_RANDOM, PS_DELAY, PS_SINR, PS_ROUNDROBIN, PS_BROADCAST
    };

    enum PeerState
    {
      UNKNOWN, ACTIVE, INACTIVE
    };

    struct NeighborData
    {
        NeighborData () :
            n_contact(Simulator::Now()), n_state(ACTIVE), n_bufferSize(0), n_latestChunk(0), n_sinr(0),
            n_chunksRatio(0.0)
        {
        }
        NeighborData (Time start, PeerState state, uint32_t size, uint32_t c_id, double sinr, double cratio) :
            n_contact(start), n_state(state), n_bufferSize(size), n_latestChunk(c_id), n_sinr(sinr),
            n_chunksRatio(cratio)
        {
        }
        Time n_contact;                 /// Last contact.
        enum PeerState n_state;         /// Neighbor state.
        uint32_t n_bufferSize;          /// Neighbor buffer size.
        uint32_t n_latestChunk;         /// Neighbor latest chunk.
        double n_sinr;                  /// Neighbor SINR.
        double n_chunksRatio;           /// Neighbor chunks' ratio.

        /**
         * \return time last contact.
         * Get neighbor last contact.
         */

        Time
        GetLastContact () const;

        /**
         * \param contact Time contact.
         * Set neighbor contact.
         */

        void
        SetLastContact (Time contact);

        /**
         * \return Neighbor state.
         * Get neighbor state.
         */
        PeerState
        GetPeerState () const;

        /**
         * \param state neighbor state
         * Set neighbor state.
         */
        void
        SetPeerState (PeerState state);

        /**
         * \return buffer size
         * Get neighbor buffer size
         */
        uint32_t
        GetBufferSize () const;

        /**
         * \param buffer size
         * Set neighbor buffer size
         */
        void
        SetBufferSize (uint32_t size);

        /**
         * \return Neighbor last chunk.
         * Get neighbor last chunk.
         */
        uint32_t
        GetLastChunk () const;

        /**
         * \param Neighbor last chunk.
         * Set neighbor last chunk.
         */
        void
        SetLastChunk (uint32_t last);

        /**
         *
         * \return Neighbor SINR.
         * Get neighbor SINR.
         */
        double
        GetSINR () const;

        /**
         * \param Neighbor SINR.
         * Set neighbor SINR.
         */
        void
        SetSINR (double sinr);

        /**
         * \return Neighbor chunk ratio.
         * Get neighbor chunk ratio.
         */

        double
        GetChunkRatio () const;

        /**
         * \param Neighbor chunk ratio.
         * Set neighbor chunk ratio.
         */

        void
        SetChunkRatio (double ratio);

        /**
         *
         * \param size chunk buffer size.
         * \param last last chunk received.
         * \param ratio chunks' ratio.
         * Update neighbor information.
         */
        void
        Update (uint32_t size, uint32_t last, double ratio);
    };

    static inline std::ostream&
    operator << (std::ostream& outStream, const NeighborData& neighbor)
    {
      return outStream << "State=" << neighbor.n_state << " Contact=" << neighbor.n_contact << " Sinr="
          << neighbor.n_sinr << " Ratio=" << neighbor.n_chunksRatio;
    }

    /**
     * \brief Define a data structure to collect neighbors.
     *
     * Define a data structure to collect information of neighbors.
     */

    class NeighborsSet
    {

      public:
        NeighborsSet ();
        virtual
        ~NeighborsSet ();
        typedef std::pair<Neighbor, NeighborData> NeigborPair;

      public:
        /**
         * \param neighbor Neighbor
         * \param data Neighbor data
         * \return True if success, false otherwise.
         *
         * Add a neighbor and its data.
         */
        bool
        AddNeighbor (const Neighbor neighbor, NeighborData data);

        /**
         * \param neighbor Neighbor
         * \return True if success, false otherwise.
         *
         * Add a neighbor.
         */
        bool
        AddNeighbor (Neighbor neighbor);

        /**
         * \param addr Address
         * \param port Port
         * \return True if success, false otherwise.
         * Delete a neighbor by address and port
         */
        bool
        DelNeighbor (Ipv4Address addr, uint32_t port);

        /**
         * \param neighbor Neighbor
         * \return True if success, false otherwise.
         * Delete a neighbor by Neighbor strucure.
         */
        bool
        DelNeighbor (Neighbor neighbor);

        /**
         * \param Neighbor
         * \return True if is a neighbor, false otherwise.
         * Check whether a node is a neighbor or not.
         */
        bool
        IsNeighbor (const Neighbor neighbor);

        /**
         * \return Neighborhood size.
         * Number of neighbors.
         */
        size_t
        GetSize ();

        /**
         * \param addr Address.
         * \param port Port.
         * \return Pointer to neighbor, null otherwise.
         * Get neighbor by address and port.
         */
        NeighborData*
        GetNeighbor (Ipv4Address addr, uint32_t port);

        /**
         * \param neighbor Neighbor entity.
         * \return Pointer to neighbor, null otherwise.
         * Get neighbor by neighbor data.
         */
        NeighborData*
        GetNeighbor (Neighbor neighbor);

        /**
         * \param index Neighbor index in the list
         * \return Neighbor.
         * Get the i-th neighbor
         */
        Neighbor
        GetNeighbor (uint32_t index);

        /**
         * Remove expired neighbors entries.
         */
        void
        Purge ();

        /**
         * \param weight value.
         * Weight for the selection function.
         */
        void
        SetSelectionWeight (double weight);

        /**
         * \return Weight value.
         * Get the weight value.
         */
        double
        GetSelectionWeight () const;

        /**
         * Clear the neighbor vector.
         */
        void
        ClearNeighborhood ();

        /**
         * \param policy Peer selection policy.
         * \return Neighbor.
         * Select a neighbor according to the given policy.
         */
        Neighbor
        SelectNeighbor (PeerPolicy policy);

        /**
         * \return Neighbor.
         * Select a neighbor randomly.
         */
        Neighbor
        SelectPeerByRandom ();

        /**
         *
         * \param policy Criteria used to sort the neighbor vector.
         * Sort the neighbor according to a given criteria.
         */
        void
        SortNeighborhood (PeerPolicy policy);

        /**
         * \return Select a Neighbor by SINR
         * Get a neighbor by SINR
         */
        Neighbor
        SelectPeerBySINR ();

        /**
         * \param time Neighbor lifetime
         * Set Neighbor lifetime
         */
        void
        SetExpire (Time time);

        /**
         * \return Neighbor lifetime
         * Get Neighbor lifetime
         */
        Time
        GetExpire () const;

        /**
         * Clear neighbor data structure.
         */
        void
        Clear ();

      protected:
        std::map<Neighbor, NeighborData> m_neighbor_set; /// Map of neighbors.
        double m_selectionWeight;                        /// Weight used for peer selection.
        double *m_neighborProbability;                   /// Pointer to array of probabilities.
        std::vector<NeigborPair> m_neighborProbVector;   /// Vector of neighbor pair to compute probabilities.
        Time m_expire;                                   /// Neighbor record expiration.

        struct SnrCmp
        {
            bool
            operator() (const NeigborPair &lhs, const NeigborPair &rhs)
            {
              return lhs.second.GetSINR() > rhs.second.GetSINR();
            }
        };
    };
} // namespace ns3
#endif

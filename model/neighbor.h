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

#ifndef __NEIGHBOR_H__
#define __NEIGHBOR_H__

#include <stdint.h>
#include "chunk-video.h"
#include <ns3/ipv4-address.h>
#include <ns3/nstime.h>

namespace ns3
{
    /**
     * Data structure containing neighbors' information.
     */
    struct Neighbor
    {
        Neighbor () :
            n_address(Ipv4Address::GetAny()), n_port(0)
        {
        }
        Neighbor (Ipv4Address n, uint32_t i) :
            n_address(n), n_port(i)
        {
        }
        Ipv4Address n_address; /// Neighbor address.
        uint32_t n_port;       /// Neighbor port.

        /**
         *
         * \return Neighbor address.
         * Get neighbor address.
         */

        Ipv4Address
        GetAddress ();

        /**
         *
         * \return Neighbor port.
         * Get neighbor port.
         */
        uint32_t
        GetPort ();
    };

    static inline bool
    operator == (const Neighbor &a, const Neighbor &b)
    {
      return (a.n_address == b.n_address) && (a.n_port == b.n_port);
    }

    static inline bool
    operator < (const Neighbor &a, const Neighbor &b)
    {
      return (a.n_address < b.n_address);
    }

    static inline std::ostream&
    operator << (std::ostream& outStream, const Neighbor& neighbor)
    {
      return outStream << "IP=" << neighbor.n_address << " Port=" << neighbor.n_port;
    }

} // namespace ns3
#endif /* __NEIGHBOR_H__ */

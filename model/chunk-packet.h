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

#ifndef __CHUNK_HEADER_H__
#define __CHUNK_HEADER_H__

#include "chunk-video.h"
#include <stdio.h>
#include <ns3/header.h>
#include <ns3/ipv4-address.h>
#include <iostream>

const uint32_t CHUNK_HEADER_SIZE = 4;
const uint32_t MSG_CHUNK_SIZE = (4 + 8 + 2 + 2);
const uint32_t MSG_PULL_SIZE = 4;
const uint32_t MSG_HELLO_SIZE = 4 * 3;

enum ChunkMessageType
{
  MSG_PULL, MSG_CHUNK, MSG_HELLO
};

namespace ns3
{
  namespace streaming
  {
    /**
     * \ingroup VideoPushAppliaction
     * \brief Define the chunk header format.
     *
     */

//	0              1               2               3
//	0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//	|     Type      |   Reserved    |            Checksum           |
//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    class ChunkHeader : public Header
    {
      public:
        ChunkHeader (ChunkMessageType type);
        ChunkHeader ();
        virtual
        ~ChunkHeader ();

      private:
        ChunkMessageType m_type;
        uint8_t m_reserved;
        uint16_t m_checksum;

      public:
        ///\name Header serialization/deserialization
        //\{
        static TypeId
        GetTypeId (void);
        virtual TypeId
        GetInstanceTypeId (void) const;
        virtual void
        Print (std::ostream &os) const;
        virtual uint32_t
        GetSerializedSize (void) const;
        virtual void
        Serialize (Buffer::Iterator start) const;
        virtual uint32_t
        Deserialize (Buffer::Iterator start);
        virtual ChunkMessageType
        GetType ();
        virtual void
        SetType (ChunkMessageType type);
        virtual uint8_t
        GetReserved ();
        virtual void
        SetReserved (uint8_t reserved);
        virtual uint16_t
        GetChecksum ();
        virtual void
        SetChecksum (uint16_t checksum);

        //\}

        //	0               1               2               3
        //	0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
        //	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //	|                      Chunk Identifier                         |
        //	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //	|                          Chunk                                |
        //	|                        Timestamp                              |
        //	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //	|                         Chunk Size                            |
        //	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //	|                   Chunk Attributes Size                       |
        //	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //	|                        Chunk Data                          ....
        //	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //	|                        Chunk Attributes                    ....
        //	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

        struct ChunkMessage
        {
            ChunkMessage():
              m_chunk()
            {};
            ChunkMessage(ChunkVideo chunk):
              m_chunk(chunk)
            {};
            virtual ~ChunkMessage();
            ChunkVideo m_chunk; // Chunk Data
            virtual void
            Print (std::ostream &os) const;
            virtual uint32_t
            GetSerializedSize (void) const;
            virtual void
            Serialize (Buffer::Iterator start) const;
            virtual uint32_t
            Deserialize (Buffer::Iterator start);
            virtual ChunkVideo
            GetChunk ();
            virtual void
            SetChunk (ChunkVideo chunk);
        };

        //	0               1               2               3
        //	0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
        //	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //	|                      Chunk Identifier                         |
        //	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

        struct PullMessage
        {
            PullMessage (uint32_t chunkId):
              m_chunkID (chunkId)
            {};
            PullMessage ():
              m_chunkID (0)
            {};
            virtual ~PullMessage();
            uint32_t m_chunkID; // Chunk ID to pull
            virtual void
            Print (std::ostream &os) const;
            virtual uint32_t
            GetSerializedSize (void) const;
            virtual void
            Serialize (Buffer::Iterator start) const;
            virtual uint32_t
            Deserialize (Buffer::Iterator start);
            virtual uint32_t
            GetChunk ();
            virtual void
            SetChunk (uint32_t chunkid);
        };

        //	0               1               2               3
        //	0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
        //	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //	|                      Chunks Received							|
        //	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

        struct HelloMessage
        {
            HelloMessage ():
              m_lastChunk (0), m_chunksRec (0), m_chunksRatio (0)
              {}
            HelloMessage (uint32_t last, uint32_t rec, uint32_t ratio):
              m_lastChunk (last), m_chunksRec (rec), m_chunksRatio (ratio)
              {}
            virtual ~HelloMessage ();
//	  Ipv4Address m_destination; // Destination Address
            uint32_t m_lastChunk; /// Chunks received
            uint32_t m_chunksRec; /// Chunks received
            uint32_t m_chunksRatio; /// Chunks ratio
//  	  uint32_t m_neighborhoodSize; // Neighborhood size
            virtual void
            Print (std::ostream &os) const;
            virtual uint32_t
            GetSerializedSize (void) const;
            virtual void
            Serialize (Buffer::Iterator start) const;
            virtual uint32_t
            Deserialize (Buffer::Iterator start);
//  	  virtual Ipv4Address GetDestination ();
//	  virtual void SetDestination (Ipv4Address destination);
            virtual uint32_t
            GetLastChunk ();
            virtual void
            SetLastChunk (uint32_t last);
            virtual uint32_t
            GetChunksReceived ();
            virtual void
            SetChunksReceived (uint32_t chunksRec);
            virtual uint32_t
            GetChunksRatio ();
            virtual void
            SetChunksRatio (uint32_t chunksRec);
//  	  virtual uint32_t GetNeighborhoodSize ();
//  	  virtual void SetNeighborhoodSize (uint32_t neighSize);
        };

      private:
        struct
        {
            ChunkMessage chunk;
            PullMessage pull;
            HelloMessage hello;
        } m_chunk_message;

      public:

        ChunkMessage&
        GetChunkMessage ()
        {
          if (m_type == 0)
            {
              m_type = MSG_CHUNK;
            }
          else
            {
              NS_ASSERT(m_type == MSG_CHUNK);
            }
          return m_chunk_message.chunk;
        }

        PullMessage&
        GetPullMessage ()
        {
          if (m_type == 0)
            {
              m_type = MSG_PULL;
            }
          else
            {
              NS_ASSERT(m_type == MSG_PULL);
            }
          return m_chunk_message.pull;
        }

        HelloMessage&
        GetHelloMessage ()
        {
          if (m_type == 0)
            {
              m_type = MSG_HELLO;
            }
          else
            {
              NS_ASSERT(m_type == MSG_HELLO);
            }
          return m_chunk_message.hello;
        }

    };

  } //end namespace video
} //end namespace ns3

#endif /* __CHUNK_HEADER_H__ */

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
 * Source:  http://www.ietf.org/rfc/rfc3973.txt
 *
 * Authors: Alessandro Russo <russo@disi.unitn.it>
 *          University of Trento, Italy
 *          University of California, Los Angeles U.S.A.
 */

#ifndef __CHUNK_HEADER_H__
#define __CHUNK_HEADER_H__

#include <stdio.h>
#include "ns3/header.h"
#include "chunk-video.h"
#include <iostream>

#define CHUNK_HEADER_SIZE 4
#define CHUNK_SIZE (4 + 4 + 1 + 8 + 4)
#define CHUNK_ID_SIZE 4

enum ChunkMessageType
{
	PULL,
	CHUNK
};

namespace ns3 {
namespace streaming {


/**
* \ingroup VideoPushAppliaction
* \brief Chunk Header
*/

//	0               1               2               3
//	0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//	|     Type      |          Reserved             |               |
//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

class ChunkHeader : public Header
{
public:
  ChunkHeader (ChunkMessageType type);
  ChunkHeader ();
  virtual ~ChunkHeader ();

private:
  ChunkMessageType m_type;
  uint8_t m_reserved;
  uint16_t m_checksum;

public:
  ///\name Header serialization/deserialization
  //\{
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual ChunkMessageType GetType ();
  virtual void SetType (ChunkMessageType type);
  virtual uint8_t GetReserved ();
  virtual void SetReserved (uint8_t reserved);
  virtual uint16_t GetChecksum ();
  virtual void SetChecksum (uint16_t checksum);

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
	  ChunkVideo m_chunk; // Chunk Data
	  virtual void Print (std::ostream &os) const;
	  virtual uint32_t GetSerializedSize (void) const;
	  virtual void Serialize (Buffer::Iterator start) const;
	  virtual uint32_t Deserialize (Buffer::Iterator start);
	  virtual ChunkVideo GetChunk ();
	  virtual void SetChunk (ChunkVideo chunk);
  };

  //	0               1               2               3
  //	0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
  //	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //	|                      Chunk Identifier                         |
  //	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  struct PullMessage
  {
	  uint32_t m_chunkID; // Chunk ID to pull
	  virtual void Print (std::ostream &os) const;
	  virtual uint32_t GetSerializedSize (void) const;
	  virtual void Serialize (Buffer::Iterator start) const;
	  virtual uint32_t Deserialize (Buffer::Iterator start);
	  virtual uint32_t GetChunk ();
	  virtual void SetChunk (uint32_t chunkid);
  };

private:
	struct{
		ChunkMessage chunk;
		PullMessage pull;
	} m_chunk_message;


public:

  ChunkMessage& GetChunkMessage ()
  {
    if (m_type == 0)
      {
    	m_type = CHUNK;
      }
    else
      {
        NS_ASSERT (m_type == CHUNK);
      }
    return m_chunk_message.chunk;
  }

  PullMessage& GetPullMessage ()
  {
    if (m_type == 0)
      {
    	m_type = PULL;
      }
    else
      {
        NS_ASSERT (m_type == PULL);
      }
    return m_chunk_message.pull;
  }


};

}//end namespace video
}//end namespace ns3

#endif /* __PIM_DM__HEADER_H__ */

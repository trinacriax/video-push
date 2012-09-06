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

///
/// \file	pimdm-header.cc
/// \brief	This file implements the PIM DM packet headers and all the functions needed for their manipulation.

#include "chunk-packet.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include <iostream>

namespace ns3 {
namespace streaming {

NS_LOG_COMPONENT_DEFINE ("ChunkHeader");

NS_OBJECT_ENSURE_REGISTERED (ChunkHeader);


ChunkHeader::ChunkHeader (ChunkMessageType type) :
		m_type (type)
{}
ChunkHeader::ChunkHeader () :
		m_type (MSG_HELLO)
{}

ChunkHeader::~ChunkHeader ()
{}

TypeId
ChunkHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::video-push::ChunkHeader")
    .SetParent<Header> ()
    .AddConstructor<ChunkHeader> ()
    ;
  return tid;
}
TypeId
ChunkHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

ChunkMessageType
ChunkHeader::GetType ()
{
	return m_type;
}

void
ChunkHeader::SetType (ChunkMessageType type)
{
	m_type = type;
}

uint8_t
ChunkHeader::GetReserved ()
{
	return m_reserved;
}

void
ChunkHeader::SetReserved (uint8_t reserved)
{
	m_reserved = reserved;
}

uint16_t
ChunkHeader::GetChecksum ()
{
	return m_checksum;
}

void
ChunkHeader::SetChecksum (uint16_t checksum)
{
	m_checksum = checksum;
}

uint32_t
ChunkHeader::GetSerializedSize (void) const
{
  uint32_t size = CHUNK_HEADER_SIZE;
  switch (m_type)
  {
	  case MSG_PULL:
	  {
		  size += m_chunk_message.pull.GetSerializedSize();
		  break;
	  }
	  case MSG_CHUNK:
	  {
		  size += m_chunk_message.chunk.GetSerializedSize();
		  break;
	  }
	  case MSG_HELLO:
	  {
		  size += m_chunk_message.hello.GetSerializedSize();
		  break;
	  }
	  default:
	  {
		  NS_ASSERT (false);
		  break;
	  }
  }
  return size;
}

void
ChunkHeader::Print (std::ostream &os) const
{
  os<< "ChunkHeader: Type="<< m_type << ", Resv=" << (uint16_t)m_reserved << ", Checksum="<< m_checksum<< "\n";
}

void
ChunkHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  uint8_t type = (uint8_t)m_type;
  i.WriteU8 (type);
  i.WriteU8 (m_reserved);
  i.WriteHtonU16 (m_checksum);
  switch (m_type)
  {
	  case MSG_PULL:
	  {
		  m_chunk_message.pull.Serialize(i);
		  break;
	  }
	  case MSG_CHUNK:
	  {
		  m_chunk_message.chunk.Serialize(i);
		  break;
	  }
	  case MSG_HELLO:
	  {
		  m_chunk_message.hello.Serialize(i);
		  break;
	  }
	  default:
	  {
		  NS_ASSERT (false);
		  break;
	  }
  }
}

uint32_t
ChunkHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint32_t size = 0;
  m_type = ChunkMessageType (i.ReadU8());
  size +=1;
  m_reserved = i.ReadU8 ();
  size +=1;
  m_checksum = i.ReadNtohU16 ();
  size +=2;
  switch (m_type)
  {
	  case MSG_PULL:
	  {
		  size += m_chunk_message.pull.Deserialize (i);
		  break;
	  }
	  case MSG_CHUNK:
	  {
		  size += m_chunk_message.chunk.Deserialize (i);
		  break;
	  }
	  case MSG_HELLO:
	  {
		  size += m_chunk_message.hello.Deserialize (i);
		  break;
	  }
	  default:
	  {
		  NS_ASSERT (false);
		  break;
	  }
  }
  return size;
}

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

uint32_t
ChunkHeader::ChunkMessage::GetSerializedSize (void) const
{
  uint32_t size = MSG_CHUNK_SIZE;
  return size;
}

void
ChunkHeader::ChunkMessage::Print (std::ostream &os) const
{
  os<< "ChunkHeader "<< m_chunk <<"\n";
}

void
ChunkHeader::ChunkMessage::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtonU32 (m_chunk.c_id);
  i.WriteHtonU64 (m_chunk.c_tstamp);
  i.WriteHtonU16 (m_chunk.c_size);
  i.WriteHtonU16 (m_chunk.c_attributes_size);
//  for(uint32_t s = 0; s < m_chunk.c_size ; s++){
//  	  i.WriteU8(m_chunk.c_data[s]);
//    }
//  for(uint32_t s = 0; s < m_chunk.c_attributes_size ; s++){
//	i.WriteU8(m_chunk.c_attributes[s]);
//  }
}

uint32_t
ChunkHeader::ChunkMessage::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint32_t size = 0;
  m_chunk.c_id = i.ReadNtohU32();
  size +=4;
  m_chunk.c_tstamp = i.ReadNtohU64();
  size +=8;
  m_chunk.c_size = i.ReadNtohU16();
  size +=2;
  m_chunk.c_attributes_size = i.ReadNtohU16();
  size +=2;
//  m_chunk.c_data = (uint8_t*)calloc(m_chunk.c_size, sizeof(uint8_t));
//  for(uint32_t s = 0; s < m_chunk.c_size ; s++){
//	m_chunk.c_data[s] = i.ReadU8();
//  }
//  size += m_chunk.c_size;
//  m_chunk.c_attributes = (uint8_t*)calloc(m_chunk.c_attributes_size , sizeof(uint8_t));
//  for(uint32_t s = 0; s < m_chunk.c_attributes_size ; s++){
//  	m_chunk.c_attributes[s] = i.ReadU8();
//  }
//  size += m_chunk.c_attributes_size;
  return size;
}

ChunkVideo
ChunkHeader::ChunkMessage::GetChunk ()
{
	return m_chunk;
}

void
ChunkHeader::ChunkMessage::SetChunk (ChunkVideo chunk)
{
	m_chunk = chunk;
}

//	0               1               2               3
//	0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//	|                      Chunk Identifier                         |
//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

uint32_t
ChunkHeader::PullMessage::GetSerializedSize (void) const
{
  uint32_t size = MSG_PULL_SIZE;
  return size;
}

void
ChunkHeader::PullMessage::Print (std::ostream &os) const
{
  os<< "Pull chunk: "<< m_chunkID <<"\n";
}

void
ChunkHeader::PullMessage::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtonU32 (m_chunkID);
}

uint32_t
ChunkHeader::PullMessage::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint32_t size = MSG_PULL_SIZE;
  m_chunkID = i.ReadNtohU32();
  return size;
}

uint32_t
ChunkHeader::PullMessage::GetChunk ()
{
	return m_chunkID;
}

void
ChunkHeader::PullMessage::SetChunk (uint32_t chunk)
{
	NS_ASSERT (chunk>0);
	m_chunkID = chunk;
}

//	0               1               2               3
//	0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//	|                      Chunks Received							|
//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

uint32_t
ChunkHeader::HelloMessage::GetSerializedSize (void) const
{
  uint32_t size = MSG_HELLO_SIZE;
  return size;
}

void
ChunkHeader::HelloMessage::Print (std::ostream &os) const
{
  os<< "Destination: " << m_destination << ", Last Chunk: " << m_lastChunk<< ", Received: "<< m_chunksRec << ", Ratio: " << m_chunksRatio <<"\n";
}

void
ChunkHeader::HelloMessage::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtonU32 (m_destination.Get());
  i.WriteHtonU32 (m_lastChunk);
  i.WriteHtonU32 (m_chunksRec);
  i.WriteHtonU32 (m_chunksRatio);
}

uint32_t
ChunkHeader::HelloMessage::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint32_t size = MSG_HELLO_SIZE;
  m_destination = Ipv4Address(i.ReadNtohU32());
  m_lastChunk = i.ReadNtohU32();
  m_chunksRec = i.ReadNtohU32();
  m_chunksRatio = i.ReadNtohU32();
  return size;
}

Ipv4Address
ChunkHeader::HelloMessage::GetDestination()
{
	return m_destination;
}

void
ChunkHeader::HelloMessage::SetDestination (Ipv4Address destination)
{
	m_destination = destination;
}

uint32_t
ChunkHeader::HelloMessage::GetLastChunk ()
{
	return m_lastChunk;
}

void
ChunkHeader::HelloMessage::SetLastChunk (uint32_t last)
{
	NS_ASSERT (last>=0);
	m_lastChunk = last;
}

uint32_t
ChunkHeader::HelloMessage::GetChunksReceived ()
{
	return m_chunksRec;
}

void
ChunkHeader::HelloMessage::SetChunksReceived (uint32_t chunks)
{
	NS_ASSERT (chunks>=0);
	m_chunksRec = chunks;
}

uint32_t
ChunkHeader::HelloMessage::GetChunksRatio ()
{
	return m_chunksRatio;
}

void
ChunkHeader::HelloMessage::SetChunksRatio (uint32_t chunks)
{
	NS_ASSERT (chunks>=0);
	m_chunksRatio = chunks;
}
} // namespace pidm
} // namespace ns3

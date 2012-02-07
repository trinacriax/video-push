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

ChunkHeader::ChunkHeader (ChunkVideo chunk)
{m_chunk = chunk;}

ChunkHeader::ChunkHeader ()
{m_chunk = ChunkVideo();}

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

uint32_t
ChunkHeader::GetSerializedSize (void) const
{
  uint32_t size = 4 + 4 + 1 + 8 + 4;// + m_chunk.c_attributes_size+ m_chunk.c_size ;
  return size;
}

ChunkVideo* ChunkHeader::GetChunk() {
	return m_chunk.Copy();
}

void
ChunkHeader::Print (std::ostream &os) const
{
  os<< "ChunkHeader "<< m_chunk <<"\n";
}

void
ChunkHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtonU32 (m_chunk.c_id);
  i.WriteHtonU64 (m_chunk.c_tstamp);
  i.WriteHtonU32 (m_chunk.c_size);
  i.WriteHtonU32 (m_chunk.c_attributes_size);
//  for(uint32_t s = 0; s < m_chunk.c_size ; s++){
//  	  i.WriteU8(m_chunk.c_data[s]);
//    }
//  for(uint32_t s = 0; s < m_chunk.c_attributes_size ; s++){
//	i.WriteU8(m_chunk.c_attributes[s]);
//  }
}

uint32_t
ChunkHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint32_t size = 0;
  m_chunk.c_id = i.ReadNtohU32();
  size +=4;
  m_chunk.c_tstamp = i.ReadNtohU64();
  size +=8;
  m_chunk.c_size = i.ReadNtohU32();
  size +=4;
  m_chunk.c_attributes_size = i.ReadNtohU32();
  size +=4;
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

} // namespace pidm
} // namespace ns3

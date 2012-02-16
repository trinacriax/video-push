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

namespace ns3 {
namespace streaming {

/**
* \ingroup VideoPushAppliaction
* \brief Chunk Header
*/

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



class ChunkHeader : public Header
{
public:
  ChunkHeader (ChunkVideo chunk);
  ChunkHeader ();
  virtual ~ChunkHeader ();

  ///\name Main ChunkHeader methods
  //\{
  ChunkVideo GetChunk() const {return m_chunk;}
  void SetChunk(ChunkVideo chunk){ m_chunk = chunk;}
  //\}

private:
  ChunkVideo m_chunk;

public:
  ///\name Header serialization/deserialization
  //\{
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual ChunkVideo GetChunk();
  //\}
};
  static inline std::ostream&
  operator << (std::ostream& o, const ChunkHeader &a)
  {
	  ChunkHeader chunk =  a.GetChunk();
  return o<<"Chunk: "<< chunk<<"\n";
  }


}//end namespace mbn
}//end namespace ns3

#endif /* __PIM_DM__HEADER_H__ */

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

#include "chunk-buffer.h"
#include <memory.h>
#include <ns3/log.h>

NS_LOG_COMPONENT_DEFINE("ChunkBuffer");

namespace ns3
{

  ChunkBuffer::ChunkBuffer () :
      last(0)
  {
    chunk_buffer.clear();
    chunk_state.clear();
  }

  ChunkBuffer::~ChunkBuffer ()
  {
    chunk_buffer.clear();
    chunk_state.clear();
  }

  ChunkVideo*
  ChunkBuffer::GetChunk (uint32_t chunkId)
  {
    NS_ASSERT(chunkId>0);
    ChunkVideo *copy = 0;
    if (HasChunk(chunkId))
      copy = &(chunk_buffer.find(chunkId)->second);
    return copy;
  }

  bool
  ChunkBuffer::HasChunk (uint32_t chunkId)
  {
    NS_ASSERT(chunkId>0);
    return (chunk_buffer.find(chunkId) != chunk_buffer.end());
  }

  bool
  ChunkBuffer::AddChunk (const ChunkVideo &chunk, ChunkState state)
  {
    NS_ASSERT(state==CHUNK_RECEIVED_PUSH||state==CHUNK_RECEIVED_PULL);
    bool ret = false;
    if (!HasChunk(chunk.c_id))
      {
        chunk_buffer.insert(std::pair<uint32_t, ChunkVideo>(chunk.c_id, chunk));
        SetChunkState(chunk.c_id, state);
        last = (chunk.c_id > last) ? chunk.c_id : last;
        ret = true;
      }
    return ret;
  }

  bool
  ChunkBuffer::DelChunk (uint32_t chunkId)
  {
    NS_ASSERT(chunkId>0);
    bool ret = chunk_buffer.erase(chunkId);
//    if(ret && last == chunkId)
//      while(!HasChunk(--last));
    return ret;
  }

  const size_t
  ChunkBuffer::GetBufferSize ()
  {
    return chunk_buffer.size();
  }

  std::map<uint32_t, ChunkVideo>
  ChunkBuffer::GetChunkBuffer ()
  {
    return chunk_buffer;
  }

  std::string
  ChunkBuffer::PrintBuffer ()
  {
    std::stringstream buf;
    for (std::map<uint32_t, ChunkVideo>::iterator iter = chunk_buffer.begin(); iter != chunk_buffer.end(); iter++)
      {
        buf << iter->first << ", ";
      }
    return buf.str();
  }

  uint32_t
  ChunkBuffer::GetLatestMissed (uint32_t base, uint32_t window)
  {
    uint32_t missed = (base + window <= last ? base + window : last);
    uint32_t low = base;
    low = low < 1 ? 1 : low;
    while (missed >= low && (HasChunk(missed) || GetChunkState(missed)==CHUNK_SKIPPED || GetChunkState(missed)==CHUNK_DELAYED))
      {
        missed--;
      }
    missed = (missed <= last ? missed : 0);
    missed = (missed >= low ? missed : 0);
    return missed;
  }

  uint32_t
  ChunkBuffer::GetLeastMissed (uint32_t base, uint32_t window)
  {
    NS_ASSERT(base >=0 && window > 0);
    uint32_t missed = (base<=1?1:base-1);
    while (++missed <= (base + window) && (HasChunk(missed) || GetChunkState(missed)==CHUNK_SKIPPED || GetChunkState(missed)==CHUNK_DELAYED));
    missed = (missed >= base && missed <= base + window ? missed : 0);
    return missed;
  }

  uint32_t
  ChunkBuffer::GetLastChunk ()
  {
    return last;
  }

  uint32_t
  ChunkBuffer::GetSize ()
  {
    return chunk_buffer.size();
  }

  void
  ChunkBuffer::SetChunkState (uint32_t chunkId, ChunkState state)
  {
    NS_ASSERT(chunkId>0);
    NS_ASSERT(
        ((state>=CHUNK_RECEIVED_PUSH && state<=CHUNK_RECEIVED_PULL) && HasChunk(chunkId)) || ((state>=CHUNK_SKIPPED && state<=CHUNK_MISSED) && !HasChunk(chunkId)));
    if (chunk_state.find(chunkId) == chunk_state.end())
      chunk_state.insert(std::pair<uint32_t, ChunkState>(chunkId, state));
    else
      chunk_state.find(chunkId)->second = state;
    NS_ASSERT(GetChunkState(chunkId) == state);
  }

  ChunkState
  ChunkBuffer::GetChunkState (uint32_t chunkId)
  {
    NS_ASSERT(chunkId>0);
    if (chunk_state.find(chunkId) == chunk_state.end())
      chunk_state.insert(std::pair<uint32_t, ChunkState>(chunkId, CHUNK_MISSED));
    return chunk_state.find(chunkId)->second;
  }

}


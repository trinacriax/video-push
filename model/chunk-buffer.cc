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

#include "chunk-buffer.h"
#include <memory.h>
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("ChunkBuffer");

namespace ns3{


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
	ChunkBuffer::GetChunk (uint32_t chunkid)
	{
		NS_ASSERT (chunkid>0);
		ChunkVideo *copy = 0;
		std::map<uint32_t, ChunkVideo>::iterator const result = chunk_buffer.find(chunkid);
		if (result != chunk_buffer.end())
			copy = &(result->second);
		return copy;
	}

	bool
	ChunkBuffer::HasChunk (uint32_t chunkid)
	{
		NS_ASSERT (chunkid>0);
		std::map<uint32_t, ChunkVideo>::iterator const result = chunk_buffer.find(chunkid);
		if (result != chunk_buffer.end())
			return true;
		return false;
	}

	bool
	ChunkBuffer::AddChunk (const ChunkVideo &chunk, ChunkState state)
	{
		NS_ASSERT (state==CHUNK_RECEIVED_PUSH||state==CHUNK_RECEIVED_PULL);
		std::map<uint32_t, ChunkVideo>::iterator result = chunk_buffer.find(chunk.c_id);
		if (result == chunk_buffer.end()){
			std::pair <uint32_t, ChunkVideo> entry (chunk.c_id, chunk);
			chunk_buffer.insert(entry);
			state = (chunk_state.find(chunk.c_id)==chunk_state.end()?state:CHUNK_RECEIVED_PULL);
			std::pair <uint32_t, ChunkState> sentry (chunk.c_id, state);
			chunk_state.insert(sentry);
			last = (chunk.c_id > last) ? chunk.c_id : last;
			return true;
		}
		return false;
	}

	bool
	ChunkBuffer::DelChunk(uint32_t chunkid)
	{
		NS_ASSERT (chunkid>0);
		return chunk_buffer.erase(chunkid);
	}

	const size_t
	ChunkBuffer::GetBufferSize ()
	{
		return chunk_buffer.size();
	}

	std::map<uint32_t, ChunkVideo>
	ChunkBuffer::GetChunkBuffer()
	{
		return chunk_buffer;
	}

	std::string
	ChunkBuffer::PrintBuffer()
	{
		std::stringstream buf;
		for(std::map<uint32_t, ChunkVideo>::iterator iter = chunk_buffer.begin();
			iter != chunk_buffer.end() ; iter++){
			buf<<iter->first<<", ";
		}
		return buf.str();
	}

	bool
	ChunkBuffer::ChunkSkipped (uint32_t chunkid)
	{
		NS_ASSERT (chunkid>0);
		bool ret = (chunk_state.find(chunkid) != chunk_state.end() && chunk_state.find(chunkid)->second == CHUNK_SKIPPED);
		return ret;
	}

	bool
	ChunkBuffer::ChunkMissed (uint32_t chunkid)
	{
		NS_ASSERT (chunkid>0);
		bool ret = (chunk_buffer.find(chunkid) == chunk_buffer.end());
		return ret;
	}

	bool
	ChunkBuffer::ChunkDelayed (uint32_t chunkid)
	{
		NS_ASSERT (chunkid>0);
		bool ret = (chunk_state.find(chunkid) != chunk_state.end() && chunk_state.find(chunkid)->second == CHUNK_DELAYED);
		return ret;
	}

	uint32_t
	ChunkBuffer::GetLatestMissed (uint32_t base, uint32_t window)
	{
		uint32_t missed = (base+window <= last ? base+window : last );
		int32_t low = base;
		low = low < 1 ? 1 : low;
		while (missed >= low && (HasChunk(missed) || ChunkSkipped (missed) || ChunkDelayed (missed)))
		{
			missed--;
		}
		missed = (missed<=last?missed:0);
		missed = (missed>=low?missed:0);
		return missed;
	}

	uint32_t
	ChunkBuffer::GetLeastMissed (uint32_t base, uint32_t window)
	{
		uint32_t missed = 1;
		int32_t low = base;
		missed = low < 1 ? 1 : low;
		uint32_t upper = (base+window <= last ? base+window : last );
		while (missed <= upper && (HasChunk(missed) || ChunkSkipped (missed) || ChunkDelayed (missed)))
		{
			missed++;
		}
		missed = (missed<=last?missed:0);
		missed = (missed>=low?missed:0);
		return missed;
	}

	uint32_t
	ChunkBuffer::GetLastChunk ()
	{
		return last;
	}

	void
	ChunkBuffer::SetChunkState (uint32_t chunkid, ChunkState state)
	{
		if (chunk_state.find(chunkid) == chunk_state.end())
			chunk_state.insert(std::pair <uint32_t, ChunkState> (chunkid, state));
		switch (state)
		{
			case CHUNK_RECEIVED_PULL:
			case CHUNK_RECEIVED_PUSH:
			{
				NS_ASSERT(HasChunk(chunkid));
				chunk_state.find(chunkid)->second = state;
				break;
			}
			case CHUNK_MISSED:
			case CHUNK_SKIPPED:
			case CHUNK_DELAYED:
			{
				NS_ASSERT(!HasChunk(chunkid));
				chunk_state.find(chunkid)->second = state;
				NS_ASSERT (ChunkSkipped(chunkid)||ChunkMissed(chunkid)||ChunkDelayed(chunkid));
				break;
			}
			default:
			{
				NS_ASSERT (true);
				break;
			}
		}
	}

	ChunkState
	ChunkBuffer::GetChunkState (uint32_t chunkid)
	{
		NS_ASSERT (chunkid>0);
		std::map<uint32_t, ChunkState>::iterator iter = chunk_state.find(chunkid);
		if (iter == chunk_state.end())
		{
			return CHUNK_MISSED;
		}
		else
			return iter->second;
	}

}



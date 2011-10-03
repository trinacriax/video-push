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

namespace ns3{

	ChunkVideo*
	ChunkBuffer::Copy (const ChunkVideo chunk){
		ChunkVideo *copy = new ChunkVideo(chunk.c_id, chunk.c_size, chunk.c_tstamp, chunk.c_attributes_size);
		copy->c_data = (uint8_t *) calloc(copy->c_size,sizeof(uint8_t));
		NS_ASSERT(copy->c_data);
		memcpy(copy->c_data, &chunk.c_data, chunk.c_size);
		copy->c_attributes = calloc(copy->c_attributes_size,sizeof(copy->c_data));
		NS_ASSERT(copy->c_attributes);
		memcpy(copy->c_attributes, &chunk.c_attributes, chunk.c_attributes_size);
		return copy;
	}

	ChunkVideo*
	ChunkBuffer::GetChunk (uint32_t index){
		ChunkVideo *copy = 0;
		std::map<uint32_t, ChunkVideo>::iterator const result = chunk_buffer.find(index);
		if (result != chunk_buffer.end())
			copy = Copy(result->second);
		return copy;
	}

	bool
	ChunkBuffer::AddChunk (const ChunkVideo chunk) {
		std::map<uint32_t, ChunkVideo>::iterator result = chunk_buffer.find(chunk.c_id);
		if (result == chunk_buffer.end()){
			ChunkVideo *copy = Copy(chunk);		;
			chunk_buffer.insert(std::pair <uint32_t, ChunkVideo> (chunk.c_id, *copy));
			return true;
		}
		return false;
	}

	bool
	ChunkBuffer::DelChunk(uint32_t index) {
		return chunk_buffer.erase(index);
	}

	const uint32_t
	ChunkBuffer::GetChunkSize (uint32_t index) {
		return GetChunk(index)->c_size;
	}

	const uint32_t
	ChunkBuffer::GetChunkSize (ChunkVideo chunk){
		return chunk.c_size;
	}
}



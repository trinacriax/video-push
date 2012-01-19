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


	ChunkBuffer::ChunkBuffer (){chunk_buffer.clear();}

	ChunkBuffer::~ChunkBuffer (){chunk_buffer.clear();}

	ChunkVideo*
	ChunkBuffer::GetChunk (uint32_t index){
		ChunkVideo *copy = 0;
		std::map<uint32_t, ChunkVideo>::iterator const result = chunk_buffer.find(index);
		if (result != chunk_buffer.end())
			copy = result->second.Copy();
		return copy;
	}

	bool
	ChunkBuffer::AddChunk (ChunkVideo chunk) {
		std::map<uint32_t, ChunkVideo>::iterator result = chunk_buffer.find(chunk.c_id);
		if (result == chunk_buffer.end()){
			ChunkVideo *copy = chunk.Copy();
			chunk_buffer.insert(std::pair <uint32_t, ChunkVideo> (chunk.c_id, *copy));
			return true;
		}
		return false;
	}

	bool
	ChunkBuffer::DelChunk(uint32_t index) {
		return chunk_buffer.erase(index);
	}

	const size_t
	ChunkBuffer::GetBufferSize (){
		return chunk_buffer.size();
	}

	std::map<uint32_t, ChunkVideo>
	ChunkBuffer::GetChunkBuffer(){
		return chunk_buffer;
	}


	std::string
	ChunkBuffer::PrintBuffer(){
		std::stringstream buf;
		for(std::map<uint32_t, ChunkVideo>::iterator iter = chunk_buffer.begin();
			iter != chunk_buffer.end() ; iter++){
			buf<<iter->first<<", ";
		}
		return buf.str();
	}
}



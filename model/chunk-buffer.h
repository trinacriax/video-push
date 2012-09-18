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

#ifndef __CHUNK_LIST_H__
#define __CHUNK_LIST_H__

#include "chunk-video.h"
#include "ns3/object.h"
#include <map>
#include <string>
namespace ns3{
using namespace streaming;

class ChunkBuffer{

public:

	ChunkBuffer ();

	virtual ~ChunkBuffer ();

	ChunkVideo* GetChunk (uint32_t index);
	bool HasChunk (uint32_t index);
	bool AddChunk (const ChunkVideo &chunk, ChunkState state);
	bool DelChunk (uint32_t index);

	const size_t GetBufferSize ();
	std::string PrintBuffer();
	std::map<uint32_t, ChunkVideo> GetChunkBuffer();
	ChunkState GetChunkState (uint32_t index);
	void SetChunkState (uint32_t chunkid, ChunkState state);
	bool ChunkMissed (uint32_t chunkid);
	bool ChunkDelayed (uint32_t chunkid);
	bool ChunkSkipped (uint32_t chunkid);
	uint32_t GetLeastMissed (uint32_t base, uint32_t window);
	uint32_t GetLatestMissed (uint32_t base, uint32_t window);
	uint32_t GetLastChunk ();

protected:
	std::map<uint32_t, ChunkVideo> chunk_buffer;
	std::map<uint32_t, ChunkState> chunk_state;
	uint32_t last;

};
}
#endif

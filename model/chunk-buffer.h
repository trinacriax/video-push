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

namespace ns3{
using namespace streaming;

class ChunkBuffer{

public:

	ChunkBuffer ();

	virtual ~ChunkBuffer ();

	ChunkVideo* Copy (const ChunkVideo chunk);

	ChunkVideo* GetChunk (uint32_t index);
	bool AddChunk (const ChunkVideo chunk);
	bool DelChunk (uint32_t index);

	const uint32_t GetChunkSize (uint32_t index);
	const uint32_t GetChunkSize (ChunkVideo chunk);

protected:
	std::map<uint32_t, ChunkVideo> chunk_buffer;

};
#endif

}

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

#ifndef __CHUNK_VIDEO_H__
#define __CHUNK_VIDEO_H__

#include <stdint.h>
#include <iostream>
namespace ns3{

namespace streaming{

//enum ChunkState {
//	Chunk_REC, Chunk_NOT, Chunk_DOWN
//};

	struct ChunkVideo{
		ChunkVideo () :
			c_id (0),
			c_size(0),
			c_tstamp (0),
			c_attributes_size (0)
		{
			c_attributes = 0;
			c_data = 0;
		}
		ChunkVideo (uint32_t cid, uint32_t csize, uint64_t ctstamp, uint32_t cattributes_size) :
			c_id (cid),
			c_size(csize),
			c_tstamp (ctstamp),
			c_attributes_size (cattributes_size)
		{
			c_attributes = 0;
			c_data = 0;
		}
		uint32_t c_id;
		uint32_t c_size;
		uint8_t *c_data;
		uint64_t c_tstamp;
		uint32_t c_attributes_size;
		uint8_t *c_attributes;
	};

		static inline std::ostream&
		operator << (std::ostream& o, const ChunkVideo &a)
		{
		return o<<"Id: "<< a.c_id <<" S: "<<a.c_size<<" Ts "<< a.c_tstamp << " As "<<a.c_attributes_size<<"\n";
		}

		static inline bool
		operator < (const ChunkVideo &a, const ChunkVideo &b){
			return (a.c_id < b.c_id);
		}

		static inline bool
		operator == (const ChunkVideo &a, const ChunkVideo &b){
			return (a.c_id == b.c_id )&& (a.c_size == b.c_size) && (a.c_tstamp == b.c_tstamp);
		}
}
};
#endif /* CHUNK_VIDEO_H_ */

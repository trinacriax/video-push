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
#include <stdlib.h>
#include <memory.h>
#include <limits.h>
#include "ns3/assert.h"

enum ChunkState {
		CHUNK_RECEIVED_PUSH,
		CHUNK_RECEIVED_PULL,
		CHUNK_SKIPPED,
		CHUNK_DELAYED,
		CHUNK_MISSED
};

namespace ns3{

namespace streaming{

	struct ChunkVideo{
		ChunkVideo () :
			c_id (0),
			c_tstamp (0),
			c_size(0),
			c_attributes_size (0)
		{
//			c_data = 0;
//			c_attributes = 0;
		}
		ChunkVideo (const ChunkVideo &cv) :
					c_id (cv.c_id),
					c_tstamp (cv.c_tstamp),
					c_size(cv.c_size),
					c_attributes_size (cv.c_attributes_size)
		{
		//			c_data = 0;
		//			c_attributes = 0;
		}
		ChunkVideo (const uint32_t cid, const uint64_t ctstamp, const uint16_t csize, const uint16_t cattributes_size) :
			c_id (cid),
			c_tstamp (ctstamp),
			c_size(csize),
			c_attributes_size (cattributes_size)
		{
			NS_ASSERT (cid>0);
			NS_ASSERT (ctstamp>=0 && ctstamp<=ULONG_LONG_MAX);
//			c_data = 0;
//			c_attributes = 0;
		}
		uint32_t c_id;
		uint64_t c_tstamp;
		uint16_t c_size;
		uint16_t c_attributes_size;
//		uint8_t *c_data;
//		uint8_t *c_attributes;

		ChunkVideo* Copy (){
			ChunkVideo* copy (new ChunkVideo(c_id, c_tstamp, c_size, c_attributes_size));
			return copy;
		}

		uint16_t GetSize(){return c_size;}
		uint16_t GetAttributeSize(){return c_attributes_size;}
	};

		static inline std::ostream&
		operator << (std::ostream& o, const ChunkVideo &a)
		{
		return o<<"ID: "<< a.c_id <<" Tstamp "<< a.c_tstamp << " Size "<<a.c_size << " AttrSize "<<a.c_attributes_size;
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

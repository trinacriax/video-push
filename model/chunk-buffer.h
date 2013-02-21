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
 */

#ifndef __CHUNK_BUFFER_H__
#define __CHUNK_BUFFER_H__

#include "chunk-video.h"
#include "ns3/object.h"
#include <map>
#include <string>
namespace ns3
{
  using namespace streaming;

  /**
   * \brief Provide a chunk buffer structure for video streaming application.
   *
   * The class provides a simple chunk buffer data structure
   * employed in video streaming applications.
   *
   */

  class ChunkBuffer
  {

    public:

      ChunkBuffer ();

      virtual
      ~ChunkBuffer ();

      /**
       *
       * \param chunkId chunk identifier
       * \return pointer to chunk
       *
       * Provides the i-th chunk.
       */

      ChunkVideo*
      GetChunk (uint32_t index);

      /**
       *
       * \param chunkId chunk identifier
       * \return True if the chunk is in the buffer, false otherwise.
       *
       * Check whether the chunk is in the buffer or not.
       */

      bool
      HasChunk (uint32_t index);

      /**
       *
       * \param chunk Chunk data.
       * \param state Chunk's state.
       * \return True if added, false otherwise.
       *
       * Insert a chunk into the buffer with a given state.
       */

      bool
      AddChunk (const ChunkVideo &chunk, ChunkState state);

      /**
       *
       * \param chunkId chunk identifier.
       * \return True if removed, false otherwise.
       *
       * Remove a chunk from the buffer.
       */

      bool
      DelChunk (uint32_t index);

      /**
       *
       * \return Size of the buffer.
       *
       * Size of the current buffer.
       */

      const size_t
      GetBufferSize ();

      /**
       *
       * \return A string containing the identifier of all chunks.
       *
       * Create a string with the identifiers of all chunks into the buffer.
       */

      std::string
      PrintBuffer ();

      /**
       *
       * \return a copy of the chunk buffer.
       *
       * Give the whole chunk buffer.
       */

      std::map<uint32_t, ChunkVideo>
      GetChunkBuffer ();

      /**
       *
       * \param chunkId chunk identifier.
       * \return The state of the chunk.
       *
       * Get the state of the chunk.
       */

      ChunkState
      GetChunkState (uint32_t index);

      /**
       *
       * \param chunkId chunk identifier.
       * \param state chunk's state
       *
       * Set the state of the chunk.
       */

      void
      SetChunkState (uint32_t chunkid, ChunkState state);

      /**
       *
       * \param chunkId chunk identifier.
       * \return True is the chunk is missed, false otherwise.
       *
       * Check whether the chunks state is missed or not.
       */

      bool
      ChunkMissed (uint32_t chunkid);

      /**
       *
       * \param chunkId chunk identifier.
       * \return True is the chunk is delayed, false otherwise.
       *
       * Check whether the chunks state is delayed or not.
       */

      bool
      ChunkDelayed (uint32_t chunkid);


      /**
       *
       * \param base Starting chunk identifier to look into the buffer.
       * \param window Window size to consider to find the least chunk missed.
       * \return Least missed chunk.
       *
       * Get the least missed chunk
       */

      uint32_t
      GetLeastMissed (uint32_t base, uint32_t window);

      /**
       *
       * \param base Starting chunk identifier to look into the buffer.
       * \param window Window size to consider to find the latest chunk missed.
       * \return Latest missed chunk.
       *
       * Get the latest missed chunk
       */

      uint32_t
      GetLatestMissed (uint32_t base, uint32_t window);

      /**
       *
       * \return Last chunk identifier.
       *
       * Get the last chunk identifier in the buffer.
       */

      uint32_t
      GetLastChunk ();

      /**
       *
       * \return Chunk buffer size.
       * Get the chunk buffer size.
       */

      uint32_t
      GetSize ();

    protected:
      std::map<uint32_t, ChunkVideo> chunk_buffer; /// map containing the chunks
      std::map<uint32_t, ChunkState> chunk_state;  /// map containing the chunks' state
      uint32_t last;                               /// Last chunk identifier.

  };
} // namespace ns3
#endif

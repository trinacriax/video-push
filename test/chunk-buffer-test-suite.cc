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
 * Source:  http://www.ietf.org/rfc/rfc3973.txt
 *
 * Authors: Alessandro Russo <russo@disi.unitn.it>
 *          University of Trento, Italy
 */

#include <ns3/test.h>
#include <ns3/chunk-buffer.h>
#include <ns3/packet.h>

namespace ns3
{
  using namespace streaming;

  class ChunkBufferTestCase : public TestCase
  {
    public:
      ChunkBufferTestCase ();
      virtual void
      DoRun (void);
      ChunkBuffer m_chunks;
  };

  ChunkBufferTestCase::ChunkBufferTestCase () :
      TestCase("Check Chunk Buffer")
  {
  }
  void
  ChunkBufferTestCase::DoRun (void)
  {
//	bool test = true;
    uint32_t size = 0;
    for (uint32_t i = 1; i < 1000; i++)
      {
        if (i % 100 == 0)
          continue;
        size++;
        ChunkVideo cv(i, i * 1000, i + 1200, 0);
        m_chunks.AddChunk(cv, CHUNK_RECEIVED_PUSH);
      }

    for (uint32_t i = 1; i < 1000; i++)
      {
        NS_TEST_ASSERT_MSG_EQ(m_chunks.HasChunk(i), !(i%100==0), "HasChunk");
      }

    NS_TEST_ASSERT_MSG_EQ(m_chunks.GetBufferSize(), size, "Buffer Size");
    for (uint32_t i = 1; i < 1000; i++)
      {
        if (!m_chunks.HasChunk(i))
          continue;
        ChunkVideo *cv = m_chunks.GetChunk(i);
        NS_TEST_ASSERT_MSG_EQ(cv->c_id, i, "ChunkID");
        NS_TEST_ASSERT_MSG_EQ(cv->c_tstamp, i*1000, "ChunkTS");
        NS_TEST_ASSERT_MSG_EQ(cv->c_size, i+1200, "ChunkSz");
        NS_TEST_ASSERT_MSG_EQ(cv->c_attributes_size, 0, "ChunkAz");
      }
    for (uint32_t i = 1; i < 1000; i++)
      {
        if (!m_chunks.HasChunk(i))
          continue;
        if (i % 25 != 0)
          {
            m_chunks.DelChunk(i);
            size--;
          }
      }
    NS_TEST_ASSERT_MSG_EQ(size, m_chunks.GetBufferSize(), "BufferSize");
  }

  class ChunkBufferStateTestCase : public TestCase
  {
    public:
      ChunkBufferStateTestCase ();
      virtual void
      DoRun (void);
      ChunkBuffer m_chunks;
  };

  ChunkBufferStateTestCase::ChunkBufferStateTestCase () :
      TestCase("Check Chunk State")
  {
  }

  void
  ChunkBufferStateTestCase::DoRun (void)
  {
    std::cout << "init buffer" << std::endl;
    for (uint32_t i = 1; i < 1000; i++)
      {
        if (i % 100 == 0)
          continue;
        ChunkVideo cv(i, i * 1000, i + 1200, 0);
        ChunkState state = (i % 2 == 0 ? CHUNK_RECEIVED_PUSH : CHUNK_RECEIVED_PULL);
        m_chunks.AddChunk(cv, state);
      }
    std::cout << "check state " << std::endl;
    for (uint32_t i = 1; i < 1000; i++)
      {
        ChunkState state;
        if (i % 100 == 0)
          state = CHUNK_MISSED;
        else if (i % 2 == 0)
          state = CHUNK_RECEIVED_PUSH;
        else
          state = CHUNK_RECEIVED_PULL;
        ChunkState current = m_chunks.GetChunkState(i);
        NS_TEST_ASSERT_MSG_EQ(current, state, "Chunk State");
      }
    std::cout << "set state" << std::endl;
    for (uint32_t i = 1; i < 1000; i++)
      {
        ChunkState state;
        if (i % 100 == 0)
          state = CHUNK_MISSED;
        else if (i % 2 == 0)
          state = CHUNK_RECEIVED_PULL;
        else
          state = CHUNK_RECEIVED_PUSH;
        m_chunks.SetChunkState(i, state);
      }
    std::cout << "del buffer" << std::endl;
    for (uint32_t i = 1; i < 1000; i++)
      {
        if (i % 25 == 0)
          m_chunks.DelChunk(i);
      }
    std::cout << "check buffer" << std::endl;
    for (uint32_t i = 1; i < 1000; i++)
      {
        ChunkState state;
        if (i % 100 == 0 || i % 25 == 0)
          state = CHUNK_MISSED;
        else if (i % 2 == 0)
          state = CHUNK_RECEIVED_PULL;
        else
          state = CHUNK_RECEIVED_PUSH;
        ChunkState current = m_chunks.GetChunkState(i);
        NS_TEST_ASSERT_MSG_EQ(current, state, "Chunk State");
      }
  }

  static class ChunkBufferTestSuite : public TestSuite
  {
    public:
      ChunkBufferTestSuite ();
  } j_chunkBufferTestSuite;

  ChunkBufferTestSuite::ChunkBufferTestSuite () :
      TestSuite("chunk-buffer", UNIT)
  {
    /// ./test.py -s chunk-buffer -v -c unit 1 -w -m -g
//    AddTestCase(new ChunkBufferTestCase());
    AddTestCase(new ChunkBufferStateTestCase());
  }
} // namespace ns3

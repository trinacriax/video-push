#include "ns3/test.h"
#include "ns3/chunk-buffer.h"
#include "ns3/packet.h"

namespace ns3 {

class ChunkBufferTestCase : public TestCase {
public:
	ChunkBufferTestCase ();
	virtual void DoRun (void);
	ChunkBuffer m_chunks;
};

ChunkBufferTestCase::ChunkBufferTestCase ()
  : TestCase ("Check Chunk Buffer")
{}
void
ChunkBufferTestCase::DoRun (void)
{
	bool test = true;
	uint32_t size = 0;
	for (uint32_t i = 1; i <1000; i++)
	{
		if(i%100==0) continue;
		size++;
		ChunkVideo cv (i,i*1000,i+1200,0);
		m_chunks.AddChunk(cv,CHUNK_RECEIVED_PUSH);
	}

	for (uint32_t i = 1; i <1000; i++)
	{
		NS_TEST_ASSERT_MSG_EQ(m_chunks.HasChunk(i),!(i%100==0),"HasChunk");
	}

	NS_TEST_ASSERT_MSG_EQ(m_chunks.GetBufferSize(),size,"Buffer Size");
	for (uint32_t i = 1; i <1000; i++)
	{
		if(!m_chunks.HasChunk(i))continue;
		ChunkVideo *cv = m_chunks.GetChunk(i);
		NS_TEST_ASSERT_MSG_EQ(cv->c_id, i,"ChunkID");
		NS_TEST_ASSERT_MSG_EQ(cv->c_tstamp, i*1000,"ChunkTS");
		NS_TEST_ASSERT_MSG_EQ(cv->c_size, i+1200,"ChunkSz");
		NS_TEST_ASSERT_MSG_EQ(cv->c_attributes_size, 0,"ChunkAz");
	}
	for (uint32_t i = 1; i <1000; i++)
	{
		if(!m_chunks.HasChunk(i))continue;
		if (i%25!=0)
		{
			m_chunks.DelChunk(i);
			size--;
		}
	}
	NS_TEST_ASSERT_MSG_EQ(size,m_chunks.GetBufferSize(),"BufferSize");
}

class ChunkBufferStateTestCase : public TestCase {
public:
	ChunkBufferStateTestCase ();
	virtual void DoRun (void);
	ChunkBuffer m_chunks;
};

ChunkBufferStateTestCase::ChunkBufferStateTestCase ()
  : TestCase ("Check Chunk Buffer")
{}
void
ChunkBufferStateTestCase::DoRun (void)
{
	bool test = true;
	uint32_t size = 0;
	for (uint32_t i = 1; i<1000; i++)
	{
		if(i%100==0) continue;
		ChunkVideo cv (i,i*1000,i+1200,0);
		ChunkState state = (i%2==0?CHUNK_RECEIVED_PUSH:CHUNK_RECEIVED_PULL);
		m_chunks.AddChunk(cv,state);
	}
	for (uint32_t i = 1; i<1000; i++)
	{
		ChunkState state;
		if(i%100==0) state = CHUNK_MISSED;
		else if (i%2==0) state = CHUNK_RECEIVED_PUSH;
		else state = CHUNK_RECEIVED_PULL;
		ChunkState current = m_chunks.GetChunkState(i);
		NS_TEST_ASSERT_MSG_EQ (current, state, "Chunk State");
	}
	for (uint32_t i = 1; i<1000; i++)
	{
		ChunkState state;
		if(i%100==0) state = CHUNK_MISSED;
		else if (i%2==0) state = CHUNK_RECEIVED_PULL;
		else state = CHUNK_RECEIVED_PUSH;
		m_chunks.SetChunkState(i,state);
	}
	for (uint32_t i = 1; i<1000; i++)
	{
		if(i%25==0) m_chunks.DelChunk(i);
	}
	for (uint32_t i = 1; i<1000; i++)
	{
		uint32_t missed = m_chunks.GetLeastMissed();
		if(i%25==0)
		{
			NS_TEST_ASSERT_MSG_EQ (i, missed,"LeastMissed");
			m_chunks.SetChunkState(i,CHUNK_SKIPPED);
		}
	}
}


static class ChunkBufferTestSuite : public TestSuite
{
public:
	ChunkBufferTestSuite ();
} j_chunkBufferTestSuite;

ChunkBufferTestSuite::ChunkBufferTestSuite()
  : TestSuite("chunk-buffer", UNIT)
{
  // RUN $ ./test.py -s chunk-buffer-packet -v -c unit 1
	/// ./test.py -s chunk-buffer -v -c unit 1 -w -m -g
  AddTestCase(new ChunkBufferTestCase ());
  AddTestCase(new ChunkBufferStateTestCase ());
}
}

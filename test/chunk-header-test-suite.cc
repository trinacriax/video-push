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
 * Source:  http://www.ietf.org/rfc/rfc3973.txt
 *
 * Authors: Alessandro Russo <russo@disi.unitn.it>
 *          University of Trento, Italy
 *          University of California, Los Angeles U.S.A.
 */

#include "ns3/test.h"
#include "ns3/chunk-packet.h"
#include "ns3/packet.h"

namespace ns3 {

class ChunkHeaderTestCase : public TestCase {
public:
  ChunkHeaderTestCase ();
  virtual void DoRun (void);
};

ChunkHeaderTestCase::ChunkHeaderTestCase ()
  : TestCase ("Check ChunkHeader")
{}
void
ChunkHeaderTestCase::DoRun (void)
{
  Packet packet;
  streaming::ChunkHeader msgIn;
  msgIn.SetType(CHUNK);
  msgIn.SetReserved(0);
  msgIn.SetChecksum(61255);
  msgIn.Print(std::cout);
  packet.AddHeader(msgIn);

  streaming::ChunkHeader msgOut;
  packet.RemoveHeader (msgOut);
  msgOut.Print(std::cout);
  {
	NS_TEST_ASSERT_MSG_EQ(msgOut.GetType(),CHUNK,"ChunkHeader Type");
	NS_TEST_ASSERT_MSG_EQ(msgOut.GetReserved(),0,"Reserved");
	NS_TEST_ASSERT_MSG_EQ(msgOut.GetChecksum(),61255,"Checksum");
  }

  msgIn.SetType(PULL);
  msgIn.SetReserved(1);
  msgIn.SetChecksum(31225);
  msgIn.Print(std::cout);
  packet.AddHeader(msgIn);

  packet.RemoveHeader (msgOut);
  msgOut.Print(std::cout);
  {
  NS_TEST_ASSERT_MSG_EQ(msgOut.GetType(),PULL,"ChunkHeader Type");
  NS_TEST_ASSERT_MSG_EQ(msgOut.GetReserved(),1,"Reserved");
  NS_TEST_ASSERT_MSG_EQ(msgOut.GetChecksum(),31225,"Checksum");
  }
}


class ChunkTestCase : public TestCase {
public:
	ChunkTestCase ();
  virtual void DoRun (void);
};

ChunkTestCase::ChunkTestCase ()
  : TestCase ("Check ChunkMessage")
{}
void
ChunkTestCase::DoRun (void)
{
	  Packet packet;
	  streaming::ChunkHeader msgIn;
	  msgIn.SetType(CHUNK);
	  msgIn.SetReserved(0);
	  msgIn.SetChecksum(45665);
	  msgIn.Print(std::cout);
	  {
		  streaming::ChunkHeader::ChunkMessage &chunkIn = msgIn.GetChunkMessage ();
		  streaming::ChunkVideo video (10, 987654321, 100, 10);
		  chunkIn.SetChunk(video);
		  chunkIn.Print(std::cout);
	  }
	  packet.AddHeader(msgIn);

	  streaming::ChunkHeader msgOut;
	  packet.RemoveHeader (msgOut);
	  msgOut.Print(std::cout);
	  {
		NS_TEST_ASSERT_MSG_EQ(msgOut.GetType(),CHUNK,"ChunkHeader Type");
		NS_TEST_ASSERT_MSG_EQ(msgOut.GetReserved(),0,"Reserved");
		NS_TEST_ASSERT_MSG_EQ(msgOut.GetChecksum(),45665,"Checksum");

		streaming::ChunkHeader::ChunkMessage &chunkOut = msgIn.GetChunkMessage ();
		streaming::ChunkVideo video = chunkOut.GetChunk();
		chunkOut.Print(std::cout);

		NS_TEST_ASSERT_MSG_EQ (video.c_id, 10, "ChunkIdentifier");
		NS_TEST_ASSERT_MSG_EQ (video.c_tstamp, 987654321, "Timestamp");
		NS_TEST_ASSERT_MSG_EQ (video.c_size, 100, "ChunkSize");
		NS_TEST_ASSERT_MSG_EQ (video.c_attributes_size, 10, "ChunkAttributeSize");
	  }
}

class PullTestCase : public TestCase {
public:
	PullTestCase ();
  virtual void DoRun (void);
};

PullTestCase::PullTestCase ()
  : TestCase ("Check PullMessages")
{}
void
PullTestCase::DoRun (void)
{
	  Packet packet;
	  streaming::ChunkHeader msgIn;
	  msgIn.SetType(PULL);
	  msgIn.SetReserved(1);
	  msgIn.SetChecksum(65535);
	  msgIn.Print(std::cout);
	  {
	    streaming::ChunkHeader::PullMessage &chunkIn = msgIn.GetPullMessage ();
	    chunkIn.SetChunk(5);
	    chunkIn.Print(std::cout);
	  }
	  packet.AddHeader(msgIn);

	  streaming::ChunkHeader msgOut;
	  packet.RemoveHeader (msgOut);
	  msgOut.Print(std::cout);
	  {
	  NS_TEST_ASSERT_MSG_EQ(msgOut.GetType(),PULL,"ChunkHeader Type");
	  NS_TEST_ASSERT_MSG_EQ(msgOut.GetReserved(),1,"Reserved");
	  NS_TEST_ASSERT_MSG_EQ(msgOut.GetChecksum(),65535,"Checksum");
	  streaming::ChunkHeader::PullMessage &chunkOut = msgIn.GetPullMessage ();
	  {
		  NS_TEST_ASSERT_MSG_EQ (chunkOut.GetChunk(), 5, "ChunkIdentifier PULL");
		  chunkOut.Print(std::cout);
	  }
	  }
}

static class ChunkTestSuite : public TestSuite
{
public:
	ChunkTestSuite ();
} j_ChunkTestSuite;

ChunkTestSuite::ChunkTestSuite()
  : TestSuite("chunk-header", UNIT)
{
  // RUN $ ./test.py -s chunk-header -v -c unit 1
  AddTestCase(new ChunkHeaderTestCase());
  AddTestCase(new ChunkTestCase());
  AddTestCase(new PullTestCase());
}

} // namespace ns3


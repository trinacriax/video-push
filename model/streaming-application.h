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

///
/// \file
/// \brief
///

#ifndef __STREAMING_DATA_STRUCTURE_H__
#define __STREAMING_DATA_STRUCTURE_H__

#include <list>
#include <iostream>
#include <stdio.h>
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/timer.h"

namespace ns3 {
namespace streaming {

class StreamingApplication : public Application
{

/**
 * Interface which provides the main methods for setting the protocol parameters.
 * @author Alessandro Russo <russo@disi.unitn.it>
 */
public:
	static TypeId GetTypeId (void);
	StreamingApplication ();
	virtual ~StreamingApplication();

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop


protected:
    bool addChunk(int index, int method);
    void Initialize(int n);

    void resetAll();

    void setSource(int _source);

    void setCycle(int ciclo);

    void setDebug(int debug);

    void setNumberOfChunks(int _number_of_chunks);

    int getNumberOfChunks();

    void setChunkSize(long _chunk_size);

    long getChunkSize();

    void setCompleted(long value);

    long getCompleted();

    void setPushRetry(int push_retries);

    void setPullRetry(int pull_retries);

    void setSwitchTime(long time);

    void setNewChunkDelay(long delay);

    void setPushWindow(int window);

    void setPullWindow(int window);

    void setBandwidth(int bandiwdthp);

    void setNeighborKnowledge(int value);

    void setPlayoutTime(int time_sec);

    void setCurrent(Node current);

    void setPullRounds(int rounds);
}

}

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/
 
 /**
 * @note modified from ndnSIM/apps/ndn-consumer.cpp by Ray Stubbs [stubbs.ray@gmail.com]
 **/

#include "zipf-consumer.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"

#include "utils/ndn-ns3-packet-tag.hpp"
#include "model/ndn-app-face.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/ref.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#include <boost/random/exponential_distribution.hpp>

namespace ndntac {

using namespace ns3;
using namespace ns3::ndn;

NS_OBJECT_ENSURE_REGISTERED(ZipfConsumer);

TypeId
ZipfConsumer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ndntac::ZipfConsumer")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddAttribute("Names", "Content names of the form "
                    "'[name1:popularity1][name2:popularity2]...'", StringValue("/"),
                    MakeStringAccessor(&ZipfConsumer::SetNames),
                    MakeStringChecker())
      .AddAttribute("LifeTime", "LifeTime for interest packet", StringValue("2s"),
                    MakeTimeAccessor(&ZipfConsumer::m_interestLifeTime), MakeTimeChecker())

      .AddAttribute("RetxTimer",
                    "Timeout defining how frequent retransmission timeouts should be checked",
                    StringValue("50ms"),
                    MakeTimeAccessor(&ZipfConsumer::GetRetxTimer, &ZipfConsumer::SetRetxTimer),
                    MakeTimeChecker())
      .AddAttribute("q", "parameter of improve rank for Zipf variable", StringValue("0.7"),
                    MakeDoubleAccessor(&ZipfConsumer::SetQ,
                                       &ZipfConsumer::GetQ),
                    MakeDoubleChecker<double>())

      .AddAttribute("s", "parameter of power for Zipf variable", StringValue("0.7"),
                    MakeDoubleAccessor(&ZipfConsumer::SetS,
                                       &ZipfConsumer::GetS),
                    MakeDoubleChecker<double>())
      .AddAttribute("Mean", "parameter of mean for exponential variable", DoubleValue( 5 ),
                    MakeDoubleAccessor( &ZipfConsumer::SetExponentialMean ),
                    MakeDoubleChecker<double>() )
      .AddAttribute("Bound", "bound parameter for exponential variable", DoubleValue( 10 ),
                    MakeDoubleAccessor( &ZipfConsumer::SetExponentialBound ),
                    MakeDoubleChecker<double>() );

  return tid;
}

ZipfConsumer::ZipfConsumer()
  : m_rand(CreateObject<UniformRandomVariable>())
  , m_exp_rand( CreateObject<ExponentialRandomVariable>() )
  , m_seq(0)
  , m_finished_content( false )
  , m_pending_next_content( false )
  , m_q(0.7)
  , m_s(0.7)
{
  m_rtt = CreateObject<RttMeanDeviation>();
}

void
ZipfConsumer::PrepareZipf()
{

  m_Pcum = std::vector<double>(m_names.size() + 1);

  m_Pcum[0] = 0.0;
  for (uint32_t i = 1; i <= m_names.size(); i++) {
    m_Pcum[i] = m_Pcum[i - 1] + 1.0 / std::pow(i + m_q, m_s);
  }

  for (uint32_t i = 1; i <= m_names.size(); i++) {
    m_Pcum[i] = m_Pcum[i] / m_Pcum[m_names.size()];
  }
}

void
ZipfConsumer::SetQ(double q)
{
  m_q = q;
  PrepareZipf();
}

double
ZipfConsumer::GetQ() const
{
  return m_q;
}

void
ZipfConsumer::SetS(double s)
{
  m_s = s;
  PrepareZipf();
}

double
ZipfConsumer::GetS() const
{
  return m_s;
}

uint32_t
ZipfConsumer::GetNextSeq()
{
  uint32_t content_index = 1; //[1, m_names.size()]
  double p_sum = 0;

  double p_random = m_rand->GetValue();
  while (p_random == 0) {
    p_random = m_rand->GetValue();
  }
  for (uint32_t i = 1; i <= m_names.size(); i++) {
    p_sum = m_Pcum[i]; // m_Pcum[i] = m_Pcum[i-1] + p[i], p[0] = 0;   e.g.: p_cum[1] = p[1],
                       // p_cum[2] = p[1] + p[2]
    if (p_random <= p_sum) {
      content_index = i;
      break;
    } // if
  }   // for
  // content_index = 1;
  return content_index;
}


void
ZipfConsumer::SetRetxTimer(Time retxTimer)
{
  m_retxTimer = retxTimer;
  if (m_retxEvent.IsRunning()) {
    // m_retxEvent.Cancel (); // cancel any scheduled cleanup events
    Simulator::Remove(m_retxEvent); // slower, but better for memory
  }

  // schedule even with new timeout
  m_retxEvent = Simulator::Schedule(m_retxTimer, &ZipfConsumer::CheckRetxTimeout, this);
}

Time
ZipfConsumer::GetRetxTimer() const
{
  return m_retxTimer;
}

void
ZipfConsumer::CheckRetxTimeout()
{
  Time now = Simulator::Now();

  Time rto = m_rtt->RetransmitTimeout();

  while (!m_seqTimeouts.empty()) {
    SeqTimeoutsContainer::index<i_timestamp>::type::iterator entry =
      m_seqTimeouts.get<i_timestamp>().begin();
    if (entry->time + rto <= now) // timeout expired?
    {
      uint32_t seqNo = entry->seq;
      m_seqTimeouts.get<i_timestamp>().erase(entry);
      OnTimeout(seqNo);
    }
    else
      break; // nothing else to do. All later packets need not be retransmitted
  }

  m_retxEvent = Simulator::Schedule(m_retxTimer, &ZipfConsumer::CheckRetxTimeout, this);
}

void
ZipfConsumer::SetNames( std::string names )
{
  static boost::regex name_regex("\\[([^:]+):([0-9]+)\\]" );
  boost::sregex_iterator names_itr( names.begin(),
                                    names.end(),
                                    name_regex );
  boost::sregex_iterator names_end;

  while( names_itr != names_end )
  {
      Name name( (*names_itr)[1].str() );
      uint32_t  popularity; std::stringstream( (*names_itr)[2].str() ) >> popularity;
      m_names.push_back( std::make_pair( popularity, name ) );
      names_itr++;
  }
   
  PrepareZipf();
}

void
ZipfConsumer::SetExponentialMean( double mean )
{
    m_exp_rand->SetAttribute( "Mean", DoubleValue(mean) );
}

void
ZipfConsumer::SetExponentialBound( double bound )
{
    m_exp_rand->SetAttribute( "Bound", DoubleValue(bound) );
}

// Application Methods
void
ZipfConsumer::StartApplication() // Called at time specified by Start
{

  // do base stuff
  App::StartApplication();
    
  Reset();
}

void
ZipfConsumer::StopApplication() // Called at time specified by Stop
{

  // cancel periodic packet generation
  Simulator::Cancel(m_sendEvent);

  // cleanup base stuff
  App::StopApplication();
}

void
ZipfConsumer::SendPacket()
{
  if (!m_active)
    return;

  uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid

  while (m_retxSeqs.size()) {
    seq = *m_retxSeqs.begin();
    m_retxSeqs.erase(m_retxSeqs.begin());
    break;
  }

  if (seq == std::numeric_limits<uint32_t>::max()) {
    if ( m_finished_content ) {
      if( !m_pending_next_content && m_retxSeqs.empty() )
      {
          m_pending_next_content = true;
          Simulator::Schedule( Seconds( m_exp_rand->GetValue() ),
                               &ZipfConsumer::Reset, this );
      }
      return;
    }
    seq = m_seq++;
  }

  //
  shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);
  nameWithSequence->appendSequenceNumber(seq);
  //

  // shared_ptr<Interest> interest = make_shared<Interest> ();
  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(*nameWithSequence);
  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  WillSendOutInterest(seq);
  WillSendOutInterest( interest );

  m_transmittedInterests(interest, this, m_face);
  m_face->onReceiveInterest(*interest);

  ScheduleNextPacket();
  
}


void
ZipfConsumer::Reset()
{
    if( m_names.empty() )
        return;
        
    m_seqTimeouts.clear();
    m_seqLastDelay.clear();
    m_seqFullDelay.clear();
    m_seqRetxCounts.clear();
    m_retxSeqs.clear();
    
    m_seq = 0;
    m_finished_content = false;
    m_pending_next_content = false;
    
    
    uint32_t zf = GetNextSeq();
    m_interestName = m_names[zf%m_names.size()].second;
    ScheduleNextPacket();
    
}

///////////////////////////////////////////////////
//          Process incoming packets             //
///////////////////////////////////////////////////

void
ZipfConsumer::OnData(shared_ptr<const Data> data)
{
  if (!m_active)
    return;

  App::OnData(data); // tracing inside

  // This could be a problem......
  uint32_t seq = data->getName().at(-1).toSequenceNumber();

  int hopCount = 0;
  auto ns3PacketTag = data->getTag<Ns3PacketTag>();
  if (ns3PacketTag != nullptr) { // e.g., packet came from local node's cache
    FwHopCountTag hopCountTag;
    if (ns3PacketTag->getPacket()->PeekPacketTag(hopCountTag)) {
      hopCount = hopCountTag.Get();
    }
  }

  SeqTimeoutsContainer::iterator entry = m_seqLastDelay.find(seq);
  if (entry != m_seqLastDelay.end()) {
    m_lastRetransmittedInterestDataDelay(this, seq, Simulator::Now() - entry->time, hopCount);
  }

  entry = m_seqFullDelay.find(seq);
  if (entry != m_seqFullDelay.end()) {
    m_firstInterestDataDelay(this, seq, Simulator::Now() - entry->time, m_seqRetxCounts[seq], hopCount);
  }

  m_seqRetxCounts.erase(seq);
  m_seqFullDelay.erase(seq);
  m_seqLastDelay.erase(seq);

  m_seqTimeouts.erase(seq);
  m_retxSeqs.erase(seq);

  m_rtt->AckSeq(SequenceNumber32(seq));
  
  // if data is empty then we've retrieved the whole packet
  if( data->getContentType() == ::ndn::tlv::ContentType_EoC && !m_finished_content )
  {
    m_finished_content = true;
  }
  
}

void
ZipfConsumer::OnTimeout(uint32_t sequenceNumber)
{
  // std::cout << Simulator::Now () << ", TO: " << sequenceNumber << ", current RTO: " <<
  // m_rtt->RetransmitTimeout ().ToDouble (Time::S) << "s\n";

  m_rtt->IncreaseMultiplier(); // Double the next RTO
  m_rtt->SentSeq(SequenceNumber32(sequenceNumber),
                 1); // make sure to disable RTT calculation for this sample
  m_retxSeqs.insert(sequenceNumber);
  ScheduleNextPacket();
}

void
ZipfConsumer::WillSendOutInterest(uint32_t sequenceNumber)
{

  m_seqTimeouts.insert(SeqTimeout(sequenceNumber, Simulator::Now()));
  m_seqFullDelay.insert(SeqTimeout(sequenceNumber, Simulator::Now()));

  m_seqLastDelay.erase(sequenceNumber);
  m_seqLastDelay.insert(SeqTimeout(sequenceNumber, Simulator::Now()));

  m_seqRetxCounts[sequenceNumber]++;

  m_rtt->SentSeq(SequenceNumber32(sequenceNumber), 1);
}

} // namespace ndntac

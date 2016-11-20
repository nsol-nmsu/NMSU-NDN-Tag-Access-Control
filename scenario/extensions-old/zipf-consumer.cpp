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

#include "logger.hpp"

namespace ndntac {

using namespace ns3;
using namespace ns3::ndn;

NS_OBJECT_ENSURE_REGISTERED(ZipfConsumer);

uint32_t
ZipfConsumer::s_instance_id = 0;

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
  m_instance_id = s_instance_id++;
  
  // get some number of values from both random distributions
  // to offset them from the distributions of other consumers
  for( uint32_t i = 0 ; i < m_instance_id ; i++ )
  {
    m_rand->GetValue();
    m_exp_rand->GetValue();
  }
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
  interest->setRouteTracker( ::ndn::RouteTracker() );
  BOOST_ASSERT( interest->getCurrentNetwork() == ::ndn::RouteTracker::ENTRY_NETWORK );

  WillSendOutInterest(seq);
  WillSendOutInterest( interest );

  m_transmittedInterests(interest, this, m_face);
  m_face->onReceiveInterest(*interest);
  
  logSentInterest( *interest );

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
    m_contentStartTime = ns3::Simulator::Now();
    m_contentSize = 0;
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
  
  logReceivedData( *data );
  m_contentSize += data->getContent().value_size();

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
  
  // if data is EOC then we've finished the content
  if( data->getContentType() == ::ndn::tlv::ContentType_EoC && !m_finished_content )
  {
    m_finished_content = true;
    logFinishedContent( m_interestName,
                        ns3::Simulator::Now() - m_contentStartTime,
                        m_contentSize );
  }
  
  // if data is Nack then log it
  if( data->getContentType() == ::ndn::tlv::ContentType_Nack )
    logReceivedNack( *data );
  
}

void
ZipfConsumer::OnTimeout(uint32_t sequenceNumber)
{
  // std::cout << Simulator::Now () << ", TO: " << sequenceNumber << ", current RTO: " <<
  // m_rtt->RetransmitTimeout ().ToDouble (Time::S) << "s\n";

  logTimeout( m_interestName, sequenceNumber );
  
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

void
ZipfConsumer::logFinishedContent( const Name& content,
                                  ns3::Time duration,
                                  const size_t size ) const
{
    if( !shouldLogFinishedContent() )
        return;

    static Log* log = Logger::getInstance( "simulation.log.udb" ).makeLog(
                            "Consumer",
                            "{ 'content' : $content_name, "
                            "  'duration': $duration, "
                            "  'size'    : $size, "
                            "  'what'    : $what, "
                            "  'who'     : $who }" );
    log->set( "content_name", content.toUri() );
    log->set( "duration", duration.ToInteger( Time::NS ) );
    log->set( "size", (int64_t) size );
    log->set( "what", string("FinishedContent") );
    log->set( "who", (int64_t)m_instance_id );
    log->write();
}

void
ZipfConsumer::logReceivedNack( const Data& nack ) const
{
    if( !shouldLogReceivedNack() )
        return;

  static Log* log = Logger::getInstance( "simulation.log.udb" ).makeLog(
                            "Consumer",
                            "{ 'nack-name' : $nack_name, "
                            "  'what'      : $what, "
                            "  'who'       : $who }" );
  log->set( "nack_name", nack.getName().toUri() );
  log->set( "what", string("ReceivedNack") );
  log->set( "who", (int64_t)m_instance_id );
  log->write();
}

void
ZipfConsumer::logReceivedData( const Data& data ) const
{
  if( !shouldLogReceivedData() )
      return;
  static Log* log = Logger::getInstance( "simulation.log.udb" ).makeLog(
                            "Consumer",
                            "{ 'data-name' : $data_name, "
                            "  'what'      : $what, "
                            "  'who'       : $who  }" );
  log->set( "data_name", data.getName().toUri() );
  log->set( "what", string("ReceivedData") );
  log->set( "who", (int64_t)m_instance_id );
  log->write();
}

void
ZipfConsumer::logSentInterest( const Interest& interest ) const
{
  if( !shouldLogSentInterest() )
      return;
  static Log* log = Logger::getInstance( "simulation.log.udb" ).makeLog(
                            "Consumer",
                            "{ 'interest-name' : $interest_name, "
                            "  'what'          : $what, "
                            "  'who'           : $who  }" );
  log->set( "interest_name", interest.getName().toUri() );
  log->set( "what", string("SentRequest") );
  log->set( "who", (int64_t)m_instance_id );
  log->write();
}

void
ZipfConsumer::logTimeout( const Name& req_name,
                          uint32_t req_seq ) const
{
  if( !shouldLogTimeout() )
      return;
  static Log* log = Logger::getInstance( "simulation.log.udb" ).makeLog(
                            "Consumer",
                            "{ 'interest-name' : $interest_name, "
                            "  'seq'           : $seq, "
                            "  'what'          : $what, "
                            "  'who'           : $who  }" );
  log->set( "interest_name", req_name.toUri() );
  log->set( "seq", (int64_t)req_seq );
  log->set( "what", string("Timeout") );
  log->set( "who", (int64_t)m_instance_id );
  log->write();
}

bool
ZipfConsumer::shouldLogFinishedContent( void ) const
{
    return true;
}

bool
ZipfConsumer::shouldLogReceivedNack( void ) const
{
    return true;
}

bool
ZipfConsumer::shouldLogReceivedData( void ) const
{
    return false;
}

bool
ZipfConsumer::shouldLogSentInterest( void ) const
{
    return false;
}

bool
ZipfConsumer::shouldLogTimeout( void ) const
{
    return true;
}

} // namespace ndntac

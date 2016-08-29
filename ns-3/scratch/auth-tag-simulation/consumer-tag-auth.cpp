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

#include "consumer-tag-auth.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"

NS_LOG_COMPONENT_DEFINE("ndntac.ConsumerTagAuth");

namespace ndntac
{

NS_OBJECT_ENSURE_REGISTERED(ConsumerTagAuth);

ns3::TypeId
ConsumerTagAuth::GetTypeId(void)
{
  static ns3::TypeId tid =
    ns3::TypeId("ndntac::ConsumerTagAuth")
      .SetGroupName("Ndn")
      .SetParent<ns3::ndn::Consumer>()
      .AddConstructor<ConsumerTagAuth>()

      .AddAttribute("Window", "Initial size of the window", ns3::StringValue("1"),
                    ns3::MakeUintegerAccessor(&ConsumerTagAuth::GetWindow, &ConsumerTagAuth::SetWindow),
                    ns3::MakeUintegerChecker<uint32_t>())

      .AddAttribute("PayloadSize",
                    "Average size of content object size (to calculate interest generation rate)",
                    ns3::UintegerValue(1040), ns3::MakeUintegerAccessor(&ConsumerTagAuth::GetPayloadSize,
                                                                        &ConsumerTagAuth::SetPayloadSize),
                    ns3::MakeUintegerChecker<uint32_t>())

      .AddAttribute("Size", "Amount of data in megabytes to request, relying on PayloadSize "
                            "parameter (alternative to MaxSeq attribute)",
                    ns3::DoubleValue(-1), // don't impose limit by default
                    ns3::MakeDoubleAccessor(&ConsumerTagAuth::GetMaxSize, &ConsumerTagAuth::SetMaxSize),
                    ns3::MakeDoubleChecker<double>())

      .AddAttribute("MaxSeq", "Maximum sequence number to request (alternative to Size attribute, "
                              "would activate only if Size is -1). "
                              "The parameter is activated only if Size negative (not set)",
                    ns3::IntegerValue(std::numeric_limits<uint32_t>::max()),
                    ns3::MakeUintegerAccessor(&ConsumerTagAuth::GetSeqMax, &ConsumerTagAuth::SetSeqMax),
                    ns3::MakeUintegerChecker<uint32_t>())

      .AddAttribute("InitialWindowOnTimeout", "Set window to initial value when timeout occurs",
                    ns3::BooleanValue(true),
                    ns3::MakeBooleanAccessor(&ConsumerTagAuth::m_setInitialWindowOnTimeout),
                    ns3::MakeBooleanChecker())
      .AddAttribute( "IntervalDist", "Set interest interval distribution",
                     ns3::StringValue( "Uniform( 0 1 )" ),
                     ns3::MakeStringAccessor( &ConsumerTagAuth::m_distr_string ),
                     ns3::MakeStringChecker() );

  return tid;
}

ConsumerTagAuth::ConsumerTagAuth()
  : m_payloadSize(1040)
  , m_inFlight(0)
{
  m_distr_regex = boost::regex( "\\s*(\\w+)\\s*\\((\\s*(\\w+)\\s*)*\\)\\s*" );
}

void
ConsumerTagAuth::SetWindow(uint32_t window)
{
  m_initialWindow = window;
  m_window = m_initialWindow;
}

uint32_t
ConsumerTagAuth::GetWindow() const
{
  return m_initialWindow;
}

uint32_t
ConsumerTagAuth::GetPayloadSize() const
{
  return m_payloadSize;
}

void
ConsumerTagAuth::SetPayloadSize(uint32_t payload)
{
  m_payloadSize = payload;
}

double
ConsumerTagAuth::GetMaxSize() const
{
  if (m_seqMax == 0)
    return -1.0;

  return m_maxSize;
}

void
ConsumerTagAuth::SetMaxSize(double size)
{
  m_maxSize = size;
  if (m_maxSize < 0) {
    m_seqMax = 0;
    return;
  }

  m_seqMax = floor(1.0 + m_maxSize * 1024.0 * 1024.0 / m_payloadSize);
  NS_LOG_DEBUG("MaxSeqNo: " << m_seqMax);
  // std::cout << "MaxSeqNo: " << m_seqMax << "\n";
}

uint32_t
ConsumerTagAuth::GetSeqMax() const
{
  return m_seqMax;
}

void
ConsumerTagAuth::SetSeqMax(uint32_t seqMax)
{
  if (m_maxSize < 0)
    m_seqMax = seqMax;

  // ignore otherwise
}

void
ConsumerTagAuth::ScheduleNextPacket()
{
  // if we don't have a valid AuthTag then we request a tag instead of the content
  if( m_authTag == NULL || m_authTag->isExpired() )
  {
    m_interestName = m_contentName.getPrefix( 1 ).append( "AUTH_TAG" );
  }
  else
  {
    // if the last interest requested was an AuthTag then we need to decrement
    // the sequence number to keep Consumer from skipping an interest
    if( m_interestName.get( -1 ).toUri() == "AUTH_TAG" )
      m_seq--;
    m_interestName = m_contentName;
    
  }
  


  if (m_window == static_cast<uint32_t>(0)) {
    ns3::Simulator::Remove(m_sendEvent);

    // generate interval
    double interval = 0;
    unsigned which = m_distr.which();
    if( which == 0 )
      interval = boost::get<boost::random::uniform_real_distribution<>>( m_distr )( m_rng );
    else
      interval = boost::get<boost::random::exponential_distribution<>>( m_distr )( m_rng );
    

    NS_LOG_DEBUG(
      "Next event in " << (std::min<double>(interval, m_rtt->RetransmitTimeout().ToDouble(ns3::Time::S)))
                       << " sec");
    m_sendEvent =
      ns3::Simulator::Schedule(ns3::Seconds(
                            std::min<double>(interval, m_rtt->RetransmitTimeout().ToDouble(ns3::Time::S))),
                          &Consumer::SendPacket, this);
  }
  else if (m_inFlight >= m_window) {
    // simply do nothing
  }
  else {
    if (m_sendEvent.IsRunning()) {
      ns3::Simulator::Remove(m_sendEvent);
    }

    m_sendEvent = ns3::Simulator::ScheduleNow(&Consumer::SendPacket, this);
  }
}

///////////////////////////////////////////////////
//          Process incoming packets             //
///////////////////////////////////////////////////

void
ConsumerTagAuth::OnData(shared_ptr<const ndn::Data> data )
{
  Consumer::OnData(data);

  m_window = m_window + 1;

  if (m_inFlight > static_cast<uint32_t>(0))
    m_inFlight--;
  NS_LOG_DEBUG("Window: " << m_window << ", InFlight: " << m_inFlight);


  if( data->getContentType() == ndn::tlv::ContentType_AuthGranted )
  {
      const ndn::Block& payload = data->getContent().blockFromValue();
      m_authTag = make_shared<ndn::AuthTag>( payload );
      return;
  }

  ScheduleNextPacket();
}

void
ConsumerTagAuth::OnTimeout(uint32_t sequenceNumber)
{
  if (m_inFlight > static_cast<uint32_t>(0))
    m_inFlight--;

  if (m_setInitialWindowOnTimeout) {
    // m_window = std::max<uint32_t> (0, m_window - 1);
    m_window = m_initialWindow;
  }

  NS_LOG_DEBUG("Window: " << m_window << ", InFlight: " << m_inFlight);
  Consumer::OnTimeout(sequenceNumber);
}

void
ConsumerTagAuth::WillSendOutInterest(uint32_t sequenceNumber)
{
  m_inFlight++;
  Consumer::WillSendOutInterest(sequenceNumber);
}

void
ConsumerTagAuth::StartApplication()
{
  App::StartApplication();

  m_contentName = m_interestName;

  // parse distribution specifier
  boost::smatch match;
  boost::regex_match( m_distr_string, match, m_distr_regex );
  
  if( match.size() < 2 )
    throw std::invalid_argument( "Invalid distribution specifier format" );

  std::string distr = match[1].str();
  if( distr == "Uniform" )
  {
    if( match.size() < 5 )
      throw std::runtime_error( "Wrong number of arguments for distribution specifier" );

    double lowbound = std::stod( match[3].str() );
    double highbound = std::stod( match[4].str() );
    m_distr = boost::random::uniform_real_distribution<>( lowbound, highbound );
  }
  else if( distr == "Exponential" )
  {
    if( match.size() < 4 )
      throw std::runtime_error( "Wrong number of arguments for distribution specifier" );

    double lambda = std::stod( match[3].str() );
    m_distr = boost::random::exponential_distribution<>( lambda );
  }

  ScheduleNextPacket();
}

} // namespace ndntac

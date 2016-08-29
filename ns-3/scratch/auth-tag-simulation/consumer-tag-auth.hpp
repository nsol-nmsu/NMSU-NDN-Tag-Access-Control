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
* @file ndn-consumer-tagauth.hpp
* This is a customized version of the stock 'ndn-consumer-window.hpp'
* application.  The customizations written by Ray Stubbs [stubbs.ray@gmail.com]
* include customizale randomly distributed interest intervals and use of AuthTag
* authenticaion mechanisms.
**/
/**
* @todo I've [ Ray Stubbs ] removed all trace values from the initial version of
*       this app to simplify it.  It'd probably be best to add a few traces to
*       the app.
**/

#ifndef CONSUMER_TAG_AUTH_H
#define CONSUMER_TAG_AUTH_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ns3/ndnSIM/apps/ndn-consumer.hpp"
#include "ns3/traced-value.h"
#include "ndn-cxx/auth-tag.hpp"
#include "ndn-cxx/data.hpp"

#include <boost/regex.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#include <boost/random/exponential_distribution.hpp>

namespace ndntac {

/**
 * @brief Application for sending managing windowed
 *        interests with AuthTag authenticaiton
 *
 * This application is a modification of the stock ConsumerWindow,
 * it sends interests with respect to a window at randomly distributed
 * intervals as selected by the IntervalDist attribute.
 * IntervalDist is a string resembling a function call, for example
 * for a real uniform distribution in the range 0 to 1 seconds the
 * IntervalDist should be set to "Uniform( 0 1 )" or for an exponential
 * distriution with bounds 0 to 5 and power 2 should be written as
 * "Exponential( 0 5 2 )."  Valid distributions are listed below:
 *
 * - Uniform( min max )
 * - Exponential( lambda )
 * - Others to be added
 */
class ConsumerTagAuth : public ns3::ndn::Consumer {
public:
  static ns3::TypeId
  GetTypeId();

  /**
   * \brief Default constructor
   */
  ConsumerTagAuth();

  // From App
  virtual void
  OnData(shared_ptr<const ndn::Data> contentObject);

  virtual void
  OnTimeout(uint32_t sequenceNumber);

  virtual void
  WillSendOutInterest(uint32_t sequenceNumber);

public:
  typedef void (*WindowTraceCallback)(uint32_t);

protected:
  /**
   * \brief Constructs the Interest packet and sends it using a callback to the underlying NDN
   * protocol
   */
  virtual void
  ScheduleNextPacket();
  
  virtual void
  StartApplication() override;

private:
  virtual void
  SetWindow(uint32_t window);

  uint32_t
  GetWindow() const;

  virtual void
  SetPayloadSize(uint32_t payload);

  uint32_t
  GetPayloadSize() const;

  double
  GetMaxSize() const;

  void
  SetMaxSize(double size);

  uint32_t
  GetSeqMax() const;

  void
  SetSeqMax(uint32_t seqMax);

private:
  uint32_t m_payloadSize; // expected payload size
  double m_maxSize;       // max size to request

  uint32_t m_initialWindow;
  bool m_setInitialWindowOnTimeout;

  ndn::Name                  m_contentName;
  shared_ptr<ndn::AuthTag>   m_authTag = NULL;
  boost::random::mt19937     m_rng;
  boost::variant<boost::random::uniform_real_distribution<>,
                 boost::random::exponential_distribution<> > m_distr;

  // for parsing interval distribution
  boost::regex               m_distr_regex;
  std::string                m_distr_string;

  ns3::TracedValue<uint32_t> m_window;
  ns3::TracedValue<uint32_t> m_inFlight;
};

} // namespace ndntac

#endif

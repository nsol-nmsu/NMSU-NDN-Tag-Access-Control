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
 * @note this file has been adapted from ndnSIM/apps/ndn-consumer-window.hpp
 *       by Ray Stubbs [stubbs.ray@gmail.com]
 **/

#ifndef NDNTAC_WINDOW_CONSUMER_H
#define NDNTAC_WINDOW_CONSUMER_H

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "zipf-consumer.hpp"
#include "ns3/traced-value.h"

namespace ndntac {

class WindowConsumer : public ZipfConsumer {
public:
  static ns3::TypeId
  GetTypeId();

  WindowConsumer();

  virtual void
  OnData(std::shared_ptr<const ndn::Data> contentObject);

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
  
  void
  Reset() override;

private:
  virtual void
  SetWindow(uint32_t window);

  uint32_t
  GetWindow() const;

private:
  uint32_t m_initialWindow;
  uint32_t m_maxWindowSize;
  bool m_setInitialWindowOnTimeout;

  ns3::TracedValue<uint32_t> m_window;
  ns3::TracedValue<uint32_t> m_inFlight;
};


} // namespace ndntac

#endif

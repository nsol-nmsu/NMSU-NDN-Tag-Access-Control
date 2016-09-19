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
 * @note this file has been adapted from ndnSIM/apps/ndn-consumer-window.cpp
 *       by Ray Stubbs [stubbs.ray@gmail.com]
 **/

#include "window-consumer.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"

namespace ndntac {

using namespace ns3;
using namespace ns3::ndn;

NS_OBJECT_ENSURE_REGISTERED(WindowConsumer);

TypeId
WindowConsumer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ndntac::WindowConsumer")
      .SetGroupName("Ndn")
      .SetParent<ZipfConsumer>()
      .AddConstructor<WindowConsumer>()
      
      .AddAttribute("Window", "Initial size of the window", StringValue("1"),
                    MakeUintegerAccessor(&WindowConsumer::GetWindow, &WindowConsumer::SetWindow),
                    MakeUintegerChecker<uint32_t>() )

      .AddAttribute("InitialWindowOnTimeout", "Set window to initial value when timeout occurs",
                    BooleanValue(true),
                    MakeBooleanAccessor(&WindowConsumer::m_setInitialWindowOnTimeout),
                    MakeBooleanChecker())
      .AddAttribute("MaxWindowSize",
                    "Set the maximum allowed window size,"
                    "if less than the initial size the size will not"
                    "increase from the initial size",
                    UintegerValue(20),
                    MakeUintegerAccessor( &WindowConsumer::m_maxWindowSize ),
                    MakeUintegerChecker<uint32_t>() );
                     

  return tid;
}

WindowConsumer::WindowConsumer()
  : m_inFlight(0)
{
}

void
WindowConsumer::SetWindow(uint32_t window)
{
  m_initialWindow = window;
  m_window = m_initialWindow;
}

uint32_t
WindowConsumer::GetWindow() const
{
  return m_initialWindow;
}

void
WindowConsumer::ScheduleNextPacket()
{
  if (m_window == static_cast<uint32_t>(0)) {
    Simulator::Remove(m_sendEvent);

    m_sendEvent =
      Simulator::Schedule(Seconds(
                            std::min<double>(0.5, m_rtt->RetransmitTimeout().ToDouble(Time::S))),
                          &ZipfConsumer::SendPacket, this);
  }
  else if (m_inFlight >= m_window) {
    // simply do nothing
  }
  else {
    if (m_sendEvent.IsRunning()) {
      Simulator::Remove(m_sendEvent);
    }

    m_sendEvent = Simulator::ScheduleNow(&ZipfConsumer::SendPacket, this);
  }
}

///////////////////////////////////////////////////
//          Process incoming packets             //
///////////////////////////////////////////////////

void
WindowConsumer::OnData(shared_ptr<const Data> contentObject)
{
  ZipfConsumer::OnData(contentObject);

  if( m_window < m_maxWindowSize )
    m_window = m_window + 1;

  if (m_inFlight > static_cast<uint32_t>(0))
    m_inFlight--;

  ScheduleNextPacket();
}

void
WindowConsumer::OnTimeout(uint32_t sequenceNumber)
{
  if (m_inFlight > static_cast<uint32_t>(0))
    m_inFlight--;

  if (m_setInitialWindowOnTimeout) {
    // m_window = std::max<uint32_t> (0, m_window - 1);
    m_window = m_initialWindow;
  }

  ZipfConsumer::OnTimeout(sequenceNumber);
  
}

void
WindowConsumer::WillSendOutInterest(uint32_t sequenceNumber)
{
  m_inFlight++;
  ZipfConsumer::WillSendOutInterest(sequenceNumber);
}

void WindowConsumer::Reset()
{
    m_inFlight = 0;
    m_window = m_initialWindow;
    ZipfConsumer::Reset();
}

} // namespace ndntac

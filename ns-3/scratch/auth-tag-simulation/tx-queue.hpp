/**
* @class ndntac::TxQueue
* TxQueue allows us to queue interest and data transmition operations
* to be executed sequentially.  We do this so that we can add delay
* between transmitions to simulate signature verification and queue
* lookup delays.
*
* Events are always being scheduled and executed while the queue
* is not empty, there is no functionality for starting or stopping
* execution.
*
* @author Ray Stubbs [stubbs.ray@gmail.com]
**/

#ifndef TX_QUEUE_INCLUDED
#define TX_QUEUE_INCLUDED

#include "ns3/core-module.h"
#include "ns3/ndnSIM-module.h"
#include "ndn-cxx/interest.hpp"
#include "ndn-cxx/data.hpp"
#include <boost/variant.hpp>
#include <boost/tuple/tuple.hpp>

namespace ndntac
{

  /* Events of each type and a general event */
  typedef std::pair< shared_ptr< nfd::Face >,
                     shared_ptr< const ndn::Interest > >  SendInterestEvent;
  typedef std::pair< shared_ptr< nfd::Face >,
                     shared_ptr< const ndn::Data > >      SendDataEvent;

 // we use tuple instead of pair on these next two to distinguish from the
 // above two types
 typedef std::tuple< shared_ptr< nfd::Face >,
                     shared_ptr< const ndn::Interest > >  ReceiveInterestEvent;
 typedef std::tuple< shared_ptr< nfd::Face >,
                     shared_ptr< const ndn::Data > >      ReceiveDataEvent;
  typedef ns3::Time                                       DelayEvent;
  typedef boost::variant< SendInterestEvent,
                          SendDataEvent,
                          ReceiveInterestEvent,
                          ReceiveDataEvent,
                          DelayEvent >                    TxEvent;

  class TxQueue
  {
  private:
    /* types of events */
    enum TxEventType
    {
      TxEvent_SendInterest,
      TxEvent_SendData,
      TxEvent_ReceiveInterest,
      TxEvent_ReceiveData,
      TxEvent_Delay
    };

    /* event queue */
    std::queue< TxEvent > m_queue;

    /* keeps track of wether an event is pending execution or not */
    bool m_pending = false;

    /* called to add an event of any type to the queue */
    void
    addEvent( const TxEvent& event )
    {
      m_queue.push( event );
      if( !m_pending )
      {
        m_pending = true;
        doNext();
      }
    }

    /* called to execute the next event in the queue */
    void
    doNext()
    {
      // if not events to execute then don't
      if( m_queue.size() == 0 )
      {
        m_pending = false;
        return;
      }

      // otherwise schedule all events sequentially
      const TxEvent& event = m_queue.front();
      switch( event.which() )
      {
        case 0:
        {
          // schedule send interest then schedule next event immediately
          const SendInterestEvent& send_event = boost::get<SendInterestEvent>(event);
          ns3::Simulator::ScheduleNow( &TxQueue::doSendInterest,
                                        this,
                                        send_event.first,
                                        send_event.second );
          ns3::Simulator::ScheduleNow( &TxQueue::doNext,
                                        this );
          break;
        }
        case 1:
        {
          // schedule send interest then schedule next event immediately
          const SendDataEvent& send_event = boost::get<SendDataEvent>(event);
          ns3::Simulator::ScheduleNow( &TxQueue::doSendData,
                                        this,
                                        send_event.first,
                                        send_event.second );
          ns3::Simulator::ScheduleNow( &TxQueue::doNext,
                                        this );
          break;
        }
        case 2:
        {
          // schedule receive interest then schedule next event immediately
          const ReceiveInterestEvent& receive_event = boost::get<ReceiveInterestEvent>(event);
          ns3::Simulator::ScheduleNow( &TxQueue::doReceiveInterest,
                                        this,
                                        std::get<0>(receive_event),
                                        std::get<1>(receive_event ) );
          ns3::Simulator::ScheduleNow( &TxQueue::doNext,
                                        this );
          break;
        }
        case 3:
        {
          // schedule receive data then schedule next event immediately
          const ReceiveDataEvent& receive_event = boost::get<ReceiveDataEvent>(event);
          ns3::Simulator::ScheduleNow( &TxQueue::doReceiveData,
                                        this,
                                        std::get<0>(receive_event),
                                        std::get<1>(receive_event ) );
          ns3::Simulator::ScheduleNow( &TxQueue::doNext,
                                        this );
          break;
        }
      case 4:
          // schedule next event after specified delay
          const DelayEvent& delay_event = boost::get<DelayEvent>( event );
          ns3::Simulator::Schedule( delay_event,
                                    &TxQueue::doNext,
                                    this );
          break;
      };

      m_queue.pop();
    };

    void
    doSendData( shared_ptr< nfd::Face > face,
                shared_ptr< const ndn::Data > data )
    {
      face->sendData( *data );
    }

    void
    doSendInterest( shared_ptr< nfd::Face > face,
                    shared_ptr< const ndn::Interest > interest)
    {
      face->sendInterest( *interest );
    }

    void
    doReceiveData( shared_ptr< nfd::Face > face,
                shared_ptr< const ndn::Data > data )
    {
      face->emit_onReceiveData( *data );
    }

    void
    doReceiveInterest( shared_ptr< nfd::Face > face,
                    shared_ptr< const ndn::Interest > interest)
    {
      face->emit_onReceiveInterest( *interest );
    }
  public:
    /**
    * @brief Queue an interest to be sent to the given face
    * @param face      Face to send interest to
    * @param interest  Interest to send
    **/
    void
    sendInterest( shared_ptr< nfd::Face > face,
                  shared_ptr<const ndn::Interest> interest )
    {
      addEvent( make_pair( face, interest ) );
    }

    /**
    * @brief Queue a Data to be sent to the given face
    * @param face     Face to send data to
    * @param data     Data to send
    **/
    void
    sendData( shared_ptr< nfd::Face > face,
              shared_ptr< const ndn::Data > data )
    {
      addEvent( make_pair( face, data ) );
    }

    /**
    * @brief Queues an emmission of onReceiveInterest signal
    * @param face      Face to receive interest
    * @param interest  Interest to receive
    **/
    void
    receiveInterest( shared_ptr< nfd::Face > face,
                     shared_ptr<const ndn::Interest> interest )
    {
      addEvent( make_tuple( face, interest ) );
    }

    /**
    * @brief Queue an emission of onReceiveData event
    * @param face     Face receive data
    * @param data     Data to receive
    **/
    void
    receiveData( shared_ptr< nfd::Face > face,
                 shared_ptr< const ndn::Data > data )
    {
      addEvent( make_tuple( face, data ) );
    }

    /**
    * @brief Queue a delay
    * @param delay  Length of delay
    **/
    void
    delay( const ns3::Time& delay )
    {
      addEvent( delay );
    }
  };

};

#endif // TX_QUEUE_INCLUDED

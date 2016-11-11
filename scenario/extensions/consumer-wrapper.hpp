/*
* A wrapper for any consumer extending ndntac::ZipfConsumer
*/

#ifndef CONSUMER_WRAPPER_H
#define CONSUMER_WRAPPER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/names.h"
#include "ns3/ndnSIM/apps/ndn-consumer-cbr.hpp"
#include "ndn-cxx/interest.hpp"
#include "ndn-cxx/data.hpp"
#include "ndn-cxx/name.hpp"
#include "ndn-cxx/auth-tag.hpp"
#include "zipf-consumer.hpp"
#include "window-consumer.hpp"

namespace ndntac
{
    // template should only be used on subclasses of ndntac::ZipfConsumer
    template< typename Consumer >
    class ConsumerWrapper : public Consumer
    {
    public:
        static ns3::TypeId
        GetTypeId();
      
        ConsumerWrapper();
        
        virtual void
        SendPacket();
        
    protected:
    
        void
        OnData( std::shared_ptr< const ndn::Data > data ) override;
        
        void
        StartApplication() override;
        
       void
       WillSendOutInterest( std::shared_ptr< ndn::Interest> interest ) override;
       
       void
       Reset() override;
    
    private:
      bool
      EnsureAuth();
      
      void
      SendAuthRequest();
    private:
        void
        logRequestedAuth( const ndn::Interest& interest ) const;
        
        void
        logReceivedAuth( const ndn::Data& data ) const;
        
    private:
        std::shared_ptr<ndn::AuthTag> m_auth_tag = NULL;
        ns3::EventId             m_auth_event;
        bool                     m_pending_auth = false;
        ns3::Time                m_auth_timeout;
          
    };
    
    typedef ConsumerWrapper< WindowConsumer > AuthWindowConsumer;

    template class ConsumerWrapper<WindowConsumer>;
    
    template< typename Consumer >
    ns3::TypeId
    ConsumerWrapper<Consumer>::GetTypeId()
    {
        static ns3::TypeId tid =
        ns3::TypeId( (string("ndntac::ConsumerWrapper<") + typeid(Consumer).name() + ">").c_str())
            .SetGroupName( "Ndn" )
            .SetParent<Consumer>()
            .template AddConstructor<ConsumerWrapper<Consumer>>();
        return tid;
    };
    
    template< typename Consumer >
    ConsumerWrapper<Consumer>::ConsumerWrapper()
    {
    };
    
    template< typename Consumer >
    bool
    ConsumerWrapper<Consumer>::EnsureAuth()
    {
        if( m_auth_tag != NULL && !m_auth_tag->isExpired() )
            return true;
        
        if( !m_pending_auth )
        {
            if( m_auth_event.IsRunning() )
                ns3::Simulator::Remove( m_auth_event );
            m_auth_event = ns3::Simulator::ScheduleNow 
                           ( &ConsumerWrapper<Consumer>::SendAuthRequest,
                             this );
            m_pending_auth = true;
            m_auth_timeout = ns3::Simulator::Now() + ns3::Seconds( 0.5 );
            return false;
        }
        
        if( ns3::Simulator::Now() > m_auth_timeout )
        {
            m_pending_auth = false;
            EnsureAuth();
        }
        
        return false;
        
    };
    
    template< typename Consumer >
    void
    ConsumerWrapper<Consumer>::SendAuthRequest()
    {
          auto interest = make_shared<ndn::Interest>();
          interest->setNonce(Consumer::m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
          interest->setName(Consumer::m_interestName.getPrefix( 1 ).append("AUTH_TAG") );
          ndn::time::milliseconds interestLifeTime(Consumer::m_interestLifeTime.GetMilliSeconds());
          interest->setInterestLifetime(interestLifeTime);
          interest->setAuthTag( ndn::AuthTag( 0 ) );
          interest->setRouteTracker( ::ndn::RouteTracker() );

          Consumer::WillSendOutInterest(0);

          Consumer::m_transmittedInterests(interest, this, Consumer::m_face);
          Consumer::m_face->onReceiveInterest(*interest);
          
          logRequestedAuth( *interest );
    };
    
    template< typename Consumer >
    void
    ConsumerWrapper<Consumer>::SendPacket()
    {
        if( EnsureAuth() )
        {
             Consumer::SendPacket();
        }
        else
        {
            ns3::Simulator::Schedule( ns3::Seconds( 0.2 ),
                                      &ConsumerWrapper<Consumer>::SendPacket,
                                      this );
        }
        
    };
    
    template< typename Consumer >
    void
    ConsumerWrapper<Consumer>::OnData( std::shared_ptr< const ndn::Data > data )
    {
        // data returned to the consumer should always be in
        // the consumer network
        BOOST_ASSERT( data->getCurrentNetwork() == ndn::RouteTracker::ENTRY_NETWORK );
        
        switch( data->getContentType() )
        {
            case ndn::tlv::ContentType_Auth:
            {
                const ndn::Block& payload = data->getContent().blockFromValue();
                m_auth_tag = make_shared<ndn::AuthTag>( payload );
                m_pending_auth = false;
                
                logReceivedAuth( *data );
                break;
            }
            case ndn::tlv::ContentType_AuthDenial:
                // TODO: log stuff
                break;
            case  ndn::tlv::ContentType_Blob:
                // TODO: Log stuff
                Consumer::OnData( data );
                break;
            case ndn::tlv::ContentType_Nack:
                // TODO: Log stuff
                Consumer::OnData( data );
                break;
            case ndn::tlv::ContentType_EoC:
                // TODO: Log stuff
                Consumer::OnData( data );
                break;
            default:
                // TODO: Log stuff, unexpected data type
                break;
        }
    };
    
    template< typename Consumer >
    void
    ConsumerWrapper<Consumer>::StartApplication()
    {
        Consumer::StartApplication();
    };
    
    template< typename Consumer >
    void
    ConsumerWrapper<Consumer>::WillSendOutInterest( std::shared_ptr< ndn::Interest> interest )
    {
        interest->setAuthTag( *m_auth_tag );
    }
    
    template< typename Consumer >
    void
    ConsumerWrapper<Consumer>::Reset()
    {
        m_auth_tag = NULL;
        Consumer::Reset();
    }

    template< typename Consumer >
    void
    ConsumerWrapper<Consumer>::logRequestedAuth( const ndn::Interest& interest ) const
    {
        /*
        Coordinator::LogEntry entry( "Consumer", "RequestedAuth");
        entry.add( "id", std::to_string( Consumer::m_instance_id ) );
        entry.add( "interest-name", interest.getName().toUri() );
        Coordinator::log( entry );*/
    }
    
    template< typename Consumer >
    void
    ConsumerWrapper<Consumer>::logReceivedAuth( const ndn::Data& data ) const
    {
        /*
        Coordinator::LogEntry entry( "Consumer", "ReceivedAuth");
        entry.add( "id", std::to_string( Consumer::m_instance_id ) );
        entry.add( "interest-name", data.getName().toUri() );
        Coordinator::log( entry );*/
    }
};

#endif


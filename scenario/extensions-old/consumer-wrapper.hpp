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
#include "ns3/ndnSIM/utils/dummy-keychain.hpp"

#include "logger.hpp"

#define BAD_AUTH_NO_TAG         1
#define BAD_AUTH_SIGNATURE      2
#define BAD_AUTH_EXPIRED        3
#define BAD_AUTH_ROUTE          4
#define BAD_AUTH_PREFIX         5
#define BAD_AUTH_KEYLOC         6

namespace ndntac
{
    // template should only be used on subclasses of ndntac::ZipfConsumer
    template< typename Consumer, int BadFlags >
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
    protected:
       void
       logRequestedAuth( const ndn::Interest& interest ) const;
        
       void
       logReceivedAuth( const ndn::AuthTag& auth ) const;
       
       void
       logBadConsumerReceivedData( const ndn::Data& data,
                                   int badflags ) const;
        
       bool
       shouldLogRequestedAuth( void ) const;
       
       bool
       shouldLogReceivedAuth( void ) const;
       
       bool
       shouldLogBadConsumerReceivedData( void ) const;
        
    private:
        std::shared_ptr<ndn::AuthTag> m_auth_tag = NULL;
        ns3::EventId             m_auth_event;
        bool                     m_pending_auth = false;
        ns3::Time                m_auth_timeout;
          
    };
    
    typedef ConsumerWrapper< WindowConsumer, 0 > AuthWindowConsumer;
    typedef ConsumerWrapper< WindowConsumer, BAD_AUTH_NO_TAG > NoTagWindowConsumer;
    typedef ConsumerWrapper< WindowConsumer, BAD_AUTH_SIGNATURE > BadSignatureWindowConsumer;
    typedef ConsumerWrapper< WindowConsumer, BAD_AUTH_EXPIRED > ExpiredWindowConsumer;
    typedef ConsumerWrapper< WindowConsumer, BAD_AUTH_ROUTE >   BadRouteWindowConsumer;
    typedef ConsumerWrapper< WindowConsumer, BAD_AUTH_PREFIX >  BadPrefixWindowConsumer;
    typedef ConsumerWrapper< WindowConsumer, BAD_AUTH_KEYLOC >  BadKeyLocWindowConsumer;

    template class ConsumerWrapper<WindowConsumer, 0>;
    template class ConsumerWrapper< WindowConsumer, BAD_AUTH_NO_TAG >;
    template class ConsumerWrapper< WindowConsumer, BAD_AUTH_SIGNATURE >;
    template class ConsumerWrapper< WindowConsumer, BAD_AUTH_EXPIRED >;
    template class ConsumerWrapper< WindowConsumer, BAD_AUTH_ROUTE >;
    template class ConsumerWrapper< WindowConsumer, BAD_AUTH_PREFIX >;
    template class ConsumerWrapper< WindowConsumer, BAD_AUTH_KEYLOC >;
    
    
    
    template< typename Consumer, int BadFlags >
    ns3::TypeId
    ConsumerWrapper<Consumer, BadFlags >::GetTypeId()
    {
        static ns3::TypeId tid =
            ns3::TypeId( ( string( "WrappedConsumer" ) + std::to_string( BadFlags ) ).c_str() )
            .SetGroupName( "Ndn" )
            .SetParent<Consumer>()
            .template AddConstructor< ConsumerWrapper<Consumer, BadFlags> >();
        return tid;
    };
    
    template< typename Consumer, int BadFlags >
    ConsumerWrapper<Consumer, BadFlags >::ConsumerWrapper()
    {
    };
    
    template< typename Consumer, int BadFlags >
    bool
    ConsumerWrapper<Consumer, BadFlags>::EnsureAuth()
    {
        // if BAD_AUTH_NO_TAG is set then we don't use the auth
        // tag and just say that we have authentication
        if( BAD_AUTH_NO_TAG & BadFlags )
            return true;

        if( m_auth_tag != NULL && !m_auth_tag->isExpired() )
            return true;
        
        if( !m_pending_auth )
        {
            if( m_auth_event.IsRunning() )
                ns3::Simulator::Remove( m_auth_event );
            m_auth_event = ns3::Simulator::ScheduleNow 
                           ( &ConsumerWrapper<Consumer, BadFlags>::SendAuthRequest,
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
    
    template< typename Consumer, int BadFlags >
    void
    ConsumerWrapper<Consumer, BadFlags>::SendAuthRequest()
    {
          auto interest = make_shared<ndn::Interest>();
          interest->setNonce(Consumer::m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
          interest->setName(Consumer::m_interestName.getPrefix( 1 ).append("AUTH_TAG").appendNumber( Consumer::m_instance_id ) );
          ndn::time::milliseconds interestLifeTime(Consumer::m_interestLifeTime.GetMilliSeconds());
          interest->setInterestLifetime(interestLifeTime);
          interest->setAuthTag( ndn::AuthTag( 0 ) );
          interest->setRouteTracker( ::ndn::RouteTracker() );

          Consumer::WillSendOutInterest(0);

          Consumer::m_transmittedInterests(interest, this, Consumer::m_face);
          Consumer::m_face->onReceiveInterest(*interest);
          
          logRequestedAuth( *interest );
    };
    
    template< typename Consumer, int BadFlags >
    void
    ConsumerWrapper<Consumer, BadFlags>::SendPacket()
    {
        if( EnsureAuth() )
        {
             Consumer::SendPacket();
        }
        else
        {
            ns3::Simulator::Schedule( ns3::Seconds( 0.2 ),
                                      &ConsumerWrapper<Consumer, BadFlags>::SendPacket,
                                      this );
        }
        
    };
    
    template< typename Consumer, int BadFlags >
    void
    ConsumerWrapper<Consumer, BadFlags>::OnData( std::shared_ptr< const ndn::Data > data )
    {
        // data returned to the consumer should always be in
        // the consumer network
        BOOST_ASSERT( data->getCurrentNetwork() == ndn::RouteTracker::ENTRY_NETWORK );
        
        if( data->getContentType() == ndn::tlv::ContentType_Auth )
        {
            const ndn::Block& payload = data->getContent().blockFromValue();
            m_auth_tag = make_shared<ndn::AuthTag>( payload );
            m_pending_auth = false;
            
            // apply all defects from the template tag
            if( BAD_AUTH_SIGNATURE & BadFlags )
            {
                m_auth_tag->setSignature( ndn::security::DUMMY_NDN_BAD_SIGNATURE );
            }
            if( BAD_AUTH_EXPIRED & BadFlags )
            {
                m_auth_tag->setExpirationTime( ndn::time::system_clock::now() );
            }
            if( BAD_AUTH_ROUTE & BadFlags )
            {
                m_auth_tag->setRouteHash( 0 );
            }
            if( BAD_AUTH_PREFIX & BadFlags )
            {
                m_auth_tag->setPrefix( "bad_prefix" );
            }
            if( BAD_AUTH_KEYLOC )
            {
                m_auth_tag->setKeyLocator( ndn::KeyLocator( "badloc" ) );
            }
            
            logReceivedAuth( *m_auth_tag );
        }
        else
        {
             Consumer::OnData( data );
        }
    };
    
    template< typename Consumer, int BadFlags >
    void
    ConsumerWrapper<Consumer, BadFlags>::StartApplication()
    {
        Consumer::StartApplication();
    };
    
    template< typename Consumer, int BadFlags >
    void
    ConsumerWrapper<Consumer, BadFlags>::WillSendOutInterest( std::shared_ptr< ndn::Interest> interest )
    {
        interest->setAuthTag( *m_auth_tag );
    }
    
    template< typename Consumer, int BadFlags >
    void
    ConsumerWrapper<Consumer, BadFlags>::Reset()
    {
        m_auth_tag = NULL;
        Consumer::Reset();
    }

    template< typename Consumer, int BadFlags >
    void
    ConsumerWrapper<Consumer, BadFlags>::logRequestedAuth( const ndn::Interest& interest ) const
    {
        if( !shouldLogRequestedAuth() )
            return;

      static Log* log = Logger::getInstance( "simulation.log.udb" ).makeLog(
                                "Consumer",
                                "{ 'interest-name' : $interest_name, "
                                "  'what'          : $what, "
                                "  'who'           : $who  }" );
      log->set( "interest_name", interest.getName().toUri() );
      log->set( "what", string("RequestedAuth") );
      log->set( "who", (int64_t)Consumer::m_instance_id );
      log->write();
    }
    
    template< typename Consumer, int BadFlags >
    void
    ConsumerWrapper<Consumer, BadFlags>::logReceivedAuth( const ndn::AuthTag& auth ) const
    {
        if( !shouldLogReceivedAuth() )
            return;

          static Log* log = Logger::getInstance( "simulation.log.udb" ).makeLog(
                                    "Consumer",
                                    "{ 'auth-prefix'   : $auth_prefix, "
                                    "  'auth-access'   : $auth_access, "
                                    "  'auth-expired'  : $auth_expired, "
                                    "  'what'          : $what, "
                                    "  'who'           : $who  }" );
          log->set( "auth_prefix", auth.getPrefix().toUri() );
          log->set( "auth_access", (int64_t)auth.getAccessLevel() );
          log->set( "auth_expired", auth.isExpired() );
          log->set( "what", string("ReceivedAuth") );
          log->set( "who", (int64_t)Consumer::m_instance_id );
          log->write();
    }
    
    template< typename Consumer, int BadFlags >
    void
    ConsumerWrapper<Consumer, BadFlags>::logBadConsumerReceivedData( const ndn::Data& data,
                                                                     int badflags ) const
    {
        if( !shouldLogBadConsumerReceivedData() )
            return;

          static Log* log = Logger::getInstance( "simulation.log.udb" ).makeLog(
                                    "BadConsumer",
                                    "{ 'data-name'   : $data_name, "
                                    "  'badflags'   : $badflags, "
                                    "  'what'          : $what, "
                                    "  'who'           : $who  }" );
          log->set( "data_name", data.getName().toUri() );
          log->set( "badflags", (int64_t)BadFlags );
          log->set( "what", string("ReceivedData") );
          log->set( "who", (int64_t)Consumer::m_instance_id );
          log->write();
    }
    
    // these can be modified to control what gets logged
    template< typename Consumer, int BadFlags >
    bool
    ConsumerWrapper<Consumer, BadFlags>::shouldLogReceivedAuth( void ) const
    {
        return true;
    }
    template< typename Consumer, int BadFlags >
    bool
    ConsumerWrapper<Consumer, BadFlags>::shouldLogRequestedAuth( void ) const
    {
        return true;
    }
    template< typename Consumer, int BadFlags >
    bool
    ConsumerWrapper<Consumer, BadFlags>::shouldLogBadConsumerReceivedData( void ) const
    {
        return true;
    }
    
    
};

#endif


#include "consumer-wrapper.hpp"
#include "coordinator.hpp"
#include <typeinfo>

namespace ndntac
{
    template class ConsumerWrapper<WindowConsumer>;
    
    template< typename Consumer >
    uint32_t ConsumerWrapper<Consumer>::s_instance_id = 0;
    
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
        m_instance_id = s_instance_id++;
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
          interest->setName(Consumer::m_interestName.getPrefix( 1 ).append("AUTH_TAG").appendNumber( m_instance_id ) );
          ndn::Block payload( ndn::tlv::ContentType_AuthRequest,
                              ndn::makeNonNegativeIntegerBlock( ndn::tlv::RouteHash, 0 ) );
          interest->setPayload( payload );
          ndn::time::milliseconds interestLifeTime(Consumer::m_interestLifeTime.GetMilliSeconds());
          interest->setInterestLifetime(interestLifeTime);

          Consumer::WillSendOutInterest(0);

          Consumer::m_transmittedInterests(interest, this, Consumer::m_face);
          Consumer::m_face->onReceiveInterest(*interest);
          
          Coordinator::consumerRequestedAuth( m_instance_id, interest->getName() );
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
        if( data->getContentType() == ndn::tlv::ContentType_AuthGranted )
        {
            const ndn::Block& payload = data->getContent().blockFromValue();
            m_auth_tag = make_shared<ndn::AuthTag>( payload );
            m_pending_auth = false;
            Coordinator::consumerReceivedAuth( m_instance_id, data->getName() );
        }
        else
        {
            switch( data->getContentType() )
            {
                case  ndn::tlv::ContentType_Blob:
                    Coordinator::consumerRequestSatisfied( m_instance_id, data->getName() );
                    break;
                case ndn::tlv::ContentType_Nack:
                    Coordinator::consumerRequestRejected( m_instance_id, data->getName() );
                    break;
                case ndn::tlv::ContentType_EoC:
                    Coordinator::consumerOther( m_instance_id,
                                                string( "Finished content ")
                                                + Consumer::m_interestName.toUri() );
                    break;
                default:
                    Coordinator::consumerOther( m_instance_id, "Unexpected response type" );
                    break;
            }
            
            Consumer::OnData( data );
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
};
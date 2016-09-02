#include "consumer-wrapper.hpp"
#include "coordinator.hpp"
#include <typeinfo>

namespace ndntac
{
    
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
          interest->setName(Consumer::m_interestName.getPrefix( 1 ).append("AUTH_TAG") );
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
              // the following code was copied from
              // ndnSIM/apps/ndn-consumer.cpp Consumer::SendPacket(...)
              // and minor modifications were added
              if (!Consumer::m_active)
                return;

              NS_LOG_FUNCTION_NOARGS();

              uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid

              while (Consumer::m_retxSeqs.size()) {
                seq = *Consumer::m_retxSeqs.begin();
                Consumer::m_retxSeqs.erase(Consumer::m_retxSeqs.begin());
                break;
              }

              if (seq == std::numeric_limits<uint32_t>::max()) {
                if (Consumer::m_seqMax != std::numeric_limits<uint32_t>::max()) {
                  if (Consumer::m_seq >= Consumer::m_seqMax) {
                    return; // we are totally done
                  }
                }

                seq = Consumer::m_seq++;
              }

              //
              shared_ptr<ndn::Name> nameWithSequence = make_shared<ndn::Name>(Consumer::m_interestName);
              nameWithSequence->appendSequenceNumber(seq);
              //

              // shared_ptr<Interest> interest = make_shared<Interest> ();
              shared_ptr<ndn::Interest> interest = make_shared<ndn::Interest>();
              interest->setNonce(Consumer::m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
              interest->setName(*nameWithSequence);
              interest->setAuthTag( *m_auth_tag );
              ndn::time::milliseconds interestLifeTime(Consumer::m_interestLifeTime.GetMilliSeconds());
              interest->setInterestLifetime(interestLifeTime);

              // NS_LOG_INFO ("Requesting Interest: \n" << *interest);
              NS_LOG_INFO("> Interest for " << seq);

              Consumer::WillSendOutInterest(seq);

              Consumer::m_transmittedInterests(interest, this, Consumer::m_face);
              Consumer::m_face->onReceiveInterest(*interest);
              
              Coordinator::consumerSentRequest( m_instance_id, interest->getName() );

              Consumer::ScheduleNextPacket();
              
              // end of copied content
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
    ConsumerWrapper<Consumer>::OnData( shared_ptr< const ndn::Data > data )
    {
        if( data->getName().get( 1 ) == ndn::Name::Component( "AUTH_TAG" ) )
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
                Coordinator::consumerAuthDenied( m_instance_id, data->getName() );
                throw runtime_error("This shouldn't happen" );
            }
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
    
    
};

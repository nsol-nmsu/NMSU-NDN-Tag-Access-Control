#include "router-strategy.hpp"
#include "coordinator.hpp"

namespace ndntac
{

  const ndn::Name
  RouterStrategy::STRATEGY_NAME = "ndn:/localhost/nfd/strategy/ndntac-router-strategy";
  const ns3::Time
  RouterStrategy::s_router_signature_delay = ns3::MilliSeconds( 121.641253 );
  const ns3::Time
  RouterStrategy::s_router_bloom_delay = ns3::MilliSeconds( 9.238432 );
  const ns3::Time
  RouterStrategy::s_router_interest_delay = ns3::MilliSeconds( 100 );
  uint32_t RouterStrategy::s_instance_id = 0;

  RouterStrategy::RouterStrategy( nfd::Forwarder& forwarder,
                                  const ndn::Name& name )
                                    : BestRouteStrategy( forwarder, name )
                                    , m_auth_cache( 1e-10, 10000 )
                                    , m_forwarder( forwarder )
  {
    m_instance_id = s_instance_id++;
    Coordinator::addRouter( m_instance_id );
  }

  bool
  RouterStrategy::onIncomingInterest( nfd::Face& face,
                                      const ndn::Interest& interest )
  {
    bool handled_interest = false;

    Coordinator::routerReceivedRequest( m_instance_id, interest.getName() );

    // every interest takes some amount of processing time
    // for now we simulate this with a constant delay
    m_queue.delay( s_router_interest_delay );

    // if Data is in CS
    auto on_hit = [&](const ndn::Interest& interest, const ndn::Data& data)
    {
      handled_interest = true;
      onDataHit( face, interest, data );
    };

    // if not in CS
    auto on_miss = [&](const ndn::Interest& interest )
    {
      handled_interest = false;
      onDataMiss( face, interest );
    };

    // handle interest appropriately
    m_forwarder.getCs().find( interest, on_hit, on_miss );
    return handled_interest;
  }

  void
  RouterStrategy::beforeSatisfyInterest( shared_ptr<nfd::pit::Entry> pitEntry,
                                         const nfd::Face& inFace, const ndn::Data& data)
  {
    // the data belongs to the main interest of the pit entry
    // since the response is already appropriate for the interest
    // ( either valid data or an AuthDenied packet ) the data is
    // immediately forwarded downstream all appropriate faces.

    //TODO

    // while the pit entry has related interests that can be satisfied by the
    // data, handle those as well.  These have not yet been authenticated yet
    // so that's our job
    // TODO

  }

   shared_ptr< ndn::Data >
   RouterStrategy::makeAuthDenial( const ndn::Data& data )
   {
     auto denial = make_shared< ndn::Data >( data.getName() );
     denial->setContentType( ndn::tlv::ContentType_AuthDenial );
     denial->setContent( data.wireEncode() );
     denial->setSignatureValue( ndn::Block( ndn::tlv::SignatureValue, ndn::Block( "Not Empty", 9 ) ) );
     denial->setFreshnessPeriod( ndn::time::milliseconds(0));
     return denial;
   }

   void RouterStrategy::onDataHit( nfd::Face& face,
                   const ndn::Interest& interest,
                   const ndn::Data& const_data)
   {
     // unconstify data
     ndn::Data data = const_data;

     // if we have the data, verify the auth tag
     const ndn::AuthTag& tag = interest.getAuthTag();

     // access level of 0 does not need tag
     if( data.getAccessLevel() == 0 )
     {
       Coordinator::routerSatisfiedRequest( m_instance_id, interest.getName() );
       m_queue.sendData( face.shared_from_this(), data.shared_from_this() );
       return;
     }

     // tags with invalid access level are refused
     if( data.getAccessLevel() > tag.getAccessLevel() )
     {
       Coordinator::routerDeniedRequest( m_instance_id,
                                         interest.getName(),
                                         "AuthTag access level is too low");
       m_queue.sendData( face.shared_from_this(),
                         makeAuthDenial( data ) );
       return;
     }

     // interests with expired tags are refused
     if( tag.isExpired() )
     {
       Coordinator::routerDeniedRequest( m_instance_id,
                                         interest.getName(),
                                         "AuthTag is expired");
       m_queue.sendData( face.shared_from_this(),
                         makeAuthDenial( data ) );
       return;
     }

     // tags with wrong prefixes are refused
     if( !tag.getPrefix().isPrefixOf( data.getName() ) )
     {
       Coordinator::routerDeniedRequest( m_instance_id,
                                         interest.getName(),
                                         "AuthTag prefix does not match data" );
       m_queue.sendData( face.shared_from_this(),
                         makeAuthDenial( data ) );
       return;
     }

     // tags with non matching key locators are refused
     if( !data.getSignature().hasKeyLocator() )
     {
       Coordinator::routerDeniedRequest( m_instance_id,
                                         interest.getName(),
                                         "Requested data has no key locator");
       m_queue.sendData( face.shared_from_this(),
                         makeAuthDenial( data ) );
       return;
     }
     if( tag.getKeyLocator() != data.getSignature().getKeyLocator() )
     {
       Coordinator::routerDeniedRequest( m_instance_id,
                                         interest.getName(),
                                         "AuthTag key locator does not match "
                                         "data key locator" );
       m_queue.sendData( face.shared_from_this(),
                         makeAuthDenial( data ) );
       return;
     }

     // only verify signature with a probability equivalent to
     // the opposite probability of the interest's AuthValidityProbability
     if( (uint32_t)rand() < interest.getAuthValidityProb() )
     {
       Coordinator::routerSatisfiedRequest( m_instance_id, interest.getName() );
       m_queue.sendData( face.shared_from_this(), data.shared_from_this() );
       return;
     }

     // set NoReCache tag appropriately
     if( interest.getAuthValidityProb() > 0 )
      data.setNoReCacheFlag( true );

     // AuthValidityProbability of 0 indicates that the edge router's bloom
     // filters didn't have any information on the tag so we lookup
     // the tag in our bloom if prob is 0
     if( interest.getAuthValidityProb() == 0 )
     {
       m_queue.delay( s_router_bloom_delay );
       if( m_auth_cache.contains( tag ) )
       {
         Coordinator::routerSatisfiedRequest( m_instance_id, interest.getName() );
         m_queue.sendData( face.shared_from_this(), data.shared_from_this() );
         return;
       }
     }

     // verify signature, we simulate actual verification delay by
     // adding delay to the transmit queue, signature is valid if it
     // isn't empty, sine we don't need to actually validate for the
     // simulation
     m_queue.delay( s_router_signature_delay );
     if( tag.getSignature().getValue().value_size() > 0 )
     {
       Coordinator::routerSatisfiedRequest( m_instance_id, interest.getName() );
       m_queue.sendData( face.shared_from_this(), data.shared_from_this() );
       m_auth_cache.insert( tag );
       return;
     }

     // bad signature
     Coordinator::routerDeniedRequest( m_instance_id,
                                       interest.getName(),
                                       "Invalid AuthTag signature" );
     m_queue.sendData( face.shared_from_this(), makeAuthDenial( data ) );
 }

 void RouterStrategy::onDataMiss( nfd::Face& face,
                                  const ndn::Interest& interest )
 {
   Coordinator::routerForwardedRequest( m_instance_id, interest.getName() );
   // NADA
 }
}

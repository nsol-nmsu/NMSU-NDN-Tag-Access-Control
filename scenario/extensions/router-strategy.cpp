#include "router-strategy.hpp"
#include "coordinator.hpp"
#include "ns3/ndnSIM/utils/dummy-keychain.hpp"

namespace ndntac
{

  const ndn::Name
  RouterStrategy::STRATEGY_NAME = "ndn:/localhost/nfd/strategy/ndntac-router-strategy";
  const ns3::Time
  RouterStrategy::s_router_signature_delay = ns3::NanoSeconds( 30345 );
  const ns3::Time
  RouterStrategy::s_router_bloom_delay = ns3::NanoSeconds( 2535 );
  const ns3::Time
  RouterStrategy::s_router_interest_delay = ns3::MilliSeconds( 0 );
  uint32_t RouterStrategy::s_instance_id_offset = 0;

  RouterStrategy::RouterStrategy( nfd::Forwarder& forwarder,
                                  const ndn::Name& name )
                                    : BestRouteStrategy( forwarder, name )
                                    , m_auth_cache( 1e-10, 10000 )
                                    , m_forwarder( forwarder )
  {
    m_instance_id = rand() + s_instance_id_offset++;
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
    // get the in records for the entry
    auto in_records = pitEntry->getInRecords();
    
    // we need to authenticate each interest individually
    // so we remove all current in records and re-insert
    // the valid ones only
    pitEntry->deleteInRecords();
    
    // the first in record corresponds to the interest
    // that was forwarded, the the upstream router already validated
    // the request, so if the data is anything but a Nack or AuthDenial
    // then we can keep the interest, otherwise we remove it
    auto it = in_records.begin();
    if( data.getContentType() == ndn::tlv::ContentType_Nack
        || data.getContnetType() == ndn::tlv::ContentType_AuthDenial )
    {
        pitEntry->insertOrUpdateInRecord( it->getFace(), it->getInterest() );
    }
    else
    {
        // TODO: log upstream failed authentication
    }
    
    it++;
    
    // for all other interests we check their authentication
    for( ; it != in_records.end(); it++ )
    {
        if( validateAccess( it->getInterest(), data ) )
        {
            pitEntry->insertOrUpdateInRecord( it->getFace(), it->getInterest() );
        }
        else
        {
            // TODO: log failed authentication
        }
    }


  }

   shared_ptr< ndn::Data >
   RouterStrategy::makeAuthDenial( const ndn::Data& data )
   {
     auto denial = make_shared< ndn::Data >( data.getName() );
     denial->setContentType( ndn::tlv::ContentType_AuthDenial );
     denial->setContent( data.wireEncode() );
     denial->setFreshnessPeriod( ndn::time::milliseconds(0));
     denial->setSignature( ndn::security::DUMMY_NDN_SIGNATURE );
     denial->wireEncode();
     return denial;
   }

   void RouterStrategy::onDataHit( nfd::Face& face,
                   const ndn::Interest& interest,
                   const ndn::Data& const_data)
   {
     // unconstify data
     ndn::Data& data = const_cast<ndn::Data&>( const_data );

     if( validateAccess( interest, data ) )
     {
         // this tells the edge router that it has already
         // cached the AuthTag used on this data, and shouldn't
         // recache it, this is to reduce the workload of the
         // edge router, preventing it from making repetitive
         // bloom filter insertions of the same tag
         if( interest.getAuthValidityProb() > 0 )
          data.setNoReCacheFlag( true );
          
       Coordinator::routerSatisfiedRequest( m_instance_id,
                                            interest.getName() );
       m_queue.sendData( face.shared_from_this(), data.shared_from_this() );
       return;
     }

     // bad signature, deny request
     Coordinator::routerDeniedRequest( m_instance_id,
                                       interest.getName(),
                                       "Failed validation" );
     m_queue.sendData( face.shared_from_this(),
                       makeAuthDenial( data ) );
 }

 void
 RouterStrategy::onDataMiss( nfd::Face& face,
                             const ndn::Interest& interest )
 {
   Coordinator::routerForwardedRequest( m_instance_id, interest.getName() );
 }
 
 bool
 RouterStrategy::validateAccess( const ndn::Interest& interest, const ndn::Data& data )
 {
    // extract tag
    const ndn::AuthTag& tag = interest.getAuthTag();
    
    // access level of 0 is public
    if( data.getAccessLevel() == 0 )
        return true;
    
    // ensure valid access level
    if( data.getAccessLevel() > tag.getAccessLevel() )
        return false;
    
    /**
    * We can move the expiration and prefix check
    * to the edge router, TODO
    **/
    // make sure the tag isn't expired
    if( tag.isExpired() )
        return false;
    
    // ensure matching prefix
    if( !tag.getPrefix().isPrefixOf( data.getName() ) )
        return false;
    
     // tags with non matching key locators are denied
     if( !data.getSignature().hasKeyLocator() )
         return false;
     if( tag.getKeyLocator() != data.getSignature().getKeyLocator() )
        return false;
     
     // only verify signature with a probability equivalent to
     // the opposite probability of the interest's AuthValidityProbability
     if( (uint32_t)rand() < interest.getAuthValidityProb() )
        return true;

     // AuthValidityProbability of 0 indicates that the edge router's bloom
     // filters didn't have any information on the tag so we lookup
     // the tag in our bloom if prob is 0
     if( interest.getAuthValidityProb() == 0 )
     {
       
       // simulate bloom lookup with delay
       m_queue.delay( s_router_bloom_delay );
       if( m_auth_cache.contains( tag ) )
         return true;
     }
     
     // verify signature, we simulate actual verification delay by
     // adding delay to the transmit queue, signature is valid if it
     // isn't doesn't equal DUMMY_BAD_SIGNATURE, which has 0 as its
     // first byte
     m_queue.delay( s_router_signature_delay );
     if( tag.getSignature().getValue().value_size() > 0
         && tag.getSignature().getValue().value()[0] != 0 )
     {
            // insert the tag into this router's auth cache
            m_auth_cache.insert( tag );
            
            return true;
     }
    
    // if any of the checks fail then return false
    return false;

 }
}

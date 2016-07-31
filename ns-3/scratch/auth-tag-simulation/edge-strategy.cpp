#include "edge-strategy.hpp"
#include "ndn-cxx/encoding/block-helpers.hpp"
#include "coordinator.hpp"

namespace ndntac
{

  const ndn::Name
  EdgeStrategy::STRATEGY_NAME = "ndn:/localhost/nfd/strategy/ndntac-edge-strategy";
  const ns3::Time
  EdgeStrategy::s_edge_signature_delay = ns3::MilliSeconds( 121.641253 );
  const ns3::Time
  EdgeStrategy::s_edge_bloom_delay = ns3::MilliSeconds( 9.238432 );
  const ns3::Time
  EdgeStrategy::s_edge_interest_delay = ns3::MilliSeconds( 100 );
  uint32_t EdgeStrategy::s_instance_id = 0;

  EdgeStrategy::EdgeStrategy( nfd::Forwarder& forwarder,
                              const ndn::Name& name )
                                : RouterStrategy( forwarder, name )
                                , m_positive_cache( 1e-10, 10000 )
                                , m_negative_cache( 1e-10, 10000 )
  {
    m_instance_id = s_instance_id++;
  }

  bool
  EdgeStrategy::onIncomingInterest( nfd::Face& face,
                                    const ndn::Interest& const_interest )
  {
    // TODO: might need to add some interest freshness checks here

    ndn::Interest interest( const_interest );
    const ndn::AuthTag tag = interest.getAuthTag();

    // if interest is an AuthRequest then set its route hash
    if( interest.getPayload().type() == ndn::tlv::ContentType_AuthRequest )
    {
      interest
      .setPayload( ndn::Block( ndn::tlv::ContentType_AuthRequest,
                   ndn::encoding
                   ::makeNonNegativeIntegerBlock( ndn::tlv::RouteHash,
                                                 interest.getRouteHash() ) ) );
    }

    // if access level is 0 we don't need to do anything
    if( tag.getAccessLevel() == 0 )
    {
      return RouterStrategy::onIncomingInterest( face, interest );
    }

    // ensure valid route hash
    if( tag.getRouteHash() != interest.getRouteHash() )
    {
      Coordinator::edgeDroppingRequest( m_instance_id,
                                        interest.getName(),
                                        "Invalid route hash");
      auto data = make_shared< ndn::Data >( interest.getName() );
      data->setContentType( ndn::tlv::ContentType_Nack );
      data->setFreshnessPeriod( ndn::time::seconds( 0 ) );
      m_queue.sendData( face.shared_from_this(), data );
      return true;
    }

    // if tag is in positive cache then set validity prob accordingly
    if( m_positive_cache.contains( tag ) )
    {
      uint32_t prob = ( 1.0 - m_positive_cache.getEffectiveFPP() ) * 0xFFFFFFFF;

      Coordinator::edgeSettingValidityProbability(m_instance_id,
                                                  interest.getName(),
                                                  prob,
                                                  "AuthTag in positive cache");
      interest.setAuthValidityProb( prob );
    }
    // if not in positive but negative then verify tag ourselves
    else if( m_negative_cache.contains( tag ) )
    {
      // simulate verification time with a delay
      m_queue.delay( s_edge_signature_delay );

      // non-negative signature value indicates valid signature for simulation
      if( interest.getSignature().getValue().value_size() == 0 )
      {
        Coordinator::edgeDroppingRequest( m_instance_id,
                                          interest.getName(),
                                          "Negative cache, bad signature");
        auto data = make_shared< ndn::Data >( interest.getName() );
        data->setContentType( ndn::tlv::ContentType_Nack );
        data->setFreshnessPeriod( ndn::time::seconds( 0 ) );
        m_queue.sendData( face.shared_from_this(), data );
        return true;
      }

      // set validity prob to max
      Coordinator::edgeSettingValidityProbability(m_instance_id,
                                          interest.getName(),
                                          0xFFFFFFFF,
                                          "Negative cache, verified manually");
      interest.setAuthValidityProb( 0xFFFFFFFF );
    }
    // if not in either cache
    else
    {
      Coordinator::edgeSettingValidityProbability(m_instance_id,
                                                  interest.getName(),
                                                  0,
                                                  "Neither cache" );
      interest.setAuthValidityProb( 0 );
    }

    // do normal router stuff
    return RouterStrategy::onIncomingInterest( face, interest );
  }

  void
  EdgeStrategy::beforeSatisfyInterest( shared_ptr<nfd::pit::Entry> pitEntry,
                                       const nfd::Face& inFace, const ndn::Data& data)
  {
    const ndn::AuthTag& tag = pitEntry->getInterest().getAuthTag();

    if( tag.getAccessLevel() > 0 )
    {
      // if tag access > 0 then cache the tag if data is not denial
      if( data.getContentType() != ndn::tlv::ContentType_AuthDenial )
      {
        if( !data.getNoReCacheFlag() )
        {
          Coordinator::edgeCachingTag( m_instance_id,
                                       data.getName(),
                                       "positive");
          m_positive_cache.insert( tag );
        }
      }
      // if auth denial then send nack
      else
      {
        Coordinator::edgeCachingTag( m_instance_id,
                                     data.getName(),
                                     "negative");
        m_negative_cache.insert( tag );
        auto data = make_shared< ndn::Data >( pitEntry->getInterest().getName() );
        data->setContentType( ndn::tlv::ContentType_Nack );
        data->setFreshnessPeriod( ndn::time::seconds( 0 ) );

        auto in_records = pitEntry->getInRecords();
        for( auto it = in_records.begin() ; it != in_records.end() ; it++ )
        {
          m_queue.sendData( it->getFace(), data );
        }

        m_forwarder.getPit().erase( pitEntry );
      }
    }

  }

}

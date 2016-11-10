#include "edge-strategy.hpp"
#include "ndn-cxx/encoding/block-helpers.hpp"
#include "ns3/ndnSIM/utils/dummy-keychain.hpp"
#include "coordinator.hpp"
#include "ndn-cxx/encoding/block-helpers.hpp"

namespace ndntac
{

  const ndn::Name
  EdgeStrategy::STRATEGY_NAME = "ndn:/localhost/nfd/strategy/ndntac-edge-strategy";
  const ns3::Time
  EdgeStrategy::s_edge_signature_delay = ns3::NanoSeconds( 30345 );
  const ns3::Time
  EdgeStrategy::s_edge_bloom_delay = ns3::NanoSeconds( 2535 );
  
  
  EdgeStrategy::EdgeStrategy( nfd::Forwarder& forwarder,
                              const ndn::Name& name )
    : RouterStrategy( forwarder, name )
    , m_positive_cache( 1e-10, 10000 )
    , m_negative_cache( 1e-10, 10000 )
  { }

bool
EdgeStrategy::filterOutgoingData
( const nfd::Face& face,
  const ndn::Interest& interest,
  ndn::Data& data,
  ns3::Time& delay )
{
    // data should never enter the internet
    // from a consumer network
    BOOST_ASSERT( data.getCurrentNetwork() != ndn::RouteTracker::ENTRY_NETWORK );
        
    // since this is an edge router, we change the
    // data's route tracker location accordingly
    // depending on whether the data is entering
    // or leaving the network
    switch( data.getCurrentNetwork() )
    {
        // data is moving from producer network into
        // an internet network
        case ndn::RouteTracker::EXIT_NETWORK:
            data.setCurrentNetwork( ndn::RouteTracker::INTERNET_NETWORK );
            break;
        // data moving from an internet network to a consumer network
        case ndn::RouteTracker::INTERNET_NETWORK:
            data.setCurrentNetwork( ndn::RouteTracker::ENTRY_NETWORK );
            break;
        default:
            //NADA
            break;
    }
    
    return RouterStrategy::filterOutgoingData( face, interest,
                                                data, delay );
}

bool
EdgeStrategy::filterOutgoingInterest
( const nfd::Face& face,
  ndn::Interest& interest,
  ns3::Time& delay )
{
    // an interest should never enter a network from
    // the producer network
    BOOST_ASSERT( interest.getCurrentNetwork() != ndn::RouteTracker::EXIT_NETWORK );

    // since this is an edge router, we change the
    // interest's route tracker location accordingly
    // depending on whether the interest is entering
    // or leaving the network
    switch( interest.getCurrentNetwork() )
    {
        // interest is moving from consumer network into
        // an internet network
        case ndn::RouteTracker::ENTRY_NETWORK:
            interest.setCurrentNetwork( ndn::RouteTracker::INTERNET_NETWORK );
            break;
        // interest moving from an internet network to a produer network
        case ndn::RouteTracker::INTERNET_NETWORK:
            interest.setCurrentNetwork( ndn::RouteTracker::EXIT_NETWORK );
            break;
        default:
            // NADA
            break;
    }
    
    // if the interest doesn't have an auth tag, or its auth level
    // is 0 then we don't do any authentication
    if( !interest.hasAuthTag()
      || interest.getAuthTag().getAccessLevel() == 0 )
    {
        return RouterStrategy::filterOutgoingInterest( face,
                                                        interest,
                                                        delay );
    }
    
    const ndn::AuthTag& auth = interest.getAuthTag();

    // if the auth tag provided by the interest is expired
    // then we can drop the interest
    if( auth.isExpired() )
    {
        onInterestDropped( interest, face );
        return false;
    }
    
    // if the interest and auth tag have different prefixes
    // then we drop the interest
    if( !auth.getPrefix().isPrefixOf( interest.getName() ) )
    {
        onInterestDropped( interest, face );
        return false;
    }
    
    // if the interest auth is in the positive auth cache then
    // we set its auth validity probability
    if( m_positive_cache.contains( auth ) )
    {
        double fpp = m_positive_cache.getEffectiveFPP();
        double prob = (double)1.0 / fpp;
        uint64_t iprob = prob*0xFFFFFFFF;
        interest.setAuthValidityProb( iprob );
    }
    else
    {
        // if it's in the negative cache then we validate
        // its signature manually
        if( m_negative_cache.contains( auth ) )
        {
            // we simulate verification delay by incrementing
            // the processing delay
            delay += EdgeStrategy::s_edge_signature_delay;
            if( auth.getSignature().getValue().value_size() > 0
              && auth.getSignature().getValue().value()[0] != 0 )
            {
                // if signature is valid then set auth validity
                // and put auth into positive cache
                interest.setAuthValidityProb( 0xFFFFFFFF );
                m_positive_cache.insert( auth );
            }
            else
            {
                // otherwise we drop the interest
                onInterestDropped( interest, face );
                return false;
            }
        }
        else
        {
            // if the auth isn't in either cache then
            // we set validity probability to 0
            interest.setAuthValidityProb( 0 );
        }
    }
    
    return RouterStrategy::filterOutgoingInterest( face, interest,
                                                   delay );
}

void
EdgeStrategy::toNack( ndn::Data& data, const ndn::Interest& interest )
{
    // ensure that denied data doesn't leave the network
    data.setContent( ndn::Block() );
    data.wireEncode();
    
    // do whatever a normal router would do
    RouterStrategy::toNack( data, interest );
}

void
EdgeStrategy::onDataDenied( const ndn::Data& data, const ndn::Interest& interest )
{
    // if the data is denied then we add the
    // tag used to retrieve it to the negative
    // cache; however we only want to do this
    // when the data is leaving the internet
    // and going into the client network
    if( data.getCurrentNetwork() == ndn::RouteTracker::ENTRY_NETWORK )
        m_negative_cache.insert( interest.getAuthTag() );
    
    // do whatever a normal router would do
    RouterStrategy::onDataDenied( data, interest );
}

void
EdgeStrategy::onDataSatisfied( const ndn::Data& data, const ndn::Interest& interest )
{
    // if the data is satisfied then we add it to
    // the positive cache if it isn't marked with
    // the no recache flag and it's exiting the
    // internet into the consumer network
    if( data.getCurrentNetwork() == ndn::RouteTracker::ENTRY_NETWORK
      && data.getNoReCacheFlag() )
        m_positive_cache.insert( interest.getAuthTag() );
    
    RouterStrategy::onDataSatisfied( data, interest );
}


}

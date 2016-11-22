#include "router-strategy.hpp"
#include "ns3/ndnSIM/utils/dummy-keychain.hpp"
#include "tracers.hpp"

namespace ndntac
{

uint32_t
RouterStrategy::s_instance_id = 0;

const ndn::Name
RouterStrategy::STRATEGY_NAME = "ndn:/localhost/nfd/strategy/ndntac-router-strategy";
string
RouterStrategy::s_config = "config/router_config.jx9";
const ns3::Time
RouterStrategy::s_router_signature_delay = ns3::NanoSeconds( 30345 );
const ns3::Time
RouterStrategy::s_router_bloom_delay = ns3::NanoSeconds( 2535 );
const ns3::Time
RouterStrategy::s_router_interest_delay = ns3::MilliSeconds( 0 );

RouterStrategy::RouterStrategy( nfd::Forwarder& forwarder,
                              const ndn::Name& name )
                                : BestRouteStrategy( forwarder, name )
                                , m_auth_cache( 1e-10, 10000 )
                                , m_forwarder( forwarder )
{
    m_instance_id = s_instance_id++;
}
  
bool
RouterStrategy::filterOutgoingData
( const nfd::Face& face,
  const ndn::Interest& interest,
  ndn::Data& data,
  ns3::Time& delay )
{   

    // here we use the route hash to determine if this
    // data was sent in response to the given interest,
    // if it was then the upstream router already validated
    // or rejected the interest, so we don't need to revalidate.
    // Since the three route hashes are XORs of all edges traversed
    // and the interest RouteTracker should be transfered to the
    // data it retrieves, we can check if the data was retrieved
    // by this interest by comparing all three hashes
    if( data.getExitRoute() == interest.getExitRoute()
      && data.getInternetRoute() == interest.getInternetRoute()
      && data.getEntryRoute() == interest.getEntryRoute() )
    {
       toPreserve( data, interest );
       onDataPreserved( data, interest, delay );
       tracers::router->sent_data( data );
       return true;
    }
    
    // we replace the data's route tracker with
    // that of its requesting interest
    data.setRouteTracker( interest.getRouteTracker() );

    // if the data's access level is 0 (public)
    // then forward without authenticating
    if( data.getAccessLevel() == 0 )
    {
        toSatisfy( data, interest );
        onDataSatisfied( data, interest, delay );
        tracers::router->validation
        ( tracers::ValidationSuccessSkipped );
        tracers::router->sent_data( data );
        return true;
    }
    
    // if interest doesn't have an auth tag then reject
    if( interest.hasAuthTag() == false )
    {
        toNack( data, interest );
        onDataDenied( data, interest, delay, "missing auth tag" );
        tracers::router->validation
        ( tracers::ValidationFailureNoAuth );
        tracers::router->sent_data( data );
        return true;
    }
    
    const ndn::AuthTag& auth =  interest.getAuthTag();
    
    // if data access level is greater than auth level
    // then reject
    if( data.getAccessLevel() > auth.getAccessLevel() )
    {
        toNack( data, interest );
        onDataDenied( data, interest, delay,
                      "insufficient access level" );
        tracers::router->validation
        ( tracers::ValidationFailureLowAuth );
        tracers::router->sent_data( data );
        return true;
    }
    
    // if the auth and data aren't signed by the
    // same entity then reject the data
    if( !data.getSignature().hasKeyLocator()
      || auth.getKeyLocator() != data.getSignature().getKeyLocator() )
    {
        toNack( data, interest );
        onDataDenied( data, interest, delay,
                      "missmatched key locators" );
        tracers::router->validation
        ( tracers::ValidationFailureBadKeyLoc );
        tracers::router->sent_data( data );
        return true;
    }
    
    ///// we verify all other interests ourselves
    
    // if auth validity > 0 then we tell the edge router
    // not to recache
    if( interest.getAuthValidityProb() > 0 )
    {
        data.setNoReCacheFlag( true );
    }

    // with a probability equivalent to the interest's
    // AuthValidityProbability we'll forward the data
    // without verifying the auth signature
    if( (uint32_t)rand() < interest.getAuthValidityProb() )
    {
        toSatisfy( data, interest );
        onDataSatisfied( data, interest, delay );
        tracers::router->validation
        ( tracers::ValidationSuccessValProb );
        tracers::router->sent_data( data );
        return true;
    }
    

    // use the auth cache for optimization
    // this step likely isn't really that usefull
    // we may remove it later
    tracers::router->bloom_lookup( auth, s_router_bloom_delay );
    delay += s_router_bloom_delay;
    if( interest.getAuthValidityProb() > 0
      && m_auth_cache.contains( auth ) )
    {
        toSatisfy( data, interest );
        onDataSatisfied( data, interest, delay );
        tracers::router->validation
        ( tracers::ValidationSuccessBloom );
        tracers::router->sent_data( data );
        return true;
    }
    
    // we simulate signature verification computation
    // overhead by setting the delay to an estimated
    // signature verification delay, since the signature
    // is a dummy; we just consider any signature with
    // the first byte set to 0 to be a bad signature,
    // and all others to be good
    tracers::router->sigverif( auth, s_router_signature_delay );
    delay += RouterStrategy::s_router_signature_delay;
    if( auth.getSignature().getValue().value_size() > 0
      && auth.getSignature().getValue().value()[0] != 0 )
    {
        toSatisfy( data, interest );
        onDataSatisfied( data, interest, delay );
        tracers::router->validation
        ( tracers::ValidationSuccessSig );
        tracers::router->sent_data( data );
        
        
        tracers::router->bloom_insert( auth, s_router_bloom_delay );
        delay += s_router_bloom_delay;
        m_auth_cache.insert( auth );
        return true;
    }
    
    // if any checks fail then reject the interest
    toNack( data, interest );
    onDataDenied( data, interest, delay, "bad signature" );
    tracers::router->validation
    ( tracers::ValidationFailureSig );
    tracers::router->sent_data( data );
    return true;
}

bool
RouterStrategy::filterOutgoingInterest
( const nfd::Face&,
  ndn::Interest& interest,
  ns3::Time& delay )
{
    tracers::router->sent_interest( interest );
    return true;
}

void
RouterStrategy::toNack( ndn::Data& data, const ndn::Interest& interest )
{
    data.setContentType( ndn::tlv::ContentType_Nack );
    data.wireEncode();
}

void
RouterStrategy::toSatisfy( ndn::Data& data, const ndn::Interest& interest )
{
    data.setContentType( ndn::tlv::ContentType_Blob );
    data.wireEncode();
}

void
RouterStrategy::toPreserve( ndn::Data& data, const ndn::Interest& interest )
{
    // NADA
}

void
RouterStrategy::onDataDenied( const ndn::Data& data,
                              const ndn::Interest& interest,
                              ns3::Time& delay,
                              const std::string& why )
{
    // NADA
}

void
RouterStrategy::onDataSatisfied( const ndn::Data& data,
                                 const ndn::Interest& interest,
                                 ns3::Time& delay )
{
    // NADA
}

void
RouterStrategy::onDataPreserved( const ndn::Data& data,
                                 const ndn::Interest& interest,
                                 ns3::Time& delay )
{
    if( data.getContentType() == ndn::tlv::ContentType_Nack )
        onDataDenied( data, interest, delay, "upstream" );
    else
        onDataSatisfied( data, interest, delay );
}

void
RouterStrategy::onInterestDropped( const ndn::Interest& interest,
                                   const nfd::Face& face,
                                   const std::string& why )
{
    // NADA
}

}

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

RouterStrategy::RouterStrategy( nfd::Forwarder& forwarder,
                              const ndn::Name& name )
                                : BestRouteStrategy( forwarder, name )
                                , m_auth_cache( 1e-10, 10000 )
                                , m_forwarder( forwarder )
{
m_instance_id = rand();
}
  
bool
RouterStrategy::filterOutgoingData
( const nfd::Face& face,
  const ndn::Interest& interest,
  ndn::Data& data,
  ns3::Time& delay )
{   
    // if the data's access level is 0 (public)
    // then forward without authenticating
    if( data.getAccessLevel() == 0 )
    {
        toPreserve( data, interest );
        onDataPreserved( data, interest );
        logDataSent( data );
        return true;
    }

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
       onDataPreserved( data, interest );
       logDataSent( data );
       return true;
    }
    
    // if interest doesn't have an auth tag then reject
    if( interest.hasAuthTag() == false )
    {
        toNack( data, interest );
        onDataDenied( data, interest );
        logDataDenied( data, "no auth tag" );
        return true;
    }
    
    const ndn::AuthTag& auth =  interest.getAuthTag();
    
    // if data access level is greater than auth level
    // then reject
    if( data.getAccessLevel() > auth.getAccessLevel() )
    {
        toNack( data, interest );
        onDataDenied( data, interest );
        logDataDenied( data, auth, "insufficient access" );
        return true;
    }
    
    // if the auth and data aren't signed by the
    // same entity then reject the data
    if( !data.getSignature().hasKeyLocator()
      || auth.getKeyLocator() != data.getSignature().getKeyLocator() )
    {
        toNack( data, interest );
        onDataDenied( data, interest );
        logDataDenied( data, auth, "missmatched key locators" );
        return true;
    }
    
    ///// we verify all other interests ourselves
    
    // if auth validity > 0 then we tell the edge router
    // not to recache
    if( interest.getAuthValidityProb() > 0 )
    {
        data.setNoReCacheFlag( true );
        logNoReCacheFlagSet( data, interest );
    }

    // with a probability equivalent to the interest's
    // AuthValidityProbability we'll forward the data
    // without verifying the auth signature
    if( (uint32_t)rand() < interest.getAuthValidityProb() )
    {
        toSatisfy( data, interest );
        onDataSatisfied( data, interest );
        logDataSent( data, auth );
        return true;
    }
    

    // use the auth cache for optimization
    // this step likely isn't really that usefull
    // we may remove it later
    if( interest.getAuthValidityProb() > 0
      && m_auth_cache.contains( auth ) )
    {
        toSatisfy( data, interest );
        onDataSatisfied( data, interest );
        return true;
    }
    
    // we simulate signature verification computation
    // overhead by setting the delay to an estimated
    // signature verification delay, since the signature
    // is a dummy; we just consider any signature with
    // the first byte set to 0 to be a bad signature,
    // and all others to be good
    delay += RouterStrategy::s_router_signature_delay;
    if( auth.getSignature().getValue().value_size() > 0
      && auth.getSignature().getValue().value()[0] != 0 )
    {
        toSatisfy( data, interest );
        onDataSatisfied( data, interest );
        return true;
    }
    
    // if any checks fail then reject the interest
    toNack( data, interest );
    onDataDenied( data, interest );
    return true;
}

bool
RouterStrategy::filterOutgoingInterest
( const nfd::Face&,
  ndn::Interest& interest,
  ns3::Time& delay )
{
    logInterestSent( interest );
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
                              const ndn::Interest& interest )
{
    // NADA
}

void
RouterStrategy::onDataSatisfied( const ndn::Data& data,
                                 const ndn::Interest& interest )
{
    // NADA
}

void
RouterStrategy::onDataPreserved( const ndn::Data& data,
                                 const ndn::Interest& interest )
{
    if( data.getContentType() == ndn::tlv::ContentType_Nack )
        onDataDenied( data, interest );
    else
        onDataSatisfied( data, interest );
}

void
RouterStrategy::onInterestDropped( const ndn::Interest& interest,
                                   const nfd::Face& face )
{
    // NADA
}

  void
  RouterStrategy::logDataDenied( const ndn::Data& data,
                                const ndn::AuthTag& auth,
                                const std::string& why ) const
  {
    Coordinator::LogEntry entry( "Router", "DataDenied");
    entry.add( "data-name", data.getName().toUri() );
    entry.add( "data-access", std::to_string( (unsigned)data.getAccessLevel() ) );
    entry.add( "auth-prefix", auth.getPrefix().toUri() );
    entry.add( "auth-access", std::to_string( (unsigned)auth.getAccessLevel() ) );
    entry.add( "auth-expired", auth.isExpired() ? "true" : "false" );
    entry.add( "why", why );
    Coordinator::log( entry );
  }
  void
  RouterStrategy::logDataDenied( const ndn::Data& data,
                               const std::string& why ) const
  {
    Coordinator::LogEntry entry( "Router", "DataDenied");
    entry.add( "data-name", data.getName().toUri() );
    entry.add( "data-access", std::to_string( (unsigned)data.getAccessLevel() ) );
    entry.add( "why", why );
    Coordinator::log( entry );
  }
  
  void
  RouterStrategy::logDataSent( const ndn::Data& data,
                              const ndn::AuthTag& auth ) const
  {
    Coordinator::LogEntry entry( "Router", "DataSent");
    entry.add( "data-name", data.getName().toUri() );
    entry.add( "data-access", std::to_string( (unsigned)data.getAccessLevel() ) );
    entry.add( "auth-prefix", auth.getPrefix().toUri() );
    entry.add( "auth-access", std::to_string( (unsigned)auth.getAccessLevel() ) );
    entry.add( "auth-expired", auth.isExpired() ? "true" : "false" );
    Coordinator::log( entry );
  }
  void
  RouterStrategy::logDataSent( const ndn::Data& data) const
  {
    Coordinator::LogEntry entry( "Router", "DataSent");
    entry.add( "data-name", data.getName().toUri() );
    entry.add( "data-access", std::to_string( (unsigned)data.getAccessLevel() ) );
    Coordinator::log( entry );
  }
  
  void
  RouterStrategy::logNoReCacheFlagSet( const ndn::Data& data,
                                      const ndn::Interest& interest ) const
  {
    Coordinator::LogEntry entry( "Router", "NoReCacheFlagSet");
    entry.add( "data-name", data.getName().toUri() );
    entry.add( "interest-val-prob", std::to_string( interest.getAuthValidityProb() ) );
    Coordinator::log( entry );
  }

  void
  RouterStrategy::logReceivedRequest( const ndn::Interest& interest ) const
  {
    Coordinator::LogEntry entry( "Router", "ReceivedRequest");
    const ndn::AuthTag& auth = interest.getAuthTag();
    entry.add( "interest-name", interest.getName().toUri() );
    entry.add( "auth-prefix", auth.getPrefix().toUri() );
    entry.add( "auth-access", std::to_string( (unsigned)auth.getAccessLevel() ) );
    entry.add( "auth-expired", auth.isExpired() ? "true" : "false" );
    Coordinator::log( entry );
  }
  
  void
  RouterStrategy::logInterestSent( const ndn::Interest& interest ) const
  {
    Coordinator::LogEntry entry( "Router", "SentInterest");
    entry.add( "interest-name", interest.getName().toUri() );
    Coordinator::log( entry );
  }

}

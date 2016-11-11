#include "router-strategy.hpp"
#include "logger.hpp"
#include "log.hpp"
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
       return true;
    }
    
    // if interest doesn't have an auth tag then reject
    if( interest.hasAuthTag() == false )
    {
        toNack( data, interest );
        onDataDenied( data, interest, "missing auth tag" );
        return true;
    }
    
    const ndn::AuthTag& auth =  interest.getAuthTag();
    
    // if data access level is greater than auth level
    // then reject
    if( data.getAccessLevel() > auth.getAccessLevel() )
    {
        toNack( data, interest );
        onDataDenied( data, interest, "insufficient access level" );
        return true;
    }
    
    // if the auth and data aren't signed by the
    // same entity then reject the data
    if( !data.getSignature().hasKeyLocator()
      || auth.getKeyLocator() != data.getSignature().getKeyLocator() )
    {
        toNack( data, interest );
        onDataDenied( data, interest, "missmatched key locators" );
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
    onDataDenied( data, interest, "bad signature" );
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
                              const ndn::Interest& interest,
                              const std::string& why )
{
    if( interest.hasAuthTag() )
        logDataDenied( data, interest.getAuthTag(),  why );
    else
        logDataDenied( data,  why );
}

void
RouterStrategy::onDataSatisfied( const ndn::Data& data,
                                 const ndn::Interest& interest )
{
    if( interest.hasAuthTag() )
        logDataSent( data, interest.getAuthTag() );
    else
        logDataSent( data  );
}

void
RouterStrategy::onDataPreserved( const ndn::Data& data,
                                 const ndn::Interest& interest )
{
    if( data.getContentType() == ndn::tlv::ContentType_Nack )
        onDataDenied( data, interest, "upstream" );
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
  static Log* log = Logger::getInstance( "simulation.log.udb" ).makeLog(
                            "Router",
                            "{ 'data-name'     : $data_name, "
                            "  'data-access'   : $data_access, "
                            "  'auth-prefix'   : $auth_prefix, "
                            "  'auth-access'   : $auth_access, "
                            "  'auth-expired'  : $auth_expired, "
                            "  'what'          : $what,"
                            "  'why'           : $why }" );
  log->set( "data_name", data.getName().toUri() );
  log->set( "data_access", (uint64_t)data.getAccessLevel() );
  log->set( "auth_prefix", auth.getPrefix().toUri() );
  log->set( "auth_access", (uint64_t)auth.getAccessLevel() );
  log->set( "auth_expired", auth.isExpired() );
  log->set( "what", string("DataDenied") );
  log->set( "why", why );
  log->write();
}

void
RouterStrategy::logDataDenied( const ndn::Data& data,
                             const std::string& why ) const
{
  static Log* log = Logger::getInstance( "simulation.log.udb" ).makeLog(
                            "Router",
                            "{ 'data-name'     : $data_name, "
                            "  'data-access'   : $data_access, "
                            "  'what'          : $what, "
                            "  'why'           : $why }" );
  log->set( "data_name", data.getName().toUri() );
  log->set( "data_access", (uint64_t)data.getAccessLevel() );
  log->set( "what", string("DataDenied") );
  log->set( "why", why );
  log->write();
}

void
RouterStrategy::logDataSent( const ndn::Data& data,
                            const ndn::AuthTag& auth ) const
{
  static Log* log = Logger::getInstance( "simulation.log.udb" ).makeLog(
                            "Router",
                            "{ 'data-name'     : $data_name, "
                            "  'data-access'   : $data_access, "
                            "  'auth-prefix'   : $auth_prefix, "
                            "  'auth-access'   : $auth_access, "
                            "  'auth-expired'  : $auth_expired, "
                            "  'what'          : $what }" );
  log->set( "data_name", data.getName().toUri() );
  log->set( "data_access", (uint64_t)data.getAccessLevel() );
  log->set( "auth_prefix", auth.getPrefix().toUri() );
  log->set( "auth_access", (uint64_t)auth.getAccessLevel() );
  log->set( "auth_expired", auth.isExpired() );
  log->set( "what", string("DataSent") );
  log->write();
}
void
RouterStrategy::logDataSent( const ndn::Data& data) const
{
  static Log* log = Logger::getInstance( "simulation.log.udb" ).makeLog(
                            "Router",
                            "{ 'data-name'     : $data_name, "
                            "  'data-access'   : $data_access, "
                            "  'what'          : $what }" );
  log->set( "data_name", data.getName().toUri() );
  log->set( "data_access", (uint64_t)data.getAccessLevel() );
  log->set( "what", "DataSent" );
  log->write();
}

void
RouterStrategy::logNoReCacheFlagSet( const ndn::Data& data,
                                    const ndn::Interest& interest ) const
{
  static Log* log = Logger::getInstance( "simulation.log.udb" ).makeLog(
                            "Router",
                            "{ 'data-name'        : $data_name, "
                            "  'interest-vprob'   : $vprob, "
                            "  'what'             : $what }" );
  log->set( "data_name", data.getName().toUri() );
  log->set( "vprob", (uint64_t)interest.getAuthValidityProb() );
  log->set( "what", string("NoReCacheFlagSet") );
  log->write();
}

void
RouterStrategy::logReceivedRequest( const ndn::Interest& interest ) const
{
  static Log* log = Logger::getInstance( "simulation.log.udb" ).makeLog(
                            "Router",
                            "{ 'interest-name' : $interest_name, "
                            "  'auth-prefix'   : $auth_prefix, "
                            "  'auth-access'   : $auth_access, "
                            "  'auth-expired'  : $auth_expired, "
                            "  'what'          : $what  }" );
  const ndn::AuthTag& auth = interest.getAuthTag();
  log->set( "interest_name", interest.getName().toUri() );
  log->set( "auth_prefix", auth.getPrefix().toUri() );
  log->set( "auth_access", (uint64_t)auth.getAccessLevel() );
  log->set( "auth_expired", auth.isExpired() );
  log->set( "what", string("ReceivedRequest") );
  log->write();
  
}

void
RouterStrategy::logInterestSent( const ndn::Interest& interest ) const
{
  static Log* log = Logger::getInstance( "simulation.log.udb" ).makeLog(
                            "Router",
                            "{ 'interest-name' : $interest_name, "
                            "  'what'          : $what }" );
  log->set( "interest_name", interest.getName().toUri() );
  log->set( "what", string("SentInterest") );
  log->write();
}

}

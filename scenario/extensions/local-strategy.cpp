#include "local-strategy.hpp"
#include "coordinator.hpp"
#include "ns3/ndnSIM/utils/dummy-keychain.hpp"

namespace ndntac
{

  const ndn::Name
  LocalStrategy::STRATEGY_NAME = "ndn:/localhost/nfd/strategy/ndntac-local-strategy";

  LocalStrategy::LocalStrategy( nfd::Forwarder& forwarder,
                                const ndn::Name& name )
                                    : RouterStrategy( forwarder, name )
  { };

  bool
  LocalStrategy::onIncomingInterest( nfd::Face& face,
                                     const ndn::Interest& interest )
  {
    const_cast<ndn::Interest&>( interest ).updateRouteHash( m_instance_id );
    return RouterStrategy::onIncomingInterest( face, interest );
  }

  bool
  LocalStrategy::onIncomingData( nfd::Face& face,
                                  const ndn::Data& const_data )
  {
    ndn::Data& data = const_cast<ndn::Data&>(const_data);
    data.updateRouteHash( m_instance_id );
    
    // data detects change in route hash so needs to be
    // re-encoded
    data.wireEncode();
    return RouterStrategy::onIncomingData( face, data );
  }
}

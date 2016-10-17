/**
* @class ndntac::RouterStrategy
* @brief Router strategy with AuthTag verification
*
* A forwarding strategy that implements tag access control
*
* @author Ray Stubbs [stubbs.ray@gmail.com]
**/
#include "ns3/core-module.h"
#include "ns3/ndnSIM/apps/ndn-app.hpp"
#include "ndn-cxx/name.hpp"
#include "ndn-cxx/interest.hpp"
#include "ndn-cxx/data.hpp"
#include "ndn-cxx/auth-tag.hpp"
#include "ndn-cxx/encoding/tlv.hpp"
#include "ns3/ndnSIM/NFD/daemon/fw/best-route-strategy.hpp"
#include "ns3/ndnSIM/NFD/daemon/face/face.hpp"
#include "tx-queue.hpp"
#include "auth-cache.hpp"
#include "router-strategy.hpp"


#ifndef LOCAL_STRATEGY__INCLUDED
#define LOCAL_STRATEGY__INCLUDED

namespace ndntac
{
  class LocalStrategy : public RouterStrategy
  {
    public:

      LocalStrategy( nfd::Forwarder& forwarder,
                     const ndn::Name& name = STRATEGY_NAME );

      bool
      onIncomingInterest( nfd::Face& face,
                          const ndn::Interest& interest ) override;
      
      bool
      onIncomingData( nfd::Face& face,
                      const ndn::Data& Data ) override;
    public:
       static const ndn::Name STRATEGY_NAME;
    };

};

#endif // LOCAL_STRATEGY__INCLUDED

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


#ifndef ROUTER_STRATEGY__INCLUDED
#define ROUTER_STRATEGY__INCLUDED

namespace ndntac
{
  class RouterStrategy : public nfd::fw::BestRouteStrategy
  {
    public:

      RouterStrategy( nfd::Forwarder& forwarder,
                      const ndn::Name& name = STRATEGY_NAME );

      bool
      onIncomingInterest( nfd::Face& face,
                          const ndn::Interest& interest ) override;

      void
      beforeSatisfyInterest( shared_ptr<nfd::pit::Entry> pitEntry,
                             const nfd::Face& inFace, const ndn::Data& data)
                             override;
    private:

       shared_ptr< ndn::Data >
       makeAuthDenial( const ndn::Data& data );

       void onDataHit( nfd::Face& face,
                       const ndn::Interest& interest,
                       const ndn::Data& data);
       void onDataMiss( nfd::Face& face,
                        const ndn::Interest& interest );

    public:
       static const ndn::Name STRATEGY_NAME;
    protected:
            TxQueue m_queue;
            AuthCache m_auth_cache;
            nfd::Forwarder& m_forwarder;

    protected:
            static uint32_t s_instance_id_offset;
            uint32_t m_instance_id;
    private:
            static const ns3::Time s_router_signature_delay;
            static const ns3::Time s_router_bloom_delay;
            static const ns3::Time s_router_interest_delay;
    };

};

#endif // ROUTER_STRATEGY__INCLUDED

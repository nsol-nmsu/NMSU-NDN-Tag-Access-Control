/**
* @class ndntac::RouterStrategy
* @brief Producer application with AuthTag authentication.
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


#ifndef EDGE_STRATEGY__INCLUDED
#define EDGE_STRATEGY__INCLUDED

namespace ndntac
{
  class EdgeStrategy : public RouterStrategy
  {
    public:

      EdgeStrategy( nfd::Forwarder& forwarder,
                      const ndn::Name& name = STRATEGY_NAME );

      bool
      onIncomingInterest( nfd::Face& face, const ndn::Interest& interest )
      override;

      void
      beforeSatisfyInterest( shared_ptr<nfd::pit::Entry> pitEntry,
                             const nfd::Face& inFace, const ndn::Data& data)
                             override;

    public:
       static const ndn::Name STRATEGY_NAME;
    private:
            AuthCache  m_positive_cache;
            AuthCache  m_negative_cache;
            uint32_t m_instance_id;

            static uint32_t s_instance_id;
            static const ns3::Time s_edge_signature_delay;
            static const ns3::Time s_edge_bloom_delay;
            static const ns3::Time s_edge_interest_delay;
    };

};

#endif // EDGE_STRATEGY__INCLUDED

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
      filterOutgoingData( const nfd::Face& face,
                          const ndn::Interest& interest,
                          ndn::Data& data,
                          ns3::Time& delay ) override;
      
      bool
      filterOutgoingInterest( const nfd::Face&,
                              ndn::Interest& interest,
                              ns3::Time& delay ) override;
    protected:
      virtual void
      onDataDenied( const ndn::Data& data,
                    const ndn::Interest& interest,
                    ns3::Time& delay,
                    const std::string& why );
      virtual void
      onDataSatisfied( const ndn::Data& data,
                       const ndn::Interest& interest,
                       ns3::Time& delay );
      
      virtual void
      onDataPreserved( const ndn::Data& data,
                       const ndn::Interest& interest,
                       ns3::Time& delay );
      
      virtual void
      onInterestDropped( const ndn::Interest& interest,
                         const nfd::Face&,
                         const std::string& why );
      
      virtual void
      toNack( ndn::Data& data, const ndn::Interest& interest );
      
      virtual void
      toSatisfy( ndn::Data& data, const ndn::Interest& interest );
      
      virtual void
      toPreserve( ndn::Data& data, const ndn::Interest& interest );

    public:
       static std::string s_config;
       static const ndn::Name STRATEGY_NAME;
    protected:
            TxQueue m_queue;
            AuthCache m_auth_cache;
            nfd::Forwarder& m_forwarder;

    protected:
            uint32_t m_instance_id;
            static uint32_t s_instance_id;
    private:
            static const ns3::Time s_router_signature_delay;
            static const ns3::Time s_router_bloom_delay;
            static const ns3::Time s_router_interest_delay;
    };

};

#endif // ROUTER_STRATEGY__INCLUDED

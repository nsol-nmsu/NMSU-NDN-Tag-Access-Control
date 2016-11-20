/**
* @class ndntac::EdgeStrategy
* @brief Strategy for edge router, acts as first line of defense
*        for the network
*
* A forwarding strategy that implements tag access control
* and special security functions specific to the edge router
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
      filterOutgoingData( const nfd::Face& face,
                          const ndn::Interest& interest,
                          ndn::Data& data,
                          ns3::Time& delay ) override;
      
      bool
      filterOutgoingInterest( const nfd::Face&,
                              ndn::Interest& interest,
                              ns3::Time& delay ) override;
    protected:
    
      void
      onDataDenied( const ndn::Data& data,
                    const ndn::Interest& interest,
                    const std::string& why ) override;
      void
      onDataSatisfied( const ndn::Data& data,
                       const ndn::Interest& interest ) override;
      
      void
      toNack( ndn::Data& data, const ndn::Interest& interest ) override;

    protected: // logging
        
      virtual void
      logAuthValidityProbSet( const ndn::Interest& interest,
                              uint32_t prob,
                              const std::string& why ) const;
      
      virtual void
      logPositiveCacheInsert( const ndn::AuthTag& auth,
                              const std::string& why ) const;
      
      virtual void
      logNegativeCacheInsert( const ndn::AuthTag& auth,
                              const std::string& why ) const;
      
      virtual bool
      shouldLogAuthValidityProbSet( void ) const;
      
      virtual bool
      shouldLogPositiveCacheInsert( void ) const;
      
      virtual bool
      shouldLogNegativeCacheInsert( void ) const; 
      
      

    public:
       static std::string s_config;
       static const ndn::Name STRATEGY_NAME;
    private:
            AuthCache   m_positive_cache;
            AuthCache   m_negative_cache;

            static const ns3::Time s_edge_signature_delay;
            static const ns3::Time s_edge_bloom_delay;
    };

};

#endif // EDGE_STRATEGY__INCLUDED

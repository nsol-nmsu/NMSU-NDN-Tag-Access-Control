/**
* @class ndntac::Producer
* @brief Producer application with AuthTag authentication.
*
* This application simulates a data producer.  The producer
* takes one required attribute 'Directory' which is a directory
* specifying a producer directory to to simulate.  The app will
* make data of all files under the directory, which will be
* named [Directory]/[Fle-Path].  The producer also provides AuthTags
* for its content, which are named [Directory]/auth.  An optional
* 'Prefix' attribute is also available, which can be used to replace
* the directory as the producer's prefix.
*
* @author Ray Stubbs [stubbs.ray@gmail.com]
**/
#include "ns3/core-module.h"
#include "ns3/ndnSIM/apps/ndn-app.hpp"
#include "ns3/ndnSIM/NFD/daemon/fw/tx-queue.hpp"
#include "ndn-cxx/name.hpp"
#include "ndn-cxx/interest.hpp"
#include "ndn-cxx/data.hpp"
#include "ndn-cxx/auth-tag.hpp"
#include "ndn-cxx/encoding/tlv.hpp"
#include "auth-cache.hpp"
#include <memory>


#ifndef PRODUCER__INCLUDED
#define PRODUCER__INCLUDED

namespace ndntac
{
  class Producer : public ns3::ndn::App
  {
    public:
      static ns3::TypeId
      GetTypeId();

      Producer();

      void
      OnInterest( std::shared_ptr< const ndn::Interest> interest ) override;

    protected:
      void
      StartApplication() override;
      void
      StopApplication() override;
      
      void
      logDataDenied( const ndn::Data& data,
                     const ndn::AuthTag& auth,
                     const std::string& why ) const;
      void
      logDataDenied( const ndn::Data& data,
                     const std::string& why ) const;
      void
      logDataSent( const ndn::Data& data,
                   const ndn::AuthTag& auth ) const;
      void
      logDataSent( const ndn::Data& data ) const;
      void
      logNoReCacheFlagSet( const ndn::Data& data,
                           const ndn::Interest& interest ) const;

      void
      logSentAuth( const ndn::AuthTag& auth ) const;
      
      void
      logReceivedRequest( const ndn::Interest& interest ) const;
      
      void
      logProducerStart( void ) const;

    private:

     void
     onDataRequest( std::shared_ptr< const ndn::Interest > interest );
     
     void
     onAuthRequest( std::shared_ptr< const ndn::Interest > interest );
     
     void
     toNack( ndn::Data& data );

    private:
            TxQueue m_queue;
            string m_names_string;
            std::map<ndn::Name, std::pair< size_t, uint8_t >> m_names;
            ndn::Name m_prefix;
            uint32_t m_instance_id;

            static uint32_t s_instance_id;
            static const ns3::Time s_producer_signature_delay;
            static const ns3::Time s_producer_bloom_delay;
            static const ns3::Time s_producer_interest_delay;
    public:
            static const size_t s_segment_size;
    };

};

#endif // PRODUCER__INCLUDED

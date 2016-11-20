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
#include "unqlite.hpp"
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
          OnInterest
         ( std::shared_ptr< const ndn::Interest> interest ) override;

    protected:
          void
          StartApplication() override;
          void
          StopApplication() override;

    private:

         virtual void
         onDataRequest
         ( std::shared_ptr< const ndn::Interest > interest );
         
         virtual void
         onAuthRequest
         ( std::shared_ptr< const ndn::Interest > interest );
         
         virtual void
         toNack( ndn::Data& data );

    private:

        TxQueue m_tx_queue;
        uint32_t m_instance_id;
        static uint32_t s_instance_id;
        
        struct Config
        {
            // load config from file
            Config( const std::string& file, uint32_t id );
            
            // producer's content
            struct Content
            { size_t size;
              uint8_t access_level; };
            std::map
            < ndn::Name, Content > contents;
            
            // producer prefix
            ndn::Name prefix;
            
            // delay for each signature verification
            ns3::Time sigverif_delay;
            
            // delay for each bloom loockup
            ns3::Time bloom_delay;
            
            // base delay for processing each request
            // without signature or bloom loockup overhead
            ns3::Time request_delay;
        };
        Config  m_config;
        
    public:
            static const size_t s_segment_size;
            static std::string s_config;
    };

};

#endif // PRODUCER__INCLUDED

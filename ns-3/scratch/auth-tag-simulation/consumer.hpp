/**
* @class ndntac::Consumer
* @brief Consumer application with AuthTag authentication.
*
* This application simulates a data consumer.  The consumer starts out
* with a list of available producers, this is a colon seperated list set
* an the "KnownProducers" attribute.  At startup, and whenever the interest
* queue is empty the consumer will select a random producer from the list of
* known producers and request its '[producer-prefix]/index.html' file if it
* has the producer's AuthTag, if not then it will request the producer's
* AuthTag '[producer-name]/AUTH_TAG.'  Upon receipt of Data of type
* ContentType_Blob the consumer will follow any links ( <a>...</a> ) in the
* data by requesting the corresponding content.  Upon receipt of an AuthTag
* the consumer will add it to its tag collection.  The consumer makes
* requests at random intervals between "IntervalMin" and "IntervalMax"
* attributes.
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
#include "file-data-producer.hpp"
#include "auth-data-producer.hpp"
#include "tx-queue.hpp"


#ifndef CONSUMER__INCLUDED
#define CONSUMER__INCLUDED

namespace ndntac
{
  class Consumer : public ns3::ndn::App
  {
    public:
      static ns3::TypeId
      GetTypeId();

      Consumer();


    protected:
      void
      StartApplication() override;
      void
      StopApplication() override;

      void
      OnData( shared_ptr< const ndn::Data > data ) override;

    private:

      void
      sendNext();

      bool
      hasAuth( const ndn::Name& name );

      void
      requestAuth( const ndn::Name& name );

      void
      requestData( const ndn::Name& name );

      void
      parseKnownProducers( const string& plist );

    private:
            TxQueue m_queue;
            std::queue< shared_ptr< ndn::Interest > > m_interest_queue;
            std::vector< ndn::Name >                  m_known_producers;
            ns3::Time m_min_interval;
            ns3::Time m_max_interval;
            string m_known_producers_list;
            std::map< ndn::Name, ndn::AuthTag > m_auth_tags;
            uint32_t m_instance_id;

            static uint32_t s_instance_id;
            static const ns3::Time s_consumer_signature_delay;
            static const ns3::Time s_consumer_bloom_delay;
            static const ns3::Time s_consumer_interest_delay;
    };

    NS_OBJECT_ENSURE_REGISTERED(Consumer);

};

#endif // CONSUMER__INCLUDED

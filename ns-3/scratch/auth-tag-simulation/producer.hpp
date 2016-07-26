/**
* @class ndntac::Producer
* @brief Producer application with AuthTag authentication.
*
* This application simulates a data producer.  The producer
* takes one required attribute 'Directory' which is a directory
* specifying a producer directory to to simulate.  The app will
* make data of all files under the directory, which will be
* named [Directory]/[Fle-Path].  The producer also provides AuthTags
* for its content, which are named [Directory]/auth.
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
      OnInterest( shared_ptr< const ndn::Interest> interest ) override;

      // need access to incoming face of interest, if we .sendData() to
      // m_face it will just be sent back to us since m_face is our own face
      void
      onIncomingInterest( shared_ptr< const ndn::Interest > interest,
                          ns3::Ptr< ns3::ndn::App > app,
                          shared_ptr< nfd::Face > face);

    protected:
      void
      StartApplication() override;
      void
      StopApplication() override;

    private:
       void
       makeProducers( const string path,
                std::map< ndn::Name, shared_ptr< DataProducer > > container );

       shared_ptr< ndn::Data >
       makeAuthDenial( const ndn::Data& data );

    private:
            TxQueue m_queue;
            string m_dir;
            std::map< ndn::Name, shared_ptr< DataProducer > > m_producers;
            uint32_t m_instance_id;

            static uint32_t s_instance_id;
            static const ns3::Time s_producer_signature_delay;
            static const ns3::Time s_producer_bloom_delay;
            static const ns3::Time s_producer_interest_delay;
    };

    NS_OBJECT_ENSURE_REGISTERED(Producer);

};

#endif // PRODUCER__INCLUDED

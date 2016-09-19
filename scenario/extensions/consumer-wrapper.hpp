/*
* A wrapper for any consumer extending ndntac::ZipfConsumer
*/

#ifndef CONSUMER_WRAPPER_H
#define CONSUMER_WRAPPER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/names.h"
#include "ns3/ndnSIM/apps/ndn-consumer-cbr.hpp"
#include "ndn-cxx/interest.hpp"
#include "ndn-cxx/data.hpp"
#include "ndn-cxx/name.hpp"
#include "ndn-cxx/auth-tag.hpp"
#include "zipf-consumer.hpp"
#include "window-consumer.hpp"

namespace ndntac
{
    // template should only be used on subclasses of ndntac::ZipfConsumer
    template< typename Consumer >
    class ConsumerWrapper : public Consumer
    {
    public:
        static ns3::TypeId
        GetTypeId();
      
        ConsumerWrapper();
        
        virtual void
        SendPacket();
        
    protected:
    
        void
        OnData( std::shared_ptr< const ndn::Data > data ) override;
        
        void
        StartApplication() override;
        
       void
       WillSendOutInterest( std::shared_ptr< ndn::Interest> interest ) override;
       
       void
       Reset() override;
    
    private:
          bool
          EnsureAuth();
          
          void
          SendAuthRequest();
    private:
        std::shared_ptr<ndn::AuthTag> m_auth_tag = NULL;
        ns3::EventId             m_auth_event;
        bool                     m_pending_auth = false;
        ns3::Time                m_auth_timeout;
        
        uint32_t                 m_instance_id;
        static uint32_t          s_instance_id;
          
    };
    
    typedef ConsumerWrapper< WindowConsumer > AuthWindowConsumer;
    
};

#endif


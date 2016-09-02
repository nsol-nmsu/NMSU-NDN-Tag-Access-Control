/*
* A wrapper for ndnSIM consumers, adds AuthTag support.
*/

#ifndef CONSUMER_WRAPPER_H
#define CONSUMER_WRAPPER_H

namespace ndntac
{
    // template should only be used on subclasses of ns3::ndn::Consumer
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
        OnData( shared_ptr< const ndn::Data > data ) override;
        
        void
        StartApplication() override;
    
    private:
          bool
          EnsureAuth();
          
          void
          SendAuthRequest();
    private:
        shared_ptr<ndn::AuthTag> m_auth_tag = NULL;
        ns3::EventId             m_auth_event;
        bool                     m_pending_auth = false;
        ns3::Time                m_auth_timeout;
        
        uint32_t                 m_instance_id;
        static uint32_t          s_instance_id;
          
    };
};

#endif


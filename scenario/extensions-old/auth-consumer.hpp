#ifndef NDNTAC_AUTH_CONSUMER_H
#define NDNTAC_AUTH_CONSUMER_H

#include "window-consumer.hpp"

namespace ndntac {


class AuthConsumer : public WindowConsumer
{
public:
  static ns3::TypeId
  GetTypeId();

  virtual bool
  EnsureAuth( void );
  
  void
  SendAuthRequest( void );
  
  void
  SendPacket( void ) override;
  
  virtual void
  OnData( std::shared_ptr< const ndn::Data > data ) override;
 
  void
  WillSendOutInterest( std::shared_ptr< ndn::Interest > interest ) override;
  
  void
  Reset( void ) override;
  
protected: // logging
    void
    logRequestedAuth( const ndn::Interest& interest ) const;
    
    void
    logReceivedAuth( const ndn::AuthTag& auth ) const;
    
    bool
    shouldLogRequestedAuth( void ) const;
    
    bool
    shouldLogReceivedAuth( void ) const;
public:
    static std::string s_config;
private:
    std::shared_ptr<ndn::AuthTag> m_auth_tag = NULL;
    ns3::EventId             m_auth_event;
    bool                     m_pending_auth = false;
    ns3::Time                m_auth_timeout;
    Config                   m_config;
};

} // namespace ndntac

#endif

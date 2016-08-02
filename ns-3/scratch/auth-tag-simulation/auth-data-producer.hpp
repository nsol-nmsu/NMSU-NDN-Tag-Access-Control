/**
* @class AuthDataProducer
* A DataProducer implementation for producing auth tags
*
* @author Ray Stubbs [stubbs.ray@gmail.com]
**/
#ifndef AUTH_DATA_PRODUCER__INCLUDED
#define AUTH_DATA_PRODUCER__INCLUDED

#include "data-producer.hpp"
#include "ndn-cxx/util/time.hpp"
#include "ndn-cxx/interest.hpp"
#include "ndn-cxx/data.hpp"
#include "ndn-cxx/name.hpp"
#include "ns3/ndnSIM/utils/dummy-keychain.hpp"

namespace ndntac
{
  class AuthDataProducer : public DataProducer
  {
  public:

    AuthDataProducer( const ndn::Name& prefix, uint8_t access_level ):
      m_prefix( prefix ), m_access_level( access_level ) {};

    shared_ptr< ndn::Data >
    makeData( shared_ptr< const ndn::Interest > interest ) override
    {
      using namespace ndn;

      const ndn::Block& payload = interest->getPayload();
      if( payload.type() != tlv::ContentType_AuthRequest )
      {
        // nack
        auto nack = make_shared< Data >( interest->getName() );
        nack->setContentType( tlv::ContentType_Nack );
        nack->setAccessLevel( 0 );
        nack->setFreshnessPeriod( time::seconds( 0 ) );
        nack->setSignature( ndn::security::DUMMY_NDN_SIGNATURE  );
        nack->wireEncode();
        return nack;
      }

      uint64_t route_hash = readNonNegativeInteger( payload.get( tlv::RouteHash ) );

      AuthTag tag;
      tag.setPrefix( m_prefix );
      tag.setAccessLevel( m_access_level );
      tag.setActivationTime( time::system_clock::now() - time::seconds( 10 ) );
      tag.setExpirationTime( time::system_clock::now() + time::days( 1 ) );
      tag.setRouteHash( route_hash );
      if( interest->getSignature().hasKeyLocator() )
        tag.setConsumerLocator( interest->getSignature().getKeyLocator() );
      else
        tag.setConsumerLocator( KeyLocator() );
      tag.setSignature( ndn::security::DUMMY_NDN_SIGNATURE );

      auto data = make_shared< Data >( interest->getName() );
      data->setContentType( tlv::ContentType_AuthGranted );
      data->setContent( tag.wireEncode() );
      data->setAccessLevel( 0 );
      data->setFreshnessPeriod( time::seconds( 0 ) );
      data->setSignature( ndn::security::DUMMY_NDN_SIGNATURE  );
      data->wireEncode();
      return data;
    }

    uint8_t
    getAccessLevel() override
    {
      return 0;
    }

  private:
    ndn::Name     m_prefix;
    uint8_t       m_access_level;

  };
}

#endif // AUTH_DATA_PRODUCER__INCLUDED

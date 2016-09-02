#include "producer.hpp"
#include "coordinator.hpp"
#include "ns3/ndnSIM/utils/dummy-keychain.hpp"
#include <sstream>

extern "C"
{
  #include <dirent.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <unistd.h>
}

namespace ndntac
{
  const ns3::Time Producer::s_producer_signature_delay = ns3::MilliSeconds( 121.641253 );
  const ns3::Time Producer::s_producer_bloom_delay = ns3::MilliSeconds( 9.238432 );
  const ns3::Time Producer::s_producer_interest_delay = ns3::MilliSeconds( 100 );
  const size_t Producer::s_segment_size = 512;
  uint32_t Producer::s_instance_id = 0;


  NS_OBJECT_ENSURE_REGISTERED(Producer);

  ns3::TypeId
  Producer::GetTypeId()
  {
          static ns3::TypeId tid
              = ns3::TypeId("ndntac::Producer")
                .SetParent<ns3::ndn::App>()
                .AddConstructor<Producer>()
                .AddAttribute(
                      "Names",
                      "List of names of the format \"[name1:size1:accesslevel][name2:size2:accesslevel]...\"",
                       ns3::StringValue("index.html"),
                       ns3::MakeStringAccessor(&Producer::m_names_string),
                       ns3::MakeStringChecker() )
               .AddAttribute(
                     "Prefix",
                     "Prefix to answer by",
                      ns3::StringValue(""),
                      ns3::ndn::MakeNameAccessor(&Producer::m_prefix),
                      ns3::ndn::MakeNameChecker() );
          return tid;
  }

  Producer::Producer():
    m_auth_cache( 1e-10, 10000 )
  {
    m_instance_id = s_instance_id++;
    Coordinator::addProducer( m_instance_id );
  }
  
   void
   Producer::onDataRequest( shared_ptr< const ndn::Interest > interest )
   {
   
      // ensure that name is valid
      ndn::Name subname = interest->getName().getSubName( m_prefix.size() );
      while( subname.get( -1 ).isSequenceNumber() )
        subname = subname.getPrefix( -1 );
      auto name_entry = m_names.find( subname );
      if( name_entry == m_names.end() )
        return;
        
      // figure out segment number, we use sequence number to indicate segment
      // because all the ndnSIM consumers use sequence numbers
      uint64_t segment = 0;
      if( interest->getName().get(-1).isSegment() )
        segment = interest->getName().get(-1).toSequenceNumber();
    
      // read data
      static uint8_t dummy_segment[s_segment_size] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                                                     "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                                                     "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                                                     "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                                                     "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                                                     "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                                                     "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                                                     "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                                                     "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                                                     "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                                                     "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                                                     "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                                                     "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                                                     "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                                                     "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                                                     "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
      size_t content_size = name_entry->second.first;
      size_t seg_size =  ( segment*s_segment_size + s_segment_size > content_size )
                         ? content_size - segment*s_segment_size
                         : s_segment_size;
      
      // make data
      uint8_t access_level = name_entry->second.second;
      shared_ptr<ndn::Data> data = make_shared<ndn::Data>( interest->getName() );
      data->setContentType( ndn::tlv::ContentType_Blob );
      data->setContent( dummy_segment, seg_size );
      data->setAccessLevel( access_level );
      data->setFreshnessPeriod( ndn::time::days( 1 ) );
      ndn::Signature sig = ndn::security::DUMMY_NDN_SIGNATURE;
      sig.setKeyLocator( ndn::KeyLocator( m_prefix ) );
      data->setSignature( sig  );
      data->wireEncode();

      ///////// check that the interest's authentication ( AuthTag ) is valid //////
      const ndn::AuthTag& tag = interest->getAuthTag();

      // tags with invalid access level are refused
      if( access_level > tag.getAccessLevel() )
      {
        Coordinator::producerDeniedRequest( m_prefix,
                                            interest->getName(),
                                            "AuthTag access level is too low" );
        m_queue.receiveData( m_face, makeAuthDenial( *data ) );
        return;
      }

      //interests with expired tags are refused
      if( tag.isExpired() )
      {
        Coordinator::producerDeniedRequest( m_prefix,
                                            interest->getName(),
                                            "AuthTag is expired" );
        m_queue.receiveData( m_face, makeAuthDenial( *data ) );
        return;
      }

      // tags with wrong prefixes are refused
      if( !tag.getPrefix().isPrefixOf( data->getName() ) )
      {
        Coordinator::producerDeniedRequest( m_prefix,
                                            interest->getName(),
                                            "AuthTag prefix does not "
                                            "match data prefix" );
        m_queue.receiveData( m_face, makeAuthDenial( *data ) );
        return;
      }

      // tags with non matching key locators are refused
      if( !data->getSignature().hasKeyLocator() )
      {
        Coordinator::producerDeniedRequest( m_prefix,
                                          interest->getName(),
                                          "Requested data has no key locator" );
        m_queue.receiveData( m_face, makeAuthDenial( *data ) );
        return;
      }
      if( tag.getKeyLocator() != data->getSignature().getKeyLocator() )
      {
        Coordinator::producerDeniedRequest( m_prefix,
                                            interest->getName(),
                                            "AuthTag key locator does not "
                                            "match data key locator" );
        m_queue.receiveData( m_face, makeAuthDenial( *data ) );
        return;
      }

      // set NoReCache flag if necessary
      if( interest->getAuthValidityProb() > 0 )
        data->setNoReCacheFlag( true );

      // check if auth is cached ( delay adds lookup time to simulation )
      if( interest->getAuthValidityProb() == 0 )
      {
          m_queue.delay( s_producer_bloom_delay );
          if( m_auth_cache.contains( tag ) )
          {
            Coordinator::producerSatisfiedRequest( m_prefix, interest->getName() );
            m_queue.receiveData( m_face, data );
            return;
          }
      }

      // verify signature, we simulate actual verification delay by
      // adding delay to the transmit queue, signature is valid if it
      // isn't empty, since we don't need to actually validate for the
      // simulation
      m_queue.delay( s_producer_signature_delay );
      if( tag.getSignature().getValue().value_size() > 0 )
      {
        Coordinator::producerSatisfiedRequest( m_prefix, interest->getName() );
        m_queue.receiveData( m_face, data );
        m_auth_cache.insert( tag );
        return;
      }

      // signature was invalid
      m_queue.receiveData( m_face, makeAuthDenial( *data ) );
   }
 
   void
   Producer::onAuthRequest( shared_ptr< const ndn::Interest > interest )
   {
      // since this is a simulation we don't do real identity
      // authentication, we just give an AuthTag to whomever asks for one
      const ndn::Block& payload = interest->getPayload();
      payload.parse();
      if( payload.type() != ndn::tlv::ContentType_AuthRequest )
        return;

      ndn::Block hash = payload.get( ndn::tlv::RouteHash );
      uint64_t route_hash = readNonNegativeInteger( hash );

      ndn::AuthTag tag;
      tag.setPrefix( m_prefix );
      tag.setAccessLevel( 3 );
      tag.setActivationTime( ndn::time::system_clock::now() - ndn::time::seconds( 10 ) );
      tag.setExpirationTime( ndn::time::system_clock::now() + ndn::time::days( 1 ) );
      tag.setRouteHash( route_hash );
      if( interest->getSignature().hasKeyLocator() )
        tag.setConsumerLocator( interest->getSignature().getKeyLocator() );
      else
        tag.setConsumerLocator( ndn::KeyLocator() );
      
      ndn::Signature sig = ndn::security::DUMMY_NDN_SIGNATURE;
      sig.setKeyLocator( ndn::KeyLocator( m_prefix ) );
      tag.setSignature( sig );

      auto data = make_shared< ndn::Data >( interest->getName() );
      data->setContentType( ndn::tlv::ContentType_AuthGranted );
      data->setContent( tag.wireEncode() );
      data->setAccessLevel( 0 );
      data->setFreshnessPeriod( ndn::time::seconds( 0 ) );
      data->setSignature( sig  );
      data->wireEncode();
      
      m_queue.receiveData( m_face, data );
   }

  void
  Producer::OnInterest( shared_ptr< const ndn::Interest > interest )
  {
      App::OnInterest( interest );
      
      Coordinator::producerReceivedRequest( m_prefix, interest->getName() );

      // every interest takes some amount of processing time
      // for now we simulate this with a constant delay
      m_queue.delay( s_producer_interest_delay );
      
      if( interest->getName().getPrefix( m_prefix.size() ) == m_prefix )
      {
        if( interest->getName().get( m_prefix.size() ) == ndn::name::Component("AUTH_TAG") )
                onAuthRequest( interest );
        else
                onDataRequest( interest );
      }
  }

  void
  Producer::StartApplication()
  {
    App::StartApplication();
    Coordinator::producerStarted( m_prefix );
    
    // register route
    ns3::ndn::FibHelper::AddRoute( GetNode(), m_prefix, m_face, 0 );

    // parse names
    static boost::regex name_regex("\\[([\\w/_]+):([0-9]+)(T|G|M|K|B)?:([0-9]+)\\]" );
    boost::sregex_iterator names_itr( m_names_string.begin(),
                                      m_names_string.end(),
                                      name_regex );
    boost::sregex_iterator names_end;
    
    while( names_itr != names_end )
    {
        ndn::Name name( (*names_itr)[1].str() );
        size_t    size; std::stringstream( (*names_itr)[2].str() ) >> size;
        char      mult = (*names_itr)[3].str().empty() ? 'B' : (*names_itr)[3].str()[0];
        unsigned  access_level; std::stringstream( (*names_itr)[4].str() ) >> access_level;
        
        switch( mult )
        {
            // fallthrough all
            case 'T':
                size *= 1024;
            case 'G':
                size *= 1024;
            case 'M':
                size *= 1024;
            case 'K':
                size *= 1024;
            case 'B':
                break;
                
        }
        
        m_names[ name ] = std::make_pair( size, access_level );
        names_itr++;
    }
  }

  void
  Producer::StopApplication()
  {
    App::StopApplication();
    Coordinator::producerStopped( m_prefix );
  }

  shared_ptr< ndn::Data >
  Producer::makeAuthDenial( const ndn::Data& data )
  {
    auto denial = make_shared< ndn::Data >( data.getName() );
    denial->setContentType( ndn::tlv::ContentType_AuthDenial );
    denial->setContent( data.wireEncode() );
    denial->setFreshnessPeriod( ndn::time::milliseconds(0));
    denial->setSignature( ndn::security::DUMMY_NDN_SIGNATURE );
    denial->wireEncode();
    return denial;
  }
}

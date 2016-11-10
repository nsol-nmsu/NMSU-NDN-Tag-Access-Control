#include "producer.hpp"
#include "coordinator.hpp"
#include "ns3/ndnSIM/utils/dummy-keychain.hpp"
#include <sstream>
#include <boost/regex.hpp>

extern "C"
{
  #include <dirent.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <unistd.h>
}

namespace ndntac
{
  const ns3::Time Producer::s_producer_signature_delay = ns3::NanoSeconds( 30345 );
  const ns3::Time Producer::s_producer_bloom_delay = ns3::NanoSeconds( 2535 );
  const ns3::Time Producer::s_producer_interest_delay = ns3::MilliSeconds( 0 );
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

  Producer::Producer()
  {
    m_instance_id = s_instance_id++;
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
      if( interest->getName().get(-1).isSequenceNumber() )
        segment = interest->getName().get(-1).toSequenceNumber();
    
      // read data
      static uint8_t dummy_segment[s_segment_size] =
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
         "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
         "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
      size_t content_size = name_entry->second.first;
      size_t seg_size;
      if( segment*s_segment_size >= content_size )
        seg_size = 0;
      else if( segment*s_segment_size + s_segment_size > content_size )
        seg_size = content_size - segment*s_segment_size;
      else
        seg_size = s_segment_size;
        
      // make data
      uint8_t access_level = name_entry->second.second;
      shared_ptr<ndn::Data> data = make_shared<ndn::Data>( interest->getName() );
      data->setContentType( seg_size == 0
                            ? ndn::tlv::ContentType_EoC
                            : ndn::tlv::ContentType_Blob );
      data->setContent( dummy_segment, seg_size );
      data->setAccessLevel( access_level );
      data->setFreshnessPeriod( ndn::time::days( 1 ) );
      ndn::Signature sig = ndn::security::DUMMY_NDN_SIGNATURE;
      data->setRouteTracker( interest->getRouteTracker() );
      sig.setKeyLocator( ndn::KeyLocator( m_prefix ) );
      data->setSignature( sig  );
      data->wireEncode();
      BOOST_ASSERT( data->getCurrentNetwork() == ndn::RouteTracker::EXIT_NETWORK );

      ///////// check that the interest's authentication ( AuthTag ) is valid //////
      
      // if data access level is 0 then forward without authentication
      if( data->getAccessLevel() == 0 )
      {
        logDataSent( *data );
        m_queue.receiveData( m_face, data );
      }
      
      // interests without tags are refused
      if( !interest->hasAuthTag() )
      {
        logDataDenied( *data, "no auth tag" );
        toNack( *data );
        m_queue.receiveData( m_face, data );
        return;
      }
      
      
      const ndn::AuthTag& tag = interest->getAuthTag();

      // tags with invalid access level are refused
      if( access_level > tag.getAccessLevel() )
      {
        logDataDenied( *data, tag, "insufficient access rights" );
        toNack( *data );
        m_queue.receiveData( m_face, data );
        return;
      }

      //interests with expired tags are refused
      if( tag.isExpired() )
      {
        logDataDenied( *data, tag, "auth tag is expired" );
        toNack( *data );
        m_queue.receiveData( m_face, data );
        return;
      }

      // tags with wrong prefixes are refused
      if( !tag.getPrefix().isPrefixOf( data->getName() ) )
      {
        logDataDenied( *data, tag, "tag prefix does not match data" );
        toNack( *data );
        m_queue.receiveData( m_face, data );
        return;
      }

      // tags with non matching key locators are refused
      if( !data->getSignature().hasKeyLocator() )
      {
        logDataDenied( *data, tag, "data doesn't have key locator" );
        toNack( *data );
        m_queue.receiveData( m_face, data );
        return;
      }
      if( tag.getKeyLocator() != data->getSignature().getKeyLocator() )
      {
        logDataDenied( *data, tag, "tag's key locator doesn't match data's" );
        toNack( *data );
        m_queue.receiveData( m_face, data );
        return;
      }

      // set NoReCache flag if necessary
      if( interest->getAuthValidityProb() > 0 )
	  {
	    logNoReCacheFlagSet( *data, *interest );
        data->setNoReCacheFlag( true );
      }


      // verify signature, we simulate actual verification delay by
      // adding delay to the transmit queue, signature is valid if it
      // isn't equal to DUMMY_BAD_SIGNATURE, which has 0 as its first
      // byte
      m_queue.delay( s_producer_signature_delay );
      if( tag.getSignature().getValue().value_size() > 0
          && tag.getSignature().getValue().value()[0] != 0 )
      {
        logDataSent( *data, tag );
        m_queue.receiveData( m_face, data );
        return;
      }

      // signature was invalid
      logDataDenied( *data, tag, "bad signature" );
      toNack( *data );
      m_queue.receiveData( m_face, data );
   }
 
   void
   Producer::onAuthRequest( shared_ptr< const ndn::Interest > interest )
   {
      // since this is a simulation we don't do real identity
      // authentication, we just give an AuthTag to whomever asks for one
      uint64_t route_hash = interest->getEntryRoute();

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
      data->setContentType( ndn::tlv::ContentType_Auth );
      data->setContent( tag.wireEncode() );
      data->setAccessLevel( 0 );
      data->setFreshnessPeriod( ndn::time::seconds( 0 ) );
      data->setRouteTracker( interest->getRouteTracker() );
      data->setSignature( sig  );
      data->wireEncode();
      BOOST_ASSERT( data->getCurrentNetwork() == ndn::RouteTracker::EXIT_NETWORK );
      
      logSentAuth( tag );
      m_queue.receiveData( m_face, data );
   }

  void
  Producer::OnInterest( shared_ptr< const ndn::Interest > interest )
  {
      App::OnInterest( interest );
      BOOST_ASSERT( interest->getCurrentNetwork() == ndn::RouteTracker::EXIT_NETWORK );
      
      logReceivedRequest( *interest );

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
    
    logProducerStart();
    
    // register route
    ns3::ndn::FibHelper::AddRoute( GetNode(), m_prefix, m_face, 0 );

    // parse names
    static boost::regex name_regex("\\[([^:]+):([0-9]+)(T|G|M|K|B)?:([0-9]+)\\]" );
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
  }

  void
  Producer::toNack( ndn::Data& data )
  {
    data.setContentType( ndn::tlv::ContentType_Nack );
    data.wireEncode();
  }
  
  void
  Producer::logDataDenied( const ndn::Data& data,
                           const ndn::AuthTag& auth,
                           const std::string& why ) const
  {
    Coordinator::LogEntry entry( "Producer", "DataDenied");
    entry.add( "id", std::to_string( m_instance_id ) );
    entry.add( "data-name", data.getName().toUri() );
    entry.add( "data-access", std::to_string( (unsigned)data.getAccessLevel() ) );
    entry.add( "auth-prefix", auth.getPrefix().toUri() );
    entry.add( "auth-access", std::to_string( (unsigned)auth.getAccessLevel() ) );
    entry.add( "auth-expired", auth.isExpired() ? "true" : "false" );
    entry.add( "why", why );
    Coordinator::log( entry );
  }
  void
  Producer::logDataDenied( const ndn::Data& data,
                           const std::string& why ) const
  {
    Coordinator::LogEntry entry( "Producer", "DataDenied");
    entry.add( "id", std::to_string( m_instance_id ) );
    entry.add( "data-name", data.getName().toUri() );
    entry.add( "data-access", std::to_string( (unsigned)data.getAccessLevel() ) );
    entry.add( "why", why );
    Coordinator::log( entry );
  }
  
  void
  Producer::logDataSent( const ndn::Data& data,
                         const ndn::AuthTag& auth ) const
  {
    Coordinator::LogEntry entry( "Producer", "DataSent");
    entry.add( "id", std::to_string( m_instance_id ) );
    entry.add( "data-name", data.getName().toUri() );
    entry.add( "data-access", std::to_string( (unsigned)data.getAccessLevel() ) );
    entry.add( "auth-prefix", auth.getPrefix().toUri() );
    entry.add( "auth-access", std::to_string( (unsigned)auth.getAccessLevel() ) );
    entry.add( "auth-expired", auth.isExpired() ? "true" : "false" );
    Coordinator::log( entry );
  }
  void
  Producer::logDataSent( const ndn::Data& data) const
  {
    Coordinator::LogEntry entry( "Producer", "DataSent");
    entry.add( "id", std::to_string( m_instance_id ) );
    entry.add( "data-name", data.getName().toUri() );
    entry.add( "data-access", std::to_string( (unsigned)data.getAccessLevel() ) );
    Coordinator::log( entry );
  }
  
  void
  Producer::logNoReCacheFlagSet( const ndn::Data& data,
                                const ndn::Interest& interest ) const
  {
    Coordinator::LogEntry entry( "Producer", "NoReCacheFlagSet");
    entry.add( "id", std::to_string( m_instance_id ) );
    entry.add( "data-name", data.getName().toUri() );
    entry.add( "interest-val-prob", std::to_string( interest.getAuthValidityProb() ) );
    Coordinator::log( entry );
  }

  void
  Producer::logSentAuth( const ndn::AuthTag& auth ) const
  {
    Coordinator::LogEntry entry( "Producer", "AuthSent");
    entry.add( "id", std::to_string( m_instance_id ) );
    entry.add( "auth-prefix", auth.getPrefix().toUri() );
    entry.add( "auth-access", std::to_string( (unsigned)auth.getAccessLevel() ) );
    entry.add( "auth-expired", auth.isExpired() ? "true" : "false" );
    Coordinator::log( entry );
  }

  void
  Producer::logReceivedRequest( const ndn::Interest& interest ) const
  {
    Coordinator::LogEntry entry( "Producer", "ReceivedRequest");
    const ndn::AuthTag& auth = interest.getAuthTag();
    entry.add( "id", std::to_string( m_instance_id ) );
    entry.add( "interest-name", interest.getName().toUri() );
    entry.add( "auth-prefix", auth.getPrefix().toUri() );
    entry.add( "auth-access", std::to_string( (unsigned)auth.getAccessLevel() ) );
    entry.add( "auth-expired", auth.isExpired() ? "true" : "false" );
    Coordinator::log( entry );
  }

  void
  Producer::logProducerStart( void ) const
  {
    Coordinator::LogEntry entry( "Producer", "ProducerStarted");
    entry.add( "id", std::to_string( m_instance_id ) );
    entry.add( "prefix", m_prefix.toUri() );
    entry.add( "contents", m_names_string );
    Coordinator::log( entry );
  }
}

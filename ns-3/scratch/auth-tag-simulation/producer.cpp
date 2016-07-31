#include "producer.hpp"
#include "coordinator.hpp"

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
                      "Directory",
                      "Serving file directory",
                       ns3::StringValue("."),
                       MakeStringAccessor(&Producer::m_dir),
                       ns3::MakeStringChecker() )
               .AddAttribute(
                     "Prefix",
                     "If set to non-empty value, replaces directory path as producer prefix",
                      ns3::StringValue(""),
                      MakeStringAccessor(&Producer::m_prefix_str),
                      ns3::MakeStringChecker() );
          return tid;
  }

  Producer::Producer():
    m_auth_cache( 1e-10, 10000 )
  {
    m_instance_id = s_instance_id++;
    Coordinator::addProducer( m_instance_id );
  }

  void
  Producer::OnInterest( shared_ptr< const ndn::Interest > interest )
  {
      App::OnInterest( interest );
      Coordinator::producerReceivedRequest( m_prefix, interest->getName() );

      // every interest takes some amount of processing time
      // for now we simulate this with a constant delay
      m_queue.delay( s_producer_interest_delay );

      // check if we have the appropriate data producer
      shared_ptr< DataProducer > producer = NULL;
      auto it = m_producers.find( interest->getName() );
      if( it == m_producers.end() )
      {
        Coordinator::producerOther( m_prefix,
                                    string( "No matching data for " )
                                    + interest->getName().toUri() );
        return;
      }

      // make data
      auto data = producer->makeData( interest );

      //--- check that the interest's authentication ( AuthTag ) is valid ----//
      const ndn::AuthTag& tag = interest->getAuthTag();

      // check that data actually matches interest
      if( !interest->matchesData( *data ) )
      {
        Coordinator::producerOther( m_prefix,
                                    "No matching data for "
                                    + interest->getName().toUri() );
        return;
      }

      // access level of 0 does not need tag
      if( data->getAccessLevel() == 0 )
      {
        Coordinator::producerSatisfiedRequest( m_prefix, interest->getName() );
        m_queue.receiveData( m_face, data );
        return;
      }

      // tags with invalid access level are refused
      if( data->getAccessLevel() > tag.getAccessLevel() )
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
      m_queue.delay( s_producer_bloom_delay );
      if( m_auth_cache.contains( tag ) )
      {
        Coordinator::producerSatisfiedRequest( m_prefix, interest->getName() );
        m_queue.receiveData( m_face, data );
        return;
      }

      // verify signature, we simulate actual verification delay by
      // adding delay to the transmit queue, signature is valid if it
      // isn't empty, sine we don't need to actually validate for the
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
  Producer::StartApplication()
  {
    App::StartApplication();
    if( m_prefix_str.empty() )
      m_prefix = m_dir;
    else
      m_prefix = m_prefix_str;

    ns3::ndn::FibHelper::AddRoute( GetNode(), m_prefix, m_face, 0 );

    // make and register file producers
    makeProducers( m_dir, "", m_producers );

    // make and register auth producer
    auto auth_producer = make_shared<AuthDataProducer>( m_prefix, 3 );
    m_producers[ndn::Name(m_prefix).append("AUTH_TAG")] = auth_producer;

    Coordinator::producerStarted( m_prefix );
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
    denial->setSignatureValue( ndn::Block( ndn::tlv::SignatureValue, ndn::Block( "Not Empty", 9 ) ) );
    denial->setFreshnessPeriod( ndn::time::milliseconds(0));
    return denial;
  }


  void
  Producer::makeProducers( const string& rootpath, const string& filepath,
                           std::map< ndn::Name, shared_ptr< DataProducer> > container )
  {

      // if item is a file then make producer
      string path = rootpath + "/" + filepath ;
      struct stat s;
      stat( path.c_str(), &s );
      if( S_ISREG( s.st_mode ) )
      {
          // filename determines access level
          unsigned access_level;
          if( sscanf( filepath.c_str(), "[%u]", &access_level ) < 1 )
          {
            // otherwise access level is random
            access_level = rand() % 3;
          }

          auto producer =
              make_shared<FileDataProducer>( path,
                                             access_level );
          container[ndn::Name( m_prefix ).append( ndn::Name( filepath ) )] = producer;
          return;
      }
      else // otherwise recurse on all subdirs
      {
          auto dir = opendir( path.c_str() );
          if( dir == NULL )
              throw std::ifstream
                       ::failure( "Error opending directory" );

          struct dirent* node;

          while( ( node = readdir( dir ) ) != NULL )
          {
              // don't want hidden files
              if( node->d_name[0] == '.' )
                  continue;

              makeProducers( rootpath, filepath + string( ( filepath.empty() )? "" : "/"  ) + node->d_name,
                             container );
          }
          closedir( dir );
          return;
      }
  }
}

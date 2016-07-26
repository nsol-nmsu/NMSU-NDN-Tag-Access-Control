#include "producer.hpp"

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
                       ns3::MakeStringChecker() );
          return tid;
  }

  Producer::Producer()
  {
    m_instance_id = s_instance_id++;
  }

  void
  Producer::OnInterest( shared_ptr< const ndn::Interest > interest )
  {
      App::OnInterest( interest );

      // every interest takes some amount of processing time
      // for now we simulate this with a constant delay
      m_queue.delay( s_producer_interest_delay );

      // check if we have the appropriate data producer
      shared_ptr< DataProducer > producer = NULL;
      auto it = m_producers.find( interest->getName() );
      if( it == m_producers.end() )
        return;

      // make data
      auto data = producer->makeData( interest );

      //--- check that the interest's authentication ( AuthTag ) is valid ----//
      const ndn::AuthTag& tag = interest->getAuthTag();

      // access level of 0 does not need tag
      if( data->getAccessLevel() == 0 )
      {
        m_queue.receiveData( m_face, data );
        return;
      }

      // tags with invalid access level are refused
      if( data->getAccessLevel() > tag.getAccessLevel() )
      {
        m_queue.receiveData( m_face,
                             makeAuthDenial( *data ) );
        return;
      }

      // tags with expried interest are refused
      if( tag.isExpired() )
      {
        m_queue.receiveData( m_face,
                             makeAuthDenial( *data ) );
        return;
      }

      // tags with wrong prefixes are refused
      if( tag.getPrefix() != m_dir )
      {
        m_queue.receiveData( m_face,
                             makeAuthDenial( *data ) );
        return;
      }

      // tags with non matching key locators are refused
      if( !data->getSignature().hasKeyLocator() )
      {
        m_queue.receiveData( m_face,
                             makeAuthDenial( *data ) );
        return;
      }
      if( tag.getKeyLocator() != data->getSignature().getKeyLocator() )
      {
        m_queue.receiveData( m_face,
                             makeAuthDenial( *data ) );
        return;
      }

      // check that data actually matches interest
      if( !interest->matchesData( *data ) )
        return;

      // tags with unmatching route hash are refused
      if( tag.getRouteHash() != interest->getRouteHash() )
      {
        m_queue.receiveData( m_face,
                             makeAuthDenial( *data ) );
        return;
      }

      // verify signature, we simulate actual verification delay by
      // adding delay to the transmit queue, signature is valid if it
      // isn't empty, sine we don't need to actually validate for the
      // simulation
      m_queue.delay( s_producer_signature_delay );
      if( tag.getSignature().getValue().value_size() == 0 )
      {
        m_queue.receiveData( m_face,
                              makeAuthDenial( *data ) );
        return;
      }

      // all checks were passed, send data
      m_queue.receiveData( m_face, data );
  }

  void
  Producer::StartApplication()
  {
    App::StartApplication();
    ndn::Name my_name( m_dir );
    ns3::ndn::FibHelper::AddRoute( GetNode(), my_name, m_face, 0 );

    // make and register file producers
    makeProducers( m_dir, m_producers );

    // make and register auth producer
    auto auth_producer = make_shared<AuthDataProducer>( my_name, 3 );
    m_producers[ndn::Name(m_dir + "/AUTH_TAG")] = auth_producer;

  }

  void
  Producer::StopApplication()
  {
    App::StopApplication();
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
  Producer::makeProducers( const string path,
                    std::map< ndn::Name, shared_ptr< DataProducer> > container )
  {

      // if item is a file then make producer
      struct stat s;
      stat( path.c_str(), &s );
      if( S_ISREG( s.st_mode ) )
      {
          // filename determines access level
          unsigned access_level;
          if( sscanf( path.c_str(), "[%u]", &access_level ) < 1 )
          {
            // otherwise access level is random
            access_level = rand() % 3;
          }

          auto producer =
              make_shared<FileDataProducer>( path,
                                             access_level );
          container[ndn::Name( path )] = producer;
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

              makeProducers( path + "/" + node->d_name,
                             container );
          }
          closedir( dir );
          return;
      }
  }
}

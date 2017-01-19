#include "producer.hpp"
#include "ns3/ndnSIM-module.h"
#include "ns3/ndnSIM/utils/dummy-keychain.hpp"
#include <sstream>
#include <boost/regex.hpp>
#include "tracers.hpp"

extern "C"
{
  #include <dirent.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <unistd.h>
}

namespace ndntac
{

using namespace std;
using namespace ndn;
using namespace ns3;
using namespace ns3::ndn;

string
Producer::s_config = "config/producer_config.jx9";
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
            .AddAttribute( "Prefix",
                           "Producer prefix",
                           NameValue( "/" ),
                           MakeNameAccessor( &Producer::GetPrefix ),
                           MakeNameChecker() );
      return tid;
}

Producer::Producer()
    : m_instance_id( s_instance_id++ )
    , m_config( s_config, m_instance_id )
{
}

const Name&
Producer::GetPrefix( void ) const
{
    return m_config.prefix;
}

void
Producer::onDataRequest( shared_ptr< const Interest > interest )
{

  // ensure that name is valid
  Name subname = interest->getName()
                 .getSubName( m_config.prefix.size() );
  while( subname.get( -1 ).isSegment() )
    subname = subname.getPrefix( -1 );
  auto name_entry = m_config.contents.find( subname );
  if( name_entry == m_config.contents.end() )
    return;
    
  // figure out segment number
  uint64_t segment = 0;
  if( interest->getName().get(-1).isSegment() )
    segment = interest->getName().get(-1).toSegment();

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
  size_t content_size = name_entry->second.size;
  size_t seg_size;
  if( segment*s_segment_size >= content_size )
    seg_size = 0;
  else if( segment*s_segment_size + s_segment_size > content_size )
    seg_size = content_size - segment*s_segment_size;
  else
    seg_size = s_segment_size;
    
  // make data
  uint8_t access_level = name_entry->second.access_level;
  shared_ptr<Data> data = make_shared< Data >( interest->getName() );
  data->setContentType( seg_size == 0
                        ? tlv::ContentType_EoC
                        : tlv::ContentType_Blob );
  data->setContent( dummy_segment, seg_size );
  data->setAccessLevel( access_level );
  data->setFreshnessPeriod( ::ndn::time::days( 1 ) );
  Signature sig = security::DUMMY_NDN_SIGNATURE;
  data->setRouteTracker( interest->getRouteTracker() );
  sig.setKeyLocator( KeyLocator( m_config.prefix ) );
  data->setSignature( sig  );
  data->wireEncode();
  BOOST_ASSERT( data->getCurrentNetwork()
              == RouteTracker::EXIT_NETWORK );

  ///////// check that the interest's authentication ( AuthTag ) is valid //////
  
  // if data access level is 0 then forward without authentication
  if( data->getAccessLevel() == 0 )
  {
    tracers::producer->validation
    ( tracers::ValidationSuccessSkipped );
    tracers::producer->sent_data( *data );
    m_tx_queue.receiveData( m_face, data );
  }
  
  // interests without tags are refused
  if( !interest->hasAuthTag() )
  {
    tracers::producer->validation
    ( tracers::ValidationFailureNoAuth );
    toNack( *data );
    tracers::producer->sent_data( *data );
    m_tx_queue.receiveData( m_face, data );
    return;
  }
  
  
  const AuthTag& tag = interest->getAuthTag();

  // tags with invalid access level are refused
  if( access_level > tag.getAccessLevel() )
  {
    tracers::producer->validation
    ( tracers::ValidationFailureLowAuth );
    toNack( *data );
    tracers::producer->sent_data( *data );
    m_tx_queue.receiveData( m_face, data );
    return;
  }

  //interests with expired tags are refused
  if( tag.isExpired() )
  {
    
    tracers::producer->validation
    ( tracers::ValidationFailureExpired );
    toNack( *data );
    tracers::producer->sent_data( *data );
    m_tx_queue.receiveData( m_face, data );
    return;
  }

  // tags with wrong prefixes are refused
  if( !tag.getPrefix().isPrefixOf( data->getName() ) )
  {
    tracers::producer->validation
    ( tracers::ValidationFailureBadPrefix );
    toNack( *data );
    tracers::producer->sent_data( *data );
    m_tx_queue.receiveData( m_face, data );
    return;
  }

  // tags with non matching key locators are refused
  if( !data->getSignature().hasKeyLocator() )
  {
    tracers::producer->validation
    ( tracers::ValidationFailureBadKeyLoc );
    toNack( *data );
    tracers::producer->sent_data( *data );
    m_tx_queue.receiveData( m_face, data );
    return;
  }
  if( tag.getKeyLocator() != data->getSignature().getKeyLocator() )
  {
    tracers::producer->validation
    ( tracers::ValidationFailureBadKeyLoc );
    toNack( *data );
    tracers::producer->sent_data( *data );
    m_tx_queue.receiveData( m_face, data );
    return;
  }

  // tags with bad route hash are refused
  if( tag.getRouteHash() != interest->getEntryRoute() )
  {
    tracers::producer->validation
    ( tracers::ValidationFailureBadRoute );
    toNack( *data );
    tracers::producer->sent_data( *data );
    m_tx_queue.receiveData( m_face, data );
    return;
  }

  // set NoReCache flag if necessary
  if( interest->getAuthValidityProb() > 0 )
  {
    data->setNoReCacheFlag( true );
  }


  // verify signature, we simulate actual verification delay by
  // adding delay to the transmit queue, signature is valid if it
  // isn't equal to DUMMY_BAD_SIGNATURE, which has 0 as its first
  // byte
  //tracers::producer->sigverif
  //( tag, m_config.sigverif_delay );
  //m_tx_queue.delay( m_config.sigverif_delay );
  if( tag.getSignature().getValue().value_size() > 0
      && tag.getSignature().getValue().value()[0] != 0 )
  {
    //tracers::producer->validation
    //( tracers::ValidationSuccessSig );
    tracers::producer->sent_data( *data );
    m_tx_queue.receiveData( m_face, data );
    return;
  }

  // signature was invalid
  //tracers::producer->validation
  //( tracers::ValidationFailureSig );
  toNack( *data );
  tracers::producer->sent_data( *data );
  m_tx_queue.receiveData( m_face, data );
}

void
Producer::onAuthRequest( shared_ptr< const Interest > interest )
{
  // since this is a simulation we don't do real identity
  // authentication, we just give an AuthTag to whomever asks for one
  uint64_t route_hash = interest->getEntryRoute();

  AuthTag tag;
  tracers::producer->tag_created( tag );
  tag.setPrefix( m_config.prefix );
  tag.setAccessLevel( 3 );
  tag.setActivationTime( ::ndn::time::system_clock::now()
                         - ::ndn::time::seconds( 10 ) );
  tag.setExpirationTime( ::ndn::time::system_clock::now()
                         + ::ndn::time::days( 1 ) );
  tag.setRouteHash( route_hash );
  if( interest->getSignature().hasKeyLocator() )
    tag.setConsumerLocator( interest->getSignature().getKeyLocator() );
  else
    tag.setConsumerLocator( KeyLocator() );
  
  Signature sig = security::DUMMY_NDN_SIGNATURE;
  sig.setKeyLocator( KeyLocator( m_config.prefix ) );
  tag.setSignature( sig );

  auto data = make_shared< Data >( interest->getName() );
  data->setContentType( tlv::ContentType_Auth );
  data->setContent( tag.wireEncode() );
  data->setAccessLevel( 0 );
  data->setFreshnessPeriod( ::ndn::time::seconds( 0 ) );
  data->setRouteTracker( interest->getRouteTracker() );
  data->setSignature( sig  );
  data->wireEncode();
  BOOST_ASSERT( data->getCurrentNetwork()
              ==  RouteTracker::EXIT_NETWORK );
  
  tracers::producer->sent_data( *data );
  m_tx_queue.receiveData( m_face, data );
}

void
Producer::OnInterest( shared_ptr< const Interest > interest )
{
  App::OnInterest( interest );
  BOOST_ASSERT( interest->getCurrentNetwork()
              == RouteTracker::EXIT_NETWORK );

  tracers::producer->received_interest
  ( *interest );

  if( interest->getName().getPrefix( m_config.prefix.size() )
    == m_config.prefix )
  {
    if( interest->getName().get( m_config.prefix.size() )
      == Name::Component("AUTH_TAG") )
            onAuthRequest( interest );
    else
            onDataRequest( interest );
  }
}

void
Producer::StartApplication()
{
    App::StartApplication();

    // register route
    FibHelper::AddRoute( GetNode(), m_config.prefix, m_face, 0 );
}

void
Producer::StopApplication()
{
    App::StopApplication();
}

void
Producer::toNack( Data& data )
{
    data.setContentType( tlv::ContentType_Nack );
    data.wireEncode();
}

Producer::Config::Config( const string& file, uint32_t id )
{
    // set default values
    prefix = Name("unnamed");
    sigverif_delay = NanoSeconds( 30345 );
    bloom_delay    = NanoSeconds( 2535 );
    
    // database and vm structs
    unqlite* db;
    unqlite_vm* vm;
    
    // initialize database
    int rc = unqlite_open( &db, ":mem:", UNQLITE_OPEN_READONLY );
    if( rc != UNQLITE_OK )
    {
        // something went wrong
        const char* err;
        int errlen;
        unqlite_config( db, UNQLITE_CONFIG_JX9_ERR_LOG,
                        &err, &errlen );
        cout << "Error: creating unqlite database: "
             << err << endl;
         exit(1);
    }

    // initialize unqlite vm
    rc = unqlite_compile_file( db, file.c_str(), &vm );
    if( rc != UNQLITE_OK )
    {
        // something went wrong
        const char* err;
        int errlen;
        unqlite_config( db, UNQLITE_CONFIG_JX9_ERR_LOG,
                        &err, &errlen );
        cout << "Error: compiling config script: "
             << err << endl;
         exit(1);
    }

    unqlite_value* id_val = unqlite_vm_new_scalar( vm );
    unqlite_value_int64( id_val, id );
    rc = unqlite_vm_config( vm, UNQLITE_VM_CONFIG_CREATE_VAR,
                            "ID", id_val );
    if( rc != UNQLITE_OK )
    {
        // something went wrong
        const char* err;
        int errlen;
        unqlite_config( db, UNQLITE_CONFIG_JX9_ERR_LOG,
                        &err, &errlen );
        cout << "Error: exporting ID to config: "
             << err << endl;
         exit(1);
    }
    unqlite_vm_release_value( vm, id_val );


    // execute config script
    rc = unqlite_vm_exec( vm );
    if( rc != UNQLITE_OK )
    {
        // something went wrong
        const char* err;
        int errlen;
        unqlite_config( db, UNQLITE_CONFIG_JX9_ERR_LOG,
                        &err, &errlen );
        cout << "Error: executing config script: "
             << err << endl;
         exit(1);
    }
    
    // retrieve config values
    const char* str;
    int len;
    unqlite_value* val;
    
    val = unqlite_vm_extract_variable( vm, "prefix" );
    if( val )
    {
        str = unqlite_value_to_string( val, &len );
        prefix = string(str, len );
    }

    val = unqlite_vm_extract_variable( vm, "sigverif_delay" );
    if( unqlite_value_is_float( val ) )
        sigverif_delay = Seconds( unqlite_value_to_double( val ) );
    if( unqlite_value_is_int( val ) )
        sigverif_delay = Seconds( unqlite_value_to_int64( val ) );

    val = unqlite_vm_extract_variable( vm, "bloom_delay" );
    if( unqlite_value_is_float( val ) )
       bloom_delay = Seconds( unqlite_value_to_double( val ) );
    if( unqlite_value_is_int( val ) )
        bloom_delay = Seconds( unqlite_value_to_int64( val ) );
 
    val = unqlite_vm_extract_variable( vm, "contents" );
    if( val && unqlite_value_is_json_array( val ) )
    {
        size_t count = unqlite_array_count( val );
        for( size_t i = 0 ; i < count ; i++ )
        {
            stringstream ss;
            string key;
            ss << i;
            ss >> key;
            unqlite_value* elem = unqlite_array_fetch
                                  ( val, key.c_str(), key.size() );
            
            if( elem && unqlite_value_is_json_object( elem ) )
            {
                unqlite_value* name_val = unqlite_array_fetch
                                      ( elem, "name", -1 );
                unqlite_value* size_val = unqlite_array_fetch
                                      ( elem, "size", -1 );
                unqlite_value* access_val =
                    unqlite_array_fetch( elem, "access_level", -1 );
                if( name_val && size_val && access_val
                  && unqlite_value_is_string( name_val )
                  && unqlite_value_is_int( size_val )
                  && unqlite_value_is_int( access_val ) )
                {
                    str = unqlite_value_to_string( name_val, &len );
                    size_t size = unqlite_value_to_int( size_val );
                    uint8_t access_level = unqlite_value_to_int
                                           ( access_val );
                    contents.emplace( string( str, len ),
                                      Config::Content
                                      { size, access_level } );
                }
            }
        }
    }
 
    
    // release the vm
    unqlite_vm_release( vm );
    
    // close the db
    unqlite_close( db );
}

}

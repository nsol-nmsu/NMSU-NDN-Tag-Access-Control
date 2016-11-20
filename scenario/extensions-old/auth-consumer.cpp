#include "auth-consumer.hpp"
#include "logger.hpp"

namespace ndntac {

using namespace std;
using namespace ndn;
using namespace ns3;
using namespace ns3::ndn;

string
AuthConsumer::s_config = "config/consumer_config.jx9";

ns3::TypeId
AuthConsumer::GetTypeId( void )
{
    static ns3::TypeId tid =
        ns3::TypeId( "ndntac::AuthConsumer" )
        .SetGroupName( "Ndn" )
        .SetParent< WindowConsumer >()
        .template AddConstructor< AuthConsumer >();
    return tid;
}

bool
AuthConsumer::EnsureAuth( void )
{
    if( m_auth_tag != NULL && !m_auth_tag->isExpired() )
        return true;

    if( !m_pending_auth )
    {
        if( m_auth_event.IsRunning() )
            ns3::Simulator::Remove( m_auth_event );
        m_auth_event = ns3::Simulator::ScheduleNow
                       ( &AuthConsumer::SendAuthRequest, this );
        m_pending_auth = true;
        m_auth_timeout = ns3::Simulator::Now() + ns3::Seconds( 0.5 );
        return false;
    }

    if( ns3::Simulator::Now() > m_auth_timeout )
    {
        m_pending_auth = false;
        EnsureAuth();
    }

    return false;
}

void
AuthConsumer::SendAuthRequest( void )
{
  auto interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, numeric_limits<uint32_t>::max()));
  interest->setName(m_interestName.getPrefix( 1 )
                                  .append("AUTH_TAG")
                                  .appendNumber( m_instance_id ) );
  ::ndn::time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);
  interest->setAuthTag( AuthTag( 0 ) );
  interest->setRouteTracker( RouteTracker() );

  WillSendOutInterest(0);

  m_transmittedInterests(interest, this, m_face);
  m_face->onReceiveInterest(*interest);
  
  logRequestedAuth( *interest );
}

void
AuthConsumer::SendPacket( void )
{
    if( EnsureAuth() )
    {
         WindowConsumer::SendPacket();
    }
    else
    {
        Simulator::Schedule( ns3::Seconds( 0.2 ),
                             &WindowConsumer::SendPacket,
                             this );
    }
}

void
AuthConsumer::OnData( shared_ptr< const Data > data )
{
   if( data->getContentType() == tlv::ContentType_Auth )
    {
        const Block& payload = data->getContent().blockFromValue();
        m_auth_tag = make_shared<AuthTag>( payload );
        m_pending_auth = false;
        logReceivedAuth( *m_auth_tag );
    }
    else
    {
         WindowConsumer::OnData( data );
    }
}

void
AuthConsumer::WillSendOutInterest( shared_ptr< Interest > interest )
{
    if( m_auth_tag != NULL )
        interest->setAuthTag( *m_auth_tag );
}

void
AuthConsumer::Reset( void )
{
    m_auth_tag = NULL;
    WindowConsumer::Reset();
}


void
AuthConsumer::logRequestedAuth( const Interest& interest ) const
{
    if( !shouldLogRequestedAuth() )
        return;

  static Log* log = Logger::getInstance( "simulation.log.udb" ).makeLog(
                            "Consumer",
                            "{ 'interest-name' : $interest_name, "
                            "  'what'          : $what, "
                            "  'who'           : $who  }" );
  log->set( "interest_name", interest.getName().toUri() );
  log->set( "what", string("RequestedAuth") );
  log->set( "who", (int64_t)m_instance_id );
  log->write();
}

void
AuthConsumer::logReceivedAuth( const AuthTag& auth ) const
{
    if( !shouldLogReceivedAuth() )
        return;

      static Log* log = Logger::getInstance( "simulation.log.udb" ).makeLog(
                                "Consumer",
                                "{ 'auth-prefix'   : $auth_prefix, "
                                "  'auth-access'   : $auth_access, "
                                "  'auth-expired'  : $auth_expired, "
                                "  'what'          : $what, "
                                "  'who'           : $who  }" );
      log->set( "auth_prefix", auth.getPrefix().toUri() );
      log->set( "auth_access", (int64_t)auth.getAccessLevel() );
      log->set( "auth_expired", auth.isExpired() );
      log->set( "what", string("ReceivedAuth") );
      log->set( "who", (int64_t)m_instance_id );
      log->write();
}

// these can be modified to control what gets logged
bool
AuthConsumer::shouldLogReceivedAuth( void ) const
{
    return true;
}

bool
AuthConsumer::shouldLogRequestedAuth( void ) const
{
    return true;
}

AuthConsumer::Config::Config( const string& file )
{
    // set default values
    enable_no_auth = false;
    enable_bad_auth_sig = false;
    enable_expired_auth = false;
    enable_bad_auth_route = false;
    enable_bad_auth_prefix = false;
    enable_bad_auth_keyloc = false;
    enable_trace = false;
    trace_interval = Seconds(10);
    
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
    unqlite_value* val;
    
    val = unqlite_vm_extract_variable( vm, "enable_no_auth" );
    if( val && unqlite_value_is_bool( val ) )
        enable_no_auth = unqlite_value_to_bool( val );
    
    val = unqlite_vm_extract_variable( vm, "enable_bad_auth_sig" );
    if( val && unqlite_value_is_bool( val ) )
        enable_bad_auth_sig = unqlite_value_to_bool( val );
    
    val = unqlite_vm_extract_variable( vm, "enable_expired_auth" );
    if( val && unqlite_value_is_bool( val ) )
        enable_expired_auth = unqlite_value_to_bool( val );
    
    val = unqlite_vm_extract_variable( vm, "enable_bad_auth_route" );
    if( val && unqlite_value_is_bool( val ) )
        enable_bad_auth_route = unqlite_value_to_bool( val );
    
    val = unqlite_vm_extract_variable( vm, "enable_bad_auth_prefix" );
    if( val && unqlite_value_is_bool( val ) )
        enable_bad_auth_prefix = unqlite_value_to_bool( val );
    
    val = unqlite_vm_extract_variable( vm, "enable_bad_auth_keyloc" );
    if( val && unqlite_value_is_bool( val ) )
        enable_bad_auth_keyloc = unqlite_value_to_bool( val );
    
    val = unqlite_vm_extract_variable( vm, "enable_trace" );
    if( unqlite_value_is_bool( val ) )
        enable_trace = unqlite_value_to_bool( val );
    
    val = unqlite_vm_extract_variable( vm, "trace_interval" );
    if( unqlite_value_is_float( val ) )
        trace_interval = Seconds( unqlite_value_to_double( val ) );
    
    val = unqlite_vm_extract_variable( vm, "contents" );
    if( val && unqlite_value_is_json_array( val ) )
    {
        size_t count = unqlite_array_count( val );
        stringstream ss;
        string key;
        for( size_t i = 0 ; i < count ; i++ )
        {
            ss << i;
            ss >> key;
            unqlite_value* elem = unqlite_array_fetch( val, key.c_str(), key.size() );
            
            if( elem && unqlite_value_is_json_object( elem ) )
            {
                unqlite_value* name = unqlite_array_fetch( elem, "name", -1 );
                unqlite_value* popularity = unqlite_array_fetch( elem, "popularity", -1 );
                if( name && popularity
                  && unqlite_value_is_string( name )
                  && unqlite_value_is_int( popularity ) )
                {
                    const char* name_str;
                    int len;
                    name_str = unqlite_value_to_string( name, &len );
                    uint32_t pop = unqlite_value_to_int( popularity );
                    contents.emplace_back( string( name_str, len ), pop );
                }
            }
        }
    }
 
    
    // release the vm
    unqlite_vm_release( vm );
    
    // close the db
    unqlite_close( db );
}

} // namespace ndntac

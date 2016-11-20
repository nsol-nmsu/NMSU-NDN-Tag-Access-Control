#include "consumer.hpp"
#include "ns3/ndnSIM-module.h"
#include "utils/ndn-rtt-mean-deviation.hpp"
#include "ns3/ndnSIM/utils/dummy-keychain.hpp"

namespace ndntac
{

using namespace std;
using namespace ndn;
using namespace ns3;
using namespace ns3::ndn;

string Consumer::s_config = "config/consumer_config.jx9";
uint32_t Consumer::s_instance_id = 0;

TypeId
Consumer::GetTypeId()
{
    static TypeId
    tid = TypeId("ndntac::Consumer")
          .SetParent<ns3::ndn::App>()
          .AddConstructor<Consumer>()
          .AddTraceSource
          ( "InterestTrace",
            "Number of interests received",
            MakeTraceSourceAccessor( &Consumer::m_interest_trace ),
            "InterestTraceCallback" )
          .AddTraceSource
          ( "DataTrace",
            "Number of datas received",
            MakeTraceSourceAccessor( &Consumer::m_data_trace ),
            "DataTraceCallback" )
          .AddTraceSource
          ( "DeniedTrace",
            "Number of denied requests",
            MakeTraceSourceAccessor( &Consumer::m_data_trace ),
            "DeniedTraceCallback" )
          .AddTraceSource
          ( "DeservedTrace",
            "Number of denials received for valid auth tag",
            MakeTraceSourceAccessor( &Consumer::m_deserved_trace ),
            "DeservedTraceCallback" )
          .AddTraceSource
          ( "UndeservedTrace",
            "Number of packets received for bad interests",
            MakeTraceSourceAccessor( &Consumer::m_undeserved_trace ),
            "UndeservedTraceCallback" )
          .AddTraceSource
          ( "UnrequestedTrace",
            "Number of datas received that weren't requested",
            MakeTraceSourceAccessor(&Consumer::m_unrequested_trace),
            "UnrequestedTraceCallback" )
          .AddTraceSource
          ( "AuthTrace",
            "Number of Auth Tags consumed",
            MakeTraceSourceAccessor( &Consumer::m_auth_trace ),
            "AuthTraceCallback" )
          .AddTraceSource
          ( "RetxTrace",
            "Total number of interest retransmissions",
            MakeTraceSourceAccessor( &Consumer::m_retx_trace ),
            "RetxTraceCallback" )
          .AddTraceSource
          ( "TimeoutTrace",
            "Number of timeouts",
            MakeTraceSourceAccessor( &Consumer::m_retx_trace ),
            "TimeoutTraceCallback" )
          .AddTraceSource
          ( "PartialDelayTrace",
            "Delay between the last retx and data receipt",
            MakeTraceSourceAccessor(&Consumer::m_partial_delay_trace),
            "PartialDelayTraceCallback" )
          .AddTraceSource
          ( "TotalDelayTrace",
            "Delay between first request and data receipt",
            MakeTraceSourceAccessor(&Consumer::m_total_delay_trace),
            "TotalDelayTraceCallback" );
                           
    return tid;
}

Consumer::Consumer( void )
    : m_instance_id( s_instance_id++ )
    , m_config( s_config, m_instance_id )
    , m_seqno( 0 )
    , m_segno( 0 )
    , m_rtt( CreateObject<RttMeanDeviation>() )
    , m_pending_auth( false )
    , m_urng( CreateObject<UniformRandomVariable>() )
    , m_erng( CreateObject<ExponentialRandomVariable>() )
{
    
    // set initiali window
    m_window = m_config.initial_window_size;
    
    // exponential random variable and mean for gap selection
    m_erng->SetAttribute( "Mean", DoubleValue( m_config.exp_mean ) );
    m_erng->SetAttribute( "Bound", DoubleValue( m_config.exp_bound ) );
         
}

void
Consumer::StartApplication( void )
{
    App::StartApplication();
    
    // start a content
    StartContent( NextContent() );
    
    // start retx timer
    StartRetXTimer();
}

void
Consumer::StopApplication( void )
{
    App::StopApplication();
    if( m_retx_event.IsRunning() )
        Simulator::Cancel( m_retx_event );
}

void
Consumer::OnData( std::shared_ptr< const Data > data )
{
    App::OnData( data );
    
    // if the data was unrequested then update trace
    auto it = m_seqs.find( data->getName() );
    if( it == m_seqs.end() )
    {
        m_unrequested_trace++;
        return;
    }
    
    // store sequence number
    uint32_t seq = it->second;
    
    // get the associated request information
    PendingEntry& info = m_pending[seq];
    
    // update the partial delay trace
    m_partial_delay_trace = ( m_partial_delay_trace*m_data_trace
                            + Simulator::Now() - info.lx_time )
                            / ( m_data_trace + 1);
    
    // update the total delay trace
    m_total_delay_trace = ( m_partial_delay_trace*m_data_trace
                          + Simulator::Now() - info.fx_time )
                          / ( m_data_trace + 1);
    // update data trace
    m_data_trace++;
    
    // if nack then update nack traces
    if( data->getContentType() == tlv::ContentType_Nack )
    {
        m_denied_trace++;
        
        // if our auth tag is valid then we shouldn't
        // have received a nack, so update the trace for
        // the abnormality
        if( !m_config.enable_no_auth
          && !m_config.enable_bad_auth_sig
          && !m_config.enable_expired_auth
          && !m_config.enable_bad_route
          && !m_config.enable_bad_prefix
          && !m_config.enable_bad_keyloc
          && m_auth && !m_auth->isExpired() )
        {
            m_deserved_trace++;
        }
        
        // do data denied stuff
        OnRequestDenied( seq );
    }
    else
    {
    
        // if auth then update auth trace and do auth setup
        if( data->getContentType() == tlv::ContentType_Auth )
        {
            m_auth_trace++;
            OnReceiveAuth( data );
        }
        // else if EoC then select a new content
        if( data->getContentType() == tlv::ContentType_EoC )
        {
            StartContent( NextContent() );
        }
        
        // do on data satisfied stuff
        OnRequestSatisfied( seq );
    }
    
    // increment window size if below max
    if( m_window < m_config.max_window_size )
        m_window++;
    
    // refill the window
    FillWindow();
}

void
Consumer::OnReceiveAuth( std::shared_ptr< const Data > data )
{
    if( m_config.enable_no_auth )
        return;
    
    m_auth.reset( new AuthTag( data->getContent().blockFromValue() ) );
    
    if( m_config.enable_bad_auth_sig )
        m_auth->setSignature( security::DUMMY_NDN_BAD_SIGNATURE );
    
    if( m_config.enable_expired_auth )
        m_auth->setExpirationTime( ::ndn::time::system_clock::now() );
    
    if( m_config.enable_bad_route )
        m_auth->setRouteHash( 0 );
    
    if( m_config.enable_bad_prefix )
        m_auth->setPrefix( "bad/prefix" );
    
    if( m_config.enable_bad_keyloc )
        m_auth->setKeyLocator( KeyLocator( "dummy/loctor" ) );
}

void
Consumer::OnRequestSatisfied( uint32_t seq )
{
    auto it = m_pending.find( seq );
    BOOST_ASSERT( it != m_pending.end() );
    
    Name name = it->second.interest->getName();
    auto it2 = m_seqs.find( name );
    BOOST_ASSERT( it != m_pending.end() );
    
    m_pending.erase( it );
    m_seqs.erase( it2 );
    
    // notify the rtt estimator
    m_rtt->AckSeq( SequenceNumber32(seq) );
}

void
Consumer::OnRequestDenied( uint32_t seq )
{
    // if denied because of expired tag then reauthenticate
    // and retransmit request
    if( m_auth && m_auth->isExpired() )
    {
        m_retx_queue.push( seq );
    }
    else
    {
        auto it = m_pending.find( seq );
        BOOST_ASSERT( it != m_pending.end() );
        
        Name name = it->second.interest->getName();
        auto it2 = m_seqs.find( name );
        BOOST_ASSERT( it != m_pending.end() );
        
        m_pending.erase( it );
        m_seqs.erase( it2 );
        
        // notify rtt estimator
        m_rtt->AckSeq( SequenceNumber32(seq) );
    }
}

void
Consumer::OnTimeout( uint32_t seq )
{
    // update trace
    m_timeout_trace++;
    
    // increment rtt
    m_rtt->IncreaseMultiplier();
    
    // add sequnce to retx queue
    m_retx_queue.push( seq );
    
    // set window to initial size
    m_window = m_config.initial_window_size;
    
    // ensure window fill
    FillWindow();
}

const Name&
Consumer::NextContent( void )
{
    double psum = 0;
    for( auto it = m_config.contents.begin()
       ; it != m_config.contents.end()
       ; it++ )
    {
        psum += it->prob;
    }
    
    double rn = m_urng->GetValue( 0, psum );
    double bound = 0;
    for( auto it = m_config.contents.begin()
       ; it != m_config.contents.end()
       ; it++ )
    {
        bound += it->prob;
        if( rn <= bound )
            return it->name;
    }
}

Time
Consumer::NextGap( void )
{
    return Seconds( m_erng->GetValue() );
}

void
Consumer::StartContent( const Name& content )
{
    m_content_name = content;
    
    Time now = Simulator::Now();
    if( now >= m_config.start_time )
        Simulator::Schedule( NextGap(), &Consumer::FillWindow, this );
    else
        Simulator::Schedule( m_config.start_time - now ,
                             &Consumer::FillWindow, this );
}

void
Consumer::FillWindow( void )
{
    while( m_pending.size() < m_window )
        SendNext();
}

void
Consumer::SendNext( void )
{   
    // if we have authentication then either retransmit
    // from the retx queue or set the name to the next
    // packet of the content
    Name name;
    if( HasAuth() )
    {
        // if the retx queue isn't empty then we retransmit
        if( !m_retx_queue.empty() )
        {
            RetX( m_retx_queue.front() );
            m_retx_queue.pop();
            return;
        }
        
        // otherwise we set the name to the next segment of
        // the current content
        name = m_content_name;
        name.appendSegment( m_segno );
    }
    // otherwise set the name to request authentication
    else
    {
        name = m_content_name;
        name.append( "AUTH_TAG" );
    }
    
    // make and configure interest
    uint32_t
    nonce( m_urng->GetValue( 0, numeric_limits<uint32_t>::max() ) );
    
    ::ndn::time::milliseconds
    lifetime( m_config.interest_lifetime.GetMilliSeconds() );
    
    auto interest = make_shared<Interest>( name );
    interest->setNonce( nonce );
    interest->setInterestLifetime( lifetime );
    interest->setRouteTracker( RouteTracker() );
    if( m_auth )
        interest->setAuthTag( *m_auth );
    
    // add to m_pending and m_seq
    PendingEntry info;
    info.interest      = interest;
    info.fx_time        = Simulator::Now();
    info.lx_time        = Simulator::Now();
    info.retx_timeout   = m_rtt->RetransmitTimeout();
    m_pending[m_seqno]  = info;
    m_seqs[name] = m_seqno;
    
    // queue interest
    m_tx_queue.receiveInterest( m_face, interest );
    
    // notify m_rtt
    m_rtt->SentSeq( SequenceNumber32(m_seqno), 1 );
    
    // update trace
    m_interest_trace++;
    
    // increment counters
    m_seqno++;
    m_segno++;
}

void
Consumer::RetX( uint32_t seq )
{
    // adjust entry
    PendingEntry& info = m_pending[seq];
    info.lx_time = Simulator::Now();
    info.retx_timeout = m_rtt->RetransmitTimeout();
    
    // requeue
    m_tx_queue.receiveInterest( m_face,
                                info.interest );
    
    // notify m_rtt
    m_rtt->SentSeq( SequenceNumber32(m_seqno), 1 );
    
    // update trace
    m_retx_trace++;
}

void
Consumer::CheckTimeouts( void )
{
    for( auto it = m_pending.begin()
       ; it != m_pending.end()
       ; it++ )
    {
        PendingEntry& info = it->second;
        if( Simulator::Now() > info.lx_time + info.retx_timeout )
        {
            OnTimeout( it->first );
        }
    }
}

bool
Consumer::HasAuth( void )
{
    return m_config.enable_no_auth
           || ( m_auth
                && !m_auth->isExpired()
                && m_auth->getPrefix().isPrefixOf( m_content_name ) );
}

void
Consumer::StartRetXTimer( void )
{
    Consumer::CheckTimeouts();
    
    m_retx_event = Simulator::Schedule( m_config.retx_timer,
                                        &Consumer::StartRetXTimer,
                                        this );
}

Consumer::Config::Config( const string& file, uint32_t id )
{
    // set default values
    start_time = Seconds( 0 );
    interest_lifetime = Seconds( 2 );
    retx_timer = Seconds( 2 );
    initial_window_size = 5;
    max_window_size = 50;
    exp_mean = 2;
    exp_bound = 10;
    enable_no_auth = false;
    enable_bad_auth_sig = false;
    enable_expired_auth = false;
    enable_bad_route = false;
    enable_bad_prefix = false;
    enable_bad_keyloc = false;
    
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
    
    // export id to vm
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

    val = unqlite_vm_extract_variable( vm, "start_time" );
    if( unqlite_value_is_float( val ) )
        start_time = Seconds( unqlite_value_to_double( val ) );

    val = unqlite_vm_extract_variable( vm, "interest_lifetime" );
    if( unqlite_value_is_float( val ) )
       interest_lifetime = Seconds( unqlite_value_to_double( val ) );
        
    val = unqlite_vm_extract_variable( vm, "retx_timer" );
    if( unqlite_value_is_float( val ) )
        retx_timer = Seconds( unqlite_value_to_double( val ) );
    
    val = unqlite_vm_extract_variable( vm, "initial_window_size" );
    if( val && unqlite_value_is_int( val ) )
        initial_window_size = unqlite_value_to_int64( val );

    val = unqlite_vm_extract_variable( vm, "max_window_size" );
    if( unqlite_value_is_int( val ) )
        initial_window_size = unqlite_value_to_int64( val );
    
    val = unqlite_vm_extract_variable( vm, "exp_mean" );
    if( val && unqlite_value_is_float( val ) )
        exp_mean = unqlite_value_to_double( val );
    
    val = unqlite_vm_extract_variable( vm, "exp_bound" );
    if( unqlite_value_is_float( val ) )
        exp_bound = unqlite_value_to_double( val );
    
    val = unqlite_vm_extract_variable( vm, "enable_no_auth" );
    if( unqlite_value_is_bool( val ) )
        enable_no_auth = unqlite_value_to_bool( val );

    val = unqlite_vm_extract_variable( vm, "enable_bad_auth_sig" );
    if( unqlite_value_is_bool( val ) )
        enable_bad_auth_sig = unqlite_value_to_bool( val );

    val = unqlite_vm_extract_variable( vm, "enable_expired_auth" );
    if( unqlite_value_is_bool( val ) )
        enable_expired_auth = unqlite_value_to_bool( val );

    val = unqlite_vm_extract_variable( vm, "enable_bad_route" );
    if( unqlite_value_is_bool( val ) )
        enable_bad_route = unqlite_value_to_bool( val );

    val = unqlite_vm_extract_variable( vm, "enable_bad_prefix" );
    if( unqlite_value_is_bool( val ) )
        enable_bad_prefix = unqlite_value_to_bool( val );

    val = unqlite_vm_extract_variable( vm, "enable_bad_keyloc" );
    if( unqlite_value_is_bool( val ) )
        enable_bad_keyloc = unqlite_value_to_bool( val );
        
    
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
            unqlite_value* elem =
                unqlite_array_fetch( val, key.c_str(), key.size() );
            
            if( elem && unqlite_value_is_json_object( elem ) )
            {
                unqlite_value* name_val =
                    unqlite_array_fetch( elem, "name", -1 );
                unqlite_value* prob_val =
                    unqlite_array_fetch( elem, "prob", -1 );
                if( name_val && prob_val
                  && unqlite_value_is_string( name_val )
                  && unqlite_value_is_float( prob_val ) )
                {
                    str = unqlite_value_to_string( name_val, &len );
                    string name( str, len );
                    double prob = unqlite_value_to_double( prob_val );
                    contents.emplace_back
                             ( Config::Content{ name, prob } );
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

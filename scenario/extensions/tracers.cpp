#include "tracers.hpp"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/ndnSIM/utils/tracers/l2-rate-tracer.hpp"
#include "ns3/ndnSIM/utils/tracers/ndn-l3-rate-tracer.hpp"

namespace ndntac
{
namespace tracers
{


using namespace ndn;
using namespace ns3;
using namespace ns3::ndn;
using namespace std;

TypeId
ProducerTrace::GetTypeId( void )
{
  static ns3::TypeId tid
      = ns3::TypeId("ndntac::ProducerTrace")
        .SetParent<Object>()
        .AddConstructor<ProducerTrace>()
        .AddTraceSource( "TagCreatedTrace",
                         "Invoked when the producer creates an "
                         " auth tag",
                         MakeTraceSourceAccessor
                         ( &ProducerTrace::tag_created ),
                         "TagCreatedTrace" )
        .AddTraceSource( "SigVerifTrace",
                         "Invoked when the producer verifies "
                         "a signature",
                         MakeTraceSourceAccessor
                         ( &ProducerTrace::sigverif ),
                         "SigVerifTrace" )
        .AddTraceSource( "BloomLookupTrace",
                         "Called when the producer performs"
                         " a bloom lookup",
                         MakeTraceSourceAccessor
                         ( &ProducerTrace::bloom_lookup ),
                         "BloomLookupTrace" )
        .AddTraceSource( "BloomInsertTrace",
                         "Called when the producer performs a "
                         "bloom insertion",
                         MakeTraceSourceAccessor
                         ( &ProducerTrace::bloom_insert ),
                         "BloomInsertTrace" )
        .AddTraceSource( "ValidationTrace",
                         "Called when the producer attempts a "
                         "tag validation",
                         MakeTraceSourceAccessor
                         ( &ProducerTrace::validation ),
                         "ValidationTrace" )
        .AddTraceSource( "ReceivedInterestTrace",
                         "Called when an interest is received",
                         MakeTraceSourceAccessor
                         ( &ProducerTrace::received_interest ),
                         "ReceivedInterestTrace" )
       .AddTraceSource( "SentDataTrace",
                        "Called when a data is sent",
                        MakeTraceSourceAccessor
                        ( &ProducerTrace::sent_data ),
                        "SentDataTrace" );
       return tid;
};
Ptr< ProducerTrace > producer;

TypeId
ConsumerTrace::GetTypeId( void )
{
  static ns3::TypeId tid
      = ns3::TypeId("ndntac::ConsumerTrace")
        .SetParent<Object>()
        .AddConstructor<ConsumerTrace>()
        .AddTraceSource( "InterestTrace",
                         "Interest sent",
                         MakeTraceSourceAccessor
                         ( &ConsumerTrace::interest ),
                         "InterestTrace" )
        .AddTraceSource( "DataTrace",
                         "Received data",
                         MakeTraceSourceAccessor
                         ( &ConsumerTrace::data ),
                         "DataTrace" )
        .AddTraceSource( "DeniedTrace",
                         "Received nack",
                         MakeTraceSourceAccessor
                         ( &ConsumerTrace::denied ),
                         "DeniedTrace" )
        .AddTraceSource( "DeservedTrace",
                         "Received nack when had valid auth",
                         MakeTraceSourceAccessor
                         ( &ConsumerTrace::deserved ),
                         "DeservedTrace" )
        .AddTraceSource( "UndeservedTrace",
                         "Received data when had invalid auth",
                         MakeTraceSourceAccessor
                         ( &ConsumerTrace::undeserved ),
                         "UndeservedTrace" )
        .AddTraceSource( "UnrequestedTrace",
                         "Received unrequested data",
                         MakeTraceSourceAccessor
                         ( &ConsumerTrace::unrequested ),
                         "UnrequestedTrace" )
       .AddTraceSource( "AuthReceivedTrace",
                        "AuthTag received",
                        MakeTraceSourceAccessor
                        ( &ConsumerTrace::auth_received ),
                        "AuthReceivedTrace" )
       .AddTraceSource( "AuthDisposedTrace",
                        "AuthTag disposed",
                        MakeTraceSourceAccessor
                        ( &ConsumerTrace::auth_disposed ),
                        "AuthDisposedTrace" )
       .AddTraceSource( "RetXTrace",
                        "Retransmission",
                        MakeTraceSourceAccessor
                        ( &ConsumerTrace::retx ),
                        "RetXTrace" )
       .AddTraceSource( "TimeoutTrace",
                        "Timeout",
                        MakeTraceSourceAccessor
                        ( &ConsumerTrace::timeout ),
                        "TimeoutTrace" );
       return tid;
};
Ptr< ConsumerTrace > consumer;

TypeId
RouterTrace::GetTypeId( void )
{
  static ns3::TypeId tid
      = ns3::TypeId("ndntac::RouterTrace")
        .SetParent<Object>()
        .AddConstructor<RouterTrace>()
        .AddTraceSource( "SigVerifTrace",
                         "Invoked when the router verifies "
                         "a signature",
                         MakeTraceSourceAccessor
                         ( &RouterTrace::sigverif ),
                         "SigVerifTrace" )
        .AddTraceSource( "BloomLookupTrace",
                         "Called when the router performs"
                         " a bloom lookup",
                         MakeTraceSourceAccessor
                         ( &RouterTrace::bloom_lookup ),
                         "BloomLookupTrace" )
        .AddTraceSource( "BloomInsertTrace",
                         "Called when the router performs a "
                         "bloom insertion",
                         MakeTraceSourceAccessor
                         ( &RouterTrace::bloom_insert ),
                         "BloomInsertTrace" )
        .AddTraceSource( "ValidationTrace",
                         "Called when the router attempts a "
                         "tag validation",
                         MakeTraceSourceAccessor
                         ( &RouterTrace::validation ),
                         "ValidationTrace" )
        .AddTraceSource( "SentInterestTrace",
                         "Called when are router forwards an interest",
                         MakeTraceSourceAccessor
                         ( &RouterTrace::sent_interest ),
                         "SentInterestTrace" )
       .AddTraceSource( "SentDataTrace",
                        "Called when a data is sent",
                        MakeTraceSourceAccessor
                        ( &RouterTrace::sent_data ),
                        "SentDataTrace" );
       return tid;
};
Ptr< RouterTrace > router;


TypeId
EdgeTrace::GetTypeId( void )
{
  static ns3::TypeId tid
      = ns3::TypeId("ndntac::EdgeTrace")
        .SetParent<Object>()
        .AddConstructor<EdgeTrace>()
        .AddTraceSource( "SigVerifTrace",
                         "Invoked when the edge verifies "
                         "a signature",
                         MakeTraceSourceAccessor
                         ( &EdgeTrace::sigverif ),
                         "SigVerifTrace" )
        .AddTraceSource( "BloomLookupTrace",
                         "Called when the router performs"
                         " a bloom lookup",
                         MakeTraceSourceAccessor
                         ( &EdgeTrace::bloom_lookup ),
                         "BloomLookupTrace" )
        .AddTraceSource( "BloomInsertTrace",
                         "Called when the router performs a "
                         "bloom insertion",
                         MakeTraceSourceAccessor
                         ( &EdgeTrace::bloom_insert ),
                         "BloomInsertTrace" )
        .AddTraceSource( "DataEnteredTrace",
                         "Data entered the network",
                         MakeTraceSourceAccessor
                         ( &EdgeTrace::data_entered ),
                         "DataEnteredTrace" )
        .AddTraceSource( "DataLeftTrace",
                         "Data left the network",
                         MakeTraceSourceAccessor
                         ( &EdgeTrace::data_left ),
                         "DataLeftTrace" )
       .AddTraceSource( "InterestEnteredTrace",
                        "Interest entered the network",
                        MakeTraceSourceAccessor
                        ( &EdgeTrace::interest_entered ),
                        "InterestEnteredTrace" )
       .AddTraceSource( "InterestLeftTrace",
                        "Interest left the network",
                        MakeTraceSourceAccessor
                        ( &EdgeTrace::interest_left ),
                        "InterestLeftTrace" )
       .AddTraceSource( "AuthCachedTrace",
                        "Interest entered the network",
                        MakeTraceSourceAccessor
                        ( &EdgeTrace::auth_cached ),
                        "AuthCachedTrace" )
       .AddTraceSource( "BlockedInterestTrace",
                        "Blocked interest from network",
                        MakeTraceSourceAccessor
                        ( &EdgeTrace::blocked_interest ),
                        "BlockedInterestTrace" );
       return tid;
};
Ptr< EdgeTrace > edge;

// trace streams
ofstream tags_created_trace_stream;
ofstream tags_active_trace_stream;
ofstream tag_sigverif_trace_stream;
ofstream tag_bloom_trace_stream;
ofstream overhead_trace_stream;
//ofstream drop_trace_stream;
//ofstream rate_trace_stream;
ofstream validation_trace_stream;
ofstream transmission_trace_stream;
ofstream edgeblock_trace_stream;
ofstream consumer_trace_stream;

// intervals
Time tags_created_trace_interval;
Time tags_active_trace_interval;
Time tag_sigverif_trace_interval;
Time tag_bloom_trace_interval;
Time overhead_trace_interval;
// Time drop_trace_interval;
// Time rate_trace_interval;
Time validation_trace_interval;
Time transmission_trace_interval;
Time edgeblock_trace_interval;
Time consumer_trace_interval;

// logger even ids
EventId tags_created_event;
EventId tags_active_event;
EventId tag_sigverif_event;
EventId tag_bloom_event;
EventId overhead_event;
EventId validation_event;
EventId transmission_event;
EventId edgeblock_event;
EventId consumer_event;

// trackers
uint64_t tags_created = 0;
uint64_t tags_active = 0;
uint64_t interests_transmitted = 0;
uint64_t datas_transmitted = 0;
uint64_t tags_transmitted = 0;
uint64_t interest_bytes_transmitted = 0;
uint64_t data_bytes_transmitted = 0;
uint64_t tag_bytes_transmitted = 0;
uint64_t sigverifs = 0;
Time     sigverifs_delay = Seconds( 0 );
uint64_t bloom_lookups = 0;
Time     bloom_lookups_delay = Seconds( 0 );
uint64_t bloom_inserts = 0;
Time     bloom_inserts_delay = Seconds( 0 );
uint64_t validations_success_valprob = 0;
uint64_t validations_success_bloom = 0;
uint64_t validations_success_sig = 0;
uint64_t validations_success_skipped = 0;
uint64_t validations_failure_sig = 0;
uint64_t validations_failure_noauth = 0;
uint64_t validations_failure_lowauth = 0;
uint64_t validations_failure_badkeyloc = 0;
uint64_t validations_failure_expired = 0;
uint64_t validations_failure_badprefix = 0;
uint64_t validations_failure_badroute = 0;
uint64_t authcached_positive_upstream = 0;
uint64_t authcached_positive_moved = 0;
uint64_t authcached_negative_upstream = 0;
uint64_t edge_blocked_expired = 0;
uint64_t edge_blocked_bad_prefix = 0;
uint64_t edge_blocked_bad_route = 0;
uint64_t consumer_denied = 0;
uint64_t consumer_deserved = 0;
uint64_t consumer_undeserved = 0;
uint64_t consumer_unrequested = 0;
uint64_t consumer_retx = 0;
uint64_t consumer_timeout = 0;
uint64_t consumer_received = 0;
uint64_t consumer_requested = 0;
Time     consumer_partial_delay = Seconds( 0 );
Time     consumer_delay = Seconds( 0 );


// callbacks
void
ProducerTagCreatedCallback
( const AuthTag& tag )
{
    tags_created++;
}

void
ProducerSigVerifCallback
( const AuthTag& tag, Time delay )
{
    sigverifs++;
    sigverifs_delay += delay;
}

void
ProducerBloomLookupCallback
( const AuthTag& tag, Time delay )
{
    bloom_lookups++;
    bloom_lookups_delay += delay;
}

void
ProducerBloomInsertCallback
( const AuthTag& tag, Time delay )
{
    bloom_inserts++;
    bloom_inserts_delay += delay;
}

void
ProducerValidationCallback
( ValidationDetail detail )
{
    switch( detail )
    {
        case ValidationSuccessValProb:
            validations_success_valprob++;
            break;
        case ValidationSuccessBloom:
            validations_success_bloom++;
            break;
        case ValidationSuccessSig:
            validations_success_sig++;
            break;
        case ValidationSuccessSkipped:
            validations_success_skipped++;
            break;
        case ValidationFailureSig:
            validations_failure_sig++;
            break;
        case ValidationFailureNoAuth:
            validations_failure_noauth++;
            break;
        case ValidationFailureLowAuth:
            validations_failure_lowauth++;
            break;
        case ValidationFailureBadKeyLoc:
            validations_failure_badkeyloc++;
            break;
        case ValidationFailureExpired:
            validations_failure_expired++;
            break;
        case ValidationFailureBadPrefix:
            validations_failure_badprefix++;
            break;
        case ValidationFailureBadRoute:
            validations_failure_badroute++;
            break;
    }
}

void
ProducerReceivedInterestCallack
( const Interest& interest )
{
    // nada
}

void
ProducerSentDataCallback
( const Data& data )
{
    datas_transmitted++;
    data_bytes_transmitted += data.wireEncode().size();
}

void
ConsumerSentInterestCallback
( const Interest& interest )
{
    consumer_requested++;
    interests_transmitted++;
    interest_bytes_transmitted += interest.wireEncode().size();
    
    if( interest.hasAuthTag() )
    {
        tags_transmitted++;
        tag_bytes_transmitted +=
            interest.getAuthTag().wireEncode().size();
    }
}

void
ConsumerReceivedDataCallback
( const Data& data, Time sent, Time last_retx, Time received )
{
    consumer_received++;
    consumer_partial_delay += ( received - last_retx );
    consumer_delay += ( received - sent );
}

void
ConsumerDeniedCallback
( uint32_t seq )
{
    consumer_denied++;
}

void
ConsumerDeservedCallback
( uint32_t seq )
{
    consumer_deserved++;
}

void
ConsumerUndeservedCallback
( uint32_t seq )
{
    consumer_undeserved++;
}

void
ConsumerUnrequestedCallback
( void )
{
    consumer_unrequested++;
}

void
ConsumerReceivedAuthCallback
( const AuthTag& tag )
{
    tags_active++;
}

void
ConsumerDisposedAuthCallback
( const AuthTag& tag )
{
    tags_active--;
}

void
ConsumerRetXCallback
( uint32_t seq )
{
    consumer_retx++;
}

void
ConsumerTimeoutCallback
( uint32_t seq )
{
    consumer_timeout++;
}

void
RouterSigVerifCallback
( const AuthTag& tag, Time delay )
{
    sigverifs++;
    sigverifs_delay += delay;
}

void
RouterBloomLookupCallback
( const AuthTag& tag, Time delay )
{
    bloom_lookups++;
    bloom_lookups_delay += delay;
}

void
RouterBloomInsertCallback
( const AuthTag& tag, Time delay )
{
    bloom_inserts++;
    bloom_inserts_delay += delay;
}

void
RouterValidationCallback
( ValidationDetail detail )
{
    switch( detail )
    {
        case ValidationSuccessValProb:
            validations_success_valprob++;
            break;
        case ValidationSuccessBloom:
            validations_success_bloom++;
            break;
        case ValidationSuccessSig:
            validations_success_sig++;
            break;
        case ValidationSuccessSkipped:
            validations_success_skipped++;
            break;
        case ValidationFailureSig:
            validations_failure_sig++;
            break;
        case ValidationFailureNoAuth:
            validations_failure_noauth++;
            break;
        case ValidationFailureLowAuth:
            validations_failure_lowauth++;
            break;
        case ValidationFailureBadKeyLoc:
            validations_failure_badkeyloc++;
            break;
        case ValidationFailureExpired:
            validations_failure_expired++;
            break;
        case ValidationFailureBadPrefix:
            validations_failure_badprefix++;
            break;
        case ValidationFailureBadRoute:
            validations_failure_badroute++;
            break;
    }
}

void
RouterSentInterestCallback
( const Interest& interest )
{
    interests_transmitted++;
    interest_bytes_transmitted += interest.wireEncode().size();
    
    if( interest.hasAuthTag() )
    {
        tags_transmitted++;
        tag_bytes_transmitted +=
            interest.getAuthTag().wireEncode().size();
    }
}

void
RouterSentDataCallback
( const Data& data )
{
    datas_transmitted++;
    data_bytes_transmitted += data.wireEncode().size();
}

void
EdgeDataEnteredCallback
( const Data& data )
{
    // NADA
}

void
EdgeDataLeftCallback
( const Data& data )
{
    // NADA
}

void
EdgeInterestEnteredCallback
( const Interest& interest )
{
    // NADA
}

void
EdgeInterestLeftCallback
( const Interest& interest )
{
    // NADA
}

void
EdgeAuthCachedCallback
( const AuthTag& tag, AuthCachedDetail detail )
{
    // NADA
}

void
EdgeBlockedInterestCallback
( const Interest& interest, BlockedDetail detail )
{
    switch( detail )
    {
        case BlockedExpired:
            edge_blocked_expired++;
            break;
        case BlockedBadPrefix:
            edge_blocked_bad_prefix++;
            break;
        case BlockedBadRoute:
            edge_blocked_bad_route++;
            break;
    }
}

void
EdgeSigVerifCallback
( const AuthTag& tag, Time delay )
{
    sigverifs++;
    sigverifs_delay += delay;
}

void
EdgeBloomLookupCallback
( const AuthTag& tag, Time delay )
{
    bloom_lookups++;
    bloom_lookups_delay += delay;
}

void
EdgeBloomInsertCallback
( const AuthTag& tag, Time delay )
{
    bloom_inserts++;
    bloom_inserts_delay += delay;
}

// loggers
void
TagsCreatedLogger( void )
{
    if( !tags_created_trace_stream.good() )
        return;

    tags_created_trace_stream
    << Simulator::Now() << '\t' << tags_created << endl;
    
    tags_created_event = Simulator::Schedule
                        ( tags_created_trace_interval,
                          &TagsCreatedLogger );
}

void
TagsActiveLogger( void )
{
    if( !tags_active_trace_stream.good() )
        return;

    tags_active_trace_stream
    << Simulator::Now() << '\t' << tags_active << endl;
    
    tags_active_event = Simulator::Schedule
                        ( tags_active_trace_interval,
                          &TagsActiveLogger );
}

void
TagSigVerifLogger( void )
{
    if( !tag_sigverif_trace_stream.good() )
        return;
    tag_sigverif_trace_stream
    << Simulator::Now() << '\t' << sigverifs
                        << '\t' << sigverifs_delay << endl;
    tag_sigverif_event = Simulator::Schedule
                         ( tag_sigverif_trace_interval,
                           &TagSigVerifLogger );
}

void
TagBloomLogger( void )
{
    if( !tag_bloom_trace_stream.good() )
        return;

    tag_bloom_trace_stream
    << Simulator::Now() << '\t' << bloom_lookups
                        << '\t' << bloom_lookups_delay
                        << '\t' << bloom_inserts
                        << '\t' << bloom_inserts_delay << endl;
    tag_bloom_event = Simulator::Schedule
                     ( tag_bloom_trace_interval, &TagBloomLogger );
}

void
OverheadLogger( void )
{
    if( !overhead_trace_stream.good() )
        return;

    Time delay = sigverifs_delay
               + bloom_lookups_delay
               + bloom_inserts_delay;

    // approximation
    uint64_t bytes = // 3 bytes for NoReCacheFlag
                     datas_transmitted*3
                     // 5 bytes for AuthValidityProb
                   + interests_transmitted*5
                     // auth tag overhead
                   + tag_bytes_transmitted;
    
    overhead_trace_stream
    << Simulator::Now() << '\t' << bytes << "bytes"
                        << '\t' << delay << endl;
    overhead_event = Simulator::Schedule
                    ( overhead_trace_interval, &OverheadLogger );
}

/*
void
DropLogger( void )
{

}*/
/*
void
RateLogger( void )
{

}*/

void
ValidationLogger( void )
{
    if( !validation_trace_stream.good() )
        return;

    uint64_t total_success = validations_success_sig
                           + validations_success_valprob
                           + validations_success_bloom
                           + validations_success_skipped;
   uint64_t total_failure  = validations_failure_sig
                           + validations_failure_noauth
                           + validations_failure_lowauth
                           + validations_failure_badkeyloc
                           + validations_failure_expired
                           + validations_failure_badprefix;

    validation_trace_stream
    << Simulator::Now()  << '\t' << total_success
                         << '\t' << validations_success_sig
                         << '\t' << validations_success_valprob
                         << '\t' << validations_success_bloom
                         << '\t' << validations_success_skipped
                         << '\t' << total_failure
                         << '\t' << validations_failure_sig
                         << '\t' << validations_failure_noauth
                         << '\t' << validations_failure_lowauth
                         << '\t' << validations_failure_badkeyloc
                         << '\t' << validations_failure_expired
                         << '\t' << validations_failure_badprefix
                         << endl;
    validation_event = Simulator::Schedule
                      ( validation_trace_interval,
                        &ValidationLogger );
}

void
TransmissionLogger( void )
{
    if( !transmission_trace_stream.good() )
        return;

    transmission_trace_stream
    << Simulator::Now() << '\t' << datas_transmitted 
                        << '\t' << data_bytes_transmitted
                        << '\t' << interests_transmitted
                        << '\t' << interest_bytes_transmitted
                        << '\t' << data_bytes_transmitted
                                 + interest_bytes_transmitted
                        << endl;
    transmission_event = Simulator::Schedule
                        ( transmission_trace_interval,
                          &TransmissionLogger );
}

void
EdgeBlockLogger( void )
{
    if( !edgeblock_trace_stream.good() )
        return;

    edgeblock_trace_stream
    << Simulator::Now() << '\t' << edge_blocked_expired
                        << '\t' << edge_blocked_bad_prefix
                        << '\t' << edge_blocked_expired
                                 + edge_blocked_bad_prefix
                        << endl;
    edgeblock_event = Simulator::Schedule
                     ( edgeblock_trace_interval, &EdgeBlockLogger );
}

void
ConsumerLogger( void )
{
    if( !consumer_trace_stream.good() )
        return;

    consumer_trace_stream
    << Simulator::Now() << '\t' << consumer_requested
                        << '\t' << consumer_received
                        << '\t' << consumer_denied
                        << '\t' << consumer_deserved
                        << '\t' << consumer_undeserved
                        << '\t' << consumer_unrequested
                        << '\t' << consumer_retx
                        << '\t' << consumer_timeout
                        << '\t' << consumer_partial_delay
                                   / consumer_received
                        << '\t' << consumer_delay
                                   / consumer_received
                        << endl;
    consumer_event = Simulator::Schedule
                     ( consumer_trace_interval, &ConsumerLogger );
}

// initialization and finalization
struct Setup
{
Setup( void )
{
    producer = CreateObject< ProducerTrace >();
    consumer = CreateObject< ConsumerTrace >();
    router   = CreateObject< RouterTrace >();
    edge     = CreateObject< EdgeTrace >();
    
    producer->TraceConnectWithoutContext
    ( "TagCreatedTrace",
      MakeCallback( &ProducerTagCreatedCallback ) );
    producer->TraceConnectWithoutContext
    ( "SigVerifTrace",
      MakeCallback( &ProducerSigVerifCallback ) );
    producer->TraceConnectWithoutContext
    ( "BloomLookupTrace",
      MakeCallback( &ProducerBloomLookupCallback ) );
    producer->TraceConnectWithoutContext
    ( "BloomInsertTrace",
      MakeCallback( &ProducerBloomInsertCallback ) );
    producer->TraceConnectWithoutContext
    ( "ValidationTrace",
      MakeCallback( &ProducerValidationCallback ) );
    producer->TraceConnectWithoutContext
    ( "ReceivedInterestTrace",
      MakeCallback( &ProducerReceivedInterestCallack ) );
    producer->TraceConnectWithoutContext
    ( "SentDataTrace",
      MakeCallback( &ProducerSentDataCallback ) );


    consumer->TraceConnectWithoutContext
    ( "InterestTrace",
      MakeCallback( &ConsumerSentInterestCallback ) );
    consumer->TraceConnectWithoutContext
    ( "DataTrace",
      MakeCallback( &ConsumerReceivedDataCallback ) );
    consumer->TraceConnectWithoutContext
    ( "DeniedTrace",
      MakeCallback( &ConsumerDeniedCallback ) );
    consumer->TraceConnectWithoutContext
    ( "DeservedTrace",
      MakeCallback( &ConsumerDeservedCallback ) );
    consumer->TraceConnectWithoutContext
    ( "UndeservedTrace",
      MakeCallback( &ConsumerUndeservedCallback ) );
    consumer->TraceConnectWithoutContext
    ( "UnrequestedTrace",
      MakeCallback( &ConsumerUnrequestedCallback ) );
    consumer->TraceConnectWithoutContext
    ( "AuthReceivedTrace",
      MakeCallback( &ConsumerReceivedAuthCallback ) );
    consumer->TraceConnectWithoutContext
    ( "AuthDisposedTrace",
      MakeCallback( &ConsumerDisposedAuthCallback ) );
    consumer->TraceConnectWithoutContext
    ( "RetXTrace",
      MakeCallback( &ConsumerRetXCallback ) );
    consumer->TraceConnectWithoutContext
    ( "TimeoutTrace",
      MakeCallback( &ConsumerTimeoutCallback ) );

    router->TraceConnectWithoutContext
    ( "SigVerifTrace",
      MakeCallback( &RouterSigVerifCallback ) );
    router->TraceConnectWithoutContext
    ( "BloomLookupTrace",
      MakeCallback( &RouterBloomLookupCallback ) );
    router->TraceConnectWithoutContext
    ( "BloomInsertTrace",
      MakeCallback( &RouterBloomInsertCallback ) );
    router->TraceConnectWithoutContext
    ( "ValidationTrace",
      MakeCallback( &RouterValidationCallback ) );
    router->TraceConnectWithoutContext
    ( "SentInterestTrace",
      MakeCallback( &RouterSentInterestCallback ) );
    router->TraceConnectWithoutContext
    ( "SentDataTrace",
      MakeCallback( &RouterSentDataCallback ) );
      
    edge->TraceConnectWithoutContext
    ( "SigVerifTrace",
      MakeCallback( &EdgeSigVerifCallback ) );
    edge->TraceConnectWithoutContext
    ( "BloomLookupTrace",
      MakeCallback( &EdgeBloomLookupCallback ) );
    edge->TraceConnectWithoutContext
    ( "BloomInsertTrace",
      MakeCallback( &EdgeBloomInsertCallback ) );
    edge->TraceConnectWithoutContext
    ( "DataEnteredTrace",
      MakeCallback( &EdgeDataEnteredCallback ) );
    edge->TraceConnectWithoutContext
    ( "DataLeftTrace",
      MakeCallback( &EdgeDataLeftCallback ) );
    edge->TraceConnectWithoutContext
    ( "InterestEnteredTrace",
      MakeCallback( &EdgeInterestEnteredCallback ) );
    edge->TraceConnectWithoutContext
    ( "InterestLeftTrace",
      MakeCallback( &EdgeInterestLeftCallback ) );
    edge->TraceConnectWithoutContext
    ( "AuthCachedTrace",
      MakeCallback( &EdgeAuthCachedCallback ) );
    edge->TraceConnectWithoutContext
    ( "BlockedInterestTrace",
      MakeCallback( &EdgeBlockedInterestCallback ) );
};

~Setup( void )
{
    tags_created_trace_stream.close();
    tags_active_trace_stream.close();
    tag_sigverif_trace_stream.close();
    tag_bloom_trace_stream.close();
    overhead_trace_stream.close();
    //drop_trace_stream.close();
    //rate_trace_stream.close();
    validation_trace_stream.close();
    transmission_trace_stream.close();
    
    Simulator::Cancel( tags_created_event );
    Simulator::Cancel( tags_active_event );
    Simulator::Cancel( tag_sigverif_event );
    Simulator::Cancel( tag_bloom_event );
    Simulator::Cancel( overhead_event );
    Simulator::Cancel( validation_event );
    Simulator::Cancel( transmission_event );
};
};
Setup setup;

// initializers
void
EnableTagsCreatedTrace
( const string& logfile,
  Time interval )
{
    tags_created_trace_stream.open( logfile );
    if( !tags_created_trace_stream.good() )
        cerr << "Error opening log file '" << logfile << "'" << endl;
    tags_created_trace_interval = interval;

    tags_created_event =
        Simulator::Schedule( interval, &TagsCreatedLogger );
}

void
EnableTagsActiveTrace
( const string& logfile,
  Time interval )
{
    tags_active_trace_stream.open( logfile );
    if( !tags_active_trace_stream.good() )
        cerr << "Error opening log file '" << logfile << "'" << endl;
    tags_active_trace_interval = interval;

    tags_active_event =
        Simulator::Schedule( interval, &TagsActiveLogger );
}


void
EnableTagSigVerifTrace
( const string& logfile,
  Time interval )
{
    tag_sigverif_trace_stream.open( logfile );
    if( !tag_sigverif_trace_stream.good() )
        cerr << "Error opening log file '" << logfile << "'" << endl;
    tag_sigverif_trace_interval = interval;

    tag_sigverif_event =
        Simulator::Schedule( interval, &TagSigVerifLogger );
}


void
EnableTagBloomTrace
( const string& logfile,
  Time interval )
{
    tag_bloom_trace_stream.open( logfile );
    if( !tag_bloom_trace_stream.good() )
        cerr << "Error opening log file '" << logfile << "'" << endl;
    tag_bloom_trace_interval = interval;

    tag_bloom_event =
        Simulator::Schedule( interval, &TagBloomLogger );
}


void
EnableOverheadTrace
( const string& logfile,
  Time interval )
{
    overhead_trace_stream.open( logfile );
    if( !overhead_trace_stream.good() )
        cerr << "Error opening log file '" << logfile << "'" << endl;
    overhead_trace_interval = interval;

    overhead_event =
        Simulator::Schedule( interval, &OverheadLogger );
}


void
EnableDropTrace
( const string& logfile,
  Time interval )
{
    L2RateTracer::InstallAll( logfile, interval );
}


void
EnableRateTrace
( const string& logfile,
  Time interval )
{
    L3RateTracer::InstallAll( logfile, interval );
}


void
EnableValidationTrace
( const string& logfile,
  Time interval )
{
    validation_trace_stream.open( logfile );
    if( !validation_trace_stream.good() )
        cerr << "Error opening log file '" << logfile << "'" << endl;
    validation_trace_interval = interval;

    validation_event =
        Simulator::Schedule( interval, &ValidationLogger );
}


void
EnableTransmissionTrace
( const string& logfile,
  Time interval )
{
    transmission_trace_stream.open( logfile );
    if( !transmission_trace_stream.good() )
        cerr << "Error opening log file '" << logfile << "'" << endl;
    transmission_trace_interval = interval;

    transmission_event =
        Simulator::Schedule( interval, &TransmissionLogger );
}


void
EnableEdgeBlockTrace
( const string& logfile,
  Time interval )
{
    edgeblock_trace_stream.open( logfile );
    if( !edgeblock_trace_stream.good() )
        cerr << "Error opening log file '" << logfile << "'" << endl;
    edgeblock_trace_interval = interval;

    edgeblock_event =
        Simulator::Schedule( interval, &EdgeBlockLogger );
}

void
EnableConsumerTrace
( const string& logfile,
  Time interval )
{
    consumer_trace_stream.open( logfile );
    if( !consumer_trace_stream.good() )
        cerr << "Error opening log file '" << logfile << "'" << endl;
    consumer_trace_interval = interval;
    
    consumer_trace_stream
    << "# 1. Time\n"
    << "# 2. Number of interests sent by all consumers\n"
    << "# 3. Number of datas received by all consumers\n"
    << "# 4. Number of nacks received by all consumers\n"
    << "# 5. Number of wrongful nacks\n"
    << "# 6. Number of wrongful datas\n"
    << "# 7. Number of unrequested datas\n"
    << "# 8. Number of retransmissions\n"
    << "# 9. Number of timeouts\n"
    << "# 10. Average delay between last retx and data receipt\n"
    << "# 11. Average delay between initial tx and data receipt\n";

    consumer_event =
        Simulator::Schedule( interval, &ConsumerLogger );
}

};
};

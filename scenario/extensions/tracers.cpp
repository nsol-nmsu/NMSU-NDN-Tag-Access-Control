#include "tracers.hpp"

namespace ndntac
{
namespace tracers
{


using namespace ndn;
using namespace ns3;
//using namespace ns3::ndn;
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
                        ( &EdgeTrace::interest_entered ),
                        "BlockedInterestTrace" );
       return tid;
};

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
Time edgeblock_interval;

// logger even ids
EventId tags_created_event;
EventId tags_active_event;
EventId tag_sigverif_event;
EventId tag_bloom_event;
EventId overhead_event;
EventId validation_event;
EventId transmission_event;
EventId edgeblock_event;

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
Time     sigverifs_delay;
uint64_t bloom_lookups = 0;
Time     bloom_lookups_delay = 0;
uint64_t bloom_inserts = 0;
Time     bloom_inserts_delay = 0;
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
uint64_t authcached_positive_upstream = 0;
uint64_t authcached_positive_moved = 0;
uint64_t authcached_negative_upstream = 0;
uint64_t edge_blocked_expired = 0;
uint64_t edge_blocked_bad_prefix = 0;


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
    
}

void
ProducerBloomInsertCallback
( const AuthTag& tag, Time delay )
{

}

void
ProducerValidationCallback
( ValidationDetail detail )
{

}

void
ProducerReceivedInterestCallack
( const Interest& interest )
{

}

void
ProducerSentDataCallback
( const Data& data )
{

}

void
ConsumerSentInterestCallback
( const Interest& interest )
{

}

void
ConsumerReceivedDataCallback
( const Data& data )
{

}

void
ConsumerDeniedCallback
( uint32_t seq )
{

}

void
ConsumerDeservedCallback
( uint32_t seq )
{

}

void
ConsumerUndeservedCallback
( uint32_t seq )
{

}

void
ConsumerUnrequestedCallback
( void )
{

}

void
ConsumerReceivedAuthCallback
( const AuthTag& tag )
{

}

void
ConsumerDisposedAuthCallback
( const AuthTag& tag )
{

}

void
ConsumerRetXCallback
( uint32_t seq )
{

}

void
ConsumerTimeoutCallback
( uint32_t seq )
{

}

void
RouterSigVerifCallback
( const AuthTag& tag, Time delay )
{

}

void
RouterBloomLookupCallback
( const AuthTag& tag, Time delay )
{

}

void
RouterBloomInsertCallback
( const AuthTag& tag, Time delay )
{

}

void
RouterValidationCallback
( ValidationDetail detail )
{

}

void
RouterSentInterestCallback
( const Interest& interest )
{

}

void
RouterSentDataCallback
( const Data& data )
{

}

void
EdgeDataEnteredCallback
( const Data& data )
{

}

void
EdgeDataLeftCallback
( const Data& data )
{

}

void
EdgeInterestEnteredCallback
( const Interest& interest )
{

}

void
EdgeInterestLeftCallback
( const Interest& interest )
{

}

void
EdgeAuthCachedCallback
( const AuthTag& tag, AuthCachedDetail detail )
{

}

void
EdgeBlockedInterestCallback
( const Interest& interest, BlockedDetail detail )
{

}

void
EdgeSigVerifCallback
( const AuthTag& tag, Time delay )
{

}

void
EdgeBloomLookupCallback
( const AuthTag& tag, Time delay )
{

}

void
EdgeBloomInsertCallback
( const AuthTag& tag, Time delay )
{

}

// loggers
void
TagsCreatedLogger( void )
{
    if( !tags_created_trace_stream.good() )
        return;

    tags_created_trace_stream
    << Simulator::Now() << '\t' << tags_created << endl;
}

void
TagsActiveLogger( void )
{
    if( !tags_active_trace_stream.good() )
        return;

    tags_active_trace_stream
    << Simulator::Now() << '\t' << tags_active << endl;
}

void
TagSigVerifLogger( void )
{
    if( !tag_sigverif_trace_stream.good() )
        return;
    tag_sigverif_trace_stream
    << Simulator::Now() << '\t' << sigverifs
                        << '\t' << sigverifs_delay << endl;
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
}

void
OverheadLogger( void )
{
    if( !overhead_trace_stream.good() )
        return;

    Time delay = sigverifs_delay
               + bloom_lookups_delay
               + bloom_inserts_delay;

    uint64_t bytes = // 3 bytes for NoReCacheFlag
                     datas_transmitted*2
                     // 5 bytes for AuthValidityProb
                   + interests_transmitted*4
                     // auth tag overhead
                   + tags_bytes_transmitted;
    
    overhead_trace_stream
    << Simulator::Now() << '\t' << bytes << "bytes"
                        << '\t' << delay << endl;
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
      MakeCallback( &ProducerReceivedInterestCallback ) );
    producer->TraceConnectWithoutContext
    ( "SentDataTrace",
      MakeCallback( &ProducerSentDataCallback ) );


    consumer->TraceConnectWithoutContext
    ( "InterestTrace",
      MakeCallback( &ConsumerInterestCallback ) );
    consumer->TraceConnectWithoutContext
    ( "DataTrace",
      MakeCallback( &ConsumerDataCallback ) );
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
      MakeCallback( &ConsumerAuthReceivedCallback ) );
    consumer->TraceConnectWithoutContext
    ( "AuthDisposedTrace",
      MakeCallback( &ConsumerAuthDisposedCallback ) );
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
      MakeCallback( &RouterInsertCallback ) );
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
    ( "DataEnteredTrace,
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

    transmission_event
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

};
};

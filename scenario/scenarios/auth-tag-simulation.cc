/**
* @brief Scenario generator and simulator
* Creates a random topology and runs simulation on it.
**/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/brite-module.h"
#include "ns3/names.h"
#include "ns3/ndnSIM/apps/ndn-consumer-cbr.hpp"

#include "producer.hpp"
#include "consumer.hpp"
#include "router-strategy.hpp"
#include "edge-strategy.hpp"
#include "pconfig-reader.hpp"
#include "include-only-once.hpp"

#include "unqlite.hpp"


using namespace std;
using namespace ndn;
using namespace ns3;
using namespace ns3::ndn;
using namespace ndntac;

namespace ndntac
{

// config struct, represents a scenario configuration
struct Config
{
    // loads config from file
    Config( const string& file );
    
    unsigned nproducers; // number of producers to generate
    unsigned nconsumers; // number of consumers to generate
    unsigned nsubnets;   // number of subnets to generate
    unsigned nedges;     // number of edge routers to select
    
    string network_config;  // BRITE config file for generating network
    string subnet_config;   // BRITE config file for generating subnets
    string producer_config; // config file for producers
    string consumer_config; // config file for consumrs
    string router_config;   // config file for routers
    string edge_config;     // config file for edge routers
    
    Time simulation_time; // how much time to simulate
    
    // enables trace that keeps track of total number of
    // auth tags that have been created at each interval in
    // the simulation
    bool enable_tags_created_trace;
    // trace interval for the above
    Time tags_created_trace_interval;
    
    // enables a trace that keeps track of the number of
    // of active tags at each interval in the simulation,
    // active tags are any valid; unexpired tags
    bool enable_tags_active_trace;
    // the trace interval for the above
    Time tags_active_trace_interval;
    
    // enables a trace that keeps track of the total
    // number of tag signature verifications that have
    // occured at each interval in the simulation
    bool enable_tag_sigverif_trace;
    // the interval for the above
    Time tag_sigverif_trace_interval;
    
    // enables a trace that keeps track of the total
    // number of bloom filter lookups that have occured
    // at each simulation interval
    bool enable_tag_bloom_trace;
    // the interval for the above
    Time tag_bloom_trace_interval;
    
    // enables a trace that keeps track of the
    // total computational ( delay ) and transmition
    // overhead that have occured due to NDNTAC
    // authentication
    bool enable_overhead_trace;
    // the interval for the above
    Time overhead_trace_interval;
    
    // enables a trace that keeps track of the
    // total number of interests dropped by the
    // network at each interval
    bool enable_drop_trace;
    // the interval for the above
    Time drop_trace_interval;
    
    // enables a trace that keeps track of the total
    // numbers of successful and failed auth tag
    // validation, and the number of auth tags
    // that failed validation by each check mechanism
    bool enable_validation_trace;
    // the interval for the above
    Time validation_trace_interval;
    
    // enable a trace that keeps track of the total
    // number of interest and data packets transmitted
    bool enable_transmission_trace;
    // the interval for the above
    Time transmission_trace_interval;
};

// prototypes
void
makeCluster
( PointToPointHelper& p2p_helper,
  InternetStackHelper& internet_helper,
  NodeContainer& out,
  const string& config );

void
makeTopo
( const Config& config,
  NodeContainer& producers_out,
  NodeContainer& consumers_out,
  NodeContainer& routers_out,
  NodeContainer& edges_out );

int main( int argc, char* argv[] )
{
    CommandLine cmd;
    cmd.Parse( argc, argv );
    
    // load configuration
    Config config( "config/simulation_config.jx9" );
    
    // create a topology
    NodeContainer producer_nodes;
    NodeContainer consumer_nodes;
    NodeContainer router_nodes;
    NodeContainer edge_nodes;
    makeTopo( config,
              producer_nodes,
              consumer_nodes,
              router_nodes,
              edge_nodes );
    
    // initialize and install universal helpers
    StackHelper ndn_helper;
    ndn_helper.InstallAll();
    GlobalRoutingHelper routing_helper;
    routing_helper.InstallAll();
    
    // install producer app to producers
    ndntac::Producer::s_config = config.producer_config;
    AppHelper producer_app( "ndntac::Producer" );
    producer_app.Install( producer_nodes );
    
    // install consumer app to consumers
    ndntac::Consumer::s_config = config.consumer_config;
    AppHelper consumer_app( "ndntac::Consumer" );
    consumer_app.Install( consumer_nodes );
    
    // install router strategy on all nodes except edge routers
    RouterStrategy::s_config = config.router_config;
    StrategyChoiceHelper::Install<RouterStrategy>( router_nodes, "/" );
    StrategyChoiceHelper::Install<RouterStrategy>( producer_nodes, "/" );
    StrategyChoiceHelper::Install<RouterStrategy>( consumer_nodes, "/" );
    
    // edge routers get the edge strategy
    EdgeStrategy::s_config = config.edge_config;
    StrategyChoiceHelper::Install<EdgeStrategy>( edge_nodes, "/" );

    // configure routes
    GlobalRoutingHelper::CalculateRoutes();
    
    
    Simulator::Stop( config.simulation_time );
    Simulator::Run();
    Simulator::Destroy();
    return 0;
};


// loads and executes a simulation config script
Config::Config( const string& file  )
{
    // load the config with default options
    // these will be modified later if the config
    // file provides them
    nproducers = 10;
    nconsumers = 10;
    nsubnets   = 5;
    nedges     = 5;
    network_config  = "network_config.brite";
    subnet_config   = "subnet_config.brite";
    producer_config = "producer_config.jx9";
    consumer_config = "consumer_config.jx9";
    router_config   = "router_config.jx9";
    edge_config     = "edge_config.jx9";
    simulation_time = Seconds( 10 );
    enable_tags_created_trace   = false;
    tags_created_trace_interval = Seconds(10);
    enable_tags_active_trace    = false;
    tags_active_trace_interval  = Seconds(10);
    enable_tag_sigverif_trace   = false;
    tag_sigverif_trace_interval = Seconds(10);
    enable_tag_bloom_trace      = false;
    tag_bloom_trace_interval    = Seconds(10);
    enable_overhead_trace       = false;
    overhead_trace_interval     = Seconds(10);
    enable_drop_trace           = false;
    drop_trace_interval         = Seconds(10);
    enable_validation_trace     = false;
    validation_trace_interval   = Seconds(10);
    enable_transmission_trace   = false;
    transmission_trace_interval = Seconds(10);
    
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
    
    val = unqlite_vm_extract_variable( vm, "nproducers" );
    if( val && unqlite_value_is_int( val ) )
        nproducers = unqlite_value_to_int64( val );
    
    val = unqlite_vm_extract_variable( vm, "nconsumers" );
    if( val && unqlite_value_is_int( val ) )
        nconsumers = unqlite_value_to_int64( val );
    
    val = unqlite_vm_extract_variable( vm, "nedges" );
    if( val && unqlite_value_is_int( val ) )
        nedges = unqlite_value_to_int64( val );
    
    const char* str_val;
    int str_len;
    
    val = unqlite_vm_extract_variable( vm, "network_config" );
    if( val && unqlite_value_is_string( val ) )
    {
       str_val = unqlite_value_to_string( val, &str_len );
       network_config.assign( str_val, str_len );
    }
    
    val = unqlite_vm_extract_variable( vm, "subnet_config" );
    if( val && unqlite_value_is_string( val ) )
    {
       str_val = unqlite_value_to_string( val, &str_len );
       subnet_config.assign( str_val, str_len );
    }
    
    val = unqlite_vm_extract_variable( vm, "producer_config" );
    if( val && unqlite_value_is_string( val ) )
    {
       str_val = unqlite_value_to_string( val, &str_len );
       producer_config.assign( str_val, str_len );
    }

    val = unqlite_vm_extract_variable( vm, "consumer_config" );
    if( val && unqlite_value_is_string( val ) )
    {
       str_val = unqlite_value_to_string( val, &str_len );
       consumer_config.assign( str_val, str_len );
    }
    
    val = unqlite_vm_extract_variable( vm, "router_config" );
    if( val && unqlite_value_is_string( val ) )
    {
       str_val = unqlite_value_to_string( val, &str_len );
       router_config.assign( str_val, str_len );
    }
    
    val = unqlite_vm_extract_variable( vm, "edge_config" );
    if( val && unqlite_value_is_string( val ) )
    {
       str_val = unqlite_value_to_string( val, &str_len );
       edge_config.assign( str_val, str_len );
    }
    
    val = unqlite_vm_extract_variable( vm, "simulation_time" );
    if( val && unqlite_value_is_float( val ) )
        simulation_time = Seconds( unqlite_value_to_double( val ) );
    
    val = unqlite_vm_extract_variable
          ( vm, "enable_tags_created_trace" );
    if( val && unqlite_value_is_bool( val ) )
    {
        enable_tags_created_trace = unqlite_value_to_bool( val );
    }
    val = unqlite_vm_extract_variable
          ( vm, "tags_created_trace_interval" );
    if( val && unqlite_value_is_float( val ) )
    {
        tags_created_trace_interval =
            Seconds( unqlite_value_to_double( val ) );
    }
    
    
    val = unqlite_vm_extract_variable
          ( vm, "enable_tags_active_trace" );
    if( val && unqlite_value_is_bool( val ) )
    {
        enable_tags_active_trace = unqlite_value_to_bool( val );
    }
    val = unqlite_vm_extract_variable
          ( vm, "tags_active_trace_interval" );
    if( val && unqlite_value_is_float( val ) )
    {
        tags_active_trace_interval =
            Seconds( unqlite_value_to_double( val ) );
    }
    
    
    val = unqlite_vm_extract_variable
          ( vm, "enable_tag_sigverif_trace" );
    if( val && unqlite_value_is_bool( val ) )
    {
        enable_tag_sigverif_trace = unqlite_value_to_bool( val );
    }
    val = unqlite_vm_extract_variable
          ( vm, "tag_sigverif_trace_interval" );
    if( val && unqlite_value_is_float( val ) )
    {
        tag_sigverif_trace_interval =
            Seconds( unqlite_value_to_double( val ) );
    }
    
    
    val = unqlite_vm_extract_variable
          ( vm, "enable_tag_bloom_trace" );
    if( val && unqlite_value_is_bool( val ) )
    {
        enable_tag_bloom_trace = unqlite_value_to_bool( val );
    }
    val = unqlite_vm_extract_variable
          ( vm, "tag_bloom_trace_interval" );
    if( val && unqlite_value_is_float( val ) )
    {
        tag_bloom_trace_interval =
            Seconds( unqlite_value_to_double( val ) );
    }
    
    
    val = unqlite_vm_extract_variable
          ( vm, "enable_overhead_trace" );
    if( val && unqlite_value_is_bool( val ) )
    {
        enable_overhead_trace = unqlite_value_to_bool( val );
    }
    val = unqlite_vm_extract_variable
          ( vm, "overhead_trace_interval" );
    if( val && unqlite_value_is_float( val ) )
    {
        overhead_trace_interval =
            Seconds( unqlite_value_to_double( val ) );
    }
    
    val = unqlite_vm_extract_variable
          ( vm, "enable_drop_trace" );
    if( val && unqlite_value_is_bool( val ) )
    {
        enable_drop_trace = unqlite_value_to_bool( val );
    }
    val = unqlite_vm_extract_variable
    ( vm, "drop_trace_interval" );
    if( val && unqlite_value_is_float( val ) )
    {
        drop_trace_interval =
            Seconds( unqlite_value_to_double( val ) );
    }
    
    
    val = unqlite_vm_extract_variable
          ( vm, "enable_validation_trace" );
    if( val && unqlite_value_is_bool( val ) )
    {
        enable_validation_trace = unqlite_value_to_bool( val );
    }
    val = unqlite_vm_extract_variable
          ( vm, "validation_trace_interval" );
    if( val && unqlite_value_is_float( val ) )
    {
        validation_trace_interval =
            Seconds( unqlite_value_to_double( val ) );
    }
    
    
    val = unqlite_vm_extract_variable
          ( vm, "enable_transmission_trace" );
    if( val && unqlite_value_is_bool( val ) )
    {
        enable_transmission_trace = unqlite_value_to_bool( val );
    }
    val = unqlite_vm_extract_variable
          ( vm, "transmission_trace_interval" );
    if( val && unqlite_value_is_float( val ) )
    {
        transmission_trace_interval =
            Seconds( unqlite_value_to_double( val ) );
    }
    
    // release the vm
    unqlite_vm_release( vm );
    
    // close the db
    unqlite_close( db );
}

// makes a random BRITE cluster of nodes given a BRITE
// config file
void
makeCluster
( PointToPointHelper& p2p_helper,
  InternetStackHelper& internet_helper,
  NodeContainer& out,
  const string& config )
{
    BriteTopologyHelper brite_helper( config );
    brite_helper.AssignStreams( 3 );
    brite_helper.BuildBriteTopology( internet_helper, p2p_helper );
    
    size_t as_count = brite_helper.GetNAs();
    for( size_t as_num = 0 ; as_num < as_count ; as_num++ )
    {
        size_t node_count = brite_helper.GetNNodesForAs( as_num );
        for( size_t node_num = 0 ; node_num < node_count ; node_num++ )
        {
            Ptr<Node> node = brite_helper.GetNodeForAs( as_num, node_num );
            out.Add( node );
        }
    }
}

void
makeTopo
( const Config& config,
  NodeContainer& producers_out,
  NodeContainer& consumers_out,
  NodeContainer& routers_out,
  NodeContainer& edges_out )
{
    // helpers
    PointToPointHelper p2p_helper;
    InternetStackHelper internet_helper;
    
    // for a proper topology to be generater certain conditions
    // must be met in the settings
    BOOST_ASSERT( config.nedges > 0 );
    BOOST_ASSERT( config.nproducers > 0 );
    BOOST_ASSERT( config.nconsumers > 0 );
    BOOST_ASSERT( config.nsubnets > 0 );
    
    // generate the main cluster
    NodeContainer main_network;
    makeCluster( p2p_helper, internet_helper, main_network, config.network_config );
    
    // edge routers ill be selected from amongst the
    // main_network nodes; so the container must have
    // nodes >= nedges
    BOOST_ASSERT( main_network.GetN() >= config.nedges );
    
    // generate nedges unique random indexes for the
    // main network node container
    std::set<size_t> rnums;
    while( rnums.size() < config.nedges )
    {
        rnums.insert( rand() % main_network.GetN() );
    }
    
    
    // partition main network into edge routers and normal
    // routers
    for( size_t i = 0 ; i < main_network.GetN() ; i++ )
    {
        if( rnums.find( i ) != rnums.end() )
            edges_out.Add( main_network.Get( i ) );
        else
            routers_out.Add( main_network.Get( i ) );
    }
    
    // generate some subnetworks
    NodeContainer subnet_nodes;
    for( size_t i = 0 ; i < config.nsubnets ; i++ )
    {
        // generate cluster
        NodeContainer subnet;
        makeCluster( p2p_helper, internet_helper, subnet, config.subnet_config );
        
        // add nodes to subnet collection
        subnet_nodes.Add( subnet );
        
        // graft the subnet onto the main network via
        // one of the edge routers
        size_t graft_edge = rand() % edges_out.GetN();
        p2p_helper.Install( subnet.Get( 0 ), edges_out.Get( graft_edge ) );
    }
    
    // number of subnet nodes must be >= nproducers + nconsumers
    // since the producer and consumer nodes will be selected
    // from amongst the subnet nodes
    BOOST_ASSERT( subnet_nodes.GetN() >= config.nproducers + config.nconsumers );
    
    // generate some random unique consumer indexes for the
    // subnet_nodes collection
    rnums.clear();
    while( rnums.size() < config.nconsumers )
    {
        rnums.insert( rand() % subnet_nodes.GetN() );
    }
    
    // generate some random unique producer indexes for the 
    // subnet_nodes container; these'll be different from the
    // consumer indexes
    std::set<size_t> rnums2;
    while( rnums2.size() < config.nproducers )
    {
        size_t num = rand() % subnet_nodes.GetN();
        if( rnums.find( num ) != rnums.end() )
            rnums2.insert( num );
    }
    
    // partition consumer, producer, and router nodes
    // in the subnets
    for( size_t i = 0 ; i < subnet_nodes.GetN() ; i++ )
    {
        if( rnums.find( i ) != rnums.end() )
        {
            consumers_out.Add( subnet_nodes.Get( i ) );
        }
        else if( rnums2.find( i ) != rnums2.end() )
        {
            producers_out.Add( subnet_nodes.Get( i ) );
        }
        else
        {
            routers_out.Add( subnet_nodes.Get( i ) );
        }
    }
    
    // finished
}

};


int main( int argc, char* argv[] )
{
        return ndntac::main( argc, argv );
}

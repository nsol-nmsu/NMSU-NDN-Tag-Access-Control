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

// include interface and implementation files since I haven't figured
// out how to customize ndnSIM linking
#include "tx-queue.hpp"
#include "data-producer.hpp"
#include "auth-data-producer.hpp"
#include "file-data-producer.hpp"
#include "producer.hpp"
#include "producer.cpp"
#include "router-strategy.hpp"
#include "router-strategy.cpp"
#include "edge-strategy.hpp"
#include "edge-strategy.cpp"
#include "consumer.hpp"
#include "consumer.cpp"
#include "coordinator.hpp"
#include "coordinator.cpp"

extern "C"
{
  #include <sys/stat.h>
  #include <sys/types.h>
}


using namespace std;
using namespace ndn;
using namespace ndntac;


namespace ns3
{
        const unsigned NCONSUMERS = 10;
        const unsigned NPRODUCERS = 21;
        static PointToPointHelper p2p;
        static InternetStackHelper internet;

        void makeTopo( NodeContainer& out, const string& config )
        {

            BriteTopologyHelper briteHelper( config );
            briteHelper.AssignStreams( 3 );
            briteHelper.BuildBriteTopology( internet );

            uint32_t as_count = briteHelper.GetNAs();
            for( uint32_t as_num = 0 ; as_num < as_count ; as_num++ )
            {
                uint32_t node_count = briteHelper.GetNNodesForAs( as_num );
                for( uint32_t node_num = 0 ; node_num < node_count ; node_num++ )
                {
                    Ptr<ns3::Node> node = briteHelper.GetNodeForAs( as_num, node_num );
                    out.Add( node );
                }

            }
        }


        int main( int argc, char* argv[] )
        {
                CommandLine cmd;
                cmd.Parse( argc, argv );

                // node groups
                vector<NodeContainer> producer_nodes;
                vector<NodeContainer> consumer_nodes;
                NodeContainer router_nodes;
                NodeContainer edge_router_nodes;

                // routers, the central network consists of only routers
                NodeContainer all_routers;
                makeTopo( all_routers, "scratch/auth-tag-simulation/BRITE_CENTRAL.conf" );

                // choose some of the routers as edge routers
                // and the rest as normal routers
                unsigned i = 0;
                for(  ; i < NPRODUCERS + NCONSUMERS ; i++ )
                {
                    edge_router_nodes.Add( all_routers.Get( i % all_routers.GetN() ) );
                }
                for( ; i < all_routers.GetN(); i++ )
                {
                    router_nodes.Add( all_routers.Get( i % all_routers.GetN() ) );
                }

                // generate producer clusters
                for( i = 0 ; i < NPRODUCERS ; i++ )
                {
                    NodeContainer cluster;
                    makeTopo( cluster, "scratch/auth-tag-simulation/BRITE_PRODUCERS.conf" );
                    producer_nodes.push_back( cluster );
                }

                // generate consumer clusters
                for( i = 0 ; i < NCONSUMERS ; i++ )
                {
                    NodeContainer cluster;
                    makeTopo( cluster, "scratch/auth-tag-simulation/BRITE_CONSUMERS.conf" );
                    consumer_nodes.push_back( cluster );
                }

                 // link clusters with central network
                 int edge_index = 0;

                 // make links between consumer clusters and central network
                 auto it = consumer_nodes.begin();
                 for( ; it != consumer_nodes.end()
                      ; it++ )
                 {
                     p2p.Install( it->Get( 0 ),
                                  edge_router_nodes.Get( edge_index++ ) );
                 }
                 // make links between producer clusters and central network
                 for( it = producer_nodes.begin() ;
                      it != producer_nodes.end() ;
                      it++ )
                 {
                     p2p.Install( it->Get( 0 ),
                                  edge_router_nodes.Get( edge_index++ ) );
                 }



                 // prepare global helpers
                 ndn::StackHelper ndn_helper;
                 ndn_helper.setCsSize( 10 );
                 ndn_helper.InstallAll();
                 ndn::GlobalRoutingHelper routing_helper;
                 routing_helper.InstallAll();

                 // install auth strategy on all routers
                 ndn
                 ::StrategyChoiceHelper
                 ::Install<RouterStrategy>( router_nodes, "/" );

                 // install edge strategy on edge routers
                 ndn
                 ::StrategyChoiceHelper
                 ::Install<EdgeStrategy>( edge_router_nodes, "/" );

                 // install best route strategy on all producers and consumers
                 for( auto it = producer_nodes.begin()
                      ; it != producer_nodes.end()
                      ; it++ )
                  {
                    ndn
                    ::StrategyChoiceHelper
                    ::Install<RouterStrategy>( *it, "/" );
                   }

                   for( auto it = consumer_nodes.begin()
                      ; it != consumer_nodes.end()
                      ; it++ )
                  {
                    ndn
                    ::StrategyChoiceHelper
                    ::Install<RouterStrategy>( *it, "/" );
                  }



                // install producers with assigned directories and prepare routes
                // at the same time prepare a producer list to pass to consumers
                string producer_list = "";
                it = producer_nodes.begin();
                auto dir = opendir("scratch/auth-tag-simulation/simulation-content");
                if( dir == NULL )
                  throw std::ifstream::failure( "Couldn't open directory 'scratch/auth-tag-simulation/simulation-content'");


                struct dirent* ent;
                while( ( ent = readdir( dir ) ) != NULL
                       && it != producer_nodes.end() )
                {
                    while( ent->d_name[0] == '.' )
                        ent = readdir( dir );
                    if( ent == NULL )
                        break;

                    string producer_dir = string( "scratch/auth-tag-simulation/simulation-content" )
                                          + "/" + ent->d_name;
                    ndn::AppHelper helper( "ndntac::Producer" );
                    helper.SetAttribute( "Directory", StringValue( producer_dir ) );
                    helper.SetAttribute( "Prefix", StringValue( ent->d_name ) );
                    helper.Install( *it );
                    routing_helper.AddOrigins( ent->d_name, *(it++) );
                    if( producer_list.empty() )
                    {
                      producer_list = ent->d_name;
                    }
                    else
                    {
                      producer_list += ":";
                      producer_list += ent->d_name;
                    }
                }
                closedir( dir );

                // install consumers on all consumer nodes
                ndn::AppHelper consumer_helper( "ndntac::Consumer" );
                consumer_helper.SetAttribute( "KnownProducers", StringValue(producer_list) );
                for( it = consumer_nodes.begin() ;
                     it != consumer_nodes.end() ;
                     it++ )
                {
                    consumer_helper.Install( *it );
                }


                // make routes
                routing_helper.CalculateRoutes();

                Simulator::Stop(Seconds(5));

                mkdir("scratch/auth-tag-simulation/logs", 0770 );
                L2RateTracer::InstallAll( "scratch/auth-tag-simulation/logs/DROP_TRACE.log", Seconds(0.5));
                //ndn::CsTracer::InstallAll("scratch/CS_TRACE.log", Seconds(1) );
                ndn::AppDelayTracer::InstallAll( "scratch/auth-tag-simulation/logs/APP_DELAY_TRACE.log" );


                Coordinator::simulationStarted(true);
                Simulator::Run();
                Simulator::Destroy();
                Coordinator::simulationFinished();
                return 0;

        };

};

int main( int argc, char* argv[] )
{
        return ns3::main( argc, argv );
}

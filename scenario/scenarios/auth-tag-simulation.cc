/**
* @brief Scenario generator and simulator
* Creates a random topology and runs simulation on it.
**/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/names.h"
#include "ns3/ndnSIM/apps/ndn-consumer-cbr.hpp"

#include "producer.hpp"
#include "router-strategy.hpp"
#include "edge-strategy.hpp"
#include "log-filter.hpp"
#include "coordinator.hpp"
#include "consumer-wrapper.hpp"
#include "pconfig-reader.hpp"
#include "zipf-consumer.hpp"
#include "window-consumer.hpp"


using namespace std;
using namespace ndn;
using namespace ndntac;

namespace ns3
{
        int main( int argc, char* argv[] )
        {
            
            CommandLine cmd;
            cmd.Parse( argc, argv );

            AnnotatedTopologyReader topo_reader( "", 25 );
            topo_reader.SetFileName( "scenarios/auth-tag-simulation/simulation.topo" );
            topo_reader.Read();
            
            // read configuration file
            PConfigReader preader( "scenarios/auth-tag-simulation/producers.pconfig" );
            preader.parse();
            /*
            for( auto i = preader.begin() ; i != preader.end() ; i++ )
            {
                for( auto j = i->begin() ; j != i->end() ; j++ )
                {
                    std::cout << "Name:   " << ndn::Name(i->getName()).append(j->name).toUri() << std::endl
                              << "Size:   " << j->size         << endl
                              << "Access: " << (int)j->access_level << std::endl
                              << "Popul:  " << j->popularity   << std::endl;
                }
            }*/
            
            
            ndn::StackHelper ndn_helper;
            ndn_helper.InstallAll();
            
            ndn::GlobalRoutingHelper routing_helper;
            routing_helper.InstallAll();
            
            NodeContainer all_nodes = topo_reader.GetNodes();
            NodeContainer producer_nodes;
            NodeContainer consumer_nodes;
            NodeContainer router_nodes;
            NodeContainer edge_nodes;
            
            // seperate node types
            for( auto n = all_nodes.begin() ; n != all_nodes.end() ; n++ )
            {
                    switch( Names::FindName( *n )[0] )
                    {
                            case 'p':
                                    producer_nodes.Add( *n );
                                    break;
                            case 'c':
                                    consumer_nodes.Add( *n );
                                    break;
                            case 'r':
                                    router_nodes.Add( *n );
                                    break;
                            case 'e':
                                    edge_nodes.Add( *n );
                                    break;
                            default:
                                    break;
                    }            
            }
            
            // map to keep track of available contents and their popularity
            std::map< uint32_t, ndn::Name > content_map;
            
            // setup producers and collect content names for consumers
            stringstream consumer_names;
            for( uint32_t i = 0 ; i < producer_nodes.GetN() ; i++ )
            {
                // choose a content
                PConfigReader::PConfig config = preader.get( i % preader.size() );
                
                // setup an app helper
                ndn::AppHelper producer_app("ndntac::Producer");
                producer_app.SetAttribute( "Prefix", StringValue( config.getName().toUri() ) );
                stringstream producer_names;
                
                // resolve all contents
                for( auto iter = config.begin() ;  iter != config.end() ; iter++ )
                {
                    producer_names << '[' << iter->name.toUri()
                                   << ':' << iter->size
                                   << ':' << (int)iter->access_level << ']';
                                   
                    consumer_names << '[' << Name( config.getName() ).append( iter->name )
                                   << ':' << iter->popularity << ']';
                }
                producer_app.SetAttribute( "Names", StringValue( producer_names.str() ) );
                
                // install
                producer_app.Install( producer_nodes.Get( i ) );
                
                // routing
                routing_helper.AddOrigins( config.getName().toUri(), producer_nodes.Get( i ) );
            }
            
            // setup consumers
            ndn::AppHelper consumer_app(AuthWindowConsumer::GetTypeId().GetName() );
            consumer_app.SetAttribute( "Names", StringValue( consumer_names.str() ) );
            consumer_app.SetAttribute( "MaxWindowSize", UintegerValue( 15 ) );
            consumer_app.SetAttribute( "Window", StringValue("10") );
            consumer_app.Install( consumer_nodes );
            
            // install normal router strategy on all nodes except edge routers
            ndn::StrategyChoiceHelper::Install<ndntac::RouterStrategy>(router_nodes, "/");
            ndn::StrategyChoiceHelper::Install<ndntac::RouterStrategy>( consumer_nodes, "/" );
            ndn::StrategyChoiceHelper::Install<ndntac::RouterStrategy>( producer_nodes, "/" );
            
            // and edge router strategy on all edge nodes
            ndn::StrategyChoiceHelper::Install<ndntac::EdgeStrategy>(edge_nodes, "/");
            
            ndn::GlobalRoutingHelper::CalculateRoutes();
            
            
            Coordinator::simulationStarted( "scenarios/auth-tag-simulation/logs/SIMUATION.log" );
            Simulator::Stop( Seconds( 10 ) );
            Simulator::Run();
            Simulator::Destroy();
            Coordinator::simulationFinished();
            return 0;
                
                
                
                
                /*// node groups
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
                //ndn::AppHelper consumer_helper( "ns3::ndn::ConsumerCbr" );
                //consumer_helper.SetAttribute("Prefix", StringValue("/github.com/AUTH_TAG"));
                for( it = consumer_nodes.begin() ;
                     it != consumer_nodes.end() ;
                     it++ )
                {
                    consumer_helper.Install( *it );
                }


                // make routes
                routing_helper.CalculateRoutes();

                Simulator::Stop(Seconds(1));

                mkdir("scratch/auth-tag-simulation/logs", 0770 );
                L2RateTracer::InstallAll( "scratch/auth-tag-simulation/logs/DROP_TRACE.log", Seconds(0.5));
                //ndn::CsTracer::InstallAll("scratch/CS_TRACE.log", Seconds(1) );
                ndn::AppDelayTracer::InstallAll( "scratch/auth-tag-simulation/logs/APP_DELAY_TRACE.log" );


                Coordinator::simulationStarted( Coordinator::LogFilter() );
                Simulator::Run();
                Simulator::Destroy();
                Coordinator::simulationFinished();
                return 0;*/

        };

};


int main( int argc, char* argv[] )
{
        return ns3::main( argc, argv );
}

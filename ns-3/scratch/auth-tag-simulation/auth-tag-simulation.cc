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
#include "consumer.hpp"
#include "consumer.cpp"


using namespace std;
using namespace ndn;


namespace ns3
{

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

                 // prepare global helpers
                 ndn::StackHelper ndn_helper;
                 ndn_helper.setCsSize( 10 );
                 ndn_helper.InstallAll();
                 ndn::GlobalRoutingHelper routing_helper;
                 routing_helper.InstallAll();

                 //routeing_helper.addOrigins( "test", all_routes.get(0) );

                // make routes
                routing_helper.CalculateRoutes();

                Simulator::Stop(Seconds(20));

                L2RateTracer::InstallAll( "scratch/DROP_TRACE.log", Seconds(0.5));
                //ndn::CsTracer::InstallAll("scratch/CS_TRACE.log", Seconds(1) );
                ndn::AppDelayTracer::InstallAll( "scratch/APP_DELAY_TRACE.log" );

                Simulator::Run();
                Simulator::Destroy();
                return 0;

        };

};

int main( int argc, char* argv[] )
{
        return ns3::main( argc, argv );
}

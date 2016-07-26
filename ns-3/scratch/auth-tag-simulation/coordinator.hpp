/** @file
* @brief Coordinator and logger singularity for the simulation as a whole.
*
* @author Ray Stubbs [stubbs.ray@gmail.com]
**/

#ifndef Coordinator__INCLUDED
#define Coordinator__INCLUDED

#include "ndn-cxx/name.hpp"
#include "ndn-cxx/util/time.hpp"
#include <vector>
#include <map>

namespace ndntac
{
    using namespace ndn;
    using namespace std;

   namespace Coordinator
   {
       vector< uint32_t >    producers;
       vector< uint32_t >    consumers;
       vector< uint32_t >    routers;
       vector< Name >        producers_started;
       vector< uint32_t >    consumers_started;
       vector< uint32_t >    routers_started;

       // node registration adn deregistration
       void     addProducer( uint8_t producer_instance );
       void     addConsumer( uint32_t consumer_instance );
       void     addRouter( uint32_t router_instance );
       void     removeProducer( uint8_t producer_instance );
       void     removeConsumer( uint32_t consumer_instance );
       void     removeRouter( uint32_t router_instance );

       // initialization notifiers and finalization
       void     producerStarted( const Name& producer );
       void     consumerStarted( uint32_t consumer );
       void     routerStarted( uint32_t router );
       void     producerStopped( uint8_t producer_instance );
       void     consumerStopped( uint32_t consumer_instance );
       void     routerStopped( uint32_t router_instance );


       // node counts
       size_t   producerCount();
       size_t   consumerCount();
       size_t   routerCount();
       size_t   nodeCount();

       // initialized node counts
       size_t   producerStartedCount();
       size_t   consumerStartedCount();
       size_t   routerStartedCount();

       // accessors
       vector<uint32_t>::const_iterator beginProducers();
       vector<uint32_t>::const_iterator endProducers();
       vector<uint32_t>::const_iterator beginConsumers();
       vector<uint32_t>::const_iterator endConsumers();
       vector<uint32_t>::const_iterator beginRouters();
       vector<uint32_t>::const_iterator endRouters();

       // producer notifiers
       void producerReceivedRequest( const Name& producer_name,
                                     const Name& request_name );
       void producerDeniedRequest( const Name& producer_name,
                                   const Name& request_name,
                                   const string& why );
       void producerSatisfiedRequest( const Name& producer_name,
                                      const Name& request_name );

       void producerReceivedAuthRequest( const Name& producer_name,
                                         const Name& request_name,
                                         const NdnParameterSet& credentials );
       void producerDeniedAuthRequest( const Name& producer_name,
                                       const Name& request_name,
                                       const string& why,
                                       const NdnParameterSet& credentials );
       void producerSatisfiedAuthRequest( const Name& producer_name,
                                          const Name& request_name,
                                          const NdnParameterSet& credentials );
       void producerOther( const Name& producer_name,
                           const string& msg );

       // consumer notifiers
       void consumerSentRequest( uint32_t consumer_instance,
                                 const Name& request_name );
       void consumerRequestSatisfied( uint32_t consumer_instance,
                                      const Name& request_name );
       void consumerRequestRejected( uint32_t consumer_instance,
                                     const Name& request_name );

       void consumerRequestedAuth( uint32_t consumer_instance,
                                   const Name& request_name );
       void consumerReceivedAuth( uint32_t consumer_instance,
                                  const Name& tag_name );
       void consumerAuthDenied( uint32_t consumer_instance,
                                const Name& request_name );

       void consumerFollowedLink( uint32_t consumer_instance,
                                  const Name& request_name );

       void consumerOther( uint8_t consumer_instance,
                           const string& msg );

       // router notifiers
       void routerReceivedRequest( uint32_t router_instance,
                                   const Name& request_name );
       void routerForwardedRequest( uint32_t router_instance,
                                    const Name& request_name );
       void routerDeniedRequest( uint32_t router_instance,
                                 const Name& request_name,
                                 const string& why );
       void routerSatisfiedRequest( uint32_t router_instance,
                                    const Name& request_name );

       void routerRequestedAuth( uint32_t router_instance,
                                 const Name& request_name );
       void routerAuthSatisfied( uint32_t router_instance,
                                 const Name& request_name );
       void routerAuthDenied( uint32_t router_instance,
                              const Name& request_name );

       void routerOther( uint32_t router_instance,
                         const string& msg );

       // edge router
       void edgeSettingValidityProbability( uint32_t edge_instance,
                                            const Name& request_name,
                                            uint32_t prob,
                                            const string& msg );
       void edgeDroppingRequest( uint32_t edge_instance,
                                 const Name& request_name,
                                 const string& why );
       void edgeCachingTag( uint32_t edge_instance,
                            const Name& request_name,
                            const string& filter_name );

       // simulation notifiers
       void simulationStarted( bool enable_logging );
       void simulationFinished();
       void simulationOther( const string& msg );
   }

}

#endif // Coordinator__INCLUDED

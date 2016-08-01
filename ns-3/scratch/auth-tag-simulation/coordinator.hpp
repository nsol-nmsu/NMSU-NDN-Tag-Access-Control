/** @file
* @brief Coordinator and logger singularity for the simulation as a whole.
*
* @author Ray Stubbs [stubbs.ray@gmail.com]
**/

#ifndef COORDINATOR__INCLUDED
#define COORDINATOR__INCLUDED

#include "ndn-cxx/name.hpp"
#include "ndn-cxx/util/time.hpp"
#include <vector>
#include <map>

namespace ndntac
{

   namespace Coordinator
   {
       std::vector< uint32_t >    producers;
       std::vector< uint32_t >    consumers;
       std::vector< uint32_t >    routers;
       std::vector< ndn::Name >   producers_started;
       std::vector< uint32_t >    consumers_started;
       std::vector< uint32_t >    routers_started;

       // node registration adn deregistration
       void     addProducer( uint8_t producer_instance );
       void     addConsumer( uint32_t consumer_instance );
       void     addRouter( uint32_t router_instance );
       void     removeProducer( uint8_t producer_instance );
       void     removeConsumer( uint32_t consumer_instance );
       void     removeRouter( uint32_t router_instance );

       // initialization notifiers and finalization
       void     producerStarted( const ndn::Name& producer );
       void     consumerStarted( uint32_t consumer );
       void     routerStarted( uint32_t router );
       void     producerStopped( const ndn::Name& producer_instance );
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
       void producerReceivedRequest( const ndn::Name& producer_name,
                                     const ndn::Name& request_name );
       void producerDeniedRequest( const ndn::Name& producer_name,
                                   const ndn::Name& request_name,
                                   const std::string& why );
       void producerSatisfiedRequest( const ndn::Name& producer_name,
                                      const ndn::Name& request_name );

       void producerReceivedAuthRequest( const ndn::Name& producer_name,
                                         const ndn::Name& request_name );
       void producerDeniedAuthRequest( const ndn::Name& producer_name,
                                       const ndn::Name& request_name,
                                       const std::string& why);
       void producerSatisfiedAuthRequest( const ndn::Name& producer_name,
                                          const ndn::Name& request_name );
       void producerOther( const ndn::Name& producer_name,
                           const std::string& msg );

       // consumer notifiers
       void consumerSentRequest( uint32_t consumer_instance,
                                 const ndn::Name& request_name );
       void consumerRequestSatisfied( uint32_t consumer_instance,
                                      const ndn::Name& request_name );
       void consumerRequestRejected( uint32_t consumer_instance,
                                     const ndn::Name& request_name );

       void consumerRequestedAuth( uint32_t consumer_instance,
                                   const ndn::Name& request_name );
       void consumerReceivedAuth( uint32_t consumer_instance,
                                  const ndn::Name& tag_name );
       void consumerAuthDenied( uint32_t consumer_instance,
                                const ndn::Name& request_name );

       void consumerFollowedLink( uint32_t consumer_instance,
                                  const ndn::Name& request_name );

       void consumerOther( uint8_t consumer_instance,
                           const std::string& msg );

       // router notifiers
       void routerReceivedRequest( uint32_t router_instance,
                                   const ndn::Name& request_name );
       void routerForwardedRequest( uint32_t router_instance,
                                    const ndn::Name& request_name );
       void routerDeniedRequest( uint32_t router_instance,
                                 const ndn::Name& request_name,
                                 const std::string& why );
       void routerSatisfiedRequest( uint32_t router_instance,
                                    const ndn::Name& request_name );

       void routerRequestedAuth( uint32_t router_instance,
                                 const ndn::Name& request_name );
       void routerAuthSatisfied( uint32_t router_instance,
                                 const ndn::Name& request_name );
       void routerAuthDenied( uint32_t router_instance,
                              const ndn::Name& request_name );

       void routerOther( uint32_t router_instance,
                         const std::string& msg );

       // edge router
       void edgeSettingValidityProbability( uint32_t edge_instance,
                                            const ndn::Name& request_name,
                                            uint32_t prob,
                                            const std::string& msg );
       void edgeDroppingRequest( uint32_t edge_instance,
                                 const ndn::Name& request_name,
                                 const std::string& why );
       void edgeCachingTag( uint32_t edge_instance,
                            const ndn::Name& request_name,
                            const std::string& filter_name );

       // simulation notifiers
       void simulationStarted( bool enable_logging );
       void simulationFinished();
       void simulationOther( const std::string& msg );
   }

}

#endif // COORDINATOR__INCLUDED

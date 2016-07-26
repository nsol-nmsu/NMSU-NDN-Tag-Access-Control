#include "Coordinator.hpp"
#include <mutex>

namespace ndntac
{
   namespace Coordinator
   {
       ofstream              log_file;
       recursive_mutex       lock;
       uint64_t              pending;
       bool                  logging_enabled;

       static void putPending(  )
       {
            pending++;
       }

       static void removePending( )
       {
                   pending--;
       }

       void addProducer( uint8_t producer_instance )
       {
            lock.lock();
            producers.push_back( producer_instance );
            lock.unlock();
       }
       void addConsumer( uint32_t consumer_instance )
       {
            lock.lock();
            consumers.push_back( consumer_instance );
            lock.unlock();
       }
       void addRouter( uint32_t router_instance )
       {
            lock.lock();
            routers.push_back( router_instance );
            lock.unlock();
       }

      void removeProducer( uint8_t producer_instance )
       {
            lock.lock();
            auto it = find( producers.begin(), producers.end(), producer_instance );
            if( it != producers.end() )
                producers.erase( it );
            lock.unlock();
       }
       void removeConsumer( uint32_t consumer_instance )
       {
            lock.lock();
            auto it = find( consumers.begin(), consumers.end(), consumer_instance );
            if( it != consumers.end() )
                consumers.erase( it );
            lock.unlock();
       }
       void removeRouter( uint32_t router_instance )
       {
            lock.lock();
            auto it = find( routers.begin(), routers.end(), router_instance );
            if( it != routers.end() )
                routers.erase( it );
            lock.unlock();
       }

       void producerStarted( const Name& producer )
       {
            lock.lock();
            producers_started.push_back( producer );
            lock.unlock();
       }

       NdnSigner* producerSigner( const Name& producer )
       {
            lock.lock();

            // make signer if not already available
            NdnSigner* ret = &producer_signers[producer];

            lock.unlock();
            return ret;
       }

       void consumerStarted( uint32_t consumer )
       {
            lock.lock();
            consumers_started.push_back( consumer );
            lock.unlock();
       }
       void routerStarted( uint32_t router )
       {
            lock.lock();
            routers_started.push_back( router );
            lock.unlock();
       }

      void producerStopped( const Name& producer_instance )
       {
            lock.lock();
            auto it = find( producers_started.begin(),
                            producers_started.end(),
                            producer_instance );
            if( it != producers_started.end() )
                producers_started.erase( it );
            lock.unlock();
       }
       void consumerStopped( uint32_t consumer_instance )
       {
            lock.lock();
            auto it = find( consumers_started.begin(),
                            consumers_started.end(),
                            consumer_instance );
            if( it != consumers_started.end() )
                consumers.erase( it );
            lock.unlock();
       }
       void routerStopped( uint32_t router_instance )
       {
            lock.lock();
            auto it = find( routers_started.begin(),
                            routers_started.end(),
                            router_instance );
            if( it != routers_started.end() )
                routers_started.erase( it );
            lock.unlock();
       }

       size_t producerCount()
       {
            lock.lock();
            auto count = producers.size();
            lock.unlock();
            return count;
       }
       size_t consumerCount()
       {
            lock.lock();
            auto count = consumers.size();
            lock.unlock();
            return count;
       }
       size_t routerCount()
       {
            lock.lock();
            auto count = routers.size();
            lock.unlock();
            return count;
       }
       size_t nodeCount()
       {
            lock.lock();
            auto count = routers.size();
            lock.unlock();
            return count;
       }

       size_t producerStartedCount()
       {
            lock.lock();
            auto count = producers_started.size();
            lock.unlock();
            return count;
       }
       size_t consumerStartedCount()
       {
            lock.lock();
            auto count = consumers_started.size();
            lock.unlock();
            return count;
       }
       size_t routerStartedCount()
       {
            lock.lock();
            auto count = routers_started.size();
            lock.unlock();
            return count;
       }

       vector<uint32_t>::const_iterator beginProducers()
       {
            lock.lock();
            auto it = producers.begin();
            lock.unlock();
            return it;
       }
       vector<uint32_t>::const_iterator endProducers()
       {
            lock.lock();
            auto it = producers.end();
            lock.unlock();
            return it;
       }
       vector<uint32_t>::const_iterator beginConsumers()
       {
            lock.lock();
            auto it = consumers.begin();
            lock.unlock();
            return it;
       }
       vector<uint32_t>::const_iterator endConsumers()
       {
            lock.lock();
            auto it = consumers.end();
            lock.unlock();
            return it;
       }
       vector<uint32_t>::const_iterator beginRouters()
       {
            lock.lock();
            auto it = routers.begin();
            lock.unlock();
            return it;
       }
       vector<uint32_t>::const_iterator endRouters()
       {
            lock.unlock();
            auto it = routers.end();
            lock.unlock();
            return it;
       }

       void producerReceivedRequest( const Name& producer_name,
                                     const Name& request_name )
       {
            if( logging_enabled )
            {
                lock.lock();
                log_file << "Producer:Received" << endl
                         << "{" << endl
                         << "    time     = " <<  time::system_clock::now() << endl
                         << "    producer = " <<  producer_name.toUri() << endl
                         << "    request  = "  << request_name.toUri()  << endl
                         << "}" << endl;
                 lock.unlock();
             }
       }
       void producerDeniedRequest( const Name& producer_name,
                                   const Name& request_name,
                                   const string& why )
       {
            if( logging_enabled )
            {
                lock.lock();
                log_file << "Producer:Denied" << endl
                         << "{" << endl
                         << "    time     = " <<  time::system_clock::now() << endl
                         << "    producer = " << producer_name.toUri() << endl
                         << "    request  = " << request_name.toUri() << endl
                         << "    why      = " << why << endl
                         << "}" << endl;
                lock.unlock();
            }
       }
      void producerSatisfiedRequest( const Name& producer_name,
                                     const Name& request_name )
      {
            if( logging_enabled )
            {
                lock.lock();
                log_file << "Producer:Satisfied" << endl
                         << "{" << endl
                         << "    time     = " <<  time::system_clock::now() << endl
                         << "    producer = " << producer_name.toUri() << endl
                         << "    request  = " << request_name.toUri() << endl
                         << "}" << endl;
                 lock.unlock();
             }
      }

      void producerReceivedAuthRequest( const Name& producer_name,
                                        const Name& request_name,
                                        const NdnParameterSet& credentials )
      {
            if( logging_enabled )
            {
                lock.lock();
                log_file << "Producer:ReceivedAuth" << endl
                         << "{" << endl
                         << "    time     = " <<  time::system_clock::now() << endl
                         << "    producer = " << producer_name.toUri() << endl
                         << "    request  = " << request_name.toUri() << endl
                         << "    username = " <<
                                    ( ( credentials.hasParameter( "username" ) )
                                      ? credentials.getParameter( "username" )
                                      : "none" )
                                    << endl
                         << "    password = "<<
                                    ( ( credentials.hasParameter( "password" ) )
                                      ? credentials.getParameter( "password" )
                                      : "none" )
                                    << endl
                         << "}" << endl;
                 lock.unlock();
             }
      }
      void producerDeniedAuthRequest( const Name& producer_name,
                                      const Name& request_name,
                                      const string& why,
                                      const NdnParameterSet& credentials )
      {
            if( logging_enabled )
            {
                lock.lock();
                log_file << "Producer:DeniedAuth" << endl
                         << "{" << endl
                         << "    time     = " <<  time::system_clock::now() << endl
                         << "    producer = " << producer_name.toUri() << endl
                         << "    request  = " << request_name.toUri() << endl
                         << "    username = " <<
                                    ( ( credentials.hasParameter( "username" ) )
                                      ? credentials.getParameter( "username" )
                                      : "none" )
                                    << endl
                         << "    password = " <<
                                    ( ( credentials.hasParameter( "password" ) )
                                      ? credentials.getParameter( "password" )
                                      : "none" )
                                    << endl
                         << "    why      = " << why << endl
                         << "}" << endl;
                 lock.unlock();
             }
      }
      void producerSatisfiedAuthRequest( const Name& producer_name,
                                         const Name& request_name,
                                         const NdnParameterSet& credentials )
      {
            if( logging_enabled )
            {
                lock.lock();
                log_file << "Producer:SatisfiedAuth" << endl
                         << "{" << endl
                         << "    time     = " <<  time::system_clock::now() << endl
                         << "    producer = " << producer_name.toUri() << endl
                         << "    request  = " << request_name.toUri() << endl
                         << "    username = " <<
                                    ( ( credentials.hasParameter( "username" ) )
                                      ? credentials.getParameter( "username" )
                                      : "none" )
                                    << endl
                         << "    password = " <<
                                    ( ( credentials.hasParameter( "password" ) )
                                      ? credentials.getParameter( "password" )
                                      : "none" )
                                    << endl
                         << "}" << endl;
                 lock.unlock();
             }
      }

       void producerOther( const Name& producer_name,
                           const string& msg )
       {
            if( logging_enabled )
            {
                lock.lock();
                log_file << "Producer:Other" << endl
                         << "{" << endl
                         << "    time     = " <<  time::system_clock::now() << endl
                         << "    producer = " << producer_name.toUri() << endl
                         << "    msg      = " << msg << endl
                         << "}" << endl;
                 lock.unlock();
             }
       }

      void consumerSentRequest( uint32_t consumer_instance,
                                const Name& request_name )
      {
            if( logging_enabled )
            {
                lock.lock();
                putPending();

                log_file << "Consumer:Requested" << endl
                         << "{" << endl
                         << "    time     = " <<  time::system_clock::now() << endl
                         << "    consumer = " << consumer_instance << endl
                         << "    request  = " << request_name.toUri() << endl
                         << "}" << endl;
                 lock.unlock();
             }
      }

      void consumerRequestSatisfied( uint32_t consumer_instance,
                                      const Name& request_name )
      {
            if( logging_enabled )
            {
                lock.lock();
                removePending();
                log_file << "Consumer:RequestSatisfied" << endl
                         << "{" << endl
                         << "    time     = " <<  time::system_clock::now() << endl
                         << "    consumer = " << consumer_instance << endl
                         << "    request  = " << request_name.toUri() << endl
                         << "}" << endl;
                lock.unlock();
            }
      }

      void consumerRequestRejected( uint32_t consumer_instance,
                                     const Name& request_name )
      {
            if( logging_enabled )
            {
                lock.lock();
                removePending();
                log_file << "Consumer:RequestRejected" << endl
                         << "{" << endl
                         << "    time     = " <<  time::system_clock::now() << endl
                         << "    consumer = " << consumer_instance << endl
                         << "    request  = " << request_name.toUri() << endl
                         << "}" << endl;
                lock.unlock();
            }
      }

      void consumerRequestedAuth( uint32_t consumer_instance,
                                   const Name& request_name )
      {
            if( logging_enabled )
            {
                lock.lock();
                putPending();

                log_file << "Consumer:RequestedAuth" << endl
                         << "{" << endl
                         << "    time     = " <<  time::system_clock::now() << endl
                         << "    consumer = " << consumer_instance << endl
                         << "    request  = " << request_name.toUri() << endl
                         << "}" << endl;
                lock.unlock();
            }
      }
      void consumerReceivedAuth( uint32_t consumer_instance,
                                  const Name& request_name )
      {
            if( logging_enabled )
            {
                lock.lock();
                removePending();
                log_file << "Consumer:ReceivedAuth" << endl
                         << "{" << endl
                         << "    time     = " <<  time::system_clock::now() << endl
                         << "    consumer = " << consumer_instance << endl
                         << "    request  = " << request_name.toUri() << endl
                         << "}" << endl;
                lock.unlock();
            }

      }
      void consumerAuthDenied( uint32_t consumer_instance,
                                const Name& request_name )
      {
            if( logging_enabled )
            {
                lock.lock();
                removePending();
                log_file << "Consumer:AuthDenied" << endl
                         << "{" << endl
                         << "    time     = " <<  time::system_clock::now() << endl
                         << "    consumer = " << consumer_instance << endl
                         << "    request  = " << request_name.toUri() << endl
                         << "}" << endl;
                lock.unlock();
            }
      }

      void consumerFollowedLink( uint32_t consumer_instance,
                                  const Name& request_name )
      {
            if( logging_enabled )
            {
                lock.lock();
                log_file << "Consumer:FollowedLink" << endl
                         << "{" << endl
                         << "    time     = " <<  time::system_clock::now() << endl
                         << "    consumer = " << consumer_instance << endl
                         << "    link     = " << request_name.toUri() << endl
                         << "}" << endl;
                 lock.unlock();
             }
      }

        void consumerOther( uint32_t consumer_instance,
                            const string& msg )
       {
            if( logging_enabled )
            {
                lock.lock();
                log_file << "Consumer:Other" << endl
                         << "{" << endl
                         << "    time     = " <<  time::system_clock::now() << endl
                         << "    consumer = " << consumer_instance << endl
                         << "    msg      = " << msg << endl
                         << "}" << endl;
                 lock.unlock();
             }
       }

       void routerReceivedRequest( uint32_t router_instance,
                                   const Name& request_name )
       {
            if( logging_enabled )
            {
                lock.lock();
                log_file << "Router:Received" << endl
                         << "{" << endl
                         << "    time     = " << time::system_clock::now() << endl
                         << "    router   = " << router_instance << endl
                         << "    request  = " << request_name.toUri() << endl
                         << "}" << endl;
                 lock.unlock();
             }
       }
       void routerForwardedRequest( uint32_t router_instance,
                                    const Name& request_name )
       {
            if( logging_enabled )
            {
                lock.lock();
                log_file << "Router:Forwarded" << endl
                         << "{" << endl
                         << "    time     = " << time::system_clock::now() << endl
                         << "    router   = " << router_instance << endl
                         << "    request  = " << request_name.toUri() << endl
                         << "}" << endl;
                 lock.unlock();
             }
       }
       void routerDeniedRequest( uint32_t router_instance,
                                 const Name& request_name,
                                 const string& why )
       {
            if( logging_enabled )
            {
                lock.lock();
                log_file << "Router:Denied" << endl
                         << "{" << endl
                         << "    time     = " << time::system_clock::now() << endl
                         << "    router   = " << router_instance << endl
                         << "    request  = " << request_name.toUri() << endl
                         << "    why      = " << why << endl
                         << "}" << endl;
                 lock.unlock();
             }
       }
       void routerSatisfiedRequest( uint32_t router_instance,
                                    const Name& request_name )
       {
            if( logging_enabled )
            {
                lock.lock();
                log_file << "Router:Satisfied" << endl
                         << "{" << endl
                         << "    time     = " << time::system_clock::now() << endl
                         << "    router   = " << router_instance << endl
                         << "    request  = " << request_name.toUri() << endl
                         << "}" << endl;
                 lock.unlock();
             }
       }

       void routerRequestedAuth( uint32_t router_instance,
                                 const Name& request_name )
       {
            if( logging_enabled )
            {
                lock.lock();
                log_file << "Router:RequestedAuth" << endl
                         << "{" << endl
                         << "    time     = " << time::system_clock::now() << endl
                         << "    router   = " << router_instance << endl
                         << "    request  = " << request_name.toUri() << endl
                         << "}" << endl;
                 lock.unlock();
             }
       }
       void routerAuthSatisfied( uint32_t router_instance,
                                 const Name& request_name )
       {
            if( logging_enabled )
            {
                lock.lock();
                log_file << "Router:AuthSatisfied" << endl
                         << "{" << endl
                         << "    time     = " << time::system_clock::now() << endl
                         << "    router   = " << router_instance << endl
                         << "    request  = " << request_name.toUri() << endl
                         << "}" << endl;
                 lock.unlock();
             }
       }
       void routerAuthDenied( uint32_t router_instance,
                              const Name& request_name )
       {
            if( logging_enabled )
            {
                lock.lock();
                log_file << "Router:ReceivedAuth" << endl
                         << "{" << endl
                         << "    time     = " << time::system_clock::now() << endl
                         << "    router   = " << router_instance << endl
                         << "    request  = " << request_name.toUri() << endl
                         << "}" << endl;
                 lock.unlock();
             }
       }

       void routerOther( uint32_t router_instance,
                         const string& msg )
       {
            if( logging_enabled )
            {
                lock.lock();
                log_file << "Router:Other" << endl
                         << "{" << endl
                         << "    time     = " <<  time::system_clock::now() << endl
                         << "    router   = " << router_instance << endl
                         << "    msg      = " << msg << endl
                         << "}" << endl;
                 lock.unlock();
             }
       }

       void edgeSettingValidityProbability( uint32_t edge_instance,
                                            const Name& request_name,
                                            uint32_t prob,
                                            const string& msg )
       {
         lock.lock();
         log_file << "Edge:SettingValidityProbability" << endl
                  << "{" << endl
                  << "    time     = " <<  time::system_clock::now() << endl
                  << "    edge     = " << edge_instance << endl
                  << "    request  = " << request_name.toUri() << endl
                  << "    prob     = " << prob << endl
                  << "    msg      = " << msg << endl
                  << "}" << endl;
          lock.unlock();
       }

       void edgeDroppingRequest( uint32_t edge_instance,
                                 const Name& request_name,
                                 const string& why )
       {
         lock.lock();
         log_file << "Edge:DroppingRequest" << endl
                  << "{" << endl
                  << "    time     = " <<  time::system_clock::now() << endl
                  << "    edge     = " << edge_instance << endl
                  << "    request  = " << request_name.toUri() << endl
                  << "    why      = " << why << endl
                  << "}" << endl;
          lock.unlock();
       }

       void edgeCachingTag( uint32_t edge_instance,
                            const Name& request_name,
                            const string& filter_name )
       {
         lock.lock();
         log_file << "Edge:CachingTag" << endl
                  << "{" << endl
                  << "    time     = " <<  time::system_clock::now() << endl
                  << "    edge     = " << edge_instance << endl
                  << "    request  = " << request_name.toUri() << endl
                  << "    filter   = " << filter_name << endl
                  << "}" << endl;
          lock.unlock();
       }

       void simulationStarted( bool enable_logging )
       {
                logging_enabled = enable_logging;
                lock.lock();
                log_file.open("scratch/SIMUATION.log");
                log_file << "Simulation:Started" << endl
                         << "{" << endl
                         << "    time      = " << time::system_clock::now() << endl
                         << "    nodes     = " << nodeCount() << endl
                         << "    producers = " << producerCount() << endl
                         << "    consumers = " << consumerCount() << endl
                         << "}" << endl;
                 lock.unlock();
       }

       void simulationFinished()
       {
                lock.lock();
                log_file << "Simulation:Finished" << endl
                         << "{" << endl
                         << "    time      = " << time::system_clock::now() << endl
                         << "    nodes     = " << nodeCount() << endl
                         << "    producers = " << producerStartedCount() << "/"
                                               << producerCount() << endl
                         << "    consumers = " << consumerStartedCount() << "/"
                                               << consumerCount() << endl
                         << "    pending   = " << pending << endl
                         << "}" << endl;

                 log_file.close();
                 lock.unlock();
       }

        void simulationOther( const string& msg )
       {
            if( logging_enabled )
            {
                lock.lock();
                log_file << "Simulation:Other" << endl
                         << "{" << endl
                         << "    time     = " <<  time::system_clock::now() << endl
                         << "    msg      = " << msg << endl
                         << "}" << endl;
                lock.unlock();
            }
       }
   }

}

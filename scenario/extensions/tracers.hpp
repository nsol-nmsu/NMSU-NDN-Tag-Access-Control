/**
* A bundle of tracers and installers
**/

#include "ns3/traced-callback.h"
#include "ns3/core-module.h"
#include "ns3/node-container.h"
#include "ndn-cxx/interest.hpp"
#include "ndn-cxx/data.hpp"
#include "ndn-cxx/auth-tag.hpp"
#include <memory>
#include <string>


#ifndef TRACERS__INCLUDED
#define TRACERS__INCLUDED

namespace ndntac
{
namespace tracers
{

// validation trace detail
enum ValidationDetail
{
    ValidationSuccessValProb,
    ValidationSuccessBloom,
    ValidationSuccessSig,
    ValidationSuccessSkipped,
    ValidationFailureSig,
    ValidationFailureNoAuth,
    ValidationFailureLowAuth,
    ValidationFailureBadKeyLoc,
    ValidationFailureExpired,
    ValidationFailureBadPrefix,
    ValidationFailureBadRoute
};

enum AuthCachedDetail
{
    CachedPositiveUpstream,
    CachedPositiveMoved,
    CachedNegativeUpstream
};

enum BlockedDetail
{
    BlockedExpired,
    BlockedBadPrefix,
    BlockedBadRoute
};


// tracers
struct ProducerTrace : ns3::Object
{
    static ns3::TypeId
    GetTypeId( void );
    
    ns3::TracedCallback< const ndn::AuthTag& >
    tag_created;
    
    
    ns3::TracedCallback
    < const ndn::AuthTag&, ns3::Time /*delay*/>
    sigverif;
    
    ns3::TracedCallback
    < const ndn::AuthTag&, ns3::Time /*delay*/>
    bloom_lookup;
    
    ns3::TracedCallback
    < const ndn::AuthTag&, ns3::Time /*delay*/>
    bloom_insert;
    
    ns3::TracedCallback
    < ValidationDetail >
    validation;
    
    ns3::TracedCallback
    < const ndn::Interest& >
    received_interest;
    
    ns3::TracedCallback
    < const ndn::Data& >
    sent_data;
};
extern ns3::Ptr< ProducerTrace > producer;

struct ConsumerTrace : ns3::Object
{
    static ns3::TypeId
    GetTypeId( void );
    
    // trace the interests sent
    ns3::TracedCallback
    < const ndn::Interest& >
    interest;
    
    // trace the datas received, and their associated times
    ns3::TracedCallback
    < const ndn::Data&, ns3::Time /*sent*/,
      ns3::Time/*last retx*/, ns3::Time /*received*/>
    data;
    
    // received nack
    ns3::TracedCallback
    < uint32_t /*seq*/ >
    denied;
    
    // received nack even though we have a valid auth
    ns3::TracedCallback
    < uint32_t /*seq*/ >
    deserved;
    
    // received data even though we have invalid auth
    ns3::TracedCallback
    < uint32_t /*seq*/ >
    undeserved;
    
    // received a data without requesting it
    ns3::TracedCallback
    < >
    unrequested;
    
    // received an auth
    ns3::TracedCallback
    < const ndn::AuthTag& >
    auth_received;
    
    // disposed of an auth
    ns3::TracedCallback
    < const ndn::AuthTag& >
    auth_disposed;
    
    // retransmission
    ns3::TracedCallback
    < uint32_t /*seq*/ >
    retx;
    
    // timeout
    ns3::TracedCallback
    < uint32_t /*seq*/ >
    timeout;
};
extern ns3::Ptr< ConsumerTrace > consumer;


// tracers
struct RouterTrace : ns3::Object
{
    static ns3::TypeId
    GetTypeId( void );
    
    ns3::TracedCallback
    < const ndn::AuthTag&, ns3::Time /*delay*/>
    sigverif;
    
    ns3::TracedCallback
    < const ndn::AuthTag&, ns3::Time /*delay*/>
    bloom_lookup;
    
    ns3::TracedCallback
    < const ndn::AuthTag&, ns3::Time /*delay*/>
    bloom_insert;
    
    ns3::TracedCallback
    < ValidationDetail >
    validation;
    
    ns3::TracedCallback
    < const ndn::Interest& >
    sent_interest;
    
    ns3::TracedCallback
    < const ndn::Data& >
    sent_data;
};
extern ns3::Ptr< RouterTrace > router;

// tracers
struct EdgeTrace : ns3::Object
{
    static ns3::TypeId
    GetTypeId( void );
    
    // data entered network
    ns3::TracedCallback
    < const ndn::Data& >
    data_entered;
    
    // data left network
    ns3::TracedCallback
    < const ndn::Data& >
    data_left;
    
    // interest entered network
    ns3::TracedCallback
    < const ndn::Interest& >
    interest_entered;
    
    // interest left network
    ns3::TracedCallback
    < const ndn::Interest& >
    interest_left;
    
    // auth tag cached
    ns3::TracedCallback
    < const ndn::AuthTag&, AuthCachedDetail >
    auth_cached;
    
    // blocked interest
    ns3::TracedCallback
    < const ndn::Interest&, BlockedDetail >
    blocked_interest;

    // signature verified
    ns3::TracedCallback
    < const ndn::AuthTag&, ns3::Time /*delay*/>
    sigverif;
    
    // bloom lookup occured
    ns3::TracedCallback
    < const ndn::AuthTag&, ns3::Time /*delay*/>
    bloom_lookup;
    
    // bloom insert occured
    ns3::TracedCallback
    < const ndn::AuthTag&, ns3::Time /*delay*/>
    bloom_insert;
};
extern ns3::Ptr< EdgeTrace > edge;



// traces the total number of tags
// created by producers
void
EnableTagsCreatedTrace
( const std::string& logfile,
  ns3::Time interval );

// traces the total number of tags
// in use by consumers
void
EnableTagsActiveTrace
( const std::string& logfile,
  ns3::Time interval );

// traces the total number of signature
// verifications
void
EnableTagSigVerifTrace
( const std::string& logfile,
  ns3::Time interval );

// traces the total number of bloom
// lookups and inserts
void
EnableTagBloomTrace
( const std::string& logfile,
  ns3::Time interval );

// traces the total overhead introduced
// by NDNTAC, both computational ( delay )
// and transmission byte count
void
EnableOverheadTrace
( const std::string& logfile,
  ns3::Time interval );

// this enables the ndnSIM L2Tracer
void
EnableDropTrace
( const std::string& logfile,
  ns3::Time interval );

// this enables the ndnSIM L3RateTracer
void
EnableRateTrace
( const std::string& logfile,
  ns3::Time interval );


// this traces the number of validation
// successes and failures that occur
// and their reasons
void
EnableValidationTrace
( const std::string& logfile,
  ns3::Time interval );

// this traces the total number of packets
// and byte count transmitted at the router
// strategy and app layer of NDNTAC, not very
// usefull if RateTrace is already enabled
void
EnableTransmissionTrace
( const std::string& logfile,
  ns3::Time interval );

// traces the number of interests blocked by
// the edge router and the reasons
void
EnableEdgeBlockTrace
( const std::string& logfile,
  ns3::Time interval );

// traces number of datas, nacks, and whatnot
// received by the consumer
void
EnableConsumerTrace
( const std::string& logfile,
  ns3::Time interval );


};
};

#endif // TRACERS__INCLUDED

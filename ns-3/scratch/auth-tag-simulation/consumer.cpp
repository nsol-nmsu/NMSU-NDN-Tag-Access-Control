#include "consumer.hpp"

namespace ndntac
{
  const ns3::Time Consumer::s_consumer_signature_delay = ns3::MilliSeconds( 121.641253 );
  const ns3::Time Consumer::s_consumer_bloom_delay = ns3::MilliSeconds( 9.238432 );
  const ns3::Time Consumer::s_consumer_interest_delay = ns3::MilliSeconds( 100 );
  uint32_t Producer::s_instance_id = 0;

  ns3::TypeId
  Consumer::GetTypeId()
  {
          static ns3::TypeId tid
              = ns3::TypeId("ndntac::Producer")
                .SetParent<ns3::ndn::App>()
                .AddConstructor<Consumer>()
                .AddAttribute(
                      "IntervalMin",
                      "Shortest amount of time between interests",
                       ns3::StringValue("0.1s"),
                       MakeTimeAccessor(&Consumer::m_min_interval),
                       ns3::MakeTimeChecker() )
               .AddAttribute(
                     "IntervalMax",
                     "Longest amount of time between interests",
                      ns3::StringValue("0.2s"),
                      MakeTimeAccessor(&Consumer::m_max_interval),
                      ns3::MakeTimeChecker() )
              .AddAttribute(
                    "KnownProducers",
                    "Colon sepereated list of known producer prefixes",
                     ns3::StringValue(""),
                     MakeStringAccessor(&Consumer::m_known_producers_list ),
                     ns3::MakeStringChecker() );
          return tid;
  }

  Consumer::Consumer()
  {
    m_instance_id = s_instance_id++;
  }


}

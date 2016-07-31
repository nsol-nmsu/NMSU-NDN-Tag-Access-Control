#include "consumer.hpp"
#include "coordinator.hpp"
#include "ndn-cxx/auth-tag.hpp"
#include <boost/regex.hpp>

namespace ndntac
{
  const ns3::Time Consumer::s_consumer_signature_delay = ns3::MilliSeconds( 121.641253 );
  const ns3::Time Consumer::s_consumer_bloom_delay = ns3::MilliSeconds( 9.238432 );
  const ns3::Time Consumer::s_consumer_interest_delay = ns3::MilliSeconds( 100 );
  uint32_t Consumer::s_instance_id = 0;

  NS_OBJECT_ENSURE_REGISTERED(Consumer);

  ns3::TypeId
  Consumer::GetTypeId()
  {
          static ns3::TypeId tid
              = ns3::TypeId("ndntac::Consumer")
                .SetParent<ns3::ndn::App>()
                .AddConstructor<Consumer>()
                .AddAttribute(
                      "IntervalMin",
                      "Shortest amount of time between interests",
                       ns3::StringValue("0.01s"),
                       MakeTimeAccessor(&Consumer::m_min_interval),
                       ns3::MakeTimeChecker() )
               .AddAttribute(
                     "IntervalMax",
                     "Longest amount of time between interests",
                      ns3::StringValue("0.1s"),
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
        Coordinator::addConsumer( m_instance_id );
  }

  void
  Consumer::StartApplication()
  {
    App::StartApplication();
    parseKnownProducers( m_known_producers_list );
    sendNext();
    Coordinator::consumerStarted( m_instance_id );
  }

  void
  Consumer::StopApplication()
  {
    Coordinator::consumerStopped( m_instance_id );
    App::StopApplication();
  }

  void
  Consumer::OnData( shared_ptr< const ndn::Data > data )
  {

    // if it's an authentication response then parse and handle
    if( data->getContentType() == ndn::tlv::ContentType_AuthGranted )
    {
        const ndn::Block& payload = data->getContent().blockFromValue();
        ndn::AuthTag tag( payload );
        m_auth_tags[data->getName().getPrefix(1)] = tag;
        Coordinator::consumerReceivedAuth( m_instance_id, data->getName() );
        return;
    }

    if( data->getContentType() == ndn::tlv::ContentType_Nack )
    {
        Coordinator::consumerRequestRejected( m_instance_id, data->getName() );
        return;
    }

    // look for links
    Coordinator::consumerRequestSatisfied( m_instance_id, data->getName() );
    const ndn::Block& payload = data->getContent();
    string content( (char*)payload.wire(), payload.size() );

    const static boost::regex link_regex("<a>(.+?)</a>" );
    boost::smatch link_match;
    auto begin = boost::sregex_iterator( content.begin(),
                                         content.end(),
                                         link_regex );
    auto end = boost::sregex_iterator();
    for( auto it = begin ; it != end ; it++ )
    {
        Coordinator::consumerFollowedLink( m_instance_id, (*it)[1].str() );
        if( hasAuth( (*it)[1].str() ) )
        {
          requestData( (*it)[1].str() );
        }
        else
        {
          requestAuth( (*it)[1].str() );
        }
    }
  }

  void
  Consumer::sendNext()
  {
    if( m_interest_queue.size() == 0 )
    {
        // if nothing queued request from one of the known producers

        unsigned which = rand() % m_known_producers.size();
        ndn::Name name = m_known_producers[ which ];
        name.append("index.html");

        if( hasAuth( name ) )
            requestData( name );
        else
            requestAuth( name );
        sendNext();
        return;
    }

    Coordinator::consumerSentRequest( m_instance_id, m_interest_queue.front()->getName() );
    m_queue.receiveInterest( m_face, m_interest_queue.front() );
    m_interest_queue.pop();

    using ns3::Simulator;
    using ns3::Seconds;
    Simulator::Schedule( Seconds( m_min_interval + ( m_max_interval - m_min_interval) / rand() )
                          , &Consumer::sendNext
                          , this );
  }

  bool
  Consumer::hasAuth( const ndn::Name& name )
  {
    return m_auth_tags.find( name.getPrefix( 1 ) ) != m_auth_tags.end();
  }

  void
  Consumer::requestAuth( const ndn::Name& name )
  {
    m_interest_queue.push( make_shared<ndn::Interest>(name.getPrefix(1).append("AUTH_TAG") ) );
    ndn::Interest& interest = *m_interest_queue.front();
    interest.setNonce( rand() );
  }

  void
  Consumer::requestData( const ndn::Name& name )
  {
    m_interest_queue.push( make_shared<ndn::Interest>( name ) );
    ndn::Interest& interest = *m_interest_queue.front();

    if( hasAuth( name ) )
        interest.setAuthTag( m_auth_tags[ name.getPrefix( 1 ) ] );
    interest.setInterestLifetime( ndn::time::seconds( 5 ) );
    interest.setNonce( rand() );
  }

  void
  Consumer::parseKnownProducers( const std::string& plist )
  {
    auto it = plist.begin();
    while( it != plist.end() && *it != ':' ) it++;

    if( *it == ':')
    {
      m_known_producers.emplace_back( plist.substr( 0, it - plist.begin() ) );
      parseKnownProducers( plist.substr( it - plist.begin() + 1, plist.length() ) );
    }
  }

}

#include "edge-strategy.hpp"
#include "ndn-cxx/encoding/block-helpers.hpp"
#include "ns3/ndnSIM/utils/dummy-keychain.hpp"
#include "coordinator.hpp"
#include "ndn-cxx/encoding/block-helpers.hpp"

namespace ndntac
{

  const ndn::Name
  EdgeStrategy::STRATEGY_NAME = "ndn:/localhost/nfd/strategy/ndntac-edge-strategy";
  const ns3::Time
  EdgeStrategy::s_edge_signature_delay = ns3::NanoSeconds( 30345 );
  const ns3::Time
  EdgeStrategy::s_edge_bloom_delay = ns3::NanoSeconds( 2535 );
  const ns3::Time
  EdgeStrategy::s_edge_interest_delay = ns3::MilliSeconds( 0 );
  
  
  EdgeStrategy::EdgeStrategy( nfd::Forwarder& forwarder,
                              const ndn::Name& name )
                                : RouterStrategy( forwarder, name )
                                , m_positive_cache( 1e-10, 10000 )
                                , m_negative_cache( 1e-10, 10000 )
  { }

  bool
  EdgeStrategy::onIncomingInterest( nfd::Face& face,
                                    const ndn::Interest& const_interest )
  {
    // TODO: might need to add some interest freshness checks here
    
    // unconstify interest
    ndn::Interest& interest = const_cast<ndn::Interest&>( const_interest );
    
    // update route hash
    interest.updateRouteHash( m_instance_id );
    
    
    // this is a simulation hack, we append an element to
    // the end of interest names once they enter the network
    // and remove the element once they exit the network, we do
    // this to avoid reauthenticating tags at entry and exit of
    // the network.  Real routers should know which direction packets
    // are heading ( into or out of the edge router's network ), so
    // this wouldn't be necessary in a real system.
    if( interest.getName().get( -1 ) == ndn::name::Component("IN_NETWORK") )
    {
        // when a packet exits the network
        interest.setName( interest.getName().getPrefix(-1) );
        return RouterStrategy::onIncomingInterest( face, interest );
    }
    else
    {
        // when a packet enters the network
        ndn::Name name = interest.getName();
        name.append( "IN_NETWORK" );
        interest.setName( name );
    }

    // get auth tag
    const ndn::AuthTag tag = interest.getAuthTag();

    // if interest is an AuthRequest then set its route hash
    // in the name to be packaged by the authentication provider
    if( interest.getName().get(-2) == ndn::name::Component("AUTH_TAG") )
    {
        ndn::Name name = interest.getName().getPrefix(-1);
        name.appendNumber( interest.getRouteHash() );
        name.append( "IN_NETWORK" );
        interest.setName( name );
    }
    // otherwise ensure valid route hash
    else if( tag.getRouteHash() != interest.getRouteHash() )
    {
      stringstream ss;
      ss << "Invalid route hash '"
      << interest.getRouteHash()
      << "' should be '"
      << tag.getRouteHash() << "'";
      Coordinator::edgeDroppingRequest( m_instance_id,
                                        interest.getName(),
                                        ss.str() );
      // send nack
      auto data = make_shared< ndn::Data >( interest.getName().getPrefix(-1) );
      data->setContentType( ndn::tlv::ContentType_Nack );
      data->setFreshnessPeriod( ndn::time::seconds( 0 ) );
      data->setSignature( ndn::security::DUMMY_NDN_SIGNATURE );
      data->wireEncode();
      m_queue.sendData( face.shared_from_this(), data );
      return true;
    }
    
    
    // if access level is 0 we don't need to do anything
    if( tag.getAccessLevel() == 0 )
    {
      return RouterStrategy::onIncomingInterest( face, interest );
    }

    // if tag is in positive cache then set validity prob accordingly
    m_queue.delay( s_edge_bloom_delay );
    if( m_positive_cache.contains( tag ) )
    {
      uint32_t prob = ( 1.0 - m_positive_cache.getEffectiveFPP() ) * 0xFFFFFFFF;

      Coordinator::edgeSettingValidityProbability(m_instance_id,
                                                  interest.getName(),
                                                  prob,
                                                  "AuthTag in positive cache");
      interest.setAuthValidityProb( prob );
    }
    // if not in positive but negative then verify tag ourselves
    else
    {
        m_queue.delay( s_edge_bloom_delay );
        if( m_negative_cache.contains( tag ) )
        {
          // simulate verification time with a delay
          m_queue.delay( s_edge_signature_delay );

          // dummy bad signature has 0 in first byte
          if( interest.getSignature().getValue().value_size() == 0
              || interest.getSignature().getValue().value()[0] == 0 )
          {
            Coordinator::edgeDroppingRequest( m_instance_id,
                                              interest.getName(),
                                              "Negative cache, bad signature");
            auto data = make_shared< ndn::Data >( interest.getName().getPrefix(-1) );
            data->setContentType( ndn::tlv::ContentType_Nack );
            data->setFreshnessPeriod( ndn::time::seconds( 0 ) );
            data->setSignature( ndn::security::DUMMY_NDN_SIGNATURE );
            data->wireEncode();
            m_queue.sendData( face.shared_from_this(), data );
            return true;
          }

          // set validity prob to max
          Coordinator::edgeSettingValidityProbability(m_instance_id,
                                              interest.getName(),
                                              0xFFFFFFFF,
                                              "Negative cache, verified manually");
          interest.setAuthValidityProb( 0xFFFFFFFF );
        }
        // if not in either cache
        else
        {
          Coordinator::edgeSettingValidityProbability(m_instance_id,
                                                      interest.getName(),
                                                      0,
                                                      "Neither cache" );
          interest.setAuthValidityProb( 0 );
        }
    }

    // do normal router stuff
    return RouterStrategy::onIncomingInterest( face, interest );
  }

  void
  EdgeStrategy::beforeSatisfyInterest( shared_ptr<nfd::pit::Entry> pitEntry,
                                       const nfd::Face& inFace,
                                       const ndn::Data& const_data)
  {
  
    ndn::Data& data = const_cast<ndn::Data&>(const_data);
    data.updateRouteHash( m_instance_id );
    
    // this is a simulation hack, we append an element to
    // the end of data names once they enter the network
    // and remove the element once they exit the network,
    // we do this for consistency with interest names and
    // to avaoid duplicate tag caching at both ends of the
    // network
    if( data.getName().get( -1 ) == ndn::name::Component("IN_NETWORK") )
    {
        // when a packet exits the network
        data.setName( data.getName().getPrefix(-1) );
    }
    else
    {
        // when a packet enters the network
        ndn::Name name = data.getName();
        name.append( "IN_NETWORK" );
        data.setName( name );
    }
    // data detects change in its name, so we need to re-sign
    data.setSignature( ndn::security::DUMMY_NDN_SIGNATURE );
    data.wireEncode();

    const ndn::AuthTag& tag = pitEntry->getInterest().getAuthTag();

    if( tag.getAccessLevel() > 0 )
    {
      // if tag access > 0 then cache the tag if data is not denial
      if( data.getContentType() != ndn::tlv::ContentType_AuthDenial
       && data.getContentType() != ndn::tlv::ContentType_Nack )
      {
        if( !data.getNoReCacheFlag() )
        {
          Coordinator::edgeCachingTag( m_instance_id,
                                       data.getName(),
                                       "positive");
          m_positive_cache.insert( tag );
        }
      }
      // if auth denial then send nack
      else
      {
        Coordinator::edgeCachingTag( m_instance_id,
                                     data.getName(),
                                     "negative");
        m_negative_cache.insert( tag );
        
        // TODO: implement aggragation and auth denial
      }
    }

  }
}

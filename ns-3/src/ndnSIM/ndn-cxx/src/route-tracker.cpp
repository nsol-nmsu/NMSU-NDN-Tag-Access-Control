/**
* This file is an addition by Ray Stubbs [ stubbs.ray@gmail.com ]
* I haven't yet gotten around to adding the licensing header.
**/
#include "route-tracker.hpp"

namespace ndn
{
    RouteTracker::RouteTracker( const Block& wire )
    {
        wireDecode( wire );
    };

    RouteTracker::RouteTracker()
        : m_current_network( ENTRY_NETWORK )
        , m_entry_route( 0 )
        , m_internet_route( 0 )
        , m_exit_route( 0 ) {};
    
    void RouteTracker::update( uint64_t link_id )
    {
        switch( m_current_network )
        {
            case NetworkType::ENTRY_NETWORK:
                m_entry_route ^= link_id;
                break;
            case NetworkType::INTERNET_NETWORK:
                m_internet_route ^= link_id;
                break;
            case NetworkType::EXIT_NETWORK:
                m_exit_route ^= link_id;
                break;
        }
        m_wire.reset();
    }
    
    void RouteTracker::setCurrentNetwork( RouteTracker::NetworkType type )
    {
        m_current_network = type;
        m_wire.reset();
    }
    
    RouteTracker::NetworkType RouteTracker::getCurrentNetwork() const
    {
        return m_current_network;
    }
    
    uint64_t RouteTracker::getEntryRoute() const
    {
        return m_entry_route;
    }
    
    uint64_t RouteTracker::getInternetRoute() const
    {
        return m_internet_route;
    }
    
    uint64_t RouteTracker::getExitRoute() const
    {
        return m_exit_route;
    }
    

    template<encoding::Tag TAG>
    size_t RouteTracker::wireEncode( EncodingImpl< TAG > &encoder ) const
    {
        /**
        * ROUTE-TRACKER  ::= ROUTE_TRACKER-TYPE TLV-LENGTH
        *                   CurrentNetwork
        *                   EntryRoute
        *                   InternetRoute
        *                   ExitRoute
        * ( reverse encoding )
        **/

        size_t length = 0;

        // ExitRoute
        length += prependNonNegativeIntegerBlock( encoder,
                                                  tlv::ExitRoute,
                                                  m_exit_route );
        // InternetRoute
        length += prependNonNegativeIntegerBlock( encoder,
                                                  tlv::InternetRoute,
                                                  m_internet_route );
        // EntryRoute
        length += prependNonNegativeIntegerBlock( encoder,
                                                  tlv::EntryRoute,
                                                  m_entry_route );
        // CurrentNetwork
        length += prependNonNegativeIntegerBlock( encoder,
                                                  tlv::CurrentNetwork,
                                                  m_current_network );


         // Type and Size for AuthTag
         length += encoder.prependVarNumber( length );
         length += encoder.prependVarNumber( tlv::RouteTracker );
         return length;

    };
    
    template size_t
    RouteTracker::wireEncode<encoding::EncoderTag>
    ( EncodingImpl< encoding::EncoderTag > &encoder ) const;
    
    template size_t
    RouteTracker::wireEncode<encoding::EstimatorTag>
    ( EncodingImpl< encoding::EstimatorTag > &encoder ) const;


    const Block& RouteTracker::wireEncode() const
    {
        if( m_wire.hasWire() )
            return m_wire;

        EncodingEstimator estimator;
        size_t size = wireEncode( estimator );

        EncodingBuffer buffer( size, 0 );
        wireEncode( buffer );

        m_wire = buffer.block(true);

        return m_wire;
    };

    void RouteTracker::wireDecode( const Block& wire )
    {
        m_wire = wire;
        m_wire.parse();

        /**
        * ROUTE-TRACKER  ::= ROUTE_TRACKER-TYPE TLV-LENGTH
        *                   CurrentNetwork
        *                   EntryRoute
        *                   InternetRoute
        *                   ExitRoute
        **/
        
        // CurrentNetwork
        m_current_network = (NetworkType)readNonNegativeInteger( m_wire.get( tlv::CurrentNetwork ) );

        // EntryRoute
        m_entry_route = readNonNegativeInteger( m_wire.get( tlv::EntryRoute ) );

        // InternetRoute
        m_internet_route = readNonNegativeInteger( m_wire.get( tlv::InternetRoute ) );

        // ExitRoute
        m_exit_route = readNonNegativeInteger( m_wire.get( tlv::ExitRoute ) );
    };

    bool RouteTracker::hasWire() const
    {
        return m_wire.hasWire();
    };
}

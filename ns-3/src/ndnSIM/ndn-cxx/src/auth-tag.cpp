/**
* This file is an addition by Ray Stubbs [ stubbs.ray@gmail.com ]
* I haven't yet gotten around to adding the licensing header.
**/
#include "auth-tag.hpp"

namespace ndn
{
    AuthTag::AuthTag( const Block& wire )
    {
        wireDecode( wire );
    };

    AuthTag::AuthTag( uint8_t access_level ): m_access_level( access_level )
    {
    };

    AuthTag::AuthTag(): m_access_level( 0 )
    {
    };

    template<encoding::Tag TAG>
    size_t AuthTag::wireEncode( EncodingImpl< TAG > &encoder ) const
    {
        /**
        * AUTH-TAG  ::= AUTH_TAG-TYPE TLV-LENGTH
        *                   Name              -- Producer's prefix
        *                   AccessLevel
        *                   RouteHash           -- Route hash for route from comsumer to edge router
        *                   KeyLocator          -- Consumer Locator
        *                   SignatureInfo
        *                   Signature
        * ( reverse encoding )
        **/

        size_t length = 0;

        // SignatureValue
        length += encoder.prependBlock( m_signature.getValue() );

        //----------------- SignedPortion ------------------------//
        size_t signed_length = 0;

        // SignatureInfo
        signed_length += encoder.prependBlock( m_signature.getInfo() );

        // KeyLocator -- ConsumerLocator
        signed_length += encoder.prependBlock( m_consumer_locator.wireEncode() );

        // RouteHash
        signed_length += prependNonNegativeIntegerBlock( encoder,
                                                         tlv::RouteHash,
                                                         m_route_hash );

        // AccessLevel
        signed_length += prependNonNegativeIntegerBlock( encoder,
                                                         tlv::AccessLevel,
                                                         m_access_level );

        // Name --  Producer's prefix
        signed_length += encoder.prependBlock( m_prefix.wireEncode() );

        // Type and size for SignedPortion
        signed_length += encoder.prependVarNumber( signed_length );
        signed_length += encoder.prependVarNumber( tlv::SignedPortion );

        length += signed_length;

         // Type and Size for AuthTag
         length += encoder.prependVarNumber( length );
         length += encoder.prependVarNumber( tlv::AuthTag );
         return length;

    };


    const Block& AuthTag::wireEncode() const
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

    void AuthTag::wireDecode( const Block& wire )
    {
        m_wire = wire;
        m_wire.parse();

        /**
        * AUTH-TAG  ::= AUTH_TAG-TYPE TLV-LENGTH
        *                   Name                -- Producer's prefix
        *                   AccessLevel
        *                   RouteHash           -- Route hash for route from comsumer to edge router
        *                   KeyLocator          -- Consumer Locator
        *                   SignatureInfo
        *                   Signature
        **/

        const Block& sportion = m_wire.get( tlv::SignedPortion );

        // Name -- Producer's prefix
        m_prefix.wireDecode( sportion.get( tlv::Name ) );

        // AccessLevel
        m_access_level = readNonNegativeInteger( sportion.get( tlv::AccessLevel ) );

        // RouteHash
        m_route_hash = readNonNegativeInteger( sportion.get( tlv::RouteHash ) );

        // KeyLocator -- Consumer Locator
        m_consumer_locator.wireDecode( sportion.get( tlv::KeyLocator ) );

        // Signature
        m_signature = Signature( sportion.get( tlv::SignatureInfo ),
                                m_wire.get( tlv::SignatureValue ) );
    };

    bool AuthTag::hasWire()
    {
        return m_wire.hasWire();
    };
}

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2013-2015 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 */

#include "data.hpp"
#include "encoding/block-helpers.hpp"
#include "util/crypto.hpp"

namespace ndn {

BOOST_CONCEPT_ASSERT((boost::EqualityComparable<Data>));
BOOST_CONCEPT_ASSERT((WireEncodable<Data>));
BOOST_CONCEPT_ASSERT((WireEncodableWithEncodingBuffer<Data>));
BOOST_CONCEPT_ASSERT((WireDecodable<Data>));
static_assert(std::is_base_of<tlv::Error, Data::Error>::value,
              "Data::Error must inherit from tlv::Error");

Data::Data()
  : m_content(tlv::Content) // empty content
{
}

Data::Data(const Name& name)
  : m_name(name)
{
}

Data::Data(const Block& wire)
{
  wireDecode(wire);
}

Data::Data( const Data& data )
{
    m_localControlHeader = data.getLocalControlHeader();
    wireDecode( data.wireEncode() );
}

template<encoding::Tag TAG>
size_t
Data::wireEncode(EncodingImpl<TAG>& encoder, bool only_signed_portion/* = false*/) const
{
  size_t totalLength = 0;

  // Data ::= DATA-TLV TLV-LENGTH
  //            SignedPortion:
  //              Name
  //              AccessLevel
  //              MetaInfo
  //              Content
  //              SignatureInfo
  //            SignatureValue
  //            NoReCacheFlag?
  //            RouteTracker

  // (reverse encoding)

  if (!only_signed_portion && !m_signature)
    {
      BOOST_THROW_EXCEPTION(Error("Requested wire format, but data packet has not been signed yet"));
    }

  if (!only_signed_portion)
    {
      // RouteTracker
      if( m_route_tracker )
      {
          totalLength += encoder.prependBlock( m_route_tracker->wireEncode() );
      }

      // NoReCacheFlag
      if( m_no_recache_flag )
      {
        totalLength += encoder.prependVarNumber( 0 );
        totalLength += encoder.prependBlock(  Block( tlv::NoReCacheFlag ) );
      }

      // SignatureValue
      totalLength += encoder.prependBlock(m_signature.getValue());
    }

  //---------- SignedPortion --------------//
  size_t signed_length = 0;

  // SignatureInfo
  signed_length += encoder.prependBlock( m_signature.getInfo() );

  // Content
  signed_length += encoder.prependBlock( getContent() );

  // MetaInfo
  signed_length += encoder.prependBlock( getMetaInfo().wireEncode() );

  // AccessLevel
  signed_length += prependNonNegativeIntegerBlock( encoder,
                                                   tlv::AccessLevel,
                                                   m_access_level );

  // Name
  signed_length += encoder.prependBlock( getName().wireEncode() );

  // Type and Length for SignedPortion
  signed_length += encoder.prependVarNumber( signed_length );
  signed_length += encoder.prependVarNumber( tlv::SignedPortion );

  totalLength += signed_length;

  if (!only_signed_portion)
    {
      totalLength += encoder.prependVarNumber(totalLength);
      totalLength += encoder.prependVarNumber(tlv::Data);
    }
  return totalLength;
}


template size_t
Data::wireEncode<encoding::EncoderTag>(EncodingImpl<encoding::EncoderTag>& encoder,
                                       bool unsignedPortion) const;

template size_t
Data::wireEncode<encoding::EstimatorTag>(EncodingImpl<encoding::EstimatorTag>& encoder,
                                         bool unsignedPortion) const;


const Block&
Data::wireEncode(EncodingBuffer& encoder, const Block& signatureValue) const
{
  size_t totalLength = encoder.size();
  totalLength += encoder.appendBlock(signatureValue);
  if( m_no_recache_flag )
    totalLength += encoder.appendBlock( Block( tlv::NoReCacheFlag) );
  totalLength += encoder
                 .appendBlock( m_route_tracker->wireEncode() );

  encoder.prependVarNumber(totalLength);
  encoder.prependVarNumber(tlv::Data);

  const_cast<Data*>(this)->wireDecode(encoder.block());
  return m_wire;
}

const Block&
Data::wireEncode() const
{
  if (m_wire.hasWire())
    return m_wire;

  EncodingEstimator estimator;
  size_t estimatedSize = wireEncode(estimator);

  EncodingBuffer buffer(estimatedSize, 0);
  wireEncode(buffer);

  const_cast<Data*>(this)->wireDecode(buffer.block());
  return m_wire;
}

void
Data::wireDecode(const Block& wire)
{

  m_fullName.clear();
  m_wire = wire;
  m_wire.parse();

  // Data ::= DATA-TLV TLV-LENGTH
  //            SignedPortion:
  //              Name
  //              AccessLevel
  //              MetaInfo
  //              Content
  //              SignatureInfo
  //            SignatureValue
  //            NoReCacheFlag?
  //            RouteHash

  //-------------- SignedPortion ----------------- //
  const Block& sportion = m_wire.get( tlv::SignedPortion );
  sportion.parse();

  // Name
  m_name.wireDecode( sportion.get(tlv::Name));

 // AccessLevel
 auto val = sportion.find( tlv::AccessLevel );
 if( val != sportion.elements_end() )
    m_access_level =
        readNonNegativeInteger( sportion.get( tlv::AccessLevel ) );

  // MetaInfo
  m_metaInfo.wireDecode( sportion.get(tlv::MetaInfo));

  // Content
  m_content = sportion.get(tlv::Content);

  // SignatureInfo
  m_signature.setInfo( sportion.get(tlv::SignatureInfo) );

//----------------- End SignedPortion ---------------------//

  // SignatureValue
  val = m_wire.find(tlv::SignatureValue);
  if (val != m_wire.elements_end())
    m_signature.setValue(*val);

  // NoReCacheFlag
  val = m_wire.find( tlv::NoReCacheFlag );
  if( val != m_wire.elements_end() )
    m_no_recache_flag = true;

  // RouteTracker
  val = m_wire.find( tlv::RouteTracker );
  if( val != m_wire.elements_end() )
    m_route_tracker.reset( new RouteTracker( *val ) );
}

Data&
Data::setName(const Name& name)
{
  onChanged();
  m_name = name;

  return *this;
}

const Name&
Data::getFullName() const
{
  if (m_fullName.empty()) {
    if (!m_wire.hasWire()) {
      BOOST_THROW_EXCEPTION(Error("Full name requested, but Data packet does not have wire format "
                                  "(e.g., not signed)"));
    }
    m_fullName = m_name;
    m_fullName.appendImplicitSha256Digest(crypto::sha256(m_wire.wire(), m_wire.size()));
  }

  return m_fullName;
}



Data&
Data::setMetaInfo(const MetaInfo& metaInfo)
{
  onChanged();
  m_metaInfo = metaInfo;

  return *this;
}

Data&
Data::setContentType(uint32_t type)
{
  onChanged();
  m_metaInfo.setType(type);

  return *this;
}

Data&
Data::setFreshnessPeriod(const time::milliseconds& freshnessPeriod)
{
  onChanged();
  m_metaInfo.setFreshnessPeriod(freshnessPeriod);

  return *this;
}

Data&
Data::setFinalBlockId(const name::Component& finalBlockId)
{
  onChanged();
  m_metaInfo.setFinalBlockId(finalBlockId);

  return *this;
}

const Block&
Data::getContent() const
{
  if (m_content.empty())
    m_content = makeEmptyBlock(tlv::Content);

  if (!m_content.hasWire())
    m_content.encode();
  return m_content;
}

Data&
Data::setContent(const uint8_t* content, size_t contentLength)
{
  onChanged();

  m_content = makeBinaryBlock(tlv::Content, content, contentLength);

  return *this;
}

Data&
Data::setContent(const ConstBufferPtr& contentValue)
{
  onChanged();

  m_content = Block(tlv::Content, contentValue); // not a real wire encoding yet

  return *this;
}

Data&
Data::setContent(const Block& content)
{
  onChanged();

  if (content.type() == tlv::Content)
    m_content = content;
  else {
    m_content = Block(tlv::Content, content);
  }

  return *this;
}

Data&
Data::setSignature(const Signature& signature)
{
  onChanged();
  m_signature = signature;

  return *this;
}

Data&
Data::setSignatureValue(const Block& value)
{
  onChanged();
  m_signature.setValue(value);

  return *this;
}

//

Data&
Data::setIncomingFaceId(uint64_t incomingFaceId)
{
  getLocalControlHeader().setIncomingFaceId(incomingFaceId);
  // ! do not reset Data's wire !

  return *this;
}

Data&
Data::setCachingPolicy(nfd::LocalControlHeader::CachingPolicy cachingPolicy)
{
  getLocalControlHeader().setCachingPolicy(cachingPolicy);
  // ! do not reset Data's wire !

  return *this;
}

void
Data::onChanged()
{
  // The values have changed, so the wire format is invalidated

  // !!!Note!!! Signature is not invalidated and it is responsibility of
  // the application to do proper re-signing if necessary

  m_wire.reset();
  m_fullName.clear();
}

Data&
Data::operator=( const Data& other )
{
    m_localControlHeader = other.getLocalControlHeader();
    wireDecode( other.wireEncode() );
    return *this;
}

bool
Data::operator==(const Data& other) const
{
  return getName() == other.getName() &&
    getMetaInfo() == other.getMetaInfo() &&
    getContent() == other.getContent() &&
    getSignature() == other.getSignature();
}

bool
Data::operator!=(const Data& other) const
{
  return !(*this == other);
}

std::ostream&
operator<<(std::ostream& os, const Data& data)
{
  os << "Name: " << data.getName() << "\n";
  os << "MetaInfo: " << data.getMetaInfo() << "\n";
  os << "Content: (size: " << data.getContent().value_size() << ")\n";
  os << "Signature: (type: " << data.getSignature().getType() <<
    ", value_length: "<< data.getSignature().getValue().value_size() << ")";
  os << std::endl;

  return os;
}

} // namespace ndn

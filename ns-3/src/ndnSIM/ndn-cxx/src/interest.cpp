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

#include "interest.hpp"
#include "util/random.hpp"
#include "util/crypto.hpp"
#include "data.hpp"

namespace ndn {

BOOST_CONCEPT_ASSERT((boost::EqualityComparable<Interest>));
BOOST_CONCEPT_ASSERT((WireEncodable<Interest>));
BOOST_CONCEPT_ASSERT((WireEncodableWithEncodingBuffer<Interest>));
BOOST_CONCEPT_ASSERT((WireDecodable<Interest>));
static_assert(std::is_base_of<tlv::Error, Interest::Error>::value,
              "Interest::Error must inherit from tlv::Error");

Interest::Interest()
  : m_interestLifetime(time::milliseconds::min())
{
}

Interest::Interest(const Name& name)
  : m_name(name)
  , m_interestLifetime(time::milliseconds::min())
{
}

Interest::Interest(const Name& name, const time::milliseconds& interestLifetime)
  : m_name(name)
  , m_interestLifetime(interestLifetime)
{
}

Interest::Interest(const Block& wire)
{
  wireDecode(wire);
}

uint32_t
Interest::getNonce() const
{
  if (!m_nonce.hasWire())
    const_cast<Interest*>(this)->setNonce(random::generateWord32());

  if (m_nonce.value_size() == sizeof(uint32_t))
    return *reinterpret_cast<const uint32_t*>(m_nonce.value());
  else {
    // for compatibility reasons.  Should be removed eventually
    return readNonNegativeInteger(m_nonce);
  }
}

Interest&
Interest::setNonce(uint32_t nonce)
{
  if (m_wire.hasWire() && m_nonce.value_size() == sizeof(uint32_t)) {
    std::memcpy(const_cast<uint8_t*>(m_nonce.value()), &nonce, sizeof(nonce));
  }
  else {
    m_nonce = makeBinaryBlock(tlv::Nonce,
                              reinterpret_cast<const uint8_t*>(&nonce),
                              sizeof(nonce));
    m_wire.reset();
  }
  return *this;
}

void
Interest::refreshNonce()
{
  if (!hasNonce())
    return;

  uint32_t oldNonce = getNonce();
  uint32_t newNonce = oldNonce;
  while (newNonce == oldNonce)
    newNonce = random::generateWord32();

  setNonce(newNonce);
}

bool
Interest::matchesName(const Name& name) const
{
  if (name.size() < m_name.size())
    return false;

  if (!m_name.isPrefixOf(name))
    return false;

  if (getMinSuffixComponents() >= 0 &&
      // name must include implicit digest
      !(name.size() - m_name.size() >= static_cast<size_t>(getMinSuffixComponents())))
    return false;

  if (getMaxSuffixComponents() >= 0 &&
      // name must include implicit digest
      !(name.size() - m_name.size() <= static_cast<size_t>(getMaxSuffixComponents())))
    return false;

  if (!getExclude().empty() &&
      name.size() > m_name.size() &&
      getExclude().isExcluded(name[m_name.size()]))
    return false;

  return true;
}

bool
Interest::matchesData(const Data& data) const
{
  size_t interestNameLength = m_name.size();
  const Name& dataName = data.getName();
  size_t fullNameLength = dataName.size() + 1;

  // check MinSuffixComponents
  bool hasMinSuffixComponents = getMinSuffixComponents() >= 0;
  size_t minSuffixComponents = hasMinSuffixComponents ?
                               static_cast<size_t>(getMinSuffixComponents()) : 0;
  if (!(interestNameLength + minSuffixComponents <= fullNameLength))
    return false;

  // check MaxSuffixComponents
  bool hasMaxSuffixComponents = getMaxSuffixComponents() >= 0;
  if (hasMaxSuffixComponents &&
      !(interestNameLength + getMaxSuffixComponents() >= fullNameLength))
    return false;

  // check prefix
  if (interestNameLength == fullNameLength) {
    if (m_name.get(-1).isImplicitSha256Digest()) {
      if (m_name != data.getFullName())
        return false;
    }
    else {
      // Interest Name is same length as Data full Name, but last component isn't digest
      // so there's no possibility of matching
      return false;
    }
  }
  else {
    // Interest Name is a strict prefix of Data full Name
    if (!m_name.isPrefixOf(dataName))
      return false;
  }

  // check Exclude
  // Exclude won't be violated if Interest Name is same as Data full Name
  if (!getExclude().empty() && fullNameLength > interestNameLength) {
    if (interestNameLength == fullNameLength - 1) {
      // component to exclude is the digest
      if (getExclude().isExcluded(data.getFullName().get(interestNameLength)))
        return false;
      // There's opportunity to inspect the Exclude filter and determine whether
      // the digest would make a difference.
      // eg. "<NameComponent>AA</NameComponent><Any/>" doesn't exclude any digest -
      //     fullName not needed;
      //     "<Any/><NameComponent>AA</NameComponent>" and
      //     "<Any/><ImplicitSha256DigestComponent>ffffffffffffffffffffffffffffffff
      //      </ImplicitSha256DigestComponent>"
      //     excludes all digests - fullName not needed;
      //     "<Any/><ImplicitSha256DigestComponent>80000000000000000000000000000000
      //      </ImplicitSha256DigestComponent>"
      //     excludes some digests - fullName required
      // But Interests that contain the exact Data Name before digest and also
      // contain Exclude filter is too rare to optimize for, so we request
      // fullName no mater what's in the Exclude filter.
    }
    else {
      // component to exclude is not the digest
      if (getExclude().isExcluded(dataName.get(interestNameLength)))
        return false;
    }
  }

  // check PublisherPublicKeyLocator
  const KeyLocator& publisherPublicKeyLocator = this->getPublisherPublicKeyLocator();
  if (!publisherPublicKeyLocator.empty()) {
    const Signature& signature = data.getSignature();
    const Block& signatureInfo = signature.getInfo();
    Block::element_const_iterator it = signatureInfo.find(tlv::KeyLocator);
    if (it == signatureInfo.elements_end()) {
      return false;
    }
    if (publisherPublicKeyLocator.wireEncode() != *it) {
      return false;
    }
  }

  return true;
}

template<encoding::Tag TAG>
size_t
Interest::wireEncode(EncodingImpl<TAG>& encoder, bool only_signed_portion ) const
{
  size_t totalLength = 0;

  // Interest ::= INTEREST-TYPE TLV-LENGTH
  //                SignedPortion
  //                  Name
  //                  Selectors?
  //                  Nonce
  //                  InterestLifetime?
  //                  AuthTag?
  //                  Payload?
  //                  SignatureInfo
  //                SignatureValue
  //                AuthValidityProbability
  //                RouteHash

  // (reverse encoding)

  if( !only_signed_portion )
  {
    // RouteHash
    totalLength += prependNonNegativeIntegerBlock( encoder,
                                                   tlv::RouteHash,
                                                   m_route_hash );

    // AuthValidityProbability
    if( m_auth_validity_prob > 0 )
      totalLength += prependNonNegativeIntegerBlock( encoder,
                                                     tlv::AuthValidityProbability,
                                                     m_auth_validity_prob );

    // SignatureValue
    totalLength += encoder.prependBlock( m_signature.getValue() );
  }

  //----------------- SignedPortion -----------------------//
  size_t signed_length = 0;

  // SignatureInfo
  signed_length += encoder.prependBlock( m_signature.getInfo() );

  // Payload
  if( m_payload.value_size() > 0 )
    signed_length += encoder.prependBlock( m_payload );

  // AuthTag
  if( m_auth_tag.getAccessLevel() > 0 )
  {
    signed_length += m_auth_tag.wireEncode( encoder );
  }

  // InterestLifetime
  if (getInterestLifetime() >= time::milliseconds::zero() &&
      getInterestLifetime() != DEFAULT_INTEREST_LIFETIME)
    {
      signed_length += prependNonNegativeIntegerBlock( encoder,
                                                       tlv::InterestLifetime,
                                                       getInterestLifetime().count());
    }

  // Nonce
  getNonce(); // to ensure that Nonce is properly set
  signed_length += encoder.prependBlock(m_nonce);

  // Selectors
  if (hasSelectors())
  {
    signed_length += getSelectors().wireEncode(encoder);
  }

  // Name
  signed_length += getName().wireEncode( encoder );

  // SignedPortion type and length
  signed_length += encoder.prependVarNumber( signed_length );
  signed_length += encoder.prependVarNumber( tlv::SignedPortion );

  totalLength += signed_length;


  if( !only_signed_portion )
  {
   // Interest type and length
    totalLength += encoder.prependVarNumber(totalLength);
    totalLength += encoder.prependVarNumber(tlv::Interest);
  }
  return totalLength;

}

template<encoding::Tag TAG>
const Block&
Interest::wireEncode(EncodingImpl<TAG>& encoder, const Block& signatureValue ) const
{
  size_t totalLength = encoder.size();
  totalLength += encoder.appendBlock(signatureValue);
    totalLength += encoder
      .appendBlock( makeNonNegativeIntegerBlock( tlv::AuthValidityProbability,
                                                  m_auth_validity_prob ) );
  totalLength += encoder
                 .appendBlock( makeNonNegativeIntegerBlock( tlv::RouteHash,
                                                            m_route_hash ) );

  encoder.prependVarNumber(totalLength);
  encoder.prependVarNumber(tlv::Interest);

  const_cast<Interest*>(this)->wireDecode(encoder.block());
  return m_wire;
}

template size_t
Interest::wireEncode<encoding::EncoderTag>(EncodingImpl<encoding::EncoderTag>&, bool ) const;

template size_t
Interest::wireEncode<encoding::EstimatorTag>(EncodingImpl<encoding::EstimatorTag>&, bool ) const;

const Block&
Interest::wireEncode() const
{
  if (m_wire.hasWire())
    return m_wire;

  EncodingEstimator estimator;
  size_t estimatedSize = wireEncode(estimator);

  EncodingBuffer buffer(estimatedSize, 0);
  wireEncode(buffer);

  // to ensure that Nonce block points to the right memory location
  const_cast<Interest*>(this)->wireDecode(buffer.block());

  return m_wire;
}

void
Interest::wireDecode(const Block& wire)
{
  m_wire = wire;
  m_wire.parse();

  //                SignedPortion
  //                  Name
  //                  Selectors?
  //                  Nonce
  //                  InterestLifetime?
  //                  AuthTag?
  //                  Payload?
  //                  SignatureInfo
  //                SignatureValue
  //                AuthValidityProbability
  //                RouteHash

  if (m_wire.type() != tlv::Interest)
    BOOST_THROW_EXCEPTION(Error("Unexpected TLV number when decoding Interest"));

  //------------- SignedPortion -------------------//
  const Block& sportion = m_wire.get( tlv::SignedPortion );
  sportion.parse();

  // Name
  m_name.wireDecode( sportion.get(tlv::Name) );

  // Selectors
  Block::element_const_iterator val = sportion.find(tlv::Selectors);
  if (val != sportion.elements_end())
    {
      m_selectors.wireDecode(*val);
    }
  else
    m_selectors = Selectors();

  // Nonce
  m_nonce = sportion.get(tlv::Nonce);

  // InterestLifetime
  val = sportion.find(tlv::InterestLifetime);
  if (val != sportion.elements_end())
    {
      m_interestLifetime = time::milliseconds(readNonNegativeInteger(*val));
    }
  else
    {
      m_interestLifetime = DEFAULT_INTEREST_LIFETIME;
    }

  // AuthTag
  val = sportion.find( tlv::AuthTag );
  if( val != sportion.elements_end() )
    m_auth_tag.wireDecode( *val );

  // Payload
  val = sportion.find( tlv::Payload );
  if( val != sportion.elements_end() )
    m_payload = *val;

  // SignatureInfo
  m_signature.setInfo( sportion.get( tlv::SignatureInfo ) );

  //------------------- End SignedPortion ------------------------//

  // SignatureValue
  val = m_wire.find( tlv::SignatureValue );
  if( val != m_wire.elements_end() )
    m_signature.setValue( *val );

  // AuthValidityProbability
  val = m_wire.find( tlv::AuthValidityProbability );
  if( val != m_wire.elements_end() )
    m_auth_validity_prob = readNonNegativeInteger( *val );
  else
    m_auth_validity_prob = 0;

  // RouteHash
  m_route_hash = readNonNegativeInteger( m_wire.get( tlv::RouteHash ) );


}

std::ostream&
operator<<(std::ostream& os, const Interest& interest)
{
  os << interest.getName();

  char delim = '?';

  if (interest.getMinSuffixComponents() >= 0) {
    os << delim << "ndn.MinSuffixComponents=" << interest.getMinSuffixComponents();
    delim = '&';
  }
  if (interest.getMaxSuffixComponents() >= 0) {
    os << delim << "ndn.MaxSuffixComponents=" << interest.getMaxSuffixComponents();
    delim = '&';
  }
  if (interest.getChildSelector() >= 0) {
    os << delim << "ndn.ChildSelector=" << interest.getChildSelector();
    delim = '&';
  }
  if (interest.getMustBeFresh()) {
    os << delim << "ndn.MustBeFresh=" << interest.getMustBeFresh();
    delim = '&';
  }
  if (interest.getInterestLifetime() >= time::milliseconds::zero()
      && interest.getInterestLifetime() != DEFAULT_INTEREST_LIFETIME) {
    os << delim << "ndn.InterestLifetime=" << interest.getInterestLifetime().count();
    delim = '&';
  }

  if (interest.hasNonce()) {
    os << delim << "ndn.Nonce=" << interest.getNonce();
    delim = '&';
  }
  if (!interest.getExclude().empty()) {
    os << delim << "ndn.Exclude=" << interest.getExclude();
    delim = '&';
  }

  return os;
}

} // namespace ndn

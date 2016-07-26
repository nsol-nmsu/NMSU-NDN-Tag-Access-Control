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

#ifndef NDN_INTEREST_HPP
#define NDN_INTEREST_HPP

#include "common.hpp"

#include "name.hpp"
#include "selectors.hpp"
#include "util/time.hpp"
#include "management/nfd-local-control-header.hpp"
#include "tag-host.hpp"
#include "auth-tag.hpp"

namespace ndn {

class Data;

/** @var const unspecified_duration_type DEFAULT_INTEREST_LIFETIME;
 *  @brief default value for InterestLifetime
 */
const time::milliseconds DEFAULT_INTEREST_LIFETIME = time::milliseconds(4000);

/**
* @class Interest
* This class has been modified by Ray Stubbs [stubbs.ray@gmail.com]
* to accomodate the new fields introduced by NMSU's tag based verification
**/

/** @brief represents an Interest packet
 */
class Interest : public TagHost, public enable_shared_from_this<Interest>
{
public:
  class Error : public tlv::Error
  {
  public:
    explicit
    Error(const std::string& what)
      : tlv::Error(what)
    {
    }
  };

  /** @brief Create a new Interest with an empty name (`ndn:/`)
   *  @warning In certain contexts that use Interest::shared_from_this(), Interest must be created
   *           using `make_shared`. Otherwise, .shared_from_this() will throw an exception.
   */
  Interest();

  /** @brief Create a new Interest with the given name
   *  @param name The name for the interest.
   *  @note This constructor allows implicit conversion from Name.
   *  @warning In certain contexts that use Interest::shared_from_this(), Interest must be created
   *           using `make_shared`. Otherwise, .shared_from_this() will throw an exception.
   */
  Interest(const Name& name);

  /** @brief Create a new Interest with the given name and interest lifetime
   *  @param name             The name for the interest.
   *  @param interestLifetime The interest lifetime in time::milliseconds, or -1 for none.
   *  @warning In certain contexts that use Interest::shared_from_this(), Interest must be created
   *           using `make_shared`. Otherwise, .shared_from_this() will throw an exception.
   */
  Interest(const Name& name, const time::milliseconds& interestLifetime);

  /** @brief Create from wire encoding
   *  @warning In certain contexts that use Interest::shared_from_this(), Interest must be created
   *           using `make_shared`. Otherwise, .shared_from_this() will throw an exception.
   */
  explicit
  Interest(const Block& wire);

  /**
   * @brief Fast encoding or block size estimation
   */
  template<encoding::Tag TAG>
  size_t
  wireEncode(EncodingImpl<TAG>& encoder, bool only_signed_portion = false ) const;

  /**
   * @brief Finalize encoding with specified signature
   */
  template<encoding::Tag TAG>
  const Block&
  wireEncode(EncodingImpl<TAG>& encoder, const Block& signatureValue ) const;

  /**
   * @brief Encode to a wire format
   */
  const Block&
  wireEncode() const;

  /**
   * @brief Decode from the wire format
   */
  void
  wireDecode(const Block& wire);

  /**
   * @brief Check if already has wire
   */
  bool
  hasWire() const
  {
    return m_wire.hasWire();
  }

  /**
   * @brief Encode the name according to the NDN URI Scheme
   *
   * If there are interest selectors, this method will append "?" and add the selectors as
   * a query string.  For example, "/test/name?ndn.ChildSelector=1"
   */
  std::string
  toUri() const;

public: // matching
  /** @brief Check if Interest, including selectors, matches the given @p name
   *  @param name The name to be matched. If this is a Data name, it shall contain the
   *              implicit digest component
   */
  bool
  matchesName(const Name& name) const;

  /**
   * @brief Check if Interest can be satisfied by @p data.
   *
   * This method considers Name, MinSuffixComponents, MaxSuffixComponents,
   * PublisherPublicKeyLocator, and Exclude.
   * This method does not consider ChildSelector and MustBeFresh.
   *
   * @todo recognize implicit digest component
   */
  bool
  matchesData(const Data& data) const;

public: // Name and guiders
  const Name&
  getName() const
  {
    return m_name;
  }

  Interest&
  setName(const Name& name)
  {
    m_name = name;
    m_wire.reset();
    return *this;
  }

  const time::milliseconds&
  getInterestLifetime() const
  {
    return m_interestLifetime;
  }

  Interest&
  setInterestLifetime(const time::milliseconds& interestLifetime)
  {
    m_interestLifetime = interestLifetime;
    m_wire.reset();
    return *this;
  }

  const AuthTag&
  getAuthTag() const
  {
    return m_auth_tag;
  }

  Interest&
  setAuthTag( const AuthTag& tag )
  {
    m_auth_tag = tag;
    m_wire.reset();
    return *this;
  }

  uint32_t getAuthValidityProb() const
  {
    return m_auth_validity_prob;
  }

  Interest&
  setAuthValidityProb( uint32_t prob )
  {
    m_auth_validity_prob = prob;
    m_wire.reset();
    return *this;
  }

  uint64_t
  getRouteHash() const
  {
    return m_route_hash;
  }

  Interest&
  setRouteHash( uint64_t hash )
  {
    m_route_hash = hash;
    m_wire.reset();
    return *this;
  }

  const Block&
  getPayload() const
  {
    return m_payload;
  }

  Interest&
  setPayload( const Block& payload )
  {
    m_payload = payload;
    m_wire.reset();
    return *this;
  }

  Interest&
  setSignature( const Signature& sig )
  {
    m_signature = sig;
    m_wire.reset();
    return *this;
  }

  const Signature&
  getSignature() const
  {
    return m_signature;
  }

  const Interest&
  setSignatureValue( const Block& signatureValue )
  {
    m_signature.setValue( signatureValue );
    m_wire.reset();
    return *this;
  }

  /** @brief Check if Nonce set
   */
  bool
  hasNonce() const
  {
    return m_nonce.hasWire();
  }

  /** @brief Get Interest's nonce
   *
   *  If nonce was not set before this call, it will be automatically assigned to a random value
   */
  uint32_t
  getNonce() const;

  /** @brief Set Interest's nonce
   *
   *  If wire format already exists, this call simply replaces nonce in the
   *  existing wire format, without resetting and recreating it.
   */
  Interest&
  setNonce(uint32_t nonce);

  /** @brief Refresh nonce
   *
   *  It's guaranteed that new nonce value differs from the existing one.
   *
   *  If nonce is already set, it will be updated to a different random value.
   *  If nonce is not set, this method does nothing.
   */
  void
  refreshNonce();

public: // local control header
  nfd::LocalControlHeader&
  getLocalControlHeader()
  {
    return m_localControlHeader;
  }

  const nfd::LocalControlHeader&
  getLocalControlHeader() const
  {
    return m_localControlHeader;
  }

  uint64_t
  getIncomingFaceId() const
  {
    return getLocalControlHeader().getIncomingFaceId();
  }

  Interest&
  setIncomingFaceId(uint64_t incomingFaceId)
  {
    getLocalControlHeader().setIncomingFaceId(incomingFaceId);
    // ! do not reset Interest's wire !
    return *this;
  }

  uint64_t
  getNextHopFaceId() const
  {
    return getLocalControlHeader().getNextHopFaceId();
  }

  Interest&
  setNextHopFaceId(uint64_t nextHopFaceId)
  {
    getLocalControlHeader().setNextHopFaceId(nextHopFaceId);
    // ! do not reset Interest's wire !
    return *this;
  }

public: // Selectors
  /**
   * @return true if Interest has any selector present
   */
  bool
  hasSelectors() const
  {
    return !m_selectors.empty();
  }

  const Selectors&
  getSelectors() const
  {
    return m_selectors;
  }

  Interest&
  setSelectors(const Selectors& selectors)
  {
    m_selectors = selectors;
    m_wire.reset();
    return *this;
  }

  int
  getMinSuffixComponents() const
  {
    return m_selectors.getMinSuffixComponents();
  }

  Interest&
  setMinSuffixComponents(int minSuffixComponents)
  {
    m_selectors.setMinSuffixComponents(minSuffixComponents);
    m_wire.reset();
    return *this;
  }

  int
  getMaxSuffixComponents() const
  {
    return m_selectors.getMaxSuffixComponents();
  }

  Interest&
  setMaxSuffixComponents(int maxSuffixComponents)
  {
    m_selectors.setMaxSuffixComponents(maxSuffixComponents);
    m_wire.reset();
    return *this;
  }

  const KeyLocator&
  getPublisherPublicKeyLocator() const
  {
    return m_selectors.getPublisherPublicKeyLocator();
  }

  Interest&
  setPublisherPublicKeyLocator(const KeyLocator& keyLocator)
  {
    m_selectors.setPublisherPublicKeyLocator(keyLocator);
    m_wire.reset();
    return *this;
  }

  const Exclude&
  getExclude() const
  {
    return m_selectors.getExclude();
  }

  Interest&
  setExclude(const Exclude& exclude)
  {
    m_selectors.setExclude(exclude);
    m_wire.reset();
    return *this;
  }

  int
  getChildSelector() const
  {
    return m_selectors.getChildSelector();
  }

  Interest&
  setChildSelector(int childSelector)
  {
    m_selectors.setChildSelector(childSelector);
    m_wire.reset();
    return *this;
  }

  int
  getMustBeFresh() const
  {
    return m_selectors.getMustBeFresh();
  }

  Interest&
  setMustBeFresh(bool mustBeFresh)
  {
    m_selectors.setMustBeFresh(mustBeFresh);
    m_wire.reset();
    return *this;
  }

public: // EqualityComparable concept
  bool
  operator==(const Interest& other) const
  {
    return wireEncode() == other.wireEncode();
  }

  bool
  operator!=(const Interest& other) const
  {
    return !(*this == other);
  }

private:
  Name m_name;
  Selectors m_selectors;
  mutable Block m_nonce;
  time::milliseconds m_interestLifetime;
  AuthTag m_auth_tag;
  Signature m_signature;
  uint32_t m_auth_validity_prob = 0;
  uint64_t m_route_hash = 0;
  Block    m_payload;

  mutable Block m_wire;

  nfd::LocalControlHeader m_localControlHeader;
  friend class nfd::LocalControlHeader;
};

std::ostream&
operator<<(std::ostream& os, const Interest& interest);

inline std::string
Interest::toUri() const
{
  std::ostringstream os;
  os << *this;
  return os.str();
}

} // namespace ndn

#endif // NDN_INTEREST_HPP

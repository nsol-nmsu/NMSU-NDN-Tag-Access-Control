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
#include "route-tracker.hpp"

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
  
  /**
  * @brief Network types, @see m_current_network
  **/
  enum NetworkType
  {
    ENTRY_NETWORK = 0,
    INTERNET_NETWORK = 1,
    EXIT_NETWORK = 2
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
  * @brief Copy
  **/
  Interest( const Interest& other );
  
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
    if ( !m_auth_tag )
      BOOST_THROW_EXCEPTION(Error("Requested AuthTag tag does not exist"));
    return *m_auth_tag;
  }
  
  const RouteTracker&
  getRouteTracker() const
  {
    if ( !m_route_tracker )
      BOOST_THROW_EXCEPTION(Error("Requested RouteTracker does not exist"));
    return *m_route_tracker;
  }
  
  const bool
  hasAuthTag() const
  {
    return m_auth_tag != NULL;
  }
  
  const bool
  hasRouteTracker() const
  {
    return m_route_tracker != NULL;
  }

  Interest&
  setAuthTag( const AuthTag& tag )
  {
    m_auth_tag.reset( new AuthTag( tag ) );
    m_wire.reset();
    return *this;
  }
  
  Interest&
  setRouteTracker( const RouteTracker& tracker )
  {
    m_route_tracker.reset( new RouteTracker( tracker ) );
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
  getEntryRoute() const
  {
    return getRouteTracker().getEntryRoute();
  }

  uint64_t
  getInternetRoute() const
  {
    return getRouteTracker().getInternetRoute();
  }

  uint64_t
  getExitRoute() const
  {
    return getRouteTracker().getExitRoute();
  }
  
  NetworkType
  getCurrentNetwork() const
  {
    return m_current_network;
  }

  Interest&
  updateRoute( uint64_t link_id )
  {
    if( !m_route_tracker )
        setRouteTracker( RouteTracker() );
    m_route_tracker->update( link_id );
    m_wire.reset();
    return *this;
  }
  
  Interest&
  setCurrentNetwork( RouteTracker::NetworkType type )
  {
    m_route_tracker->setCurrentNetwork( type );
    return *this;
  }

  Interest&
  setSignature( const Signature& sig )
  {
    if (sig.getValue().type() != tlv::SignatureValue )
      BOOST_THROW_EXCEPTION(Error("Expected Block of type tlv::SignatureValue"));
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
    if (signatureValue.type() != tlv::SignatureValue )
      BOOST_THROW_EXCEPTION(Error("Expected Block of type tlv::SignatureValue"));
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
  unique_ptr<AuthTag> m_auth_tag;
  unique_ptr<RouteTracker> m_route_tracker;
  Signature m_signature;
  uint32_t m_auth_validity_prob = 0;
  
  ///@{
  /**
  * @brief Route hashes, all routers in the network will XOR one of these
  *        hashes with the ID of the incoming link, which hash is updated
  *        depends on router location
  *
  * Routers local to the client network will update the m_entry_route.
  * All routers in between the client local network and the provider local network
  * will update the m_internet_route hash.
  * Routers in the producer local network will update the m_exit_route
  * 
  * Keeping these routes separate allows for finer grained control route tracking,
  * we can get the full route hash by XORing all hashes together.
  **/
  uint64_t m_internet_route = 0;
  uint64_t m_entry_route = 0;
  uint64_t m_exit_route = 0;
  ///@}
  
  /**
  * @brief Keeps track of the current location of the interest
  *
  * This is used by the routers to know which route hash to update
  *
  * It'll either be set to
  * 0 = Client Local Network = ENTRY_NETWORK,
  * 1 = Intermediate Network = INTERNET_NETWORK,
  * 2 = Provider Local Network = EXIT_NETWORK
  **/
  NetworkType m_current_network;
  

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

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

 /** @class Data
 * This class and its implementation have been modified by
 * Ray Stubbs [stubbs.ray@gmail.com] to accomodate the new
 * fields introduced by NMSU's tag based access control mechanim
 */

#ifndef NDN_DATA_HPP
#define NDN_DATA_HPP

#include "common.hpp"
#include "name.hpp"
#include "encoding/block.hpp"

#include "signature.hpp"
#include "meta-info.hpp"
#include "key-locator.hpp"
#include "management/nfd-local-control-header.hpp"
#include "tag-host.hpp"
#include "route-tracker.hpp"

namespace ndn {

/** @brief represents a Data packet
 */
class Data : public TagHost, public enable_shared_from_this<Data>
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
   * @brief Create an empty Data object
   *
   * Note that in certain contexts that use Data::shared_from_this(), Data must be
   * created using `make_shared`:
   *
   *     shared_ptr<Data> data = make_shared<Data>();
   *
   * Otherwise, Data::shared_from_this() will throw an exception.
   */
  Data();

  /**
   * @brief Create a new Data object with the given name
   *
   * @param name A reference to the name
   *
   * Note that in certain contexts that use Data::shared_from_this(), Data must be
   * created using `make_shared`:
   *
   *     shared_ptr<Data> data = make_shared<Data>(name);
   *
   * Otherwise, Data::shared_from_this() will throw an exception.
   */
  Data(const Name& name);

  /**
   * @brief Create a new Data object from wire encoding
   *
   * Note that in certain contexts that use Data::shared_from_this(), Data must be
   * created using `make_shared`:
   *
   *     shared_ptr<Data> data = make_shared<Data>(wire);
   *
   * Otherwise, Data::shared_from_this() will throw an exception.
   */
  explicit
  Data(const Block& wire);
  
  /**
  * @brief Copy
  **/
  Data( const Data& );

  /**
   * @brief Fast encoding or block size estimation
   *
   * @param encoder                 EncodingEstimator or EncodingBuffer instance
   * @param only_signed_portion     Request only the portion of the data that is
   *                                supposed to be signed.  The @ref SignedPortion.
   */
  template<encoding::Tag TAG>
  size_t
  wireEncode(EncodingImpl<TAG>& encoder, bool only_signed_portion = false) const;

  /**
   * @brief Encode to a wire format
   */
  const Block&
  wireEncode() const;

  /**
   * @brief Finalize Data packet encoding with the specified SignatureValue
   *
   * @param encoder        EncodingBuffer instance, containing Name, MetaInfo, Content, and
   *                       SignatureInfo (without outer TLV header of the Data packet).
   * @param signatureValue SignatureValue block to be added to Data packet to finalize
   *                       the wire encoding
   *
   * This method is intended to be used in concert with Data::wireEncode(EncodingBuffer&, true)
   * method to optimize Data packet wire format creation:
   *
   *     Data data;
   *     ...
   *     EncodingBuffer encoder;
   *     data.wireEncode(encoder, true);
   *     ...
   *     Block signatureValue = <sign_over_unsigned_portion>(encoder.buf(), encoder.size());
   *     data.wireEncode(encoder, signatureValue)
   */
  const Block&
  wireEncode(EncodingBuffer& encoder, const Block& signatureValue) const;

  /**
   * @brief Decode from the wire format
   */
  void
  wireDecode(const Block& wire);

  /**
   * @brief Check if Data is already has wire encoding
   */
  bool
  hasWire() const;

  ////////////////////////////////////////////////////////////////////

  /**
   * @brief Get name of the Data packet
   */
  const Name&
  getName() const;

  /**
   * @brief Set name to a copy of the given Name
   *
   * @return This Data so that you can chain calls to update values
   */
  Data&
  setName(const Name& name);

  //

  /**
   * @brief Get full name of Data packet, including the implicit digest
   *
   * @throws Error if Data packet doesn't have a full name yet (wire encoding has not been
   *         yet created)
   */
  const Name&
  getFullName() const;

  /**
  * @brief Get access level
  **/
  uint8_t
  getAccessLevel() const;

  /**
  * @brief Set access level
  **/
  Data&
  setAccessLevel( uint8_t level );

  /**
   * @brief Get MetaInfo block from Data packet
   */
  const MetaInfo&
  getMetaInfo() const;

  /**
   * @brief Set metaInfo to a copy of the given MetaInfo
   *
   * @return This Data so that you can chain calls to update values.
   */
  Data&
  setMetaInfo(const MetaInfo& metaInfo);

  //

  ///////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////
  // MetaInfo proxy methods

  uint32_t
  getContentType() const;

  Data&
  setContentType(uint32_t type);

  //

  const time::milliseconds&
  getFreshnessPeriod() const;

  Data&
  setFreshnessPeriod(const time::milliseconds& freshnessPeriod);

  //

  const name::Component&
  getFinalBlockId() const;

  Data&
  setFinalBlockId(const name::Component& finalBlockId);

  //
  ///////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////

  /**
   * @brief Get content Block
   *
   * To access content value, one can use value()/value_size() or
   * value_begin()/value_end() methods of the Block class
   */
  const Block&
  getContent() const;

  /**
   * @brief Set the content from the buffer (buffer will be copied)
   *
   * @param buffer Pointer to first byte of the buffer
   * @param bufferSize Size of the buffer
   *
   * @return This Data so that you can chain calls to update values.
   */
  Data&
  setContent(const uint8_t* buffer, size_t bufferSize);

  /**
   * @brief Set the content from the block
   *
   * Depending on type of the supplied block, there are two cases:
   *
   * - if block.type() == tlv::Content, then block will be used directly as Data packet's
   *   content (no extra copying)
   *
   * - if block.type() != tlv::Content, then this method will create a new Block with type
   *   tlv::Content and put block as a nested element in the content Block.
   *
   * @param block The Block containing the content to assign
   *
   * @return This Data so that you can chain calls to update values.
   */
  Data&
  setContent(const Block& block);

  /**
   * @brief Set the content from the pointer to immutable buffer
   *
   * This method will create a Block with tlv::Content and set contentValue as a payload
   * for this block.  Note that this method is very different from setContent(const
   * Block&), since it does not require that payload should be a valid TLV element.
   *
   * @param contentValue The pointer to immutable buffer containing the content to assign
   *
   * @return This Data so that you can chain calls to update values.
   */
  Data&
  setContent(const ConstBufferPtr& contentValue);

  //

  const Signature&
  getSignature() const;

  /**
   * @brief Set the signature to a copy of the given signature.
   * @param signature The signature object which is cloned.
   */
  Data&
  setSignature(const Signature& signature);

  Data&
  setSignatureValue(const Block& value);

  /**
  * @brief Get the value of the NoReCacheFlag
  **/
  bool
  getNoReCacheFlag() const;

  /**
  * @brief Set the value of the NoReCacheFlag
  **/
  Data&
  setNoReCacheFlag( bool flag );

  /**
  * @brief Get the route hash
  **/
  const RouteTracker&
  getRouteTracker() const;
  
  /**
  * @brief Get current network type
  **/
  RouteTracker::NetworkType
  getCurrentNetwork() const;
  
  /**
  * @brief Set the current network
  **/
  void
  setCurrentNetwork( RouteTracker::NetworkType type );
  
  /**
  * @brief has route tracker
  **/
  bool
  hasRouteTracker() const;
  
  ///@{
  /**
  * @brief get specific route hashes
  **/
  uint64_t getEntryRoute() const;
  uint64_t getInternetRoute() const;
  uint64_t getExitRoute() const;
  ///@}
  
  /**
  * @brief Set the route tracker
  **/
  Data&
  setRouteTracker( const RouteTracker& tracker );

  /**
  * @brief Update the route hash with the given link identifier
  **/
  void
  updateRoute( uint32_t link_id );

  ///////////////////////////////////////////////////////////////

  nfd::LocalControlHeader&
  getLocalControlHeader();

  const nfd::LocalControlHeader&
  getLocalControlHeader() const;

  uint64_t
  getIncomingFaceId() const;

  Data&
  setIncomingFaceId(uint64_t incomingFaceId);

  nfd::LocalControlHeader::CachingPolicy
  getCachingPolicy() const;

  Data&
  setCachingPolicy(nfd::LocalControlHeader::CachingPolicy cachingPolicy);

public: // EqualityComparable concept
  Data&
  operator=( const Data& other );
  bool
  operator==(const Data& other) const;

  bool
  operator!=(const Data& other) const;

protected:
  /**
   * @brief Clear the wire encoding.
   */
  void
  onChanged();

private:
  Name m_name;
  MetaInfo m_metaInfo;
  mutable Block m_content;
  Signature m_signature;
  uint8_t   m_access_level = 0;
  bool      m_no_recache_flag = false;
  unique_ptr<RouteTracker> m_route_tracker;

  mutable Block m_wire;
  mutable Name m_fullName;

  nfd::LocalControlHeader m_localControlHeader;
  friend class nfd::LocalControlHeader;
};

std::ostream&
operator<<(std::ostream& os, const Data& data);

inline bool
Data::hasWire() const
{
  return m_wire.hasWire();
}

inline const Name&
Data::getName() const
{
  return m_name;
}

inline uint8_t
Data::getAccessLevel() const
{
  return m_access_level;
}

inline Data&
Data::setAccessLevel( uint8_t level )
{
  m_access_level = level;
  onChanged();
  return *this;
}

inline const MetaInfo&
Data::getMetaInfo() const
{
  return m_metaInfo;
}

inline uint32_t
Data::getContentType() const
{
  return m_metaInfo.getType();
}

inline const time::milliseconds&
Data::getFreshnessPeriod() const
{
  return m_metaInfo.getFreshnessPeriod();
}

inline const name::Component&
Data::getFinalBlockId() const
{
  return m_metaInfo.getFinalBlockId();
}

inline const Signature&
Data::getSignature() const
{
  return m_signature;
}

inline bool
Data::getNoReCacheFlag() const
{
  return m_no_recache_flag;
}

inline Data&
Data::setNoReCacheFlag( bool flag )
{
  m_no_recache_flag = flag;
  onChanged();
  return *this;
}

inline const RouteTracker&
Data::getRouteTracker() const
{
    if( !m_route_tracker ) BOOST_THROW_EXCEPTION(
       Error( "Requested route tracker does not exist" ) 
    );
return *m_route_tracker;
}

inline RouteTracker::NetworkType
Data::getCurrentNetwork() const
{
    if( !m_route_tracker ) BOOST_THROW_EXCEPTION(
       Error( "Attempt to get network of data without route tracker" ) 
    );
    return m_route_tracker->getCurrentNetwork();
}

inline void
Data::setCurrentNetwork( RouteTracker::NetworkType type )
{
    if( !m_route_tracker ) BOOST_THROW_EXCEPTION(
       Error( "Attempt to set network of data without route tracker" ) 
    );
    m_route_tracker->setCurrentNetwork( type );
    m_wire.reset();
}

inline bool
Data::hasRouteTracker() const
{
    return m_route_tracker != NULL;
}

inline uint64_t
Data::getEntryRoute() const
{
    if( !m_route_tracker ) BOOST_THROW_EXCEPTION(
       Error( "Attempt to get route of data without route tracker" ) 
    );
    return m_route_tracker->getEntryRoute();
}

inline uint64_t
Data::getInternetRoute() const
{
    if( !m_route_tracker ) BOOST_THROW_EXCEPTION(
       Error( "Attempt to get route of data without route tracker" ) 
    );
    return m_route_tracker->getInternetRoute();
}

inline uint64_t
Data::getExitRoute() const
{
    if( !m_route_tracker ) BOOST_THROW_EXCEPTION(
       Error( "Attempt to get route of data without route tracker" ) 
    );
    return m_route_tracker->getExitRoute();
}

inline Data&
Data::setRouteTracker( const RouteTracker& tracker )
{
    m_route_tracker.reset( new RouteTracker( tracker ) );
    m_wire.reset();
    return *this;
}

inline void
Data::updateRoute( uint32_t link_id )
{
    if( !m_route_tracker ) BOOST_THROW_EXCEPTION(
       Error( "Attempt to update route of data without route tracker" ) 
    );
    m_route_tracker->update( link_id );
    m_wire.reset();
}

inline nfd::LocalControlHeader&
Data::getLocalControlHeader()
{
  return m_localControlHeader;
}

inline const nfd::LocalControlHeader&
Data::getLocalControlHeader() const
{
  return m_localControlHeader;
}

inline uint64_t
Data::getIncomingFaceId() const
{
  return getLocalControlHeader().getIncomingFaceId();
}

inline nfd::LocalControlHeader::CachingPolicy
Data::getCachingPolicy() const
{
  return getLocalControlHeader().getCachingPolicy();
}

} // namespace ndn

#endif

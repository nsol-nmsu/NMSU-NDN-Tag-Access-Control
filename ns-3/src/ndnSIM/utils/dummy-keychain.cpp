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

#include "dummy-keychain.hpp"

#include "util/io.hpp"
#include <boost/iostreams/device/array.hpp>

namespace ndn {
namespace security {

  /**
  * @file dummy-keychain.cpp
  * This file has been modified by Ray Stubbs [stubbs.ray@gmail.com]
  * to accomodate changes made to the Data and Interest classes.
  **/

const std::string DummyPublicInfo::SCHEME = "pib-dummy";
const std::string DummyTpm::SCHEME = "tpm-dummy";

NDN_CXX_KEYCHAIN_REGISTER_PIB(DummyPublicInfo, "pib-dummy", "dummy");
NDN_CXX_KEYCHAIN_REGISTER_TPM(DummyTpm, "tpm-dummy", "dummy");

typedef DummyPublicInfo DummyPublicInfo2;
typedef DummyTpm DummyTpm2;

NDN_CXX_KEYCHAIN_REGISTER_PIB(DummyPublicInfo2, "pib-dummy2");
NDN_CXX_KEYCHAIN_REGISTER_TPM(DummyTpm2, "tpm-dummy2");

DummyPublicInfo::DummyPublicInfo(const std::string& locator)
  : SecPublicInfo(locator)
{
}

bool
DummyPublicInfo::doesIdentityExist(const Name& identityName)
{
  return true;
}

void
DummyPublicInfo::addIdentity(const Name& identityName)
{
}

bool
DummyPublicInfo::revokeIdentity()
{
  return true;
}

bool
DummyPublicInfo::doesPublicKeyExist(const Name& keyName)
{
  return true;
}

void
DummyPublicInfo::addKey(const Name& keyName, const PublicKey& publicKey)
{
}

shared_ptr<PublicKey>
DummyPublicInfo::getPublicKey(const Name& keyName)
{
  static shared_ptr<PublicKey> publicKey = nullptr;
  if (publicKey == nullptr) {
    auto cert = getCertificate( keyName );
    publicKey = make_shared<PublicKey>(cert->getPublicKeyInfo());
  }

  return publicKey;
}

KeyType
DummyPublicInfo::getPublicKeyType(const Name& keyName)
{
  return KEY_TYPE_RSA;
}

bool
DummyPublicInfo::doesCertificateExist(const Name& certificateName)
{
  return true;
}

void
DummyPublicInfo::addCertificate(const IdentityCertificate& certificate)
{
}

shared_ptr<IdentityCertificate>
DummyPublicInfo::getCertificate(const Name& certificateName)
{
  static shared_ptr<IdentityCertificate> cert = nullptr;
  if (cert == nullptr) {
    typedef boost::iostreams::stream<boost::iostreams::array_source> arrayStream;
    arrayStream
    is(reinterpret_cast<const char*>(DUMMY_CERT), sizeof(DUMMY_CERT));
    cert = io::load<IdentityCertificate>(is, io::BASE_64);
  }

  return cert;
}

Name
DummyPublicInfo::getDefaultIdentity()
{
  return "/dummy/key";
}

Name
DummyPublicInfo::getDefaultKeyNameForIdentity(const Name& identityName)
{
  return "/dummy/key/ksk-1418600391050";
}

Name
DummyPublicInfo::getDefaultCertificateNameForKey(const Name& keyName)
{
  return "/dummy/key/KEY/ksk-1418600391050/ID-CERT/%FD%00%00%01JK%2C%22%5E";
}

void
DummyPublicInfo::getAllIdentities(std::vector<Name>& nameList, bool isDefault)
{
  if (isDefault) {
    nameList.push_back("/dummy");
  }
}

void
DummyPublicInfo::getAllKeyNames(std::vector<Name>& nameList, bool isDefault)
{
  if (isDefault) {
    nameList.push_back("/dummy/key/ksk-1418600391050");
  }
}

void
DummyPublicInfo::getAllKeyNamesOfIdentity(const Name& identity, std::vector<Name>& nameList,
                                          bool isDefault)
{
  if (isDefault) {
    nameList.push_back("/dummy/key/ksk-1418600391050");
  }
}

void
DummyPublicInfo::getAllCertificateNames(std::vector<Name>& nameList, bool isDefault)
{
  if (isDefault) {
    nameList.push_back("/dummy/key/KEY/ksk-1418600391050/ID-CERT/%FD%00%00%01JK%2C%22%5E");
  }
}

void
DummyPublicInfo::getAllCertificateNamesOfKey(const Name& keyName, std::vector<Name>& nameList,
                                             bool isDefault)
{
  if (isDefault) {
    nameList.push_back("/dummy/key/KEY/ksk-1418600391050/ID-CERT/%FD%00%00%01JK%2C%22%5E");
  }
}

void
DummyPublicInfo::deleteCertificateInfo(const Name& certificateName)
{
}

void
DummyPublicInfo::deletePublicKeyInfo(const Name& keyName)
{
}

void
DummyPublicInfo::deleteIdentityInfo(const Name& identity)
{
}

void
DummyPublicInfo::setDefaultIdentityInternal(const Name& identityName)
{
}

void
DummyPublicInfo::setDefaultKeyNameForIdentityInternal(const Name& keyName)
{
}

void
DummyPublicInfo::setDefaultCertificateNameForKeyInternal(const Name& certificateName)
{
}

void
DummyPublicInfo::setTpmLocator(const std::string& tpmLocator)
{
  m_tpmLocator = tpmLocator;
}

std::string
DummyPublicInfo::getTpmLocator()
{
  return m_tpmLocator;
}

std::string
DummyPublicInfo::getScheme()
{
  return DummyPublicInfo::SCHEME;
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

DummyTpm::DummyTpm(const std::string& locator)
  : SecTpm(locator)
{
}

void
DummyTpm::setTpmPassword(const uint8_t* password, size_t passwordLength)
{
}

void
DummyTpm::resetTpmPassword()
{
}

void
DummyTpm::setInTerminal(bool inTerminal)
{
}

bool
DummyTpm::getInTerminal() const
{
  return false;
}

bool
DummyTpm::isLocked()
{
  return false;
}

bool
DummyTpm::unlockTpm(const char* password, size_t passwordLength, bool usePassword)
{
  return true;
}

void
DummyTpm::generateKeyPairInTpm(const Name& keyName, const KeyParams& params)
{
}

void
DummyTpm::deleteKeyPairInTpm(const Name& keyName)
{
}

shared_ptr<PublicKey>
DummyTpm::getPublicKeyFromTpm(const Name& keyName)
{
  return nullptr;
}

Block
DummyTpm::signInTpm(const uint8_t* data, size_t dataLength, const Name& keyName,
                    DigestAlgorithm digestAlgorithm)
{
  return Block(DUMMY_SIGNATURE, sizeof(DUMMY_SIGNATURE));
}

ConstBufferPtr
DummyTpm::decryptInTpm(const uint8_t* data, size_t dataLength, const Name& keyName,
                       bool isSymmetric)
{
  BOOST_THROW_EXCEPTION(Error("Not supported"));
}

ConstBufferPtr
DummyTpm::encryptInTpm(const uint8_t* data, size_t dataLength, const Name& keyName,
                       bool isSymmetric)
{
  BOOST_THROW_EXCEPTION(Error("Not supported"));
}

void
DummyTpm::generateSymmetricKeyInTpm(const Name& keyName, const KeyParams& params)
{
}

bool
DummyTpm::doesKeyExistInTpm(const Name& keyName, KeyClass keyClass)
{
  return true;
}

bool
DummyTpm::generateRandomBlock(uint8_t* res, size_t size)
{
  return false;
}

void
DummyTpm::addAppToAcl(const Name& keyName, KeyClass keyClass, const std::string& appPath,
                      AclType acl)
{
}

ConstBufferPtr
DummyTpm::exportPrivateKeyPkcs8FromTpm(const Name& keyName)
{
  BOOST_THROW_EXCEPTION(Error("Not supported"));
}

bool
DummyTpm::importPrivateKeyPkcs8IntoTpm(const Name& keyName, const uint8_t* buffer,
                                       size_t bufferSize)
{
  return false;
}

bool
DummyTpm::importPublicKeyPkcs1IntoTpm(const Name& keyName, const uint8_t* buffer, size_t bufferSize)
{
  return false;
}

std::string
DummyTpm::getScheme()
{
  return DummyTpm::SCHEME;
}

} // namespace security
} // namespace ndn

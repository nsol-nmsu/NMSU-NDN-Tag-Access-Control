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

static const uint8_t DUMMY_CERT[] =
"Bv0DCyH9AgAHOAgFZHVtbXkIA2tleQgDS0VZCBFrc2stMTQxODYwMDM5MTA1MAgH\
SUQtQ0VSVAgJ/QAAAUpLLCJeJgEAFAkYAQIZBAA27oAV/QFzMIIBbzAiGA8xOTkw\
MDEyNzEwNTA0NFoYDzE5NDkxMjA1MTMwOTE1WjAlMCMGA1UEKRMcL2R1bW15L2tl\
eS9rc2stMTQxODYwMDM5MTA1MDCCASAwDQYJKoZIhvcNAQEBBQADggENADCCAQgC\
ggEBAMVH4b+eCXYHqtMJkP67vXsPwcginEGsGmU/V5vaP7DGocEPSGzcdg8+CZHp\
Wsa2u/gyNED96WHLb2JJ/48Kuct3DtCyUAfpH/+xbUihvagRL/GqquYQ1bXgVCos\
lF6g5kM23vpk3CfWHQ+7IhdXYa45zNyi9caXF7t0GleZvyOnClcGKycadySq+pOa\
R4cSZTyMlLu6FN5BVu2x6trquHnPYAVwisAOuCFWFUJtRlQ5PldMK+6cPag+kDWu\
3jM6wjBOr3zNHXkolGuE6L1CQ0BQ7DNvO5GJt7Wbu4ric5jgut7ZeGGXcqsjoJ6k\
q1BhNbKDR3y6JCNdx4UsMaJO57kCAREWPxsBARw6BzgIBWR1bW15CANrZXkIA0tF\
WQgRa3NrLTE0MTg2MDAzOTEwNTAIB0lELUNFUlQICf0AAAFKSywiXhf9AQCTFQlJ\
eZ63nNPBv2GJ1dnK8rAUrnJ8H4/1sXDWm4/41y28km99d5ZG6tR9kLx66+IDk7HS\
Yuyd/5ycKhR9I8opPRUaQEIsWTOK98BrxJzzxJmkGmD1KH1M70N9vX0AUe5B9SWA\
zuZkT3VU87KZmg+Tmigd/hKK4MEC66Q1UoisRBpEgpdPX6jYn2c4qGS2Ypm9ljz1\
hglcl2uPruBg5yOYau7BsBS+Riz7pydz5PMmM7qZ1AE4qPKeh+BxCyVEBzWIq2cn\
Vg61tegntEncuEgx/5lIqxG0oN+Kbf9DaTKnvGOdD+CVNDYlSz42vYGRC5GfOgSi\
RCgZoTghTyVZikjCIgEA";

static const uint8_t DUMMY_SIGNATURE[] =
  {0x17, 0xfd, 0x01, 0x00, 0x93, 0x15, 0x09, 0x49, 0x79, 0x9e, 0xb7, 0x9c, 0xd3, 0xc1, 0xbf, 0x61,
   0x89, 0xd5, 0xd9, 0xca, 0xf2, 0xb0, 0x14, 0xae, 0x72, 0x7c, 0x1f, 0x8f, 0xf5, 0xb1, 0x70, 0xd6,
   0x9b, 0x8f, 0xf8, 0xd7, 0x2d, 0xbc, 0x92, 0x6f, 0x7d, 0x77, 0x96, 0x46, 0xea, 0xd4, 0x7d, 0x90,
   0xbc, 0x7a, 0xeb, 0xe2, 0x03, 0x93, 0xb1, 0xd2, 0x62, 0xec, 0x9d, 0xff, 0x9c, 0x9c, 0x2a, 0x14,
   0x7d, 0x23, 0xca, 0x29, 0x3d, 0x15, 0x1a, 0x40, 0x42, 0x2c, 0x59, 0x33, 0x8a, 0xf7, 0xc0, 0x6b,
   0xc4, 0x9c, 0xf3, 0xc4, 0x99, 0xa4, 0x1a, 0x60, 0xf5, 0x28, 0x7d, 0x4c, 0xef, 0x43, 0x7d, 0xbd,
   0x7d, 0x00, 0x51, 0xee, 0x41, 0xf5, 0x25, 0x80, 0xce, 0xe6, 0x64, 0x4f, 0x75, 0x54, 0xf3, 0xb2,
   0x99, 0x9a, 0x0f, 0x93, 0x9a, 0x28, 0x1d, 0xfe, 0x12, 0x8a, 0xe0, 0xc1, 0x02, 0xeb, 0xa4, 0x35,
   0x52, 0x88, 0xac, 0x44, 0x1a, 0x44, 0x82, 0x97, 0x4f, 0x5f, 0xa8, 0xd8, 0x9f, 0x67, 0x38, 0xa8,
   0x64, 0xb6, 0x62, 0x99, 0xbd, 0x96, 0x3c, 0xf5, 0x86, 0x09, 0x5c, 0x97, 0x6b, 0x8f, 0xae, 0xe0,
   0x60, 0xe7, 0x23, 0x98, 0x6a, 0xee, 0xc1, 0xb0, 0x14, 0xbe, 0x46, 0x2c, 0xfb, 0xa7, 0x27, 0x73,
   0xe4, 0xf3, 0x26, 0x33, 0xba, 0x99, 0xd4, 0x01, 0x38, 0xa8, 0xf2, 0x9e, 0x87, 0xe0, 0x71, 0x0b,
   0x25, 0x44, 0x07, 0x35, 0x88, 0xab, 0x67, 0x27, 0x56, 0x0e, 0xb5, 0xb5, 0xe8, 0x27, 0xb4, 0x49,
   0xdc, 0xb8, 0x48, 0x31, 0xff, 0x99, 0x48, 0xab, 0x11, 0xb4, 0xa0, 0xdf, 0x8a, 0x6d, 0xff, 0x43,
   0x69, 0x32, 0xa7, 0xbc, 0x63, 0x9d, 0x0f, 0xe0, 0x95, 0x34, 0x36, 0x25, 0x4b, 0x3e, 0x36, 0xbd,
   0x81, 0x91, 0x0b, 0x91, 0x9f, 0x3a, 0x04, 0xa2, 0x44, 0x28, 0x19, 0xa1, 0x38, 0x21, 0x4f, 0x25,
   0x59, 0x8a, 0x48, 0xc2};

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

/**
* @class ndntac::AuthCache
* A bloom filter wrapper for caching AuthTags.
*
* @author Ray Stubbs [stubbs.ray@gmail.com]
**/

#ifndef AUTH_CACHE_INCLUDED
#define AUTH_CACHE_INCLUDED

#include "ndn-cxx/auth-tag.hpp"
#include "bloom_filter.hpp"
#include <cryptopp/rsa.h>

namespace ndntac
{

  class AuthCache
  {
  private:
    bloom_filter m_bloom;
    double       m_max_fpp;

  public:

    /**
    * @brief Constructor
    * @param fpp  Max tolorable false positive probability
    * @param n    Max number of items to be stored in the bloom filter
    **/
    AuthCache( double fpp, uint32_t n )
    {
      m_max_fpp = fpp;

      bloom_parameters bparams;
      bparams.projected_element_count = fpp;
      bparams.false_positive_probability = n;
      bparams.compute_optimal_parameters();
      m_bloom = bloom_filter( bparams );
    }

    /**
    * @brief Insert a tag into the bloom filter
    * @param tag  Tag to insert
    **/
    void insert( const ndn::AuthTag& tag )
    {
      uint8_t digest[CryptoPP::SHA256::DIGESTSIZE];
      CryptoPP::SHA256 hash;
      const ndn::Block& encoded = tag.wireEncode();
      hash.CalculateDigest( digest, encoded.wire() , encoded.size() );
      m_bloom.insert( digest, CryptoPP::SHA256::DIGESTSIZE );
    }

    /**
    * @brief Check for a tag in the bloom filter
    * @param tag   Tag to check for
    * @return true if tag is in filter false otherwise
    **/
    bool contains( const ndn::AuthTag& tag )
    {
      if( m_bloom.effective_fpp() > m_max_fpp )
        m_bloom.clear();

      uint8_t digest[CryptoPP::SHA256::DIGESTSIZE];
      CryptoPP::SHA256 hash;
      const ndn::Block& encoded = tag.wireEncode();
      hash.CalculateDigest( digest, encoded.wire() , encoded.size() );
      return m_bloom.contains( digest, CryptoPP::SHA256::DIGESTSIZE );
    }

    /**
    * @brief Return the effective false positive probability calculated based
    *        on current filter saturation
    **/
    double getEffectiveFPP() const
    {
      return m_bloom.effective_fpp();
    }

  };

};

#endif // AUTH_CACHE_INCLUDED

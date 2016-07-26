/**
* @class AuthDataProducer
* A DataProducer implementation for producing auth tags
*
* @author Ray Stubbs [stubbs.ray@gmail.com]
**/
#ifndef FILE_DATA_PRODUCER__INCLUDED
#define FILE_DATA_PRODUCER__INCLUDED

#include "data-producer.hpp"
#include "ndn-cxx/util/time.hpp"
#include "ndn-cxx/interest.hpp"
#include "ndn-cxx/data.hpp"
#include "ndn-cxx/name.hpp"

#define FILE_SEGMENT_SIZE 1000

namespace ndntac
{
  class FileDataProducer : public DataProducer
  {
  public:

    FileDataProducer( const std::string& filename, uint8_t access_level ):
      m_filename( filename ), m_access_level( access_level ) {};

    shared_ptr< ndn::Data >
    makeData( shared_ptr< const ndn::Interest > interest ) override
    {
      using namespace ndn;

      static uint8_t buffer[FILE_SEGMENT_SIZE];

      auto data = make_shared< Data >( interest->getName() );
      data->setContentType( tlv::ContentType_Blob );

      // read from file
      std::ifstream in( m_filename );
      in.read( (char*)buffer, FILE_SEGMENT_SIZE );
      in.close();

      data->setAccessLevel( m_access_level );
      data->setContent( Block( buffer, FILE_SEGMENT_SIZE ) );
      data->setSignatureValue( Block( tlv::SignatureValue, Block( "Not Empty", 9) ) );
      return data;
    }

    uint8_t
    getAccessLevel() override
    {
      return m_access_level;
    }

  private:
    std::string   m_filename;
    uint8_t       m_access_level;
  };
}

#endif // FILE_DATA_PRODUCER__INCLUDED

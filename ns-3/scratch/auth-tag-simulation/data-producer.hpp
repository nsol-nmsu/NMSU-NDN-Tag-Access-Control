/**
* @class DataProducer
* Abstraction for data producers, given an interest produces sizeOfVarNumber
* kind of matching data, or NULL if the interest doesn't match it's data
*
@author Ray Stubbs [stubbs.ray@gmail.com]
**/
#ifndef DATA_PRODUCER__INCLUDED
#define DATA_PRODUCER__INCLUDED

namespace ndntac
{
  class DataProducer
  {
  public:
    virtual shared_ptr< ndn::Data >
    makeData( shared_ptr< const ndn::Interest > interest ) = 0;

    virtual uint8_t
    getAccessLevel() = 0;
  };

}

#endif // DATA_PRODUCER__INCLUDED

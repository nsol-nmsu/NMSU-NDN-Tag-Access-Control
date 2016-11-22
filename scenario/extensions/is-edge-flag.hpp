/**
* This is a flag meant to be aggregated with
* a net device to indicate that it forms an
* edge between two networks.  This is used
* by the modified NetDeviceFace to initialize
* the modified nfd::Face with a flag that says
* it's an edge face.
**/
#ifndef IS_EDGE_FLAG__INCLUDED
#define IS_EDGE_FLAG__INCLUDED

#include "ns3/object.h"

namespace ndntac
{

class IsEdgeFlag : public ns3::Object
{
public:
    static ns3::TypeId
    GetTypeId( void )
    {
        static ns3::TypeId tid =
            ns3::TypeId( "ndntac::IsEdgeFlag" )
            .SetParent< ns3::Object >()
            .AddConstructor<IsEdgeFlag>();
        return tid;
    };
};

NS_OBJECT_ENSURE_REGISTERED( IsEdgeFlag );

}
#endif

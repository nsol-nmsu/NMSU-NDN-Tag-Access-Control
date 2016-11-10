/** @file
* @brief Coordinator and logger singularity for the simulation as a whole.
*
* @author Ray Stubbs [stubbs.ray@gmail.com]
**/

#ifndef COORDINATOR__INCLUDED
#define COORDINATOR__INCLUDED

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/names.h"
#include "ns3/ndnSIM/apps/ndn-consumer-cbr.hpp"
#include "ndn-cxx/name.hpp"
#include "ndn-cxx/util/time.hpp"
#include "log-filter.hpp"
#include <vector>
#include <map>

namespace ndntac
{

   namespace Coordinator
   {
       // simulation notifiers
       void simulationStarted();
       void simulationStarted( const std::string& logfile );
       void simulationFinished();
       void log( const std::string& logger,
                 const std::map< std::string, std::string > entries );
   }

}

#endif // COORDINATOR__INCLUDED

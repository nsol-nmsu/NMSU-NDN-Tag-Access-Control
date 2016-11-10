#include "coordinator.hpp"
#include "unqlite.h"
#include <mutex>

namespace ndntac
{
   namespace Coordinator
   {
       using namespace std;
       using namespace ndn;

       unqlite*              log_db;
       uint64_t              log_entry = 0;
       bool                  logging_enabled;

       void simulationStarted()
       {
            logging_enabled = false;
       }
       
       void simulationStarted( const std::string& logfile )
       {
         // set flags
         logging_filtered = false;
         logging_enabled = true;
         
         // open log database
         int rc = unqlite_open( &log_db,
                                logfile.c_str(),
                                UNQLITE_OPEN_CREATE );
         if( rc == UNQLITE_OK )
         {
             map< string, string > log =
             {
                pair< string, string >( "what", "Started" )
             };
             log( "Simulation", log );
         }
         else
         {
             std::cerr << "Could not open log database" << std::endl;
             logging_enabled = false;
         }
       }

       void simulationFinished()
       {
         if( logging_enabled )
         {
             map< string, string > log =
             {
                pair< string, string >( "what", "Finished" )
             };
             log( "Simulation", log );
         }
         unqlite_close( log_db );
       }
       
       void log( const string& logger,
                 const map< string, string >& entries )
       {
            if( !logging_enabled )
                return;
            
            stringstream partial_key;
            partial_key << log_entry++ << '.' << logger << '.';
            for( auto it = entries.begin() ; it != entries.end() ; it++ )
            {
                string key = partial_key.str() + it->first;
                unqlite_kv_store( log_db, 
                                  key, key.size(),
                                  it->second.c_str(), it->second.size() );
            }
            
            string key = partial_key.str() + "time";
            stringstream time;
            time << time::system_clock::now();
            unqlite_kv_store( log_db, 
                              key, key.size(),
                              time.str().c_str(), -1 );
       }
   }
}

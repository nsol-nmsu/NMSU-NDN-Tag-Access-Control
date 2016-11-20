/**
* @brief Log filtering utilities
*
* Used to limit the logs output by the simulation
* @author Ray Stubbs [stubbs.ray@gmail.com]
**/

#ifndef LOG__INCLUDED
#define LOG__INCLUDED
#include <exception>
#include <string>
#include <map>
#include <sstream>

#include "ns3/ndnSIM-module.h"
#include "unqlite.hpp"

namespace ndntac
{
  class Logger;
  
  /**
  * @brief Represents a log entry
  *
  * Compiles to an UnQLite script to be used to
  * write the log to database
  **/
  class Log
  {
  friend class ndntac::Logger;
  public:
    ~Log( void )
    {
        unqlite_vm_release( m_vm );
    }

    /**
    * @brief Set a string field of the log entry,
    *        field is created if it doesn't exist
    **/
    void
    set( const std::string& key, const std::string& value )
    {
        // check if variable exists, if not then create
        auto it = m_vars.find( key );
        if( it == m_vars.end() )
            it = m_vars.emplace( key, unqlite_vm_new_scalar( m_vm ) ).first;
        
        // if the previous type was a string then we
        // reset the cursor, otherwise the string will
        // append insteal of repalce
        int rc = unqlite_value_reset_string_cursor( it->second );
        if( rc != UNQLITE_OK ) 
            throw std::runtime_error( "Error setting UnQLite log variable" );

        // fill value
        rc = unqlite_value_string( it->second, value.c_str(), value.size() );
        if( rc != UNQLITE_OK ) 
            throw std::runtime_error( "Error setting UnQLite log variable" );
    };

    /**
    * @brief Set a bool field of the log entry,
    *        field is created if it doesn't exist
    **/
    void
    set( const std::string& key, bool value )
    {
        // check if variable exists, if not then create
        auto it = m_vars.find( key );
        if( it == m_vars.end() )
            it = m_vars.emplace( key, unqlite_vm_new_scalar( m_vm ) ).first;
        
        // fill value
        int rc = unqlite_value_bool( it->second, value );
        if( rc != UNQLITE_OK )
            throw std::runtime_error( "Error setting UnQLite log variable" );
    };

    /**
    * @brief Set a double field of the log entry,
    *        field is created if it doesn't exist
    **/
    void
    set( const std::string& key, double value )
    {
        // check if variable exists, if not then create
        auto it = m_vars.find( key );
        if( it == m_vars.end() )
            it = m_vars.emplace( key, unqlite_vm_new_scalar( m_vm ) ).first;

        // fill value
        int rc = unqlite_value_double( it->second, value );
        if( rc != UNQLITE_OK ) 
            throw std::runtime_error( "Error setting UnQLite log variable" );
    };

    /**
    * @brief Set an int field of the log entry,
    *        field is created if it doesn't exist
    **/
    void
    set( const std::string& key, int64_t value )
    {
        // check if variable exists, if not then create
        auto it = m_vars.find( key );
        if( it == m_vars.end() )
            it = m_vars.emplace( key, unqlite_vm_new_scalar( m_vm ) ).first;
        
        // fill value
        int rc = unqlite_value_int64( it->second, value );
        if( rc != UNQLITE_OK )
            throw std::runtime_error( "Error setting UnQLite log variable" );
        
    };
    
    /**
    * @brief Writes the log out to file
    **/
    void
    write( void )
    {
        // add all variables to VM
        for( auto it = m_vars.begin() ; it != m_vars.end() ; it++ )
        {
            int rc = unqlite_vm_config( m_vm, UNQLITE_VM_CONFIG_CREATE_VAR,
                                        it->first.c_str(), it->second );
            if( rc != UNQLITE_OK ) 
                throw std::runtime_error( "Error setting UnQLite log variable" );
        }
        
        // execute
        int rc = unqlite_vm_exec( m_vm );
        if( rc != UNQLITE_OK ) 
            throw std::runtime_error( "Error executing UnQLite log code" );
        
        // reset vm for next use
        rc = unqlite_vm_reset( m_vm );
        if( rc != UNQLITE_OK ) 
            throw std::runtime_error( "Error reseting UnQLite log code" );
    };

  private:
    /**
    * @brief Constructs a log from a string JSON
    *        template, specified variables can be
    *        set with the 'set' methods
    *
    * This should be called by Logger in its makeLog() method
    *
    * @param db       UnQLite database to write log to
    * @param logger   Entity creating the log
    * @param json     JSON object to use as the log
    **/
    Log( unqlite* db, const std::string& logger, const std::string& json )
    {
        using ndn::time::system_clock;
        using ndn::time::duration_cast;
        using ndn::time::nanoseconds;
        
        // construct code
        std::stringstream code;
        code <<
        "$log = " << json << ";" <<
        "$log['time'] = $time;" <<
        "$logger = '" << logger << "';" <<
        "if( !db_exists( $logger ) ){ \
            $rc = db_create( $logger ); \
            if( !$rc ){ \
                print db_errlog(); \
                print '\\n'; \
                return; \
            } \
        }" \
        "db_store( $logger, $log );" \
        "db_commit();";
        
        int rc = unqlite_compile( db, code.str().c_str(), -1, &m_vm );
        if( rc != UNQLITE_OK ) 
        {
            if( rc == UNQLITE_COMPILE_ERR )
            {
                int len;
                const char* buf;
                unqlite_config(db,UNQLITE_CONFIG_JX9_ERR_LOG,&buf,&len);
                string msg = "Error compiling UnQLite log code: ";
                msg += buf;
                throw std::runtime_error( msg );
            }
        }
        
        // set output consumer
        rc = unqlite_vm_config( m_vm, UNQLITE_VM_CONFIG_OUTPUT,
                                Log::unqlite_consumer, NULL );
        if( rc != UNQLITE_OK ) 
        {
            if( rc == UNQLITE_COMPILE_ERR )
            {
                int len;
                const char* buf;
                unqlite_config(db,UNQLITE_CONFIG_JX9_ERR_LOG,&buf,&len);
                string msg = "Error configuring output: ";
                msg += buf;
                throw std::runtime_error( msg );
            }
        }
        
        // set the time variable
        int64_t time = duration_cast<nanoseconds>(
                         system_clock::now().time_since_epoch()
                        ).count();
        set( "time", time );
    };
    
    /**
    * @brief Can't copy
    **/
    Log( const Log& other ) = delete;
    
    /**
    * @brief Redirects unqlite output to stdout
    **/
    static int
    unqlite_consumer( const void* data, unsigned len, void* dmy)
    {
        std::cout << string( (const char*)data, len );
        return UNQLITE_OK;
    }

  private:
    unqlite_vm* m_vm;
    std::map< std::string, unqlite_value* > m_vars;
  };

};

#endif // LOG_FILTER__INCLUDED

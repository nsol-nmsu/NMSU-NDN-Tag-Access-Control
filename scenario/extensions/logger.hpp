/**
* @brief Logger
*
* Used as a central logging utility
* @author Ray Stubbs [stubbs.ray@gmail.com]
**/

#ifndef LOGGER__INCLUDED
#define LOGGER__INCLUDED

#include "unqlite.hpp"
#include <exception>
#include "log.hpp"


namespace ndntac
{

class Log;

class Logger
{
friend class ndntac::Log;
public:

    /**
    * @brief Create a new logger, or retrieve and exsiting
    *        logger to the specified database file
    **/
    static Logger&
    getInstance( const std::string& dbname )
    {
        auto it = s_loggers.find( dbname );
        if( it != s_loggers.end() )
            return *it->second;
        else
            return *s_loggers
                    .insert( make_pair( dbname, new Logger( dbname ) ) )
                    .first->second;
    }
private:
    
    /**
    * @brief Keep track of existing loggers
    **/
    static std::map< std::string, Logger* > s_loggers;

public:
    /**
    * @brief Compile a log
    *
    * @param logger  Entity that's creating the log
    * @param json    A JSON object to be logged
    **/
    Log*
    makeLog( const std::string& logger, const std::string json )
    {
        return new Log( m_db, logger, json );
    }

    /**
    * @brief Destruct
    **/
    ~Logger( void )
    {
        unqlite_close( m_db );
    }
private:
    /**
    * @brief Initialize a logger from given database file
    **/
    Logger( const std::string& dbname )
    {
        int rc = unqlite_open( &m_db, dbname.c_str(), UNQLITE_OPEN_CREATE );
        if( rc != UNQLITE_OK ) 
            throw std::runtime_error( "Error opening database file" );
    }
private:
    unqlite* m_db;
};

};
#endif // LOGGER__INCLUDED

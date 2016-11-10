#include "coordinator.hpp"

namespace ndntac
{
namespace Coordinator
{
  LogEntry::LogEntry( const std::string& logger,
                      const string& logtype,
                      const ndn::time::system_clock::TimePoint& time )
   : m_logger( logger )
   , m_logtype( logtype )
   {
     std::stringstream s;
     s << time;
     add( "time", s.str() );
   };

    void LogEntry::add( const string& key, const string& value )
    {
      m_attrs.push_back( std::make_pair( key, value ) );
    }

   string LogEntry::toString() const
    {
      std::stringstream s;
      s << m_logger << ":" << m_logtype << endl
        << "{" << endl;

      for( auto it = m_attrs.begin() ; it != m_attrs.end() ; it++ )
      {
        s <<  it->first << " = " << it->second << endl;
      }

      s << "}" << endl;
      return s.str();
    }

    LogCriteria::LogCriteria( const std::string& logger,
                              const std::string& logtype )
    : m_logger( logger )
    , m_logtype( logtype ) {};

    void LogCriteria::add( const std::string& key, const std::string& regex )
    {
      boost::regex r( regex );
      m_conditions[key] = r;
    }

    bool LogCriteria::match( const LogEntry& log ) const
    {
      if( m_logger != log.m_logger || m_logtype != log.m_logtype )
        return false;

      for( auto it = log.m_attrs.begin() ; it != log.m_attrs.end() ; it++ )
      {
        auto c = m_conditions.find( it->first );
        if( c == m_conditions.end() )
          continue;
        if( !boost::regex_match( it->second, c->second ) )
          return false;
      }

      return true;
    }

    void LogFilter::addCriteria( const LogCriteria& criteria )
    {
      m_criteria.push_back( criteria );
    }

    bool LogFilter::match( const LogEntry& log ) const
    {
      for( auto it = m_criteria.begin() ; it != m_criteria.end() ; it++ )
      {
        if( it->match( log ) )
          return true;
      }

      return false;
    }
}
}

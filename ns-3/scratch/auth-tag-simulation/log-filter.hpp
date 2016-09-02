/**
* @brief Log filtering utilities
*
* Used to limit the logs output by the simulator
* @author Ray Stubbs [stubbs.ray@gmail.com]
**/

#ifndef LOG_FILTER__INCLUDED
#define LOG_FILTER__INCLUDED

namespace ndntac
{
namespace Coordinator
{

  /**
  * @brief Represents a log entry
  **/
  class LogEntry
  {
    friend class LogCriteria;
  public:
    /**
    * Each log entry has a logger indicating which simulation entity the log
    * belongs to, a logtype indicating the a general log category, and a series
    * of attributes represented as key-value pairs.
    *
    * @param logger    Logging entity
    * @param logtype   Log category
    **/
    LogEntry( const std::string& logger,
              const string& logtype,
              const ndn::time::system_clock::TimePoint& time = ndn::time::system_clock::now() );

    /**
    * @brief Add an attribute to the log
    *
    * @param key    Attribute name
    * @param value  Attribute value
    **/
    void add( const string& key, const string& value );

    /**
    * @brief Convert the log to a printable format
    **/
    string toString() const;

  private:
    std::string m_logger;
    std::string m_logtype;
    std::map< std::string, std::string > m_attrs;
  };

  /**
  * @brief Represents a criteria used to limit logs
  **/
  class LogCriteria
  {
  public:

    /**
    * @brief Create a criteria for filtering logs from 'logger' of type 'logtype'
    *
    * @param logger  Logging entity
    * @param logtype Type of log
    **/
    LogCriteria( const string& logger, const string& logtype );

    /**
    * @brief Add a condition to the criteria
    *
    * Each condition is s key-regex pair, where the key is an attribute name
    * and the regex string is a regex agains which the attribute must match.
    * All conditions must be matched for a log to match the criteria.
    *
    * @param  key    Name of attribute that regex should be matched with
    * @param  regex  Regex string to constrain attribute value
    **/
    void add( const string& key, const string& regex );

    /**
    * @brief  Matches log against criterial
    * @param log   Log entry to be matched
    * @return true if log matches criteria
    **/
    bool match( const LogEntry& log ) const;

  private:
    std::string m_logger;
    std::string m_logtype;
    std::map< std::string, boost::regex > m_conditions;
  };

  class LogFilter
  {
  public:
      /**
      * @brief Adds a passing criterial to the filter
      *
      * A criteria is specifeid as a map of key-regex pairs where
      * the key indicates a log attribute and the regex indicates a regex
      * that the attribute must match.  All key-regex pairs must be matched
      * for the log to be accepted by the criteria.
      *
      * @param criteria   Criteria to add
      **/
      void
      addCriteria( const LogCriteria& criteria );

      /**
      * @brief Check if the given log matches any of the registered criteria
      *
      * The log is represented as a map of key-value pairs.
      *
      * @param log   Log entry to check against filter
      **/
      bool
      match( const LogEntry& log ) const;

  private:
      std::vector< LogCriteria > m_criteria;

  };

}
}

#endif // LOG_FILTER__INCLUDED

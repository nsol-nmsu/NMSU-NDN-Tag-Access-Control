#include "pconfig-reader.hpp"
#include "boost//regex.hpp"

PConfigReader::PConfig::PConfig( const ndn::Name& name )
{
    m_name = name;
}

size_t
PConfigReader::PConfig::size() const
{
    return m_contents.size();
}

void
PConfigReader::PConfig::add( const PConfigReader::PConfig::PContent& content )
{
    m_contents.push_back( content );
}

const PConfigReader::PConfig::PContent&
PConfigReader::PConfig::get( unsigned i ) const
{
    return m_contents[i];
}

std::vector<PConfigReader::PConfig::PContent>::const_iterator
PConfigReader::PConfig::begin() const
{
    return m_contents.cbegin();
}

std::vector<PConfigReader::PConfig::PContent>::const_iterator
PConfigReader::PConfig::end() const
{
    return m_contents.cend();
}

const ndn::Name&
PConfigReader::PConfig::getName() const
{
    return m_name;
}

PConfigReader::PConfigReader( const std::string& pfile )
{
    m_filename = pfile;
}

void
PConfigReader::parse()
{
    m_configs.clear();
    
    // regexps used to parse pconfig
    static boost::regex comment_regex( "#.*" );
    static boost::regex empty_regex( "" );
    static boost::regex header_regex( "\\s*\"(.+)\"\\s*:\\s*" );
    static boost::regex content_regex( "\\s*\"(.+)\"\\s+"
                                       "([0-9]+)(T|G|M|K|B)?\\s+"
                                       "([0-9]+)\\s+"
                                       "([0-9]+)\\s*");
   
   // open stream
   std::ifstream is;
   is.open( m_filename );
   if( !is.good() )
   {
        std::cerr << "Error opening pconfig file '"
                  << m_filename << std::endl;
        std::abort();
   }
   
   // parse pfile
   std::string line;
   try
   {
       while( std::getline( is, line ) )
       {
            boost::smatch match;
            if( line.empty()|| boost::regex_match( line, comment_regex ) )
                continue;
            if( boost::regex_match( line, match, header_regex ) )
            {
                m_configs.emplace_back( match[1].str() );
            }
            else if( boost::regex_match( line, match, content_regex ) )
            {
                PConfig::PContent content;
                content.name = match[1].str();
                std::stringstream( match[2].str() ) >> content.size;
                char size_unit;
                std::stringstream( match[3].str() ) >> size_unit;
                switch( size_unit )
                {
                    // fallthrough all
                    case 'T':
                        content.size *= 1024;
                    case 'G':
                        content.size *= 1024;
                    case 'M':
                        content.size *= 1024;
                    case 'K':
                        content.size *= 1024;
                    case 'B':
                        break;
                }
                
                // use temporary variable to prevent >> from treating
                // content.access_level as a char
                unsigned access_level;
                std::stringstream( match[4].str() ) >> access_level;
                content.access_level = access_level;
                
                std::stringstream( match[5].str() ) >> content.popularity;
                
                m_configs.back().add( content );
                
            }
        }
   }
   catch( std::exception e )
   {
       std::cerr << "Error parsing file '"
                 << m_filename <<"': "
                 << e.what()   << std::endl;
       is.close();
       std::abort();
    }
    is.close();
    
}

size_t
PConfigReader::size() const
{
    return m_configs.size();
}

const PConfigReader::PConfig&
PConfigReader::get( unsigned i ) const
{
    return m_configs[i];
}

std::vector<PConfigReader::PConfig>::const_iterator
PConfigReader::begin() const
{
    return m_configs.begin();
}

std::vector<PConfigReader::PConfig>::const_iterator
PConfigReader::end() const
{
    return m_configs.end();
}





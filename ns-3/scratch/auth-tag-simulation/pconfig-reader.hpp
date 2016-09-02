
#ifndef PCONFIG_READER_H
#define PCONFIG_READER_H

/**
* @brief Parser for '.p' producer configuration files
**/
class PConfigReader
{
public:

    /**
    * @brief Represents a single producer's config
    **/
    class PConfig
    {
    public:
    
        /**
        * @brief Represents a single content provided by the producer
        **/
        struct PContent
        {
            ndn::Name   name;
            size_t      size;
            uint8_t     access_level;
            uint32_t    popularity;
        };
        
        /**
        * @brief Constructor
        **/
        PConfig( const ndn::Name& name );
        
        /**
        * @brief Return number of PContents in the PConfig
        **/
        size_t
        size() const;
        
        /**
        * @brief Add a PContent
        **/
        void
        add( const PContent& content );
        
        /**
        * @brief Return the ith PContent
        **/
        const PContent&
        get( unsigned i ) const;
        
        /**
        * @brief Return iterator to first PContent
        **/
        std::vector<PContent>::const_iterator
        begin() const;
        
        /**
        * @brief Return end iterator
        **/
        std::vector<PContent>::const_iterator
        end() const;
        
        /**
        * @brief Return config's producer name
        **/
        const ndn::Name&
        getName() const;
        
    private:
    
        ndn::Name             m_name;
        std::vector<PContent> m_contents;
    };

    /**
    * @brief Construct a new reader
    * @param pfile   A producer configuration file
    **/
    PConfigReader( const std::string& pfile );
    
    /**
    * @brief Parse the currently configured config file
    **/
    void
    parse();
    
    /**
    * @brief Return the number of PConfigs available
    **/
    size_t
    size() const;
    
    /**
    * @brief Get the PConfig at index i
    **/
    const PConfig&
    get( unsigned i ) const;
    
    /**
    * @brief Return iterator to first PConfig
    **/
    std::vector<PConfig>::const_iterator
    begin() const;
    
    /**
    * @brief Return end iterator
    **/
    std::vector<PConfig>::const_iterator
    end() const;

private:
    std::string            m_filename;
    std::vector<PConfig>   m_configs;
};

#endif

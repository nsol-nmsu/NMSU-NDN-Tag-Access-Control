/**
* I haven't yet gotten around to adding the licensing header.
*
* This file and the counterpart route-tracker.cpp are additions to
* the ndn-cxx library for use in NMSU's tag based access control
* mechanism.
*
* If a packet ( either Interest or Data ) has a RouteTracker attached
* then the network will update the RouteTracker fields as the interest
* traverses networks.  If the RouteTracker is present then routers will
* XOR the interest's incoming link ID onto one of the RouteTracker hashes.
* Which hash is updated depends on the value of the m_current_network
* field which can be one of the following :
*  0 = Consumer's local network = ENTRY_NETWORK
*  1 = Some intermediate network between client and provider local network = INTERNET_NETWORK
*  2 = Provider's local network = EXIT_NETWORK
*
* If any data provider receives an interest that has a route tracker, it is
* expected to copy the tracker ( with all of the hashes ) to the data response.
*
*
* @author Ray Stubbs [stubbs.ray@gmail.com]
**/
#ifndef ROUTE_TRACKER_HPP
#define ROUTE_TRACKER_HPP

#include "common.hpp"
#include "encoding/block.hpp"
#include "signature.hpp"
#include "ndn-cxx/security/validity-period.hpp"

namespace ndn
{
    class RouteTracker
    {
        public:
            class Error : public tlv::Error
            {
            public:
              explicit
              Error(const std::string& what)
                : tlv::Error(what)
              {
              }
            };
            
            enum NetworkType
            {
                ENTRY_NETWORK = 0,
                INTERNET_NETWORK = 1,
                EXIT_NETWORK = 2
            };
            
            ///@{
            /**
            * @brief Constructors
            **/
            RouteTracker( const Block& wire );
            RouteTracker();
            ///@}
            
            /**
            * @brief Update the tracker
            **/
            void update( uint64_t link_id );
            
            /**
            * @brief Set the current network type
            **/
            void setCurrentNetwork( NetworkType type );
            
            /**
            * @brief Get the current network type
            **/
            NetworkType getCurrentNetwork() const;
            
            /**
            * @brief Get the entry route
            **/
            uint64_t getEntryRoute() const;
            
            /**
            * @brief Get the internet route
            **/
            uint64_t getInternetRoute() const;
            
            /**
            * @brief Get the exit route
            **/
            uint64_t getExitRoute() const;
            
            ///@{
            /**
            * @brief Encode the RouteTracker
            **/
            template<encoding::Tag TAG>
            size_t wireEncode( EncodingImpl< TAG > &encoder ) const;
            
            const Block& wireEncode() const;
            ///@}
            
            
            /**
            * @brief Decode the RouteTracker
            **/
            void wireDecode( const Block& wire );
            
            /**
            * @brief Check if wire exists
            **/
            bool hasWire() const;
            
    private:
            mutable Block m_wire;
            NetworkType   m_current_network;
            uint64_t      m_entry_route;
            uint64_t      m_internet_route;
            uint64_t      m_exit_route;

    };


}

#endif // ROUTE_TRACKER_HPP

#include "ndn-cxx/util/io.hpp"
#include "ns3/ndnSIM/utils/dummy-keychain.hpp"

int main()
{
    ndn::security::DummyPublicInfo dummy("dummy");
    auto cert = dummy.getCertificate("dummy");
    cert->hasWire();
    ndn::io::save( *cert, "/home/raystubbs/cery.temp", ndn::io::BASE_64 );
}

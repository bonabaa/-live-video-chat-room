/*
 * local.cxx
 *
 * copyright 2005 Derek J Smithies
 *
 *
 * Simple program to report the host name of this machine
 *                          the address of one network interface
 *
 * $Revision: 20385 $
 * $Author: rjongbloed $
 * $Date: 2008-06-04 10:40:38 +0000 (Wed, 04 Jun 2008) $
 */
#include <ptlib.h>
#include <ptlib/sockets.h>

class LocalAddress : public PProcess
{
    PCLASSINFO(LocalAddress, PProcess);
public:
    LocalAddress();
    
    void Main();
};

PCREATE_PROCESS(LocalAddress);

LocalAddress::LocalAddress()
    : PProcess("PwLib Example Factory", "local", 1, 0, ReleaseCode, 0)
{
}

void LocalAddress::Main()
{
    PStringStream progName;
    progName << "Product Name: " << GetName() << endl
             << "Manufacturer: " << GetManufacturer() << endl
             << "Version     : " << GetVersion(PTrue) << endl
             << "System      : " << GetOSName() << '-'
             << GetOSHardware() << ' '
             << GetOSVersion();
    cout << endl <<  progName << endl << endl;
    

    PUDPSocket localSocket;
    PIPSocket::Address addr;
    if(localSocket.GetNetworkInterface(addr)) {
        cout << "local address is    " << addr.AsString() << endl;
        if (addr == 0)
            cout << "sorry, that is a 0.0.0.0 address" << endl;
    } else
      cout << "Sorry, failed to get local address" << endl;
    
    cout << "local host name is  " << localSocket.GetHostName() << endl;
}

// End of local.cxx

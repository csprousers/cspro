#include "StdAfx.h"
#include "WindowsApplicationInterface.h"
#include <Wininet.h>
#pragma comment(lib, "Wininet.lib")


SharableString WindowsApplicationInterface::BarcodeRead(const std::string& /*message_text*/)
{
    return SharableString();
}


bool WindowsApplicationInterface::IsNetworkConnected(bool /*wifi*/, bool /*mobile*/)
{
    DWORD flags;
    return ( InternetGetConnectedState(&flags, 0) == TRUE );
}

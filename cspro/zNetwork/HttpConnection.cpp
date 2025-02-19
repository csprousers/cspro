#include "stdafx.h"
#include "HttpConnection.h"

#ifdef WIN_DESKTOP
#include <zNetwork/CurlHttpConnection.h>
#else
#include <zPlatformO/PlatformInterface.h>
#endif


std::unique_ptr<HttpConnection> HttpConnection::Create()
{
#ifdef WIN_DESKTOP
    return std::make_unique<CurlHttpConnection>();
#else
    return PlatformInterface::GetInstance()->GetApplicationInterface()->CreateHttpConnection();
#endif
}

#include "stdafx.h"
#include "WebViewSyncOperationMarker.h"


unsigned WebViewSyncOperationMarker::m_operationsInProcess = 0;


void WebViewSyncOperationMarker::DisplayOperationInProcessError()
{
    ASSERT(IsOperationInProgress());

    ErrorMessage::Display("A JavaScript call into CSPro is still in progress so some functionality "
                          "in this web view will not work correctly. Consider changing "
                          "the currently executing JavaScript call to be asynchronous.");
}



// temporarily here for the UseHtmlDialogs flag...
#include "UseHtmlDialogs.h"
bool UHD::flag = false;


// temporarily here for an access token for the old CSPro JavaScript interface
const std::string& OldCSProJavaScriptInterface::GetAccessToken()
{
    static const std::string access_token = CreateUuid();
    return access_token;
}

#pragma once

#include <zHtml/zHtml.h>


// a class to keep track of whether a web view is currently processing a synchronous option

class ZHTML_API WebViewSyncOperationMarker
{
private:
    WebViewSyncOperationMarker()
    {
        ASSERT(m_operationsInProcess == 0);
        ++m_operationsInProcess;
    }

    WebViewSyncOperationMarker(const WebViewSyncOperationMarker&) = delete;
    WebViewSyncOperationMarker(WebViewSyncOperationMarker&&) = delete;

public:
    ~WebViewSyncOperationMarker()
    {
        ASSERT(m_operationsInProcess == 1);

        if( m_operationsInProcess != 0 )
            --m_operationsInProcess;
    }

    static std::unique_ptr<WebViewSyncOperationMarker> MarkInProgress()
    {
        return std::unique_ptr<WebViewSyncOperationMarker>(new WebViewSyncOperationMarker());
    }

    static bool IsOperationInProgress()
    {
        return ( m_operationsInProcess != 0 );
    }

    static void DisplayErrorIfOperationInProcess()
    {
        if( IsOperationInProgress() )
            DisplayOperationInProcessError();
    }

private:
    static void DisplayOperationInProcessError();

private:
    static unsigned m_operationsInProcess;
};

#include "StdAfx.h"
#include "WindowsRuntimeView.h"
#include "WindowsRuntimeHost.h"
#include <zToolsO/Utf8.h>


BEGIN_MESSAGE_MAP(WindowsRuntimeView, HtmlViewerView)
    ON_MESSAGE(UWM::Runtime::RunUiThreadAction, OnRunUiThreadAction)
    ON_MESSAGE(UWM::Runtime::ProcessMessages, OnProcessMessages)
    ON_MESSAGE(UWM::Runtime::AllRuntimesClosed, OnAllRuntimesClosed)
END_MESSAGE_MAP()


WindowsRuntimeView::WindowsRuntimeView()
    :   m_runtimeHost(std::make_unique<WindowsRuntimeHost>(*this))
{
}


WindowsRuntimeView::~WindowsRuntimeView()
{
}


void WindowsRuntimeView::SetRuntimeTitle(const SharableString description)
{
    WindowsUtf8::SetText(this, *description);
}


void WindowsRuntimeView::OnInitialUpdate()
{
    __super::OnInitialUpdate();

    m_htmlViewCtrl.AddNavigationStartingObserver(
        [&](bool& cancel_navigation, const std::string& url)
        {
            m_runtimeHost->OnNavigationStarting(cancel_navigation, url);
        });

    m_htmlViewCtrl.AddSourceChangedObserver(
        [&](const std::string& uri)
        {
            m_runtimeHost->OnSourceChanged(uri);
        });

    m_htmlViewCtrl.AddNavigationCompletedObserver(
        [&](const bool success)
        {
            if( success )
                m_runtimeHost->OnNavigationCompleted();
        });

    m_htmlViewCtrl.AddWebEventObserver(
        [&](const std::wstring_view message_sv)
        {
            m_runtimeHost->OnWebMessageReceived(TC::ToUtf8(message_sv));
        });
}


LRESULT WindowsRuntimeView::OnRunUiThreadAction(const WPARAM wParam, LPARAM /*lParam*/)
{
    // RT_TODO perhaps use the new RunOnUIThreadAsync functionality
    m_runtimeHost->RunUiThreadAction(static_cast<int>(wParam));
    return 0;
}


LRESULT WindowsRuntimeView::OnProcessMessages(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_runtimeHost->ProcessMessageQueue();
    return 0;
}


LRESULT WindowsRuntimeView::OnAllRuntimesClosed(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    AllRuntimesClosed();
    return 0;
}


void WindowsRuntimeView::AllRuntimesClosed()
{
    ASSERT(false);
    m_runtimeHost->NavigateToAsync(Html::CSProUsersForumUrl);
}

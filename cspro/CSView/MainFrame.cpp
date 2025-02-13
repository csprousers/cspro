#include "StdAfx.h"
#include "MainFrame.h"
#include "ViewDoc.h"
#include <zToolsO/CommonObjectTransporter.h>
#include <zToolsO/UWM.h>
#include <zUtilF/UIThreadRunner.h>
#include <zUtilF/UWM.h>
#include <zEngineF/EngineUI.h>


IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_MESSAGE(UWM::ToolsO::GetObjectTransporter, OnGetObjectTransporter)
    ON_MESSAGE(WM_IMSA_PORTABLE_ENGINEUI, OnEngineUI)
    ON_MESSAGE(UWM::UtilF::RunOnUIThread, OnRunOnUIThread)
    ON_MESSAGE(UWM::UtilF::GetApplicationShutdownRunner, OnGetApplicationShutdownRunner)
END_MESSAGE_MAP()



CMainFrame::CMainFrame()
    :   m_onInitialActivateFrame(true)
{
}


CMainFrame::~CMainFrame()
{
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if( __super::OnCreate(lpCreateStruct) == -1 )
        return -1;

    // add the menu
	if( !m_wndMenuBar.Create(this) )
		return -1;

	m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

	// prevent the menu bar from taking the focus on activation
	CMFCPopupMenu::SetForceMenuFocus(FALSE);

	return 0;
}


void CMainFrame::ActivateFrame(int nCmdShow)
{
    if( m_onInitialActivateFrame )
    {
        if( CSViewSettings::ShowWindowMaximized )
            nCmdShow = SW_SHOWMAXIMIZED;

        m_onInitialActivateFrame = false;
    }       

    __super::ActivateFrame(nCmdShow);
}


void CMainFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
    std::wstring title;

    if( bAddToTitle )
    {
        ViewDoc* view_doc = assert_cast<ViewDoc*>(GetActiveDocument());
        const std::wstring* description = view_doc->GetDescription();

        if( description != nullptr )
            title = *description + _T(" - ");
    }

    title.append(_T("CSView"));

    SetWindowText(title.c_str());
}


LRESULT CMainFrame::OnGetObjectTransporter(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if( m_objectTransporter == nullptr )
    {
        class CSViewObjectTransporter : public CommonObjectTransporter
        {
            bool DisableAccessTokenCheckForExternalCallers() const override { return true; }
        };

        m_objectTransporter = std::make_unique<CSViewObjectTransporter>();
    }

    return reinterpret_cast<LRESULT>(m_objectTransporter.get());
}


LRESULT CMainFrame::OnEngineUI(WPARAM wParam, LPARAM lParam)
{
    if( m_engineUIProcessor == nullptr )
        m_engineUIProcessor = std::make_unique<EngineUIProcessor>(nullptr, false);

    return m_engineUIProcessor->ProcessMessage(wParam, lParam);
}


LRESULT CMainFrame::OnRunOnUIThread(WPARAM wParam, LPARAM /*lParam*/)
{
    UIThreadRunner* ui_thread_runner = reinterpret_cast<UIThreadRunner*>(wParam);
    ui_thread_runner->Execute();
    return 1;
}


LRESULT CMainFrame::OnGetApplicationShutdownRunner(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    return reinterpret_cast<LRESULT>(&m_applicationShutdownRunner);
}

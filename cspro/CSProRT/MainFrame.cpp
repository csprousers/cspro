#include "StdAfx.h"
#include "MainFrame.h"
#include "HomeScreenRuntime.h"
#include "RuntimeView.h"
#include "WindowsApplicationListingRuntime.h"
#include <zToolsO/UWM.h>
#include <zUtilO/FileDlg.h>
#include <zUtilO/FileUtil.h>
#include <zRuntimeO/WindowsRuntimeHost.h>


IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)

    ON_WM_CREATE()
    ON_WM_CLOSE()

    ON_COMMAND(ID_FILE_OPEN_APPLICATION, OnFileOpenApplication)
    ON_COMMAND(ID_FILE_OPEN_DIRECTORY, OnFileOpenDirectory)
    ON_COMMAND(ID_FILE_CLOSE_RUNTIME, OnFileCloseRuntime)

    ON_MESSAGE(UWM::CSProRT::CloseRuntime, OnCloseRuntime)

    ON_MESSAGE(UWM::ToolsO::DisplayErrorMessage, OnDisplayErrorMessage)

    ON_MESSAGE(UWM::UtilF::GetApplicationShutdownRunner, OnGetApplicationShutdownRunner)

END_MESSAGE_MAP()


CMainFrame::CMainFrame()
    :   m_runtimeHost(nullptr)
{
}


CMainFrame::~CMainFrame()
{
}


BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style = cs.style ^ FWS_ADDTOTITLE;
    return CFrameWnd::PreCreateWindow(cs);
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


void CMainFrame::OnClose()
{
    PostMessage(UWM::CSProRT::CloseRuntime, 1);
}


void CMainFrame::OnFileCloseRuntime()
{
    PostMessage(UWM::CSProRT::CloseRuntime, 0);
}


LRESULT CMainFrame::OnCloseRuntime(const WPARAM wParam, LPARAM /*lParam*/)
{
    const bool close_all_runtimes = ( wParam == 1 );

    if( m_runtimeHost == nullptr || m_runtimeHost->CloseRuntimes(close_all_runtimes) == 0 )
        __super::OnClose();

    return 0;
}


void CMainFrame::Initialize(WindowsRuntimeHost& runtime_host, const std::string& path_from_command_line)
{
    ASSERT(m_runtimeHost == nullptr);

    m_runtimeHost = &runtime_host;

    std::unique_ptr<Runtime> runtime;

    if( path_from_command_line.empty() )
    {
        runtime = std::make_unique<HomeScreenRuntime>();
    }

    else if( PortableFunctions::FileIsDirectory(path_from_command_line) )
    {
        runtime = std::make_unique<WindowsApplicationListingRuntime>(path_from_command_line);
    }

    else
    {
        runtime = m_runtimeHost->CreateRuntimeForApplication(path_from_command_line);
    }

    m_runtimeHost->StartRuntimeAsync(std::move(runtime));
}


void CMainFrame::OnFileOpenApplication()
{
    ASSERT(m_runtimeHost != nullptr);

    OpenFileDlg open_file_dlg(0, FileExtensions::Pff, nullptr, FileFilters::Pff, this);
    open_file_dlg.SetTitle(L"Select CSPro Application");

    if( open_file_dlg.DoModal() != IDOK )
        return;

    m_runtimeHost->StartRuntimeAsync(m_runtimeHost->CreateRuntimeForApplication(open_file_dlg.GetFilePath()));
}


void CMainFrame::OnFileOpenDirectory()
{
    ASSERT(m_runtimeHost != nullptr);

    std::optional<std::string> directory = SelectFolderDialog(m_hWnd, "Select a directory containing CSPro applications:");

    if( !directory.has_value() )
        return;

    m_runtimeHost->StartRuntimeAsync(std::make_unique<WindowsApplicationListingRuntime>(std::move(*directory)));
}


LRESULT CMainFrame::OnDisplayErrorMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ErrorMessage::DisplayPostedMessages();
    return 0;
}


LRESULT CMainFrame::OnGetApplicationShutdownRunner(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    return reinterpret_cast<LRESULT>(&m_applicationShutdownRunner);
}

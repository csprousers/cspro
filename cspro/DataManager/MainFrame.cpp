#include "StdAfx.h"
#include "MainFrame.h"
#include "CaseListingView.h"
#include "DataSourceFrame.h"
#include "DownloadDataSourceDlg.h"
#include "OpenCSWebDataDlg.h"
#include "OpenDataSourceDlg.h"
#include "TaskRunnerDlg.h"
#include <zToolsO/CommonObjectTransporter.h>
#include <zToolsO/UWM.h>
#include <zUtilO/TemporaryFile.h>
#include <zUtilF/ManageCredentialsDlg.h>
#include <zUtilF/resource_shared.h>
#include <zUtilF/UIThreadRunner.h>
#include <zUtilF/WindowsMenuManager.h>


BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWndEx)

    ON_WM_CREATE()
    ON_WM_INITMENUPOPUP()

    // File menu
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
    ON_COMMAND(ID_FILE_OPEN_DATA_SOURCE, OnFileOpenDataSource)
    ON_COMMAND(ID_FILE_OPEN_CSWEB_DATA, OnFileOpenCSWebData)
    ON_COMMAND(ID_FILE_CLOSE_ALL, OnFileCloseAll)
    ON_COMMAND(ID_FILE_DOWNLOAD_DATA_SOURCE, OnFileDownloadDataSource)
    ON_COMMAND(ID_FILE_MANAGE_CREDENTIALS, OnFileManageCredentials)

    // Windows menu
    ON_COMMAND(ID_WINDOWS_WINDOWS, OnWindowWindows)

    // status bar handlers
    ON_UPDATE_COMMAND_UI_RANGE(ID_STATUS_PANE_DATA_SOURCE_INFO, ID_STATUS_PANE_CASE_POSITION, OnUpdateStatusBar)

    // other message handlers
    ON_MESSAGE(UWM::DataManager::RunStartupActions, OnRunStartupActions)
    ON_MESSAGE(UWM::ToolsO::DisplayErrorMessage, OnDisplayErrorMessage)
    ON_MESSAGE(UWM::ToolsO::GetObjectTransporter, OnGetObjectTransporter)
    ON_MESSAGE(UWM::UtilF::RunOnUIThread, OnRunOnUIThread)
    ON_MESSAGE(UWM::UtilF::GetApplicationShutdownRunner, OnGetApplicationShutdownRunner)

    // interapp communication
    ON_MESSAGE(WM_IMSA_FILEOPEN, OnIMSAFileOpen)

END_MESSAGE_MAP()


namespace
{
    constexpr UINT StatusBarIndicators[] =
    {
        ID_SEPARATOR,           // status line indicator
        ID_STATUS_PANE_DATA_SOURCE_INFO,
        ID_STATUS_PANE_CASE_COUNT,
        ID_STATUS_PANE_CASE_KEY,
        ID_STATUS_PANE_CASE_POSITION,
        ID_INDICATOR_CAPS,
        ID_INDICATOR_NUM,
        ID_INDICATOR_OVR,
    };
}


CMainFrame::CMainFrame(std::shared_ptr<ConnectionStringFileSimulator> connection_string_file_simulator)
    :   m_className(nullptr),
        m_connectionStringFileSimulator(std::move(connection_string_file_simulator))
{
    ASSERT(m_connectionStringFileSimulator != nullptr);
}


CMainFrame::~CMainFrame()
{
}


BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if( !__super::PreCreateWindow(cs) )
        return FALSE;

    // register a class name so that the URI Handler can open
    // data sources in an existing instance of Data Manager
    if( m_className == nullptr )
    {
        WNDCLASS wndcls;
        ::GetClassInfo(AfxGetInstanceHandle(), cs.lpszClass, &wndcls);
        wndcls.hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
        wndcls.lpszClassName = IMSA_WNDCLASS_DATAMANAGER;
        VERIFY(AfxRegisterClass(&wndcls));

        m_className = wndcls.lpszClassName;
    }

    cs.lpszClass = m_className;

    return TRUE;
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if( __super::OnCreate(lpCreateStruct) == -1 )
        return -1;

    // add the menu
    if( !m_wndMenuBar.Create(this) )
    {
        return -1;
    }

    m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

    // ensure that keystrokes will be handled by the menu (while it is open)
    ASSERT(CMFCPopupMenu::GetForceMenuFocus());


    // add the status bar
    if( !m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(StatusBarIndicators, _countof(StatusBarIndicators)) )
    {
        return -1;
    }


    // dock the menu and toolbars
    CDockingManager::SetDockingMode(DT_SMART);

    DockPane(&m_wndMenuBar);

    // Switch the order of document name and application name on the window title bar. This
    // improves the usability of the taskbar because the document name is visible with the thumbnail.
    ModifyStyle(0, FWS_PREFIXTITLE);


    // post a message to run startup actions once all windows are fully set up
    PostMessage(UWM::DataManager::RunStartupActions);

    return 0;
}


LRESULT CMainFrame::OnRunStartupActions(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ASSERT(m_fileServer == nullptr);
    m_fileServer = std::make_unique<SharedHtmlLocalFileServer>();

    ASSERT(m_acceleratorKeyMap.empty());
    SetUpAcceleratorKeyLookup();

    return 1;
}


LRESULT CMainFrame::DefWindowProc(const UINT nMsg, const WPARAM wParam, const LPARAM lParam)
{
    // to ensure that the window title always appears as [MDI frame title] - [main frame title],
    // let CWnd, not CMDIFrameWndEx, handle the WM_SETTEXT message; this idea came from:
    // https://stackoverflow.com/questions/14289238/mfc-mdi-main-frame-title-is-truncated-when-maximized-child-window-has-very-lon
    if( nMsg == WM_SETTEXT )
        return CWnd::DefWindowProc(nMsg, wParam, lParam);

    return CMDIFrameWndEx::DefWindowProc(nMsg, wParam, lParam);
}


void CMainFrame::OnUpdateFrameTitle(const BOOL bAddToTitle)
{
    // as in DefWindowProc above, to properly set the window title, even when the MDI window is maximized,
    // this method is overriden (and copied from CMDIFrameWnd::OnUpdateFrameTitle), with the
    // line checking for maximization commented out

    if( ( GetStyle() & FWS_ADDTOTITLE) == 0 )
        return; // leave it alone!

    CDocument* pDocument = GetActiveDocument();
    CMDIChildWnd* pActiveChild = nullptr;

    if( bAddToTitle &&
        ( pActiveChild = MDIGetActive() ) != nullptr &&
     // ( pActiveChild->GetStyle() & WS_MAXIMIZE ) == 0 &&
        ( pDocument != nullptr || ( pDocument = pActiveChild->GetActiveDocument() ) != nullptr ) )
    {
        UpdateFrameTitleForDocument(pDocument->GetTitle());
    }

    else
    {
        if( pActiveChild != nullptr && ( pActiveChild->GetStyle() & WS_MAXIMIZE ) == 0 )
        {
            const CString strTitle = pActiveChild->GetTitle();

            if( !strTitle.IsEmpty() )
            {
                UpdateFrameTitleForDocument(strTitle);
                return;
            }
        }

        UpdateFrameTitleForDocument(nullptr);
    }
}


void CMainFrame::OnInitMenuPopup(CMenu* const pPopupMenu, const UINT nIndex, const BOOL bSysMenu)
{
    __super::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

    if( !bSysMenu && pPopupMenu->GetMenuItemID(0) == ID_VIEW_OPTIONS_PLACEHOLDER )
        assert_cast<CaseHoldingFrame*>(MDIGetActive())->PopulateViewOptionsMenu(*pPopupMenu);
}


void CMainFrame::OpenDataSource(const ConnectionString& connection_string)
{
    AfxGetApp()->OpenDocumentFile(m_connectionStringFileSimulator->GetFilePath(connection_string).c_str());
}


void CMainFrame::OnFileOpen()
{
    DataFileDlg data_file_dlg(DataFileDlg::Type::OpenExisting, true);
    data_file_dlg.SetTitle(L"Select Data Source(s)")
                 .AllowMultipleSelections();

    if( data_file_dlg.DoModal() != IDOK )
        return;

    for( const ConnectionString& connection_string : data_file_dlg.GetConnectionStrings() )
        OpenDataSource(connection_string);
}


void CMainFrame::OnFileOpenDataSource()
{
    OpenDataSourceDlg open_data_source_dlg(this);

    if( open_data_source_dlg.DoModal() != IDOK )
        return;

    OpenDataSource(open_data_source_dlg.GetConnectionString());
}


void CMainFrame::OnFileOpenCSWebData()
{
    OpenCSWebDataDlg open_csweb_data_dlg(this);

    if( open_csweb_data_dlg.DoModal() != IDOK )
        return;

    OpenDataSource(open_csweb_data_dlg.GetConnectionString());
}


LRESULT CMainFrame::OnIMSAFileOpen(const WPARAM /*wParam*/, const LPARAM /*lParam*/)
{
    // this message is sent by the URI Handler
    CString open_parameters;

    if( IMSAOpenSharedFile(open_parameters) )
        OpenDataSource(ConnectionString(UTF8_TODO::GetUtf8(open_parameters)));

    return 0;
}


void CMainFrame::OnFileCloseAll()
{
    CMDIChildWnd* active_wnd = MDIGetActive();

    while( active_wnd != nullptr )
    {
        active_wnd->GetActiveFrame()->SendMessage(WM_CLOSE);

        CMDIChildWnd* const new_active_wnd = MDIGetActive();

        // if the active window is the same as the one that was supposed to be
        // closed, it means that the user has canceled the closing operation
        if( new_active_wnd == active_wnd )
            return;

        active_wnd = new_active_wnd;
    }
}


void CMainFrame::OnFileDownloadDataSource()
{
    DownloadDataSourceDlg download_data_source_dlg(this);

    if( download_data_source_dlg.DoModal() != IDOK )
        return;

    TaskRunnerDlg task_runner_dlg(download_data_source_dlg.ReleaseSyncTask(), this);
    task_runner_dlg.SetCloseDialogOnSuccess();

    if( task_runner_dlg.DoModal() != IDOK )
        return;

    OpenDataSource(download_data_source_dlg.GetConnectionString());
}


void CMainFrame::OnFileManageCredentials()
{
    ManageCredentialsDlg manage_credentials_dlg;
    manage_credentials_dlg.DoModal();
}


void CMainFrame::OnWindowWindows()
{
    WindowsMenuManager::ShowWindowsDocDlg(*this);
}


void CMainFrame::SetStatusBarPaneText(const UINT indicator, const wchar_t* text)
{
    const int pane_index = ( indicator == ID_STATUS_PANE_DATA_SOURCE_INFO ) ? 1 :
                           ( indicator == ID_STATUS_PANE_CASE_COUNT )       ? 2 :
                           ( indicator == ID_STATUS_PANE_CASE_KEY )         ? 3 :
                         /*( indicator == ID_STATUS_PANE_CASE_POSITION )*/    4;
    ASSERT(StatusBarIndicators[pane_index] == indicator);

    if( text == nullptr )
    {
        text = L"";
    }

    // potentially update the pane width
    else
    {
        constexpr LONG Margin = 10;
        CDC* const pDC = m_wndStatusBar.GetDC();
        pDC->SelectObject(m_wndStatusBar.GetFont());
        m_wndStatusBar.SetPaneInfo(pane_index, StatusBarIndicators[pane_index], SBPS_NORMAL, pDC->GetTextExtent(text, wcslen(text)).cx + Margin);
    }

    m_wndStatusBar.SetPaneText(pane_index, text);
}


void CMainFrame::ClearStatusBarPaneText()
{
    SetStatusBarPaneText(ID_STATUS_PANE_DATA_SOURCE_INFO, nullptr);
    SetStatusBarPaneText(ID_STATUS_PANE_CASE_COUNT, nullptr);
    SetStatusBarPaneText(ID_STATUS_PANE_CASE_KEY, nullptr);
    SetStatusBarPaneText(ID_STATUS_PANE_CASE_POSITION, nullptr);
}


void CMainFrame::OnUpdateStatusBar(CCmdUI* const pCmdUI)
{
    // enable all status bar text
    pCmdUI->Enable(TRUE);
}


LRESULT CMainFrame::OnDisplayErrorMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ErrorMessage::DisplayPostedMessages();
    return 0;
}


LRESULT CMainFrame::OnGetObjectTransporter(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if( m_objectTransporter == nullptr )
    {
        class DataManagerObjectTransporter : public CommonObjectTransporter
        {
            bool DisableAccessTokenCheckForExternalCallers() const override { return true; }
        };

        m_objectTransporter = std::make_unique<DataManagerObjectTransporter>();
    }

    return reinterpret_cast<LRESULT>(m_objectTransporter.get());
}


LRESULT CMainFrame::OnRunOnUIThread(const WPARAM wParam, LPARAM /*lParam*/)
{
    UIThreadRunner* const ui_thread_runner = reinterpret_cast<UIThreadRunner*>(wParam);
    ui_thread_runner->Execute();
    return 1;
}


LRESULT CMainFrame::OnGetApplicationShutdownRunner(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    return reinterpret_cast<LRESULT>(&m_applicationShutdownRunner);
}


void CMainFrame::SetUpAcceleratorKeyLookup()
{
    constexpr UINT FirstApplicationCommandId = WM_APP;
    constexpr UINT FirstMfcCommandId = ID_FILE_NEW;

    static_assert(FirstApplicationCommandId == 0x8000 &&
                  FirstMfcCommandId == 0xE100 &&
                  FirstApplicationCommandId < FirstMfcCommandId);

    // translate accelerator keys to a format to match what will be received from WebView2
    const int count = CopyAcceleratorTable(m_hAccelTable, nullptr, 0);

    auto accelerators = std::make_unique_for_overwrite<ACCEL[]>(count);
    ACCEL* accelerator_itr = accelerators.get();
    const ACCEL* const accelerator_end = accelerator_itr + count;

    CopyAcceleratorTable(m_hAccelTable, accelerator_itr, count);

    for( ; accelerator_itr != accelerator_end; ++accelerator_itr )
    {
        ASSERT(( accelerator_itr->fVirt & FVIRTKEY ) != 0);
        ASSERT(( accelerator_itr->fVirt & FNOINVERT ) != 0);

        AcceleratorKeyData::ReturnToWebView2 result;

        // WebView2 will never handle application commands
        if( accelerator_itr->cmd < FirstMfcCommandId )
        {
            result = AcceleratorKeyData::ReturnToWebView2::Never;
        }

        // for MFC commands...
        else
        {
            switch( accelerator_itr->cmd )
            {
                // ...potentially suppress some commands that may lead to sensitive data being extracted
                case ID_EDIT_COPY:
                    result = AcceleratorKeyData::ReturnToWebView2::Potentially;
                    break;

                // ...always allow WebView2 to handle some commands
                case ID_EDIT_FIND:
                    result = AcceleratorKeyData::ReturnToWebView2::Always;
                    break;

                // ... and always handle other commands ourselves
                default:
                    result = AcceleratorKeyData::ReturnToWebView2::Never;
                    break;
            }
        }

        // add the accelerator key to the map
        m_acceleratorKeyMap[accelerator_itr->key].emplace_back(
            AcceleratorKeyData
            {
                accelerator_itr->cmd,
                {
                    ( accelerator_itr->fVirt & FALT ) != 0,
                    ( accelerator_itr->fVirt & FCONTROL ) != 0,
                    ( accelerator_itr->fVirt & FSHIFT ) != 0
                },
                result
            });
    }

    // add handlers for accelerators keys that are not in the accelerator table

    // Ctrl+Tab
    m_acceleratorKeyMap['\t'].emplace_back(
        AcceleratorKeyData
        {
            [&]() { MDINext(); },
            { false, true, false },
            AcceleratorKeyData::ReturnToWebView2::Never
        });

    // Ctrl+Shift+Tab
    m_acceleratorKeyMap['\t'].emplace_back(
        AcceleratorKeyData
        {
            [&]() { MDIPrev(); },
            { false, true, true },
            AcceleratorKeyData::ReturnToWebView2::Never
        });
}


bool CMainFrame::HandleAcceleratorKeyFromWebView2(const CaseHoldingDoc& case_holding_doc, const UINT key, const INT lParam)
{
    constexpr bool WebView2Handles = false;
    constexpr bool WeHandleOrSuppress = true;

    const auto& key_lookup = m_acceleratorKeyMap.find(key);

    // WebView2 can process anything that is not one of our accelerators
    if( key_lookup != m_acceleratorKeyMap.cend() )
    {
        // if the key matches, make sure the state matches
        const std::tuple<bool, bool, bool> alt_control_shift(( lParam & ( 1 << 29 ) ) != 0,
                                                             ( GetKeyState(VK_CONTROL) & 0x8000 ),
                                                             ( GetKeyState(VK_SHIFT) & 0x8000 ));

        for( const AcceleratorKeyData& data : key_lookup->second )
        {
            // if matching one of our accelerators...
            if( data.alt_control_shift == alt_control_shift )
            {
                // ...always allow WebView2 to handle some commands
                if( data.result == AcceleratorKeyData::ReturnToWebView2::Always )
                {
                    return WebView2Handles;
                }

                // ...determine if the dictionary's security settings allow the handling of this accelerator
                else if( data.result == AcceleratorKeyData::ReturnToWebView2::Potentially )
                {
                    return case_holding_doc.DictionaryAllowsExport() ? WebView2Handles :
                                                                       WeHandleOrSuppress;
                }

                // ... or handle the accelerator ourselves
                else
                {
                    if( std::holds_alternative<WORD>(data.command_id_or_callback) )
                    {
                        PostMessage(WM_COMMAND, std::get<WORD>(data.command_id_or_callback));
                    }

                    else
                    {
                        std::get<std::function<void()>>(data.command_id_or_callback)();
                    }

                    return WeHandleOrSuppress;
                }
            }
        }
    }

    // WebView2 can process anything that is not one of our accelerators
    return WebView2Handles;
}

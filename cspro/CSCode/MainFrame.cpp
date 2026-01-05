#include "StdAfx.h"
#include "MainFrame.h"
#include "CodeDocVirtualFileMappingHandler.h"
#include "HtmlDialogTemplatesDlg.h"
#include "LanguageSettingsPersister.h"
#include "LocalhostSettingsDlg.h"
#include <zToolsO/UWM.h>
#include <zUtilO/UIThreadRunner.h>
#include <zUtilF/DocViewIterators.h>
#include <zUtilF/resource_shared.h>
#include <zUtilF/WindowsMenuManager.h>
#include <zLogicO/ReservedWords.h>
#include <zDesignerF/DesignerObjectTransporter.h>
#include <zEngineF/EngineUI.h>
#include <zHelp/ChmFileVirtualFile.h>


BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWndEx)

    ON_WM_CREATE()
    ON_WM_ACTIVATEAPP()
    ON_WM_CLOSE()
    ON_WM_INITMENUPOPUP()

    // File menu
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)

    ON_COMMAND(ID_FILE_CLOSE_ALL, OnFileCloseAll)

    ON_COMMAND(ID_FILE_SAVE_ALL, OnFileSaveAll)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_ALL, OnUpdateFileSaveAll)

    ON_COMMAND(ID_FILE_LOCALHOST_SETTINGS, OnFileLocalhostSettings)

    // Run menu
    ON_COMMAND(ID_RUN_STOP, OnRunStop)
    ON_UPDATE_COMMAND_UI(ID_RUN_STOP, OnUpdateRunStop)

    ON_COMMAND(ID_RUN_HTML_DIALOG_TEMPLATES, OnRunHtmlDialogTemplates)

    // Windows menu
    ON_COMMAND_RANGE(WindowsMenuManager::DocumentFirstId, WindowsMenuManager::DocumentLastId, OnWindowDocument)
    ON_COMMAND(ID_WINDOWS_WINDOWS, OnWindowWindows)

    ON_COMMAND_RANGE(ID_WINDOW_BUILD, ID_WINDOW_HTML_VIEWER, OnWindowDockablePane)
    ON_UPDATE_COMMAND_UI_RANGE(ID_WINDOW_BUILD, ID_WINDOW_HTML_VIEWER, OnUpdateWindowDockablePane)

    // context menu handlers
    ON_COMMAND(ID_CLOSE_ALL_BUT_THIS, OnCloseAllButThis)
    ON_COMMAND_RANGE(ID_LOCALHOST_COPY_MAPPING, ID_LOCALHOST_OPEN_IN_WEB_BROWSER, OnLocalhostMapping)

    // toolbar handlers
    ON_MESSAGE(UWM::CSCode::SyncToolbar, OnSyncToolbar)

    // tab handlers
    ON_REGISTERED_MESSAGE(AFX_WM_ON_GET_TAB_TOOLTIP, OnGetTabToolTip)

    // status bar handlers
    ON_MESSAGE(WM_IMSA_SET_STATUSBAR_PANE, OnSetStatusBarFilePos)
    ON_MESSAGE(UWM::CSCode::SetStatusBarFileType, OnSetStatusBarFileType)
    ON_UPDATE_COMMAND_UI_RANGE(ID_STATUS_PANE_FILE_POS, ID_STATUS_PANE_FILE_TYPE, OnUpdateStatusBar)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_OVR, OnUpdateKeyOvertype)

    // other message handlers
    ON_MESSAGE(UWM::UtilO::IsReservedWord, OnIsReservedWord)
    ON_MESSAGE(UWM::UtilO::GetCodeText, OnGetCodeText)
    ON_MESSAGE(UWM::CSCode::GetCodeTextForDoc, OnGetCodeTextForDoc)
    ON_MESSAGE(UWM::UtilO::GetSharedDictionaryConst, OnGetSharedDictionaryConst)

    ON_MESSAGE(UWM::CSCode::StartLocalhost, OnStartLocalhost)
    ON_MESSAGE(UWM::CSCode::RunOperationComplete, OnRunOperationComplete)
    ON_MESSAGE(UWM::CSCode::ModelessDialogDestroyed, OnModelessDialogDestroyed)

    ON_MESSAGE(UWM::ToolsO::DisplayErrorMessage, OnDisplayErrorMessage)
    ON_MESSAGE(UWM::ToolsO::GetObjectTransporter, OnGetObjectTransporter)
    ON_MESSAGE(WM_IMSA_PORTABLE_ENGINEUI, OnEngineUI)
    ON_MESSAGE(UWM::UtilO::RunOnUIThread, OnRunOnUIThread)
    ON_MESSAGE(UWM::UtilF::GetApplicationShutdownRunner, OnGetApplicationShutdownRunner)

END_MESSAGE_MAP()


namespace
{
    constexpr UINT StatusBarIndicators[] =
    {
        ID_SEPARATOR,           // status line indicator
        ID_STATUS_PANE_FILE_POS,
        ID_STATUS_PANE_FILE_TYPE,
        ID_INDICATOR_CAPS,
        ID_INDICATOR_NUM,
        ID_INDICATOR_OVR,
    };
}


CMainFrame::CMainFrame()
{
}


CMainFrame::~CMainFrame()
{
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if( __super::OnCreate(lpCreateStruct) == -1 )
        return -1;

    // set up the tabs
    CMDITabInfo mdiTabParams;
    mdiTabParams.m_style = CMFCTabCtrl::STYLE_3D_ONENOTE;   // other styles available...
    mdiTabParams.m_bTabCustomTooltips = TRUE;               // set to TRUE to enable custom tooltips
    mdiTabParams.m_bActiveTabCloseButton = TRUE;            // set to FALSE to place close button at right of tab area
    mdiTabParams.m_bTabIcons = FALSE;                       // set to TRUE to enable document icons on MDI tabs
    mdiTabParams.m_bAutoColor = FALSE;                      // set to FALSE to disable auto-coloring of MDI tabs
    mdiTabParams.m_bDocumentMenu = TRUE;                    // enable the document menu at the right edge of the tab area
    EnableMDITabbedGroups(TRUE, mdiTabParams);

    // add the menu
    if( !m_wndMenuBar.Create(this) )
    {
        return -1;
    }

    m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

    // prevent the menu bar from taking the focus on activation
    CMFCPopupMenu::SetForceMenuFocus(FALSE);


    // add the toolbars
    auto add_toolbar = [&](CMFCToolBar& toolbar, const unsigned id)
    {
        return ( toolbar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(1, 1, 1, 1), id) &&
                 toolbar.LoadToolBar(id) );
    };

    if( !add_toolbar(m_wndMainToolBar, IDR_MAINFRAME) ||
        !add_toolbar(m_wndCodeToolBar, IDR_CODEFRAME) )
    {
        return -1;
    }

    CMFCToolBar::AddToolBarForImageCollection(IDR_TOOLBAR_EXTRA);


    // add the status bar
    if( !m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(StatusBarIndicators, _countof(StatusBarIndicators)) )
    {
        return -1;
    }


    // dock the menu and toolbars
    CDockingManager::SetDockingMode(DT_SMART);

    DockPane(&m_wndMenuBar);
    DockPane(&m_wndMainToolBar);
    DockPane(&m_wndCodeToolBar);

    PostMessage(UWM::CSCode::SyncToolbar);

    // Switch the order of document name and application name on the window title bar. This
    // improves the usability of the taskbar because the document name is visible with the thumbnail.
    ModifyStyle(0, FWS_PREFIXTITLE);

    // potentially launch the local server on startup
    if( WinSettings::Read<DWORD>(WinSettings::Type::LocalhostStartAutomatically, 0) != 0 )
        PostMessage(UWM::CSCode::StartLocalhost);

    return 0;
}


LRESULT CMainFrame::OnStartLocalhost(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    GetSharedHtmlLocalFileServer();

    return 1;
}


void CMainFrame::OnActivateApp(const BOOL bActive, const DWORD dwThreadID)
{
    __super::OnActivateApp(bActive, dwThreadID);

    if( bActive )
    {
        CMDIChildWnd* const active_wnd = MDIGetActive();

        if( active_wnd != nullptr )
            active_wnd->PostMessage(UWM::CSCode::CodeFrameActivate, static_cast<WPARAM>(-1));
    }
}


void CMainFrame::OnClose()
{
    if( IsRunOperationInProgress() )
    {
        AfxMessageBox(L"There is a running operation and CSCode cannot be closed until it is canceled or completes.");
        return;
    }

    __super::OnClose();
}


CodeDoc* CMainFrame::GetActiveCodeDoc()
{
    CMDIChildWnd* const active_wnd = MDIGetActive();
    return ( active_wnd != nullptr ) ? assert_cast<CodeDoc*>(active_wnd->GetActiveDocument()) : nullptr;
}


CodeDoc* CMainFrame::FindDocument(const std::variant<const CDocument*, std::string_view>& document_or_file_path_sv)
{
    CodeDoc* found_code_doc = nullptr;

    ForeachDoc<CodeDoc>(
        [&](CodeDoc& code_doc)
        {
            bool doc_is_open;

            if( std::holds_alternative<const CDocument*>(document_or_file_path_sv) )
            {
                doc_is_open = ( &code_doc == std::get<const CDocument*>(document_or_file_path_sv) );
            }

            else
            {
                doc_is_open = ( !code_doc.GetFilePath().empty() &&
                                SO::EqualsNoCase(code_doc.GetFilePath(), std::get<std::string_view>(document_or_file_path_sv)) );
            }

            if( doc_is_open )
            {
                found_code_doc = &code_doc;
                return false;
            }

            return true;
        });

    return found_code_doc;
}


bool CMainFrame::ActivateDocument(CodeView* const code_view)
{
    ASSERT(code_view != nullptr);

    auto document_has_view = [&](CodeDoc& code_doc)
    {
        return ( code_view == &code_doc.GetPrimaryCodeView() ||
                 code_view == code_doc.GetSecondaryCodeView() );
    };

    CodeDoc* const active_code_doc = GetActiveCodeDoc();

    if( active_code_doc != nullptr && document_has_view(*active_code_doc) )
        return true;

    // see if the document is still open
    bool doc_is_open = false;

    ForeachDoc<CodeDoc>(
        [&](CodeDoc& code_doc)
        {
            if( document_has_view(code_doc) )
            {
                doc_is_open = true;
                return false;
            }

            return true;
        });

    if( doc_is_open )
    {
        code_view->GetParentFrame()->ActivateFrame();
        return true;
    }

    return false;
}


void CMainFrame::OnInitMenuPopup(CMenu* const pPopupMenu, const UINT nIndex, const BOOL bSysMenu)
{
    __super::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

    if( !bSysMenu )
    {
        constexpr unsigned FirstRunMenuItemId = ID_RUN_PLACEHOLDER;
        constexpr unsigned FirstWindowsMenuItemId = ID_WINDOW_BUILD;

        unsigned first_menu_item_id = pPopupMenu->GetMenuItemID(0);

        // some popup (submenu) menu items in the Code menu have to be replaced
        if( first_menu_item_id == CodeMenu::FirstCodeMenuItemId )
        {
            const CodeDoc* const code_doc = GetActiveCodeDoc();
            ASSERT(code_doc != nullptr);

            const CodeView& code_view = assert_cast<CodeFrame*>(GetActiveFrame())->GetActiveCodeView();

            CodeMenu::ReplaceCodeMenuPopups(pPopupMenu, code_doc->GetLanguageSettings().GetLexerLanguage(),
                                                        code_view.GetLanguageSettings().GetLexerLanguage());
        }

        // the Run menu is always built dynamically
        else if( first_menu_item_id == FirstRunMenuItemId )
        {
            CodeDoc* const code_doc = GetActiveCodeDoc();
            ASSERT(code_doc != nullptr);

            code_doc->GetCodeFrame().PopulateRunMenu(*pPopupMenu);
        }

        // the list of documents in the Windows menu is built dynamically because
        // the automatic behavior will not work because we are not using any of the
        // standard Windows commands (e.g., ID_WINDOW_CASCADE)
        else if( first_menu_item_id == FirstWindowsMenuItemId )
        {
            WindowsMenuManager::Build(*this, pPopupMenu);
        }
    }
}


BOOL CMainFrame::OnShowMDITabContextMenu(CPoint point, const DWORD dwAllowedItems, const BOOL bTabDrop)
{
    if( bTabDrop )
        return __super::OnShowMDITabContextMenu(point, dwAllowedItems, bTabDrop);

    ASSERT(GetActiveCodeDoc() != nullptr);

    CMenu menu;
    menu.LoadMenu(IDR_TAB_CONTEXT);
    ASSERT(menu.GetMenuItemCount() == 1);

    CMenu* const submenu = menu.GetSubMenu(0);

    CMFCPopupMenu* popup_menu = new CMFCPopupMenu;
    popup_menu->SetAutoDestroy(FALSE);
    popup_menu->Create(this, point.x, point.y, submenu->GetSafeHmenu());

    return TRUE;
}


void CMainFrame::OnFileOpen()
{
    OpenFileDlg open_file_dlg(0, nullptr, nullptr, L"All Files (*.*)|*.*|CSPro Logic Files (*.apc)|*.apc||", this);
    open_file_dlg.UseInitialDirectoryOfActiveDocument(this)
                 .SetMultiSelectBuffer();

    if( open_file_dlg.DoModal() != IDOK )
        return;

    for( const std::string& file_path : open_file_dlg.GetFilePaths() )
        AfxGetApp()->OpenDocumentFile(TC::ToWide(file_path).c_str());
}


void CMainFrame::OnFileCloseAll()
{
    CodeDoc* code_doc = GetActiveCodeDoc();

    while( code_doc != nullptr )
    {
        code_doc->GetCodeFrame().SendMessage(WM_CLOSE);

        CodeDoc* const new_active_code_doc = GetActiveCodeDoc();

        // if the active document is the same as the one that was supposed to be
        // closed, it means that the user has canceled the closing operation
        if( new_active_code_doc == code_doc )
            return;

        code_doc = new_active_code_doc;
    }
}


void CMainFrame::OnFileSaveAll()
{
    ForeachDoc<CodeDoc>(
        [&](CodeDoc& code_doc)
        {
            if( code_doc.IsModified() )
            {
                code_doc.DoFileSave();

                // stop processing if the document is still modified, which means
                // the user has canceled the saving operation
                return !code_doc.IsModified();
            }

            return true;
        });
}


void CMainFrame::OnUpdateFileSaveAll(CCmdUI* const pCmdUI)
{
    bool a_document_is_modified = false;

    ForeachDoc<CodeDoc>(
        [&](CodeDoc& code_doc)
        {
            if( code_doc.IsModified() )
            {
                a_document_is_modified = true;
                return false;
            }

            return true;
        });

    pCmdUI->Enable(a_document_is_modified);
}


void CMainFrame::OnFileLocalhostSettings()
{
    LocalhostSettingsDlg dlg;
    dlg.DoModal();
}


void CMainFrame::OnRunStop()
{
    ASSERT(m_currentRunOperation != nullptr);

    if( m_currentRunOperation->IsCancelable() )
    {
        m_currentRunOperation->Cancel();
    }

    else
    {
        AfxMessageBox(L"The currently running operation is not cancelable");
    }
}


void CMainFrame::OnUpdateRunStop(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(IsRunOperationInProgress());
}


void CMainFrame::RegisterRunOperationAndRun(std::shared_ptr<RunOperation> run_operation)
{
    ASSERT(run_operation != nullptr);

    if( IsRunOperationInProgress() )
    {
        ASSERT(false);
        throw CSProException("A running operation is already in progress");
    }

    m_currentRunOperation = std::move(run_operation);

    m_currentRunOperation->Run();
}


LRESULT CMainFrame::OnRunOperationComplete(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if( IsRunOperationInProgress() )
        m_currentRunOperation->OnComplete();

    m_currentRunOperation.reset();

    return 1;
}


void CMainFrame::OnRunHtmlDialogTemplates()
{
    // if the dialog already exists, show it
    const auto& lookup = m_modelessDialogHolders.find(ModelessDialogType::HtmlDialogTemplates);

    if( lookup != m_modelessDialogHolders.cend() )
    {
        if( IsWindow(lookup->second.GetHwnd()) )
        {
            ::SetFocus(lookup->second.GetHwnd());
            return;
        }

        m_modelessDialogHolders.erase(lookup);
    }

    // otherwise create the modeless dialog
    try
    {
        HtmlDialogTemplatesDlg* const html_dialog_templates_dlg = new HtmlDialogTemplatesDlg(GetActiveCodeDoc(), this);

        WindowHelpers::CenterOnScreen(html_dialog_templates_dlg->m_hWnd);

        m_modelessDialogHolders.try_emplace(ModelessDialogType::HtmlDialogTemplates, html_dialog_templates_dlg);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void CMainFrame::OnCloseAllButThis()
{
    const CodeDoc* const active_code_doc = GetActiveCodeDoc();
    CodeDoc* previous_doc_to_close = nullptr;

    while( true )
    {
        CodeDoc* doc_to_close = nullptr;

        ForeachDoc<CodeDoc>(
            [&](CodeDoc& code_doc)
            {
                if( &code_doc == previous_doc_to_close )
                {
                    // if here, it means that the document that we previously tried to close is still open,
                    // which means the user canceled the operation
                    return false;
                }

                else if( &code_doc != active_code_doc )
                {
                    doc_to_close = &code_doc;
                    return false;
                }

                return true;
            });

        if( doc_to_close == nullptr )
            return;

        doc_to_close->GetCodeFrame().SendMessage(WM_CLOSE);

        previous_doc_to_close = doc_to_close;
    }
}


LanguageSettingsPersister& CMainFrame::GetLanguageSettingsPersister()
{
    if( m_languageSettingsPersister == nullptr )
        m_languageSettingsPersister = std::make_unique<LanguageSettingsPersister>();

    return *m_languageSettingsPersister;
}


SharedHtmlLocalFileServer& CMainFrame::GetSharedHtmlLocalFileServer()
{
    if( m_fileServer == nullptr )
        m_fileServer = std::make_unique<SharedHtmlLocalFileServer>();

    return *m_fileServer;
}


std::string CMainFrame::GetLocalhostMappingUrlForActiveDocument()
{
    const CodeDoc* const active_code_doc = GetActiveCodeDoc();
    ASSERT(active_code_doc != nullptr);

    std::string document_file_path = active_code_doc->GetFilePath();

    // a mapping might already exist
    if( !document_file_path.empty() )
    {
        for( const auto& [file_path, handler] : m_documentVirtualFileMappingHandlers )
        {
            if( SO::EqualsNoCase(file_path, document_file_path) )
                return handler->CreateUrlForDocument();
        }
    }

    // otherwise create a new mapping
    auto virtual_file_mapping_handler = std::make_unique<CodeDocVirtualFileMappingHandler>(*this, *active_code_doc);
    GetSharedHtmlLocalFileServer().CreateVirtualDirectory(*virtual_file_mapping_handler);

    std::string url = virtual_file_mapping_handler->CreateUrlForDocument();

    m_documentVirtualFileMappingHandlers.emplace_back(std::move(document_file_path), std::move(virtual_file_mapping_handler));

    return url;
}


void CMainFrame::OnLocalhostMapping(const UINT nID)
{
    const std::string url = GetLocalhostMappingUrlForActiveDocument();

    if( nID == ID_LOCALHOST_COPY_MAPPING )
    {
        WinClipboard::PutText(this, url);
    }

    else if( nID == ID_LOCALHOST_OPEN_IN_HTML_VIEWER )
    {
        HtmlViewerWnd* const html_viewer_wnd = GetHtmlViewerWnd();

        if( html_viewer_wnd != nullptr )
            html_viewer_wnd->GetHtmlViewCtrl().NavigateTo(url);
    }

    else
    {
        ASSERT(nID == ID_LOCALHOST_OPEN_IN_WEB_BROWSER);
        ShellExecute(nullptr, L"open", TC::ToWide(url).c_str(), nullptr, nullptr, SW_SHOW);
    }
}


void CMainFrame::OnWindowDocument(const UINT nID)
{
    WindowsMenuManager::ActivateDocument(*this, nID);
}


void CMainFrame::OnWindowWindows()
{
    WindowsMenuManager::ShowWindowsDocDlg(*this);
}


void CMainFrame::ShowDockablePane(CDockablePane& dockable_pane, const BOOL visibility)
{
    SetFocus();
    ShowPane(&dockable_pane, visibility, FALSE, FALSE);
    RecalcLayout();
}


void CMainFrame::OnWindowDockablePane(const UINT nID)
{
    auto need_to_create = [&](CDockablePane* const dockable_pane)
    {
        // if already created, toggle the visibility
        if( dockable_pane != nullptr )
        {
            ShowDockablePane(*dockable_pane, !dockable_pane->IsVisible());
            return false;
        }

        return true;
    };

    if( nID == ID_WINDOW_BUILD )
    {
        if( need_to_create(m_buildWnd.get()) )
            GetBuildWnd();
    }

    else if( nID == ID_WINDOW_OUTPUT )
    {
        if( need_to_create(m_outputWnd.get()) )
            GetOutputWnd();
    }

    else
    {
        ASSERT(nID == ID_WINDOW_HTML_VIEWER);

        if( need_to_create(m_htmlViewerWnd.get()) )
        {
            GetHtmlViewerWnd();

            // default to showing the CSCode helps, or the CSPro Users Forum if the helps cannot be found
            if( m_htmlViewerWnd != nullptr )
            {
                ASSERT(m_chmFileVirtualFile == nullptr);

                try
                {
                    const std::optional<std::string> cscode_help_file_path = CSProExecutables::GetExecutableHelpPath(CSProExecutables::Program::CSCode);

                    if( cscode_help_file_path.has_value() )
                        m_chmFileVirtualFile = std::make_unique<ChmFileVirtualFile>(*cscode_help_file_path);
                }
                catch(...) { }

                if( m_chmFileVirtualFile != nullptr )
                {
                    m_htmlViewerWnd->GetHtmlBrowser().NavigateTo(m_chmFileVirtualFile->GetDefaultTopicUriResolver());
                }

                else
                {
                    m_htmlViewerWnd->GetHtmlViewCtrl().NavigateTo(Html::CSProUsersForumUrl);
                }
            }
        }
    }
}


void CMainFrame::OnUpdateWindowDockablePane(CCmdUI* const pCmdUI)
{
    const CDockablePane* dockable_pane = ( pCmdUI->m_nID == ID_WINDOW_BUILD )       ? static_cast<CDockablePane*>(m_buildWnd.get()) :
                                         ( pCmdUI->m_nID == ID_WINDOW_OUTPUT )      ? static_cast<CDockablePane*>(m_outputWnd.get()) :
                                       /*( pCmdUI->m_nID == ID_WINDOW_HTML_VIEWER )*/ static_cast<CDockablePane*>(m_htmlViewerWnd.get());

    pCmdUI->SetCheck(dockable_pane != nullptr && dockable_pane->IsVisible());
}


CSCodeBuildWnd* CMainFrame::GetBuildWnd(const bool make_visible_if_not/* = true*/)
{
    if( m_buildWnd != nullptr )
    {
        if( make_visible_if_not && !m_buildWnd->IsVisible() )
            ShowDockablePane(*m_buildWnd, TRUE);
    }

    else
    {
        m_buildWnd = std::make_unique<CSCodeBuildWnd>();

        m_buildWnd->SetIndentMessageLinesAfterFirstLine(true);

        // the window will be docked, but if it is dragged out as a proper window, make sure the width and height are reasonable;
        // the height does dictate how large the window shows while docked (by default, at the bottom)
        CRect rect;
        GetClientRect(rect);
        rect.right = rect.Width() * 3 / 4;
        rect.bottom = std::min(250, rect.Height() / 4);

        if( !m_buildWnd->Create(L"Build", this, rect, TRUE, ID_BUILD_WINDOW,
                                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM | CBRS_FLOAT_MULTI) )
        {
            return ReturnProgrammingError(nullptr);
        }

        m_buildWnd->EnableDocking(CBRS_ALIGN_ANY);
        DockPane(m_buildWnd.get());

        if( !make_visible_if_not )
            ShowDockablePane(*m_buildWnd, FALSE);
    }

    return m_buildWnd.get();
}


OutputWnd* CMainFrame::GetOutputWnd()
{
    if( m_outputWnd != nullptr )
    {
        if( !m_outputWnd->IsVisible() )
            ShowDockablePane(*m_outputWnd, TRUE);
    }

    else
    {
        m_outputWnd = std::make_unique<OutputWnd>();

        // the window will be docked, but if it is dragged out as a proper window, make sure the width and height are reasonable;
        // the width does dictate how large the window shows while docked (by default, at the right)
        CRect rect;
        GetClientRect(rect);
        rect.right = rect.Width() / 3;
        rect.bottom = rect.Height() * 8 / 10;

        if( !m_outputWnd->Create(L"Output", this, rect, TRUE, ID_OUTPUT_WINDOW,
                                 WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI) )
        {
            return ReturnProgrammingError(nullptr);
        }

        m_outputWnd->EnableDocking(CBRS_ALIGN_ANY);
        DockPane(m_outputWnd.get());
    }

    return m_outputWnd.get();
}


HtmlViewerWnd* CMainFrame::GetHtmlViewerWnd()
{
    if( m_htmlViewerWnd != nullptr )
    {
        if( !m_htmlViewerWnd->IsVisible() )
            ShowDockablePane(*m_htmlViewerWnd, TRUE);
    }

    else
    {
        m_htmlViewerWnd = std::make_unique<HtmlViewerWnd>();

        // the window will be docked, but if it is dragged out as a proper window, make sure the width and height are reasonable;
        // the width does dictate how large the window shows while docked (by default, at the right)
        CRect rect;
        GetClientRect(rect);
        rect.right = rect.Width() * 2 / 3;
        rect.bottom = rect.Height() * 8 / 10;

        if( !m_htmlViewerWnd->Create(L"HTML Viewer", this, rect, TRUE, ID_HTML_VIEWER_WINDOW,
                                     WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI) )
        {
            return ReturnProgrammingError(nullptr);
        }

        m_htmlViewerWnd->EnableDocking(CBRS_ALIGN_ANY);
        DockPane(m_htmlViewerWnd.get());
    }

    return m_htmlViewerWnd.get();
}


void CMainFrame::SetStatusBarPaneText(const UINT indicator, const wchar_t* const text)
{
    int pane_index;
    bool update_pane_width;

    if( indicator == ID_STATUS_PANE_FILE_POS )
    {
        pane_index = 1;
        update_pane_width = false;
    }

    else
    {
        ASSERT(indicator == ID_STATUS_PANE_FILE_TYPE);
        pane_index = 2;
        update_pane_width = true;
    }

    ASSERT(StatusBarIndicators[pane_index] == indicator);

    if( update_pane_width )
    {
        constexpr LONG Margin = 20;
        CDC* const pDC = m_wndStatusBar.GetDC();
        pDC->SelectObject(m_wndStatusBar.GetFont());
        m_wndStatusBar.SetPaneInfo(pane_index, StatusBarIndicators[pane_index], SBPS_NORMAL,
                                   pDC->GetTextExtent(text, int32_cast(wcslen(text))).cx + Margin);
    }

    m_wndStatusBar.SetPaneText(pane_index, text);
}


LRESULT CMainFrame::OnSyncToolbar(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    const bool show_code_toolbar = ( MDIGetActive() != nullptr );

    m_wndMainToolBar.ShowPane(!show_code_toolbar, TRUE, FALSE);
    m_wndCodeToolBar.ShowPane(show_code_toolbar, FALSE, FALSE);

    return 1;
}


LRESULT CMainFrame::OnGetTabToolTip(const WPARAM /*wParam*/, const LPARAM lParam)
{
    CMFCTabToolTipInfo* const tab_tool_tip_info = reinterpret_cast<CMFCTabToolTipInfo*>(lParam);
    ASSERT(tab_tool_tip_info != nullptr);
    ASSERT_VALID(tab_tool_tip_info->m_pTabWnd);

    const CodeDoc* const code_doc = GetActiveCodeDoc();

    if( code_doc != nullptr && !code_doc->GetPathName().IsEmpty() &&
        tab_tool_tip_info->m_pTabWnd->IsMDITab() )
    {
        tab_tool_tip_info->m_strText = code_doc->GetPathName();
    }

    return 0;
}


LRESULT CMainFrame::OnSetStatusBarFilePos(const WPARAM wParam, LPARAM /*lParam*/)
{
    const wchar_t* const text = reinterpret_cast<const wchar_t*>(wParam);

    SetStatusBarPaneText(ID_STATUS_PANE_FILE_POS, text);

    return 1;
}


LRESULT CMainFrame::OnSetStatusBarFileType(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    const CodeDoc* const code_doc = GetActiveCodeDoc();

    SetStatusBarPaneText(ID_STATUS_PANE_FILE_TYPE, ( code_doc != nullptr ) ? TC::ToWide(code_doc->GetLanguageSettings().GetFileTypeDescription()).c_str() : L"");

    // because this method is called whenever a tab is created or closed, it is an easy place
    // to add messaging to keep the toolbar in sync
    PostMessage(UWM::CSCode::SyncToolbar);

    // similarly, it is a good place to notify any modeless dialogs about a document change
    for( const auto& [type, dialog_holder] : m_modelessDialogHolders )
        ::PostMessage(dialog_holder.GetHwnd(), UWM::CSCode::ActiveDocChanged, reinterpret_cast<WPARAM>(code_doc), 0);

    return 1;
}


LRESULT CMainFrame::OnModelessDialogDestroyed(const WPARAM wParam, LPARAM /*lParam*/)
{
    HWND hWnd = reinterpret_cast<HWND>(wParam);

    for( const auto& [type, dialog_holder] : m_modelessDialogHolders )
    {
        if( dialog_holder.GetHwnd() == hWnd )
        {
            m_modelessDialogHolders.erase(type);
            return 1;
        }
    }

    return ReturnProgrammingError(0);
}


void CMainFrame::OnUpdateStatusBar(CCmdUI* const pCmdUI)
{
    // enable the status bar file position and type indicators
    pCmdUI->Enable(TRUE);
}


void CMainFrame::OnUpdateKeyOvertype(CCmdUI* const pCmdUI)
{
    BOOL enable = FALSE;
    CFrameWnd* const active_frame = GetActiveFrame();

    if( active_frame != nullptr && active_frame->IsKindOf(RUNTIME_CLASS(CodeFrame)) )
    {
        CodeView& code_view = assert_cast<CodeFrame*>(active_frame)->GetActiveCodeView();
        enable = code_view.GetLogicCtrl()->GetOvertype();
    }

    pCmdUI->Enable(enable);
}


LRESULT CMainFrame::OnIsReservedWord(const WPARAM wParam, LPARAM /*lParam*/)
{
    const std::string_view text_sv = *reinterpret_cast<const std::string_view*>(wParam);
    return Logic::ReservedWords::IsReservedWord(text_sv) ? 1 : 0;
}


LRESULT CMainFrame::OnGetCodeText(const WPARAM wParam, const LPARAM lParam)
{
    const std::string& file_path = *reinterpret_cast<const std::string*>(wParam);
    std::string& text = *reinterpret_cast<std::string*>(lParam);
    bool set_text = false;

    // see if this file is open, and if so, get the text from the Scintilla control
    ForeachDoc<CodeDoc>(
        [&](CodeDoc& code_doc)
        {
            if( SO::EqualsNoCase(code_doc.GetFilePath(), file_path) )
            {
                text = code_doc.GetPrimaryCodeView().GetLogicCtrl()->GetText();
                set_text = true;
            }

            return !set_text;
        });

    return set_text ? 1 : 0;
}


LRESULT CMainFrame::OnGetCodeTextForDoc(const WPARAM wParam, const LPARAM lParam)
{
    CodeDoc* const code_doc = reinterpret_cast<CodeDoc*>(wParam);
    std::string& text = *reinterpret_cast<std::string*>(lParam);
    ASSERT(FindDocument(code_doc) == code_doc);

    text = code_doc->GetPrimaryCodeView().GetLogicCtrl()->GetText();

    return 1;
}


LRESULT CMainFrame::OnGetSharedDictionaryConst(const WPARAM wParam, const LPARAM lParam)
{
    // because the validation of CSPro specification files may involve loading a dictionary,
    // this routine stores dictionaries so that they are not constantly loaded from the disk
    const std::string& dictionary_file_path = *reinterpret_cast<const std::string*>(wParam);
    std::shared_ptr<const CDataDict>& dictionary = *reinterpret_cast<std::shared_ptr<const CDataDict>*>(lParam);

    const int64_t file_modified_time = PortableFunctions::FileModifiedTime(dictionary_file_path);
    const auto& lookup = m_loadedDictionaries.find(dictionary_file_path);

    if( lookup != m_loadedDictionaries.cend() )
    {
        // if the dictionary has already been loaded, make sure it has not been modified externally
        if( file_modified_time == std::get<0>(lookup->second) )
        {
            dictionary = std::get<1>(lookup->second);
            return 1;
        }

        // otherwise reload it
        m_loadedDictionaries.erase(dictionary_file_path);
    }

    // load the dictionary, ignoring errors
    try
    {
        dictionary = CDataDict::InstantiateAndOpen(dictionary_file_path);
        m_loadedDictionaries.try_emplace(dictionary_file_path, std::make_tuple(file_modified_time, dictionary));
        return 1;
    }
    catch(...) { }

    return 0;
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
        class CSCodeObjectTransporter : public DesignerObjectTransporter
        {
            bool DisableAccessTokenCheckForExternalCallers() const override { return true; }
        };

        m_objectTransporter = std::make_unique<CSCodeObjectTransporter>();
    }

    return reinterpret_cast<LRESULT>(m_objectTransporter.get());
}


LRESULT CMainFrame::OnEngineUI(const WPARAM wParam, const LPARAM lParam)
{
    if( m_engineUIProcessor == nullptr )
        m_engineUIProcessor = std::make_unique<EngineUIProcessor>(nullptr, false);

    return m_engineUIProcessor->ProcessMessage(wParam, lParam);
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

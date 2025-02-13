#include "StdAfx.h"
#include "MainFrame.h"
#include "CSDocFrame.h"
#include "DocSetSpecDoc.h"
#include "GlobalSettingsDlg.h"
#include <zUtilF/WindowsMenuManager.h>


BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWndEx)

    ON_WM_CREATE()
    ON_WM_ACTIVATEAPP()
    ON_WM_DESTROY()
    ON_WM_INITMENUPOPUP()

    // File menu
    ON_COMMAND(ID_FILE_NEW_DOCUMENT_SET, OnFileNewDocumentSet)

    ON_COMMAND(ID_FILE_CLOSE_ALL, OnFileCloseAll)

    ON_COMMAND(ID_FILE_SAVE_ALL, OnFileSaveAll)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_ALL, OnUpdateFileSaveAll)

    ON_COMMAND(ID_FILE_GLOBAL_SETTINGS, OnGlobalSettings)

    // Windows menu
    ON_COMMAND_RANGE(WindowsMenuManager::DocumentFirstId, WindowsMenuManager::DocumentLastId, OnWindowDocument)
    ON_COMMAND(ID_WINDOWS_WINDOWS, OnWindowWindows)

    ON_COMMAND_RANGE(ID_WINDOW_BUILD, ID_WINDOW_OUTPUT, OnWindowDockablePane)
    ON_UPDATE_COMMAND_UI_RANGE(ID_WINDOW_BUILD, ID_WINDOW_OUTPUT, OnUpdateWindowDockablePane)

    // context menu
    ON_COMMAND(ID_COPY_FULL_PATH, OnCopyFullPath)
    ON_UPDATE_COMMAND_UI(ID_COPY_FULL_PATH, OnUpdateDocumentMustBeSavedToDisk)

    ON_COMMAND(ID_COPY_FILENAME, OnCopyFilename)
    ON_UPDATE_COMMAND_UI(ID_COPY_FILENAME, OnUpdateDocumentMustBeSavedToDisk)

    ON_COMMAND(ID_OPEN_CONTAINING_FOLDER, OnOpenContainingFolder)
    ON_UPDATE_COMMAND_UI(ID_OPEN_CONTAINING_FOLDER, OnUpdateDocumentMustBeSavedToDisk)

    // toolbar handlers
    ON_MESSAGE(UWM::CSDocument::SyncToolbarAndWindows, OnSyncToolbarAndWindows)

    // status bar handlers
    ON_COMMAND(ID_STATUS_PANE_ASSOCIATED_DOCSET, OnStatusBarAssociatedDocSetClick)
    ON_UPDATE_COMMAND_UI(ID_STATUS_PANE_ASSOCIATED_DOCSET, OnUpdateStatusBarAssociatedDocSet)
    ON_MESSAGE(WM_IMSA_SET_STATUSBAR_PANE, OnSetStatusBarFilePos)
    ON_UPDATE_COMMAND_UI(ID_STATUS_PANE_FILE_POS, OnUpdateStatusBar)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_OVR, OnUpdateKeyOvertype)

    // other handlers
    ON_MESSAGE(UWM::Designer::FindOpenTextSourceEditable, OnFindOpenTextSourceEditable)
    ON_MESSAGE(UWM::CSDocument::GetOpenTextSourceEditables, OnGetOpenTextSourceEditables)

    // interapp communication
    ON_MESSAGE(WM_IMSA_FILEOPEN, OnIMSAFileOpen)

END_MESSAGE_MAP()


namespace
{
    constexpr UINT StatusBarIndicators[] = 
    {
        ID_SEPARATOR,           // status line indicator
        ID_STATUS_PANE_ASSOCIATED_DOCSET,
        ID_STATUS_PANE_FILE_POS,
        ID_INDICATOR_CAPS,
        ID_INDICATOR_NUM,
        ID_INDICATOR_OVR,
    };
}


CMainFrame::CMainFrame(GlobalSettings global_settings)
    :   m_className(nullptr),
        m_globalSettings(std::move(global_settings))
{
}


BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if( !__super::PreCreateWindow(cs) )
        return FALSE;

    if( m_className == nullptr )
    {
        WNDCLASS wndcls;
        ::GetClassInfo(AfxGetInstanceHandle(), cs.lpszClass, &wndcls);
        wndcls.hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
        wndcls.lpszClassName = IMSA_WNDCLASS_CSDOCUMENT;
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

    // set up the tabs    
	CMDITabInfo mdiTabParams;
	mdiTabParams.m_style = CMFCTabCtrl::STYLE_3D_ONENOTE;   // other styles available...
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
        !add_toolbar(m_wndDocumentToolBar, IDR_TOOLBAR_DOC) )
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

    m_wndStatusBar.EnablePaneDoubleClick();


    // dock the menu and toolbars
    CDockingManager::SetDockingMode(DT_SMART);

    DockPane(&m_wndMenuBar);
	DockPane(&m_wndMainToolBar);
	DockPane(&m_wndDocumentToolBar);

    PostMessage(UWM::CSDocument::SyncToolbarAndWindows);

	// Switch the order of document name and application name on the window title bar. This
	// improves the usability of the taskbar because the document name is visible with the thumbnail.
	ModifyStyle(0, FWS_PREFIXTITLE);

	return 0;
}


void CMainFrame::OnActivateApp(BOOL bActive, DWORD dwThreadID)
{
    __super::OnActivateApp(bActive, dwThreadID);

    if( bActive )
    {
        CMDIChildWnd* active_wnd = MDIGetActive();

        if( active_wnd != nullptr )
            active_wnd->PostMessage(UWM::CSDocument::TextEditFrameActivate, static_cast<WPARAM>(-1));
    }
}


void CMainFrame::OnDestroy()
{
    std::optional<CRect> main_frame_rect;

    auto serialize_pane_size = [&](CDockablePane& dockable_pane, int (CRect::*width_or_height)() const, double& proportion)
    {
        if( !main_frame_rect.has_value() )
            GetWindowRect(main_frame_rect.emplace());

        int main_frame_width_or_height = ((*main_frame_rect).*width_or_height)();

        if( main_frame_width_or_height == 0 )
            return;

        CRect dockable_pane_rect;
        dockable_pane.GetWindowRect(dockable_pane_rect);

        proportion = (dockable_pane_rect.*width_or_height)() / static_cast<double>(main_frame_width_or_height);
    };

    if( m_buildWnd != nullptr )
        serialize_pane_size(*m_buildWnd, &CRect::Height, m_globalSettings.build_window_proportion);

    if( m_htmlOutputWnd != nullptr )
        serialize_pane_size(*m_htmlOutputWnd, &CRect::Width, m_globalSettings.html_window_proportion);

    m_globalSettings.Save(false);

    __super::OnDestroy();
}


void CMainFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
    __super::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

    if( !bSysMenu )
    {
        constexpr unsigned FirstBuildMenuItemId = ID_BUILD_PLACEHOLDER;
        constexpr unsigned FirstWindowsMenuItemId = ID_WINDOW_BUILD;

        const unsigned first_menu_item_id = pPopupMenu->GetMenuItemID(0);

        // the Build menu is always built dynamically
        if( first_menu_item_id == FirstBuildMenuItemId )
        {
            assert_cast<DocSetBuildHandlerFrame*>(GetActiveFrame())->PopulateBuildMenu(*pPopupMenu);
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


void CMainFrame::OnWindowDocument(UINT nID)
{
    WindowsMenuManager::ActivateDocument(*this, nID);
}


void CMainFrame::OnWindowWindows()
{
    WindowsMenuManager::ShowWindowsDocDlg(*this);
}


void CMainFrame::ShowDockablePane(CDockablePane& dockable_pane, BOOL visibility)
{
	SetFocus();
	ShowPane(&dockable_pane, visibility, FALSE, FALSE);
	RecalcLayout();
}


void CMainFrame::OnWindowDockablePane(UINT nID)
{
    auto need_to_create = [&](CDockablePane* dockable_pane)
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

    else
    {
        ASSERT(nID == ID_WINDOW_OUTPUT);

        if( need_to_create(m_htmlOutputWnd.get()) )
            GetHtmlOutputWnd();
    }
}


void CMainFrame::OnUpdateWindowDockablePane(CCmdUI* pCmdUI)
{
    const CDockablePane* dockable_pane = ( pCmdUI->m_nID == ID_WINDOW_BUILD )  ? static_cast<CDockablePane*>(m_buildWnd.get()) :
                                       /*( pCmdUI->m_nID == ID_WINDOW_OUTPUT )*/ static_cast<CDockablePane*>(m_htmlOutputWnd.get());

    pCmdUI->SetCheck(dockable_pane != nullptr && dockable_pane->IsVisible());
}


CSDocumentBuildWnd* CMainFrame::GetBuildWnd(bool make_visible_if_not/* = true*/)
{
    if( m_buildWnd != nullptr )
    {
        if( make_visible_if_not && !m_buildWnd->IsVisible() )
            ShowDockablePane(*m_buildWnd, TRUE);
    }

    else
    {
        m_buildWnd = std::make_unique<CSDocumentBuildWnd>();

        m_buildWnd->SetAddNewlinesBetweenErrorsAndWarnings(true);
        m_buildWnd->SetIndentMessageLinesAfterFirstLine(true);

        // the window will be docked, but if it is dragged out as a proper window, make sure the width and height are reasonable;
        // the height does dictate how large the window shows while docked (by default, at the bottom)
        constexpr int MinHeight = 50;

        CRect rect;
        GetWindowRect(rect);
        rect.right = rect.Width() * 3 / 4;
        rect.bottom = std::max(MinHeight, std::min(rect.Height(), static_cast<int>(rect.Height() * m_globalSettings.build_window_proportion)));

	    if( !m_buildWnd->Create(_T("Build"), this, rect, TRUE, ID_BUILD_WINDOW,
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


HtmlOutputWnd* CMainFrame::GetHtmlOutputWnd()
{
    if( m_htmlOutputWnd != nullptr )
    {
        if( !m_htmlOutputWnd->IsVisible() )
            ShowDockablePane(*m_htmlOutputWnd, TRUE);
    }

    else
    {
        m_htmlOutputWnd = std::make_unique<HtmlOutputWnd>();
        m_htmlOutputWnd->GetHtmlViewCtrl().AddWebEventObserver([&](const std::wstring& message) { OnWebMessageReceived(message); });

        // the window will be docked, but if it is dragged out as a proper window, make sure the width and height are reasonable;
        // the width does dictate how large the window shows while docked (by default, at the right)
        constexpr int MinWidth = 50;

        CRect rect;
        GetClientRect(rect);
        rect.right = std::max(MinWidth, std::min(rect.Width(), static_cast<int>(rect.Width() * m_globalSettings.html_window_proportion)));
        rect.bottom = rect.Height() * 8 / 10;

	    if( !m_htmlOutputWnd->Create(_T("Output"), this, rect, TRUE, ID_HTML_OUTPUT_WINDOW,
                                     WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI) )
	    {
		    return ReturnProgrammingError(nullptr);
	    }

	    m_htmlOutputWnd->EnableDocking(CBRS_ALIGN_ANY);
	    DockPane(m_htmlOutputWnd.get());
    }

    return m_htmlOutputWnd.get();
}


BOOL CMainFrame::OnShowMDITabContextMenu(CPoint point, DWORD dwAllowedItems, BOOL bTabDrop)
{
    if( bTabDrop )
        return __super::OnShowMDITabContextMenu(point, dwAllowedItems, bTabDrop);

    ASSERT(GetActiveDoc() != nullptr);

    CMenu menu;
    menu.LoadMenu(IDR_TAB_CONTEXT);
    ASSERT(menu.GetMenuItemCount() == 1);

    CMenu* submenu = menu.GetSubMenu(0);

    CMFCPopupMenu* popup_menu = new CMFCPopupMenu;
    popup_menu->SetAutoDestroy(FALSE);
    popup_menu->Create(this, point.x, point.y, submenu->GetSafeHmenu());

    return TRUE;    
}


TextEditDoc* CMainFrame::GetActiveDoc()
{
    CMDIChildWnd* active_wnd = MDIGetActive();
    return ( active_wnd != nullptr ) ? assert_cast<TextEditDoc*>(active_wnd->GetActiveDocument()) :
                                       nullptr;
}


DocSetSpec* CMainFrame::GetActiveDocSetSpec()
{
    TextEditDoc* text_edit_doc = GetActiveDoc();
    return ( text_edit_doc != nullptr ) ? text_edit_doc->GetAssociatedDocSetSpec() :
                                          nullptr;
}


TextEditView* CMainFrame::GetActiveTextEditView()
{
    CFrameWnd* active_frame = GetActiveFrame();

    if( active_frame != nullptr )
    {
        CView* active_view = active_frame->GetActiveView();

        if( active_view != nullptr && active_view->IsKindOf(RUNTIME_CLASS(TextEditView)) )
            return assert_cast<TextEditView*>(active_view);
    }

    return nullptr;
}


std::shared_ptr<DocSetSpec> CMainFrame::FindSharedDocSetSpec(std::variant<wstring_view, DocSetSpec*> filename_or_doc_set_spec_ptr, bool open_if_not_found)
{
    ASSERT(std::holds_alternative<wstring_view>(filename_or_doc_set_spec_ptr) || !open_if_not_found);

    std::shared_ptr<DocSetSpec> matched_doc_set_spec;

    // see if the spec is open in any other document (e.g., as a component or an associated Document Set)
    ForeachDoc<TextEditDoc>(
        [&](TextEditDoc& text_edit_doc)
        {
            DocSetSpec* doc_set_spec = text_edit_doc.GetAssociatedDocSetSpec();

            if( doc_set_spec != nullptr )
            {
                if( std::holds_alternative<wstring_view>(filename_or_doc_set_spec_ptr) ? SO::EqualsNoCase(doc_set_spec->GetFilename(), std::get<wstring_view>(filename_or_doc_set_spec_ptr)) :
                                                                                         ( doc_set_spec == std::get<DocSetSpec*>(filename_or_doc_set_spec_ptr) ) )
                {
                    matched_doc_set_spec = text_edit_doc.GetSharedAssociatedDocSetSpec();
                    return false;
                }
            }

            return true;
        });

    if( open_if_not_found && matched_doc_set_spec == nullptr )
    {
        ASSERT(PortableFunctions::FileIsRegular(std::wstring(std::get<wstring_view>(filename_or_doc_set_spec_ptr))));
        matched_doc_set_spec = std::make_shared<DocSetSpec>(std::get<wstring_view>(filename_or_doc_set_spec_ptr));
    }

    return matched_doc_set_spec;
}


bool CMainFrame::IsCSDocPartOfDocSet(DocSetSpec& doc_set_spec, const std::wstring& csdoc_filename, bool match_full_path/* = true*/)
{
    try
    {
        CompileDocSetSpecIfNecessary(doc_set_spec, DocSetCompiler::SpecCompilationType::DataForTree);

        if( match_full_path )
        {
            std::shared_ptr<const DocSetComponent> matched_doc_set_component = doc_set_spec.FindComponent(csdoc_filename, true);

            if( matched_doc_set_component != nullptr )
            {
                ASSERT(matched_doc_set_component->type == DocSetComponent::Type::Document);
                return true;
            }
        }

        else
        {
            const std::vector<std::shared_ptr<DocSetComponent>>* doc_set_components_with_name = doc_set_spec.FindDocument(csdoc_filename);

            if( doc_set_components_with_name != nullptr )
            {
                ASSERT(!doc_set_components_with_name->empty() && doc_set_components_with_name->front()->type == DocSetComponent::Type::Document);
                return true;
            }
        }
    }
    catch(...) { }

    return false;
}


void CMainFrame::CompileDocSetSpecIfNecessary(DocSetSpec& doc_set_spec, DocSetCompiler::SpecCompilationType spec_compilation_type,
                                              DocSetCompiler::ErrorIssuerType error_issuer/* = DocSetCompiler::SuppressErrors { }*/)
{
    try
    {
        DocSetCompiler doc_set_compiler(m_globalSettings, std::move(error_issuer));
        doc_set_compiler.CompileSpecIfNecessary(doc_set_spec, spec_compilation_type);
    }

    catch( const CSProException& exception )
    {
        throw CSProExceptionWithFilename(doc_set_spec.GetFilename(),
                                         _T("There were errors compiling the associated Document Set '%s': %s"),
                                         PortableFunctions::PathGetFilename(doc_set_spec.GetFilename()), exception.GetErrorMessage().c_str());
    }
}


LRESULT CMainFrame::OnSyncToolbarAndWindows(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    bool show_main_frame_toolbar = ( MDIGetActive() == nullptr );

    m_wndMainToolBar.ShowPane(show_main_frame_toolbar, TRUE, FALSE);
    m_wndDocumentToolBar.ShowPane(!show_main_frame_toolbar, FALSE, FALSE);

    // when no documents are open, hide the build and HTML output windows
    if( show_main_frame_toolbar )
    {
        for( CDockablePane* dockable_pane : std::initializer_list<CDockablePane*>{ m_buildWnd.get(), m_htmlOutputWnd.get() } )
        {
            if( dockable_pane != nullptr && dockable_pane->IsVisible() )
                ShowDockablePane(*dockable_pane, FALSE);
        }
    }

    return 1;
}


void CMainFrame::OnStatusBarAssociatedDocSetClick()
{
    // when double-clicking on the associated Document Set in the status bar, open the spec
    const DocSetSpec* doc_set_spec = GetActiveDocSetSpec();

    if( doc_set_spec != nullptr )
    {
        AfxGetApp()->OpenDocumentFile(doc_set_spec->GetFilename().c_str());
    }

    // otherwise, if a CSPro Document, bring up the Associate with Document Set dialog
    else
    {
        CFrameWnd* active_frame = GetActiveFrame();

        if( active_frame != nullptr && active_frame->IsKindOf(RUNTIME_CLASS(CSDocFrame)) )
            active_frame->PostMessage(WM_COMMAND, ID_ASSOCIATE_WITH_DOCSET);
    }
}


void CMainFrame::OnUpdateStatusBarAssociatedDocSet(CCmdUI* pCmdUI)
{
    TextEditDoc* text_edit_doc = GetActiveDoc();
    std::wstring pane_text;

    if( text_edit_doc != nullptr )
    {
        const DocSetSpec* doc_set_spec = text_edit_doc->GetAssociatedDocSetSpec();

        pane_text = _T("Document Set: ") + ( ( doc_set_spec == nullptr ) ? _T("<unassociated>") :
                                                                           doc_set_spec->GetTitleOrFilenameWithoutExtension() );
    }

    pCmdUI->SetText(pane_text.c_str());
}


LRESULT CMainFrame::OnSetStatusBarFilePos(WPARAM wParam, LPARAM /*lParam*/)
{
    constexpr int pane_index = 2;
    static_assert(StatusBarIndicators[pane_index] == ID_STATUS_PANE_FILE_POS);

    const TCHAR* text = reinterpret_cast<const TCHAR*>(wParam);

    m_wndStatusBar.SetPaneText(pane_index, text);

    return 1;
}


void CMainFrame::OnUpdateStatusBar(CCmdUI* pCmdUI)
{
    // enable the status bar file position indicator
    pCmdUI->Enable(TRUE);
}


void CMainFrame::OnUpdateKeyOvertype(CCmdUI* pCmdUI)
{
    TextEditView* text_edit_view = GetActiveTextEditView();

    pCmdUI->Enable(text_edit_view != nullptr &&
                   text_edit_view->GetLogicCtrl()->GetOvertype());
}


void CMainFrame::OnUpdateDocumentMustBeSavedToDisk(CCmdUI* pCmdUI)
{
    CDocument* active_doc = GetActiveDoc();

    pCmdUI->Enable(active_doc != nullptr &&
                   !active_doc->GetPathName().IsEmpty());
}


void CMainFrame::OnCopyFullPath()
{
    const CDocument* active_doc = GetActiveDoc();
    ASSERT(active_doc != nullptr);

    WinClipboard::PutText(this, active_doc->GetPathName());
}


void CMainFrame::OnCopyFilename()
{
    const CDocument* active_doc = GetActiveDoc();
    ASSERT(active_doc != nullptr);

    WinClipboard::PutText(this, PortableFunctions::PathGetFilename(active_doc->GetPathName()));
}


void CMainFrame::OnOpenContainingFolder()
{
    const CDocument* active_doc = GetActiveDoc();
    ASSERT(active_doc != nullptr);

    OpenContainingFolder(active_doc->GetPathName());
}


LRESULT CMainFrame::OnFindOpenTextSourceEditable(WPARAM wParam, LPARAM lParam)
{
    const std::wstring& filename = *reinterpret_cast<const std::wstring*>(wParam);
    std::shared_ptr<TextSourceEditable>& out_text_source = *reinterpret_cast<std::shared_ptr<TextSourceEditable>*>(lParam);
    ASSERT(out_text_source == nullptr);

    ForeachDoc(
        [&](CDocument& doc)
        {
            if( SO::EqualsNoCase(filename, doc.GetPathName()) )
            {
                out_text_source = assert_cast<TextEditDoc&>(doc).GetSharedTextSourceEditable();
                return false;
            }

            return true;
        });

    return ( out_text_source != nullptr ) ? 1 : 0;
}


LRESULT CMainFrame::OnGetOpenTextSourceEditables(WPARAM wParam, LPARAM /*lParam*/)
{
    std::map<StringNoCase, TextSourceEditable*>& text_sources_for_open_documents = *reinterpret_cast<std::map<StringNoCase, TextSourceEditable*>*>(wParam);

    ForeachDoc<TextEditDoc>(
        [&](TextEditDoc& doc)
        {
            if( !doc.GetPathName().IsEmpty() )
                text_sources_for_open_documents.try_emplace(CS2WS(doc.GetPathName()), doc.GetTextSource());

            return true;
        });

    return 1;
}


LRESULT CMainFrame::OnIMSAFileOpen(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    // this message is sent by another instance of CSDocument to open a file in an existing instance of CSDocument
    CString filenames_json;

    if( IMSAOpenSharedFile(filenames_json) )
    {
        try
        {
            const std::vector<std::wstring> filenames = Json::Parse(filenames_json).Get<std::vector<std::wstring>>();

            for( const std::wstring& filename : filenames )
                AfxGetApp()->OpenDocumentFile(filename.c_str(), TRUE);
        }
        catch(...) { ASSERT(false); }
    }

    return 0;
}


void CMainFrame::OnFileNewDocumentSet()
{
    DocSetSpecDoc::CreateNewDocument();
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



void CMainFrame::OnFileSaveAll()
{
    ForeachDoc(
        [&](CDocument& doc)
        {
            if( doc.IsModified() )
            {
                doc.DoFileSave();

                // stop processing if the document is still modified, which means
                // the user has canceled the saving operation
                return !doc.IsModified();
            }

            return true;
        });
}


void CMainFrame::OnUpdateFileSaveAll(CCmdUI* pCmdUI)
{
    bool a_document_is_modified = false;

    ForeachDoc(
        [&](CDocument& doc)
        {
            if( doc.IsModified() )
            {
                a_document_is_modified = true;
                return false;
            }

            return true;
        });

    pCmdUI->Enable(a_document_is_modified);
}


void CMainFrame::OnGlobalSettings()
{
    GlobalSettingsDlg global_settings_dlg(m_globalSettings, this);

    if( global_settings_dlg.DoModal() != IDOK )
        return;

    m_globalSettings = global_settings_dlg.ReleaseGlobalSettings();
    m_globalSettings.Save(true);
}

#include "StdAfx.h"
#include "MainFrm.h"
#include "CapiMacrosDlg.h"
#include "PropertiesDlg.h"
#include "SelectAppDlg.h"
#include <zToolsO/FileIO.h>
#include <zToolsO/NewlineSubstitutor.h>
#include <zToolsO/UWM.h>
#include <zUtilO/TreeCtrlHelpers.h>
#include <zUtilO/UWM.h>
#include <zUtilF/UIThreadRunner.h>
#include <zLogicO/ReservedWords.h>
#include <zLogicO/SourceBuffer.h>
#include <zInterfaceF/UWM.h>
#include <zDesignerF/CompilerOutputTabViewPage.h>
#include <zDesignerF/DesignerObjectTransporter.h>
#include <zDictF/UWM.h>
#include <zTableF/TabDoc.h>
#include <zTableF/TabView.h>
#include <zTableF/TabChWnd.h>
#include <zCapiO/QSFView.h>
#include <zCapiO/UWM.h>
#include <Zsrcmgro/DesignerApplicationLoader.h>
#include <Zsrcmgro/DesignerCapiLogicCompiler.h>
#include <Zsrcmgro/DesignerCompiler.h>
#include <zEngineF/EngineUI.h>


constexpr int DICTTOOLBARPOS = 1;
constexpr int TABTOOLBARPOS = 2;
constexpr int FORMTOOLBARPOS = 3;
constexpr int ORDERTOOLBARPOS = 4;
constexpr int LANGDBARPOS = 5;

static UINT toolbars[] =
{
    IDR_MAINFRAME,
    IDR_DICT_FRAME,
    IDR_TABLE_FRAME,
    IDR_FORM_FRAME,
    IDR_ORDER_FRAME
};



/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
    ON_WM_CREATE()
    ON_WM_MENUCHAR()
    ON_WM_CLOSE()
    ON_WM_ACTIVATEAPP()
    ON_COMMAND(ID_ABOUT1, OnAbout1)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_OVR, OnUpdateKeyOvr)
    ON_WM_ENDSESSION()
    ON_WM_DROPFILES()

    // Global help commands
    ON_COMMAND(ID_HELP_FINDER, CMDIFrameWnd::OnHelpFinder)
    ON_COMMAND(ID_HELP, CMDIFrameWnd::OnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, CMDIFrameWnd::OnContextHelp)
    ON_COMMAND(ID_DEFAULT_HELP, CMDIFrameWnd::OnHelpFinder)

    ON_MESSAGE(UWM::ToolsO::GetObjectTransporter, OnGetObjectTransporter)

    ON_MESSAGE(UWM::Designer::ShowToolbar, ShowToolBar)
    ON_MESSAGE(UWM::Designer::HideToolbar, HideToolBar)
    ON_MESSAGE(UWM::Designer::SelectTab, SelectTab)

    ON_MESSAGE(UWM::Dictionary::UpdateLanguageList, OnUpdateLanguageList)
    ON_MESSAGE(UWM::Dictionary::NameChange, OnDictNameChange)
    ON_MESSAGE(UWM::Dictionary::ValueLabelChange, OnDictValueLabelChange)
    ON_MESSAGE(UWM::Dictionary::GetApplicationPff, OnGetApplicationPff)

    ON_MESSAGE(UWM::Designer::GetDictionaryType, OnGetDictionaryType)
    ON_MESSAGE(UWM::Designer::GetMessageTextSource, OnGetMessageTextSource)
    ON_MESSAGE(UWM::Designer::GetApplicationBeingLoaded, OnGetApplicationBeingLoaded)
    ON_MESSAGE(UWM::Designer::GetApplication, OnGetApplication)
    ON_MESSAGE(UWM::Designer::GetFormFileOrDictionary, OnGetFormFileOrDictionary)

    ON_MESSAGE(UWM::Designer::CanCodeFileCompilationBeSkipped, OnCanCodeFileCompilationBeSkipped)
    ON_MESSAGE(UWM::Designer::SetCodeFileSuccessfullyCompiled, OnSetCodeFileSuccessfullyCompiled)

    ON_MESSAGE(UWM::Designer::TokenizeLogic_V0, OnTokenizeLogic_V0)
    ON_MESSAGE(UWM::Designer::CreateCapiLogicCompiler, OnCreateCapiLogicCompiler)

    ON_MESSAGE(UWM::UtilF::RunOnUIThread, OnRunOnUIThread)
    ON_MESSAGE(UWM::UtilF::GetApplicationShutdownRunner, OnGetApplicationShutdownRunner)

    ON_MESSAGE(UWM::Form::ShowSourceCode, ShowSrcCode)
    ON_MESSAGE(UWM::Form::PutSourceCode, UpdateSrcCode)
    ON_MESSAGE(UWM::Form::RunActiveApplication, OnLaunchActiveApp)
    ON_MESSAGE(UWM::Form::RunActiveApplicationAsBatch, OnLaunchActiveAppAsBch)
    ON_MESSAGE(UWM::Form::PublishApplication, OnGenerateBinary)
    ON_MESSAGE(UWM::Form::PublishAndDeployApplication, OnPublishAndDeploy)
    ON_MESSAGE(UWM::Form::HasLogic, OnFIsCode)
    ON_MESSAGE(UWM::Form::HasQuestionText, OnIsQuestion)
    ON_MESSAGE(UWM::Form::IsNameUnique, IsNameUnique)
    ON_MESSAGE(UWM::Form::UpdateStatusBar, OnFormUpdateStatusBar)
    ON_MESSAGE(UWM::Form::UpdateCapiLanguages, ProcessLangs)
    ON_MESSAGE(UWM::Form::ShowCapiText, OnShowCapiText)
    ON_MESSAGE(UWM::Form::CapiMacros, OnCapiMacros)

    ON_MESSAGE(UWM::Order::ShowSourceCode, ShowOSrcCode)
    ON_MESSAGE(UWM::Order::PutSourceCode, UpdateOSrcCode)
    ON_MESSAGE(UWM::Order::RunActiveApplication, OnRunBatch)
    ON_MESSAGE(UWM::Order::HasLogic, OnIsCode)

    ON_MESSAGE(UWM::Table::ShowSourceCode, ShowTblSrcCode)
    ON_MESSAGE(UWM::Table::PutSourceCode, UpdateTabSrcCode)
    ON_MESSAGE(UWM::Table::RunActiveApplication, OnRunTab)
    ON_MESSAGE(UWM::Table::IsNameUnique, IsTabNameUnique)
    ON_MESSAGE(UWM::Table::CheckSyntax, CheckSyntax4TableLogic)
    ON_MESSAGE(UWM::Table::ReplaceLevelProcForLevel, ReplaceLvlProc4Area)
    ON_MESSAGE(UWM::Table::PutTallyProc, PutTallyProc)
    ON_MESSAGE(UWM::Table::RenameProc, RenameProc)
    ON_MESSAGE(UWM::Table::ReconcileLinkObj, ReconcileLinkObj)
    ON_MESSAGE(UWM::Table::DeleteLogic, DeleteTblLogic)

    ON_MESSAGE(ZEDITO_SEL_CHANGE, OnSelChange)
    ON_MESSAGE(ZEDITO_LOGIC_REFERENCE, OnLogicReference)
    ON_MESSAGE(WM_IMSA_SYMBOLS_ADDED, OnSymbolsAdded)
    ON_COMMAND(ID_VIEW_TOP_LOGIC, OnViewTopLogic)
    ON_MESSAGE(ZEDITO_LOGIC_AUTO_COMPLETE, OnLogicAutoComplete)
    ON_MESSAGE(ZEDITO_LOGIC_INSERT_PROC_NAME, OnLogicInsertProcName)

    ON_MESSAGE(WM_IMSA_SET_STATUSBAR_PANE, SetStatusBarPane)
    ON_MESSAGE(WM_IMSA_SETFOCUS, OnIMSASetFocus)

    ON_MESSAGE(WM_IMSA_UPDATE_SYMBOLTBL, OnUpdateSymbolTblFlag)

    ON_MESSAGE(WM_IMSA_TABCONVERT,OnIMSATabConvert)

    ON_MESSAGE(WM_IMSA_RECONCILE_QSF_FIELD_NAME, OnReconcileQsfFieldName)
    ON_MESSAGE(WM_IMSA_RECONCILE_QSF_DICT_NAME, OnReconcileQsfDictName)

    ON_UPDATE_COMMAND_UI(ID_AREA_COMBO, OnUpdateAreaComboBox)
    ON_UPDATE_COMMAND_UI(ID_TAB_ZOOM_COMBO, OnUpdateZoomComboBox)

    ON_UPDATE_COMMAND_UI(ID_OPTIONS_APPLICATION_PROPERTIES, OnUpdateIfApplicationIsAvailable)
    ON_COMMAND(ID_OPTIONS_APPLICATION_PROPERTIES, OnOptionsProperties)
    ON_MESSAGE(UWM::CSPro::SetExternalApplicationProperties, OnSetExternalApplicationProperties)

    ON_MESSAGE(UWM::Designer::ShowFileProperties, OnShowFileProperties)

    ON_MESSAGE(UWM::UtilO::IsReservedWord, OnIsReservedWord)

    ON_MESSAGE(UWM::UtilF::CanAddResources, OnCanAddResources)
    ON_MESSAGE(UWM::UtilF::CopyToResourceDirectory, OnCopyToResourceDirectory)

    ON_MESSAGE(UWM::CSPro::UpdateApplicationExternalities, OnUpdateApplicationExternalities)
    ON_MESSAGE(UWM::Designer::FindOpenTextSourceEditable, OnFindOpenTextSourceEditable)

    ON_MESSAGE(UWM::Designer::GoToLogicError, OnGoToLogicError)

    ON_MESSAGE(UWM::Edit::GetLexerLanguage, OnGetLexerLanguage)

    ON_COMMAND(ID_VIEW_PREVIEW_TEXT_TEMPLATE, OnViewPreviewTextTemplate)
    ON_UPDATE_COMMAND_UI(ID_VIEW_PREVIEW_TEXT_TEMPLATE, OnUpdateViewPreviewTextTemplate)

    ON_MESSAGE(UWM::Designer::GetDesignerIcon, OnGetDesignerIcon)
    ON_MESSAGE(UWM::ToolsO::DisplayErrorMessage, OnDisplayErrorMessage)
    ON_MESSAGE(WM_IMSA_PORTABLE_ENGINEUI, OnEngineUI)

    ON_MESSAGE(UWM::Designer::RedrawPropertyGrid, OnRedrawPropertyGrid)

    ON_MESSAGE(UWM::Interface::SelectLanguage, OnSelectLanguage)
    ON_MESSAGE(UWM::Designer::GetCurrentLanguageName, OnGetCurrentLanguageName)
    ON_COMMAND(ID_CHANGE_LANGUAGE, OnChangeDictionaryLanguage)

    // Code Menu
    ON_COMMAND(ID_CODE_PASTE_STRING_LITERAL, OnPasteStringLiteral)
    ON_UPDATE_COMMAND_UI(ID_CODE_PASTE_STRING_LITERAL, OnUpdatePasteStringLiteral)

    ON_COMMAND(ID_CODE_STRING_ENCODER, OnStringEncoder)
    ON_COMMAND(ID_CODE_PATH_ADJUSTER, OnPathAdjuster)

    ON_COMMAND(ID_CODE_SYMBOL_ANALYSIS, OnSymbolAnalysis)
    ON_UPDATE_COMMAND_UI(ID_CODE_SYMBOL_ANALYSIS, OnUpdateIfLogicIsShowing)

    ON_COMMAND_RANGE(ID_CODE_DEPRECATION_WARNINGS_NONE, ID_CODE_DEPRECATION_WARNINGS_ALL, OnDeprecationWarnings)
    ON_UPDATE_COMMAND_UI_RANGE(ID_CODE_DEPRECATION_WARNINGS_NONE, ID_CODE_DEPRECATION_WARNINGS_ALL, OnUpdateDeprecationWarnings)

    ON_COMMAND_RANGE(ID_CODE_FOLDING_LEVEL_NONE, ID_CODE_FOLDING_LEVEL_ALL, OnCodeFoldingLevel)
    ON_UPDATE_COMMAND_UI_RANGE(ID_CODE_FOLDING_LEVEL_NONE, ID_CODE_FOLDING_LEVEL_ALL, OnUpdateCodeFoldingLevel)
    ON_COMMAND_RANGE(ID_CODE_FOLDING_FOLD_ALL, ID_CODE_FOLDING_TOGGLE, OnCodeFoldingAction)
    ON_UPDATE_COMMAND_UI_RANGE(ID_CODE_FOLDING_FOLD_ALL, ID_CODE_FOLDING_TOGGLE, OnUpdateCodeFoldingAction)

END_MESSAGE_MAP()


static UINT indicators[] =
{
    ID_SEPARATOR,               // status line indicator
    ID_STATUS_PANE_INDICATOR,   // other info
    ID_INDICATOR_CAPS,
    ID_INDICATOR_NUM,
    ID_INDICATOR_SCRL,
    ID_INDICATOR_OVR
};


/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
    :   m_pszClassName(nullptr),
        m_bDictToolbar(FALSE),
        m_bFormToolbar(FALSE),
        m_bOrderToolbar(FALSE),
        m_bTabToolbar(FALSE),
        m_eProcess(ALL_STUFF),
        m_updateViewsDocument(nullptr),
        m_bRemovingPossibleDuplicateProcs(false)
{
}


CMainFrame::~CMainFrame()
{
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if( CMDIFrameWnd::OnCreate(lpCreateStruct) == -1 )
        return -1;

    // Create CSPro tool bar
    m_pWndToolBar = std::make_unique<CToolBar>();

    if (!m_pWndToolBar->CreateEx(this,TBSTYLE_FLAT,WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP,CRect(0,0,0,0), AFX_IDW_TOOLBAR )||
        !m_pWndToolBar->LoadToolBar(IDR_MAINFRAME))
    {
        TRACE0("Failed to create CSPro toolbar\n");
        return -1;      // fail to create
    }

    m_pWndToolBarImages = Load24BitColorToolbarImages(m_pWndToolBar.get(), IDR_MAINFRAME);

    for( unsigned tools_id = ID_TOOLS_DATAMANAGER; tools_id <= ID_TOOLS_TEXTCONVERTER; ++tools_id )
        m_pWndToolBar->GetToolBarCtrl().HideButton(tools_id);

    // Create Dictionary tool bar
    m_pWndDictTBar = std::make_unique<CToolBar>();

    if (!m_pWndDictTBar->CreateEx(this,TBSTYLE_FLAT,WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP,CRect(0,0,0,0),996)||
        !m_pWndDictTBar->LoadToolBar(IDR_DICT_FRAME))
    {
        TRACE0("Failed to create dict toolbar\n");
        return -1;      // fail to create
    }

    // Create Tabulation tool bar
    m_pWndTabTBar = std::make_unique<CToolBar>();

    if (!m_pWndTabTBar->CreateEx(this,TBSTYLE_FLAT,WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP,CRect(0,0,0,0), 997)||
        !m_pWndTabTBar->LoadToolBar(IDR_TABLE_FRAME))
    {
        TRACE0("Failed to create table toolbar\n");
        return -1;      // fail to create
    }

    // only used in the viewer
    m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_QUICK_QUIT, TRUE);  // hide quick quit button

    // add area combo box to toolbar in place of ID_AREA_COMBO placeholder button
    int nIndex = m_pWndTabTBar->GetToolBarCtrl().CommandToIndex(ID_AREA_COMBO);
    CRect tbButtonRect;
    m_pWndTabTBar->GetToolBarCtrl().GetItemRect(nIndex, &tbButtonRect);
    CRect rect(tbButtonRect);
    const int iAreaComboDropHeight = 250;
    const int iAreaComboWidth = 150;
    const int iAreaComboLeftSpacing = 10; // offset from button to left
    rect.top = 0;
    rect.bottom = rect.top + iAreaComboDropHeight;
    rect.left += iAreaComboLeftSpacing;
    rect.right = rect.left + iAreaComboWidth;
    if(!m_tabAreaComboBox.Create(CBS_DROPDOWNLIST | WS_VISIBLE |
        WS_TABSTOP | WS_VSCROLL, rect, this, ID_AREA_COMBO))
    {
        TRACE(_T("Failed to create combo-box\n"));
        return FALSE;
    }
    m_tabAreaComboBox.SetParent(m_pWndTabTBar.get()); // parent is toolbar but messages get sent to CMainFrame

    // add zoom combo box to toolbar in place of ID_AREA_COMBO placeholder button
    // (this overlaps the area combo box but thats ok since we never show area
    // box in print view and never show zoom ouside printview).
    rect = tbButtonRect;
    const int iZoomComboDropHeight = 250;
    const int iZoomComboWidth = 75;
    const int iZoomComboLeftSpacing = 0; // offset from button to left
    rect.top = 0;
    rect.bottom = rect.top + iZoomComboDropHeight;
    rect.left += iZoomComboLeftSpacing;
    rect.right = rect.left + iZoomComboWidth;
    if(!m_tabZoomComboBox.Create(CBS_DROPDOWNLIST | WS_VISIBLE |
        WS_TABSTOP | WS_VSCROLL, rect, this, ID_TAB_ZOOM_COMBO))
    {
        TRACE(_T("Failed to create combo-box\n"));
        return FALSE;
    }
    m_tabZoomComboBox.SetParent(m_pWndTabTBar.get()); // parent is toolbar but messages get sent to CMainFrame

    // turn the placeholder toolbar button into a separator so that buttons
    // to the right of the combo box will not get covered up
    m_pWndTabTBar->SetButtonInfo(nIndex, ID_AREA_COMBO, TBBS_SEPARATOR, rect.Width()+iZoomComboLeftSpacing);

    // add close button to toolbar in place of ID_PRINTVIEW_CLOSE placeholder button
    nIndex = m_pWndTabTBar->GetToolBarCtrl().CommandToIndex(ID_PRINTVIEW_CLOSE);
    m_pWndTabTBar->GetToolBarCtrl().GetItemRect(nIndex, &rect);
    const int iPrintViewCloseHeight = tbButtonRect.Height();
    const int iPrintViewCloseWidth = 100;
    const int iPrintViewCloseSpacing = 8; // offset from button to left
    rect.top = 0;
    rect.bottom = rect.top + iPrintViewCloseHeight;
    rect.left += iPrintViewCloseSpacing;
    rect.right = rect.left + iPrintViewCloseWidth;
    m_pWndTabTBar->SetButtonInfo(nIndex, ID_PRINTVIEW_CLOSE, TBBS_SEPARATOR, rect.Width()+iZoomComboLeftSpacing);
    const TCHAR* sPreviewClose = _T("Close");
    if(!m_printViewCloseButton.Create(sPreviewClose, WS_CHILD | WS_VISIBLE, rect, this, ID_PRINTVIEW_CLOSE))
    {
        TRACE(_T("Failed to create button\n"));
        return FALSE;
    }
    m_printViewCloseButton.SetParent(m_pWndTabTBar.get()); // parent is toolbar but messages get sent to CMainFrame

    // Create Form tool bar
    m_pWndFormTBar = CFormChildWnd::CreateFormToolBar(this);

    if( m_pWndFormTBar == nullptr )
    {
        TRACE0("Failed to create form toolbar\n");
        return -1;      // fail to create
    }

    // Create Order tool bar
    m_pWndOrderTBar = std::make_unique<CToolBar>();

    if (!m_pWndOrderTBar->CreateEx(this,TBSTYLE_FLAT,WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP,CRect(0,0,0,0), 999)||
        !m_pWndOrderTBar->LoadToolBar(IDR_ORDER_FRAME))
    {
        TRACE0("Failed to create form toolbar\n");
        return -1;      // fail to create
    }

    m_pWndOrderTBarImages = Load24BitColorToolbarImages(m_pWndOrderTBar.get(), IDR_ORDER_FRAME);

    //Create language bar
    if (!m_wndLangDlgBar.Create(this, IDD_LANGDLGBAR, CBRS_ALIGN_TOP | CBRS_TOOLTIPS | CBRS_FLYBY, IDD_LANGDLGBAR))
    {
        TRACE0("Failed to create language dialogbar \n");
        return -1;      // fail to create
    }

    // Create rebar
    if( !m_wndReBar.Create(this) ||
        !m_wndReBar.AddBar(m_pWndToolBar.get()) ||
        !m_wndReBar.AddBar(m_pWndDictTBar.get()) ||
        !m_wndReBar.AddBar(m_pWndTabTBar.get()) ||
        !m_wndReBar.AddBar(m_pWndFormTBar.get()) ||
        !m_wndReBar.AddBar(m_pWndOrderTBar.get()) ||
        !m_wndReBar.AddBar(&m_wndLangDlgBar))
    {
        /*these ints are positions of the bars defined at the top of this file. if u add a bar make sure that u specify the positions corrrectly
            const int DICTTOOLBARPOS = 1;
            const int TABTOOLBARPOS = 2;
            const int FORMTOOLBARPOS = 3;
            const int ORDERTOOLBARPOS = 4;
            const int LANGDBARPOS = 5;
        */
        TRACE0("Failed to create rebar\n");
        return -1;      // fail to create
    }


    // Create CSPro status bar
    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators,
        sizeof(indicators)/sizeof(UINT)))
    {
        TRACE0("Failed to create status bar\n");
        return -1;      // fail to create
    }

    SendMessage(WM_IMSA_SET_STATUSBAR_PANE, NULL);

    // Set tool tips for tool bars
    m_pWndToolBar->SetBarStyle(m_pWndToolBar->GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
    m_pWndDictTBar->SetBarStyle(m_pWndDictTBar->GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
    m_pWndTabTBar->SetBarStyle(m_pWndTabTBar->GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
    m_pWndFormTBar->SetBarStyle(m_pWndFormTBar->GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
    m_pWndOrderTBar->SetBarStyle(m_pWndOrderTBar->GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);

    m_wndReBar.GetReBarCtrl().ShowBand(DICTTOOLBARPOS, FALSE);
    m_wndReBar.GetReBarCtrl().ShowBand(TABTOOLBARPOS, FALSE);
    m_wndReBar.GetReBarCtrl().ShowBand(FORMTOOLBARPOS, FALSE);
    m_wndReBar.GetReBarCtrl().ShowBand(ORDERTOOLBARPOS, FALSE);
    m_wndReBar.GetReBarCtrl().ShowBand(LANGDBARPOS, FALSE);

    EnableDocking(CBRS_ALIGN_ANY);

    // This is a sizeable dialog bar.. that includes gadget resizing
    m_SizeDlgBar.SetSizeDockStyle(/*SZBARF_STDMOUSECLICKS |*/ SZBARF_DLGAUTOSIZE | SZBARF_NOCLOSEBTN | SZBARF_NORESIZEBTN);  // BMD 07 Mar 2003
    if (!m_SizeDlgBar.Create(this, IDD_DIALOGBAR, CBRS_LEFT, ID_FIXEDDLGBAR) )
    {
        TRACE0("Failed to create dialog bar\n");
        return -1;
    }

    //Only doc on the left side
    m_SizeDlgBar.EnableDocking(CBRS_ALIGN_LEFT);
    DockControlBar(&m_SizeDlgBar,AFX_IDW_DOCKBAR_LEFT);


    // restore the previous window placement, or if none exists, show maximized
    CString csRect = AfxGetApp()->GetProfileString(_T("Settings"),_T("InitialPosition"));

    if( !csRect.IsEmpty() )
    {
        WINDOWPLACEMENT wndpl;
        wndpl.length = sizeof(WINDOWPLACEMENT);
        wndpl.flags = 0;
        wndpl.ptMaxPosition = CPoint(0,0);
        wndpl.ptMinPosition = CPoint(0,0);
        wndpl.showCmd = _ttoi((const TCHAR*)csRect);
        wndpl.rcNormalPosition.left = _ttoi((const TCHAR*)csRect + 7);
        wndpl.rcNormalPosition.top = _ttoi((const TCHAR*)csRect + 14);
        wndpl.rcNormalPosition.right = _ttoi((const TCHAR*)csRect + 21);
        wndpl.rcNormalPosition.bottom = _ttoi((const TCHAR*)csRect + 28);

        if( wndpl.showCmd == SW_NORMAL || wndpl.showCmd == SW_MAXIMIZE )
            SetWindowPlacement(&wndpl);
    }

    else
        ShowWindow(SW_MAXIMIZE);


    m_csWindowText = UTF8_TODO::GetCString(Versioning::GetVersionString(true));
    SetWindowText(m_csWindowText);

    return 0;
}


std::unique_ptr<CImageList> CMainFrame::Load24BitColorToolbarImages(CToolBar* const pToolBar, const UINT nIDResource)
{
    HINSTANCE hInst = AfxFindResourceHandle(MAKEINTRESOURCE(nIDResource), RT_BITMAP);
    HBITMAP hBitmap = (HBITMAP)::LoadImage(hInst, MAKEINTRESOURCE(nIDResource), IMAGE_BITMAP,
                                           0, 0, LR_CREATEDIBSECTION);
    CBitmap bm;
    bm.Attach(hBitmap);

    auto imageStorage = std::make_unique<CImageList>();
    imageStorage->Create(16, 16, ILC_COLOR24 | ILC_MASK, 1, 1);
    imageStorage->Add(&bm, RGB(192,192,192));
    pToolBar->GetToolBarCtrl().SetImageList(imageStorage.get());

    return imageStorage;
}


BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if( !CMDIFrameWnd::PreCreateWindow(cs) ) {
        return FALSE;
    }

    if (m_pszClassName == nullptr)  {
        WNDCLASS wndcls;
        ::GetClassInfo(AfxGetInstanceHandle(), cs.lpszClass, &wndcls);
        wndcls.hIcon = ((CCSProApp*)AfxGetApp())->m_hIcon;
        CString csClassName = ((CCSProApp*)AfxGetApp())->m_csWndClassName;
        wndcls.lpszClassName = csClassName;
        VERIFY(AfxRegisterClass(&wndcls));
        m_pszClassName = csClassName;
    }
    cs.lpszClass = m_pszClassName;

    return TRUE;
}


void CMainFrame::OnActivateApp(BOOL bActive, DWORD dwThreadID)
{
    COXMDIFrameWndSizeDock::OnActivateApp(bActive, dwThreadID);

    if( bActive )
        PostMessage(UWM::CSPro::UpdateApplicationExternalities);
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnClose()
{
    CMDIFrameWnd* pWnd = (CMDIFrameWnd*)MDIGetActive();
    if(pWnd && pWnd->IsKindOf(RUNTIME_CLASS(CDictChildWnd))){
        CDDDoc* pDoc = (CDDDoc*)pWnd->GetActiveDocument();
        if (pDoc->IsPrintPreview()) {
            CMDIFrameWnd::OnClose();
            return;
        }
        if(!pDoc->IsDocOK()) {
            return;
        }
    }

    if(!IsOKToClose())
        return;

    //We get here a chance to query for all the documents before closing
    CDocTemplate* pTemplate = nullptr;
    //Get the project template and close

    //Get the applications and close
    pTemplate = GetDocTemplate(L".ent;.xtb;.bch");
    if (pTemplate)
        pTemplate->CloseAllDocuments(FALSE);


    //Get the forms and close
    pTemplate = GetDocTemplate(UTF8_TODO::GetWide(FileExtensions::WithDot(FileExtensions::Form)));
    if(pTemplate)
        pTemplate->CloseAllDocuments(FALSE);

    //Get the orders and close
    pTemplate = GetDocTemplate(UTF8_TODO::GetWide(FileExtensions::WithDot(FileExtensions::Order)));
    if(pTemplate)
        pTemplate->CloseAllDocuments(FALSE);

    //Get the tables and close
    pTemplate = GetDocTemplate(UTF8_TODO::GetWide(FileExtensions::WithDot(FileExtensions::TableSpec)));
    if(pTemplate)
        pTemplate->CloseAllDocuments(FALSE);

    //Get the dictionaries and close
    pTemplate = GetDocTemplate(UTF8_TODO::GetWide(FileExtensions::WithDot(FileExtensions::Dictionary)));
    if(pTemplate)
        pTemplate->CloseAllDocuments(FALSE);

    // save the window placement information to the registry (if the window is displayed normally or maximized)
    WINDOWPLACEMENT wndpl;
    wndpl.length = sizeof(WINDOWPLACEMENT);

    if( GetWindowPlacement(&wndpl) != FALSE && ( wndpl.showCmd == SW_NORMAL || wndpl.showCmd == SW_MAXIMIZE ) )
    {
        CString csRect;
        csRect.Format(_T("%06d %06d %06d %06d %06d"),wndpl.showCmd,wndpl.rcNormalPosition.left,wndpl.rcNormalPosition.top,wndpl.rcNormalPosition.right,wndpl.rcNormalPosition.bottom);
        AfxGetApp()->WriteProfileString(_T("Settings"),_T("InitialPosition"),csRect);
    }

    CMDIFrameWnd::OnClose();
}



// 20110128 dragging items onto CSPro caused problems because the tree never got created
// some of this code copied from MFC's winfrm.cpp
void CMainFrame::OnDropFiles(HDROP hDropInfo)
{
    SetActiveWindow();
    UINT nFiles = ::DragQueryFile(hDropInfo, (UINT)-1, nullptr, 0);

    CCSProApp* pApp = (CCSProApp*)AfxGetApp();
    CDocument* pDoc = nullptr;

    ASSERT(pApp != nullptr);

    if( nFiles > 1 ) // only try to open one file
        nFiles = 1;

    for (UINT iFile = 0; iFile < nFiles; iFile++)
    {
        TCHAR szFileName[_MAX_PATH];
        ::DragQueryFile(hDropInfo, iFile, szFileName, _MAX_PATH);
        pDoc = pApp->OpenDocumentFile(szFileName);
    }

    ::DragFinish(hDropInfo);

    if( pDoc && pApp->UpdateViews(pDoc) )
    {
        CString csErr;
        pApp->Reconcile(pDoc,csErr,false,true);
    }
}


RAII::SetValueAndRestoreOnDestruction<CDocument*> CMainFrame::SetUpdateViewsDocument(CDocument* const document)
{
    return RAII::SetValueAndRestoreOnDestruction(m_updateViewsDocument, document);
}


CDocTemplate* CMainFrame::GetDocTemplate(const std::wstring_view extension_sv)
{
    const CCSProApp* const cspro_app = assert_cast<const CCSProApp*>(AfxGetApp());
    POSITION pos = cspro_app->GetFirstDocTemplatePosition();

    while( pos != nullptr )
    {
        CDocTemplate* const doc_template = cspro_app->GetNextDocTemplate(pos);
        CString doc_extension;

        doc_template->GetDocString(doc_extension, CDocTemplate::filterExt);

        if( SO::EqualsNoCase(doc_extension, extension_sv) )
            return doc_template;
    }

    return nullptr;
}


template<typename DocumentType, typename CF>
void CMainFrame::ForeachDocument(const CF& callback_function)
{
    const CDocTemplate* doc_template;

    if constexpr(std::is_same_v<DocumentType, CAplDoc>)
    {
        const CCSProApp* const cspro_app = assert_cast<const CCSProApp*>(AfxGetApp());
        doc_template = cspro_app->GetAppTemplate();
    }

    else
    {
        doc_template = GetDocTemplate(TC::ToWide(FileExtensions::WithDot(DocumentType::GetExtension())));

        if( doc_template == nullptr )
        {
            ASSERT(false);
            return;
        }
    }

    POSITION pos = doc_template->GetFirstDocPosition();

    while( pos != nullptr )
    {
        DocumentType* const document = assert_cast<DocumentType*>(doc_template->GetNextDoc(pos));

        if( !callback_function(*document) )
            return;
    }
}


template<typename DocumentType, typename CF>
void CMainFrame::ForeachDocumentUsingDictionary(const CDataDict& dictionary, const CF& callback_function)
{
    ForeachDocument<DocumentType>(
        [&](DocumentType& document)
        {
            if( document.GetSharedDictionary().get() == &dictionary )
                return callback_function(document);

            return true;
        });
}


template<typename CF>
void CMainFrame::ForeachApplicationDocumentUsingFormFile(const CDEFormFile& form_file, const CF& callback_function)
{
    ForeachDocument<CAplDoc>(
        [&](CAplDoc& application_doc)
        {
            for( const auto& this_form_file : application_doc.GetAppObject().GetRuntimeFormFiles() )
            {
                if( this_form_file.get() == &form_file )
                    return callback_function(application_doc);
            }

            return true;
        });
}


template<typename CF>
void CMainFrame::ForeachLogicAndReportTextSource(const CF& callback_function)
{
    ForeachDocument<CAplDoc>(
        [&](CAplDoc& application_doc)
        {
            Application& application = application_doc.GetAppObject();

            auto process_text_source = [&](auto text_source, const bool main_logic_file)
            {
                auto editable_text_source = std::dynamic_pointer_cast<TextSourceEditable, TextSource>(text_source);
                return ( editable_text_source != nullptr && !callback_function(editable_text_source, main_logic_file) );
            };

            // code files
            for( CodeFile& code_file : application.GetCodeFilesIterator() )
            {
                if( process_text_source(code_file.GetSharedTextSource(), code_file.IsLogicMain()) )
                    return false;
            }

            // reports
            for( ReportFile& report_file : application.GetReportFilesIterator() )
            {
                if( process_text_source(report_file.GetSharedTextSource(), false) )
                    return false;
            }

            return true;
        });
}


LRESULT CMainFrame::OnGetObjectTransporter(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if( m_objectTransporter == nullptr )
        m_objectTransporter = std::make_unique<DesignerObjectTransporter>();

    return reinterpret_cast<LRESULT>(m_objectTransporter.get());
}


LRESULT CMainFrame::ShowToolBar(WPARAM wParam, LPARAM /*lParam*/)
{
    FrameType frame_type = static_cast<FrameType>(wParam);

    m_bDictToolbar = ( frame_type == FrameType::Dictionary );
    m_bTabToolbar = ( frame_type == FrameType::Table );
    m_bFormToolbar = ( frame_type == FrameType::Form );
    m_bOrderToolbar = ( frame_type == FrameType::Order );

    m_wndReBar.GetReBarCtrl().ShowBand(0, FALSE);
    m_wndReBar.GetReBarCtrl().ShowBand(DICTTOOLBARPOS, m_bDictToolbar);
    m_wndReBar.GetReBarCtrl().ShowBand(TABTOOLBARPOS, m_bTabToolbar);
    m_wndReBar.GetReBarCtrl().ShowBand(FORMTOOLBARPOS, m_bFormToolbar);
    m_wndReBar.GetReBarCtrl().ShowBand(ORDERTOOLBARPOS, m_bOrderToolbar);

    // the language bar is only shown for dictionaries, forms, and orders
    bool show_language_bar = false;

    if( m_bDictToolbar || m_bFormToolbar || m_bOrderToolbar )
    {
        CMDIChildWnd* pWnd = MDIGetActive();

        const DictionaryBasedDoc* dictionary_based_doc = assert_cast<const DictionaryBasedDoc*>(pWnd->GetActiveDocument());
        const CDataDict* dictionary = dictionary_based_doc->GetSharedDictionary().get();

        // only show the language bar when more than one language is used
        if( dictionary != nullptr && dictionary->GetLanguages().size() > 1 )
        {
            show_language_bar = true;
            m_wndLangDlgBar.UpdateLanguageList(*dictionary);
        }
    }

    m_wndReBar.GetReBarCtrl().ShowBand(LANGDBARPOS, show_language_bar);

    return 0;
}

LRESULT CMainFrame::HideToolBar(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_bDictToolbar = FALSE;
    m_bTabToolbar = FALSE;
    m_bFormToolbar = FALSE;
    m_bOrderToolbar = FALSE;

    m_wndReBar.GetReBarCtrl().ShowBand(0, TRUE);
    m_wndReBar.GetReBarCtrl().ShowBand(DICTTOOLBARPOS, FALSE);
    m_wndReBar.GetReBarCtrl().ShowBand(TABTOOLBARPOS, FALSE);
    m_wndReBar.GetReBarCtrl().ShowBand(FORMTOOLBARPOS, FALSE);
    m_wndReBar.GetReBarCtrl().ShowBand(ORDERTOOLBARPOS, FALSE);
    m_wndReBar.GetReBarCtrl().ShowBand(LANGDBARPOS, FALSE);

    return 0;
}


LRESULT CMainFrame::OnUpdateLanguageList(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    FrameType frame_type = CMainFrame::GetFrameType();

    // the language bar is only shown for forms and dictionaries
    if( frame_type == FrameType::Dictionary || frame_type == FrameType::Form )
        SendMessage(UWM::Designer::ShowToolbar, static_cast<WPARAM>(frame_type));

    return 1;
}


LRESULT CMainFrame::OnSelectLanguage(WPARAM wParam, LPARAM /*lParam*/)
{
    CMDIChildWnd* pWnd = MDIGetActive();
    DictionaryBasedDoc* dictionary_based_doc = assert_cast<DictionaryBasedDoc*>(pWnd->GetActiveDocument());
    const CDataDict* dictionary = dictionary_based_doc->GetSharedDictionary().get();
    ASSERT(dictionary != nullptr);

    // set the dictionary language to the new selection
    dictionary->SetCurrentLanguage(wParam);

    // redraw the dictionary tree
    m_SizeDlgBar.m_DictTree.Invalidate();

    // notify all views that the language changed
    dictionary_based_doc->UpdateAllViews(nullptr, Hint::LanguageChanged);

    // for orders and forms, the dictionary must also be updated
    auto update_dictionary_view = [&](const std::string& dictionary_file_path)
    {
        DictionaryDictTreeNode* const dictionary_dict_tree_node = m_SizeDlgBar.m_DictTree.GetDictionaryTreeNode(dictionary_file_path);

        if( dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDocument() != nullptr )
            dictionary_dict_tree_node->GetDocument()->UpdateAllViews(nullptr, Hint::LanguageChanged);
    };

    // orders
    if( dictionary_based_doc->IsKindOf(RUNTIME_CLASS(COrderDoc)) )
    {
        COrderDoc* pOrderDoc = assert_cast<COrderDoc*>(dictionary_based_doc);
        CDEFormFile& order_file = pOrderDoc->GetFormFile();

        update_dictionary_view(UTF8_TODO::GetUtf8(order_file.GetDictionaryFilename()));
    }

    // forms
    else if( dictionary_based_doc->IsKindOf(RUNTIME_CLASS(CFormDoc)) )
    {
        CFormDoc* pFormDoc = assert_cast<CFormDoc*>(dictionary_based_doc);
        CDEFormFile& form_file = pFormDoc->GetFormFile();

        update_dictionary_view(UTF8_TODO::GetUtf8(form_file.GetDictionaryFilename()));

        // update the form itself (code copied from an old implementation)
        CFormScrollView* pFormScrollView = dynamic_cast<CFormScrollView*>(pWnd->GetActiveView());

        if( pFormScrollView != nullptr )
        {
            m_SizeDlgBar.m_FormTree.SetRedraw(FALSE);                       // BMD 09 Sep 2004
            form_file.RefreshAssociatedFieldText();
            pFormScrollView->RefreshGridOccLabelStubs();
            m_SizeDlgBar.m_FormTree.SetRedraw(TRUE);                        // BMD 09 Sep 2004
            pFormScrollView->RemoveAllGrids();                              // trash any grids that were created in this view
            pFormScrollView->RemoveAllTrackers();
            pFormScrollView->RecreateGrids(pFormDoc->GetCurFormIndex());    // and recreate the ones needed for this view
            pFormScrollView->SetPointers(pFormDoc->GetCurFormIndex());
            pFormScrollView->Invalidate();                                  // then i need to refresh the view (for grids to work)
            pFormScrollView->SendMessage(WM_PAINT);
        }

        // update the question text
        WindowsDesktopMessage::Send(UWM::Form::ShowCapiText, dictionary_based_doc);
    }

    return 1;
}


LRESULT CMainFrame::OnGetCurrentLanguageName(const WPARAM wParam, const LPARAM lParam)
{
    const CDocument& document = *reinterpret_cast<CDocument*>(wParam);
    std::string& language_name = *reinterpret_cast<std::string*>(lParam);

    ASSERT(document.IsKindOf(RUNTIME_CLASS(DictionaryBasedDoc)));

    language_name = assert_cast<const DictionaryBasedDoc&>(document).GetSharedDictionary()->GetCurrentLanguage().GetName();

    return 1;
}


void CMainFrame::OnChangeDictionaryLanguage()
{
    if( m_wndLangDlgBar.IsVisible() )
        m_wndLangDlgBar.SelectNextLanguage();
}


LRESULT CMainFrame::SelectTab(WPARAM wParam, LPARAM /*lParam*/)
{
    FrameType frame_type = static_cast<FrameType>(wParam);
    CString tab_name = ( frame_type == FrameType::Dictionary ) ? DICT_TAB_LABEL :
                       ( frame_type == FrameType::Form )       ? FORM_TAB_LABEL :
                       ( frame_type == FrameType::Order )      ? ORDER_TAB_LABEL :
                       ( frame_type == FrameType::Table )      ? TABLE_TAB_LABEL :
                                                                 CString();
    ASSERT(!tab_name.IsEmpty());

    m_SizeDlgBar.SelectTab(tab_name, 0);

    return 0;
}


FrameType CMainFrame::GetFrameType(CWnd* pWnd/* = nullptr*/)
{
    if( pWnd == nullptr )
        pWnd = assert_cast<CMainFrame*>(AfxGetMainWnd())->MDIGetActive();

    return pWnd->IsKindOf(RUNTIME_CLASS(CDictChildWnd))  ? FrameType::Dictionary :
           pWnd->IsKindOf(RUNTIME_CLASS(CFormChildWnd))  ? FrameType::Form :
           pWnd->IsKindOf(RUNTIME_CLASS(COrderChildWnd)) ? FrameType::Order:
           pWnd->IsKindOf(RUNTIME_CLASS(CTableChildWnd)) ? FrameType::Table :
                                                           throw ProgrammingErrorException();
}


/////////////////////////////////////////////////////////////////////////////
//
//                      CMainFrame::OnFormUpdateStatusBar
//
/////////////////////////////////////////////////////////////////////////////

LRESULT CMainFrame::OnFormUpdateStatusBar(WPARAM /*wParam*/, LPARAM lParam)
{
    const TCHAR* status_text = (const TCHAR*)lParam;   // get string from lParam
    m_wndStatusBar.SetWindowText(status_text);
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//                      CMainFrame::OnLaunchActiveApp
//
/////////////////////////////////////////////////////////////////////////////

LRESULT CMainFrame::OnLaunchActiveApp(WPARAM /*wParam*/, LPARAM lParam)
{
    bool bShiftPressed = GetKeyState(VK_SHIFT) < 0; // 20120510 allow the user to bypass the file associations screen, if possible, by holding down shift

    CFormDoc* pForm = (CFormDoc*)lParam;

    //Check Applications which has this form as the main one if
    //there are more than one ask the user for which application to
    //run .If there is only one proceed with it .

    CWnd* pPrevInstance = CWnd::FindWindow(CSPRO_WNDCLASS_ENTRYFRM, nullptr);
    if(pPrevInstance) {
        AfxMessageBox(_T("CSEntry is already running. Please close it before you launch another instance."));
        return 0;
    }

    CAplDoc* const pDoc = GetApplicationUsingFormFile(TC::ToUtf8(pForm->GetPathName()));

    if(pDoc) {
        if (pDoc->AreAplDictsOK()) {            // BMD  28 Jun 00
            if(pDoc->IsAppModified()) {
                //If application is modified set ask the user to save
                CString sMsg;
                sMsg.FormatMessage(IDS_APPMODIFIED, pDoc->GetPathName().GetString());
                if(AfxMessageBox(sMsg,MB_YESNO) != IDYES) {
                    return 0;
                }
                else {
                    pDoc->OnSaveDocument(pDoc->GetPathName());
                }
            }

            if(!CompileAll(pForm)) {
                return 0;
            }

            CString filename_to_run = pDoc->GetPathName();

            if( bShiftPressed )
            {
                CString pff_filename = PortableFunctions::PathRemoveFileExtensionCS(filename_to_run) + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Pff));

                if( PortableFunctions::FileIsRegular(pff_filename) )
                    filename_to_run = pff_filename;
            }

            CSProExecutables::RunProgramOpeningFile(CSProExecutables::Program::CSEntry, CS2WS(filename_to_run));
        }
    }

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//                      CMainFrame::OnGenerateBinary
//
/////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnGenerateBinary(WPARAM /*wParam*/, LPARAM lParam)
{
    CFormDoc* pForm = (CFormDoc*)lParam;

    CAplDoc* const pDoc = GetApplicationUsingFormFile(TC::ToUtf8(pForm->GetPathName()));

    if(pDoc) {
        if (pDoc->AreAplDictsOK()) {            // BMD  28 Jun 00
            if(pDoc->IsAppModified()) {
                //If application is modified set ask the user to save
                CString sMsg;
                sMsg.FormatMessage(IDS_APPMODIFIED, pDoc->GetPathName().GetString());
                if(AfxMessageBox(sMsg,MB_YESNO) != IDYES) {
                    return 0;
                }
                else {
                    pDoc->OnSaveDocument(pDoc->GetPathName());
                    //Added by Savy (R) 20090618
                    //To delete the existing .enc file
                    const CString sBinFileName = UTF8_TODO::GetCString(PortableFunctions::PathReplaceFileExtension(UTF8_TODO::GetUtf8(pDoc->GetPathName()), FileExtensions::BinaryEntryPen));
                    if (PortableFunctions::FileExists(sBinFileName)) {
                        DeleteFile(sBinFileName); // 20140311 deleting the file instead of recycling it
                    }
                }
            }

            if (!CompileAll(pForm)) {
                return 0;
            }

            CString sBinName = UTF8_TODO::GetCString(PortableFunctions::PathReplaceFileExtension(UTF8_TODO::GetUtf8(pDoc->GetPathName()), FileExtensions::BinaryEntryPen));

            CFileDialog dlgFile(FALSE,
                                UTF8_TODO::GetWide(FileExtensions::BinaryEntryPen).c_str(),
                                sBinName,
                                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                                _T("Portable CSPro Applications (*.pen)|*.pen||"),
                                this);

            if (dlgFile.DoModal() == IDCANCEL) {
                return FALSE;
            }

            sBinName = dlgFile.GetPathName();

            // delete existing .pen file first if it exists
            PortableFunctions::FileDelete(sBinName);

            const std::optional<std::string> csentry_exe = CSProExecutables::GetExecutablePath(CSProExecutables::Program::CSEntry);

            if( !csentry_exe.has_value() )
                return 0;

            CString command_line = FormatText<CString>(L"\"%s\" \"%s\" /pen /binaryName \"%s\"",
                                                       UTF8_TODO::GetWide(*csentry_exe).c_str(),
                                                       pDoc->GetPathName().GetString(),
                                                       sBinName.GetString());

            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            ZeroMemory( &si, sizeof(si) );
            si.cb = sizeof(si);
            ZeroMemory( &pi, sizeof(pi) );

            BOOL bRes = ::CreateProcess(UTF8_TODO::GetWide(*csentry_exe).c_str(),     // app name
                                        command_line.GetBuffer(), // command line
                                        nullptr,                  // Process handle not inheritable
                                        nullptr,                  // Thread handle not inheritable
                                        FALSE,                    // Set handle inheritance to FALSE
                                        0,                        // No creation flags
                                        nullptr,                  // Use parent's environment block
                                        PortableFunctions::PathGetDirectory(pDoc->GetPathName()).c_str(), // Use parent's starting directory
                                        &si,                      // Pointer to STARTUPINFO structure
                                        &pi );                    // Pointer to PROCESS_INFORMATION structure

            if (bRes) {
                // wait for it to complete
                CWaitCursor wait;
                WaitForSingleObject(pi.hProcess, INFINITE);

                // Close process and thread handles.
                CloseHandle( pi.hProcess );
                CloseHandle( pi.hThread );
            }
            else {
                AfxMessageBox(_T("Error: Unable to launch binary file generator.  Check that CSPro is correctly installed on this computer"));
            }
        }
    }

    // todo: check that .enc was generated, show error if not

    return 0;

}

/////////////////////////////////////////////////////////////////////////////
//
//                      CMainFrame::OnPublishAndDeploy
//
/////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnPublishAndDeploy(WPARAM /*wParam*/, LPARAM lParam)
{
    CFormDoc* pForm = (CFormDoc*)lParam;

    CAplDoc* const pDoc = GetApplicationUsingFormFile(TC::ToUtf8(pForm->GetPathName()));

    if (pDoc) {
        CString pffPath = PortableFunctions::PathRemoveFileExtensionCS(pDoc->GetPathName()) + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Pff));
        if (!PortableFunctions::FileExists(pffPath)) {
            AfxMessageBox(_T("Cannot deploy application without a program information file (.pff) file. Please run the application once to create the program information file."));
            return 0;
        }

        if (pDoc->AreAplDictsOK()) {            // BMD  28 Jun 00
            if (pDoc->IsAppModified()) {
                //If application is modified set ask the user to save
                CString sMsg;
                sMsg.FormatMessage(IDS_APPMODIFIED, pDoc->GetPathName().GetString());
                if (AfxMessageBox(sMsg, MB_YESNO) != IDYES) {
                    return 0;
                }
                else {
                    pDoc->OnSaveDocument(pDoc->GetPathName());
                    //Added by Savy (R) 20090618
                    //To delete the existing .enc file
                    const CString sBinFileName = UTF8_TODO::GetCString(PortableFunctions::PathReplaceFileExtension(UTF8_TODO::GetUtf8(pDoc->GetPathName()), FileExtensions::BinaryEntryPen));
                    if (PortableFunctions::FileExists(sBinFileName)) {
                        DeleteFile(sBinFileName); // 20140311 deleting the file instead of recycling it
                    }
                }
            }

            if (!CompileAll(pForm)) {
                return 0;
            }

            // Check to see if the CSDeploy window we launched last time we did publish
            // and deploy exists and if so bring it to the front instead of launching a new one
            if (pDoc->m_deployWnd && IsWindow(pDoc->m_deployWnd)) {
                if (::IsIconic(pDoc->m_deployWnd))
                    ::ShowWindow(pDoc->m_deployWnd, SW_RESTORE);
                ::SetForegroundWindow(pDoc->m_deployWnd);
                return 0;
            }

            const std::optional<std::string> csdeploy_exe = CSProExecutables::GetExecutablePath(CSProExecutables::Program::CSDeploy);

            if( !csdeploy_exe.has_value() )
                return 0;

            CString command_line = FormatText<CString>(L"\"%s\" \"%s\"", UTF8_TODO::GetWide(*csdeploy_exe).c_str(), pffPath.GetString());

            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));

            BOOL bRes = ::CreateProcess(UTF8_TODO::GetWide(*csdeploy_exe).c_str(), // app name
                command_line.GetBuffer(),                      // command line
                nullptr,                                       // Process handle not inheritable
                nullptr,                                       // Thread handle not inheritable
                FALSE,                                         // Set handle inheritance to FALSE
                0,                                             // No creation flags
                nullptr,                                       // Use parent's environment block
                PortableFunctions::PathGetDirectory(pDoc->GetPathName()).c_str(), // Use parent's starting directory
                &si,                                           // Pointer to STARTUPINFO structure
                &pi);                                          // Pointer to PROCESS_INFORMATION structure

            if (bRes) {

                // Save window handle so next time we can use it to bring window to front
                // instead of starting new one
                if (WaitForInputIdle(pi.hProcess, 5000) == 0) {

                    // Sometimes the Window has not yet been created so try a few
                    // times
                    HWND deploy_window = GetThreadMainWindow(pi.dwThreadId);
                    int attempts = 0;
                    while (!deploy_window && ++attempts < 10) {
                        Sleep(500);
                        deploy_window = GetThreadMainWindow(pi.dwThreadId);
                    }

                    if (deploy_window)
                        pDoc->m_deployWnd = deploy_window;
                }

                // Close process and thread handles.
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
            else {
                AfxMessageBox(_T("Error: Unable to launch deployment tool.  Check that CSPro is correctly installed on this computer"));
            }
        }
    }

    return 0;

}

/////////////////////////////////////////////////////////////////////////////
//
//                      CMainFrame::IsOKToClose
//
/////////////////////////////////////////////////////////////////////////////

BOOL CMainFrame::IsOKToClose(){

    BOOL bOK = TRUE;
    CObjTreeCtrl& ObjTree = m_SizeDlgBar.m_ObjTree;
    HTREEITEM hItem = ObjTree.GetRootItem();

    while(hItem) {
        FileTreeNode* const file_tree_node = ObjTree.GetFileTreeNode(hItem);
        ASSERT(file_tree_node);
        bool bProcess = false;
        CDocument* const pDoc = file_tree_node->GetDocument();
        if(pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CAplDoc))) {
            CAplDoc* pApl = (CAplDoc*)pDoc;
            if(pApl->IsAppModified()) {
                CString sMsg;
                sMsg.FormatMessage(IDS_APPMODIFIED, pApl->GetPathName().GetString());
                int iRet = AfxMessageBox(sMsg,MB_YESNOCANCEL);
                if(iRet == IDYES) {
                    pApl->OnSaveDocument(pApl->GetPathName());
                }
                else if(iRet == IDCANCEL) {
                    return FALSE;
                }

            }
            bProcess = false;
        }
        else if(pDoc && pDoc->IsKindOf(RUNTIME_CLASS(COrderDoc))) {
            COrderDoc* pOrderDoc = assert_cast<COrderDoc*>(pDoc);
            if(pOrderDoc->IsOrderModified()){
                pOrderDoc->SetModifiedFlag(TRUE);
                bProcess = true;
            }
        }
        else if(pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CFormDoc))) {
            CFormDoc* pFormDoc = assert_cast<CFormDoc*>(pDoc);
            if(pFormDoc->IsFormModified()){
                pFormDoc->SetModifiedFlag(TRUE);
                bProcess = true;
            }
        }
        else if(pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CTabulateDoc))){
            CTabulateDoc* pTabDoc = (CTabulateDoc*)pDoc;
            if(pTabDoc->IsTabModified()){
                pTabDoc->SetModifiedFlag(TRUE);
                bProcess = true;
            }
        }
        else {
            bProcess = true;
        }
        if(bProcess && (pDoc && pDoc->IsModified())){   // BMD 02 Mar 2003
            CString sMsg = pDoc->GetPathName();
            sMsg += _T(" is modified. Do you want to save it?");
            int iRet = AfxMessageBox(sMsg,MB_YESNOCANCEL);
            if(iRet == IDYES) {
                pDoc->OnSaveDocument(pDoc->GetPathName());
            }
            else if(iRet == IDCANCEL) {
                return FALSE;
            }
        }
        hItem = ObjTree.GetNextSiblingItem(hItem);
    }
    return bOK;
}


CAplDoc* CMainFrame::GetApplicationUsingFormFile(const std::string& form_file_path, const bool silent/* = false*/)
{
    std::vector<CAplDoc*> application_docs;

    ForeachDocument<CAplDoc>(
        [&](CAplDoc& application_doc)
        {
            for( const std::string& this_form_file_path : application_doc.GetAppObject().GetFormFilePaths() )
            {
                if( SO::EqualsNoCase(form_file_path, this_form_file_path) )
                {
                    application_docs.emplace_back(&application_doc);
                    break;
                }
            }

            return true;
        });

    if( application_docs.empty() )
    {
        if( !silent )
            ErrorMessage::Display("The file does not belong to any application: " + form_file_path);
    }

    else if( application_docs.size() == 1 )
    {
        return application_docs.front();
    }

    else
    {
        // query for the application to work with
        SelectAppDlg select_app_dlg(application_docs);

        if( select_app_dlg.DoModal() == IDOK )
            return select_app_dlg.GetSelectedApplicaton();
    }

    return nullptr;
}


LRESULT CMainFrame::ShowSrcCode(WPARAM /*wParam*/, LPARAM lParam)
{
    CFormDoc* pFormDoc = (CFormDoc*)lParam;
    ASSERT(pFormDoc);

    CAplDoc* pDoc = ProcessFOForSrcCode(*pFormDoc);

    if (pDoc) {
        SetSourceCode(pDoc);
    }
    else {
        AfxMessageBox(_T("No Application associated with this form file"));
        return -1;
    }

    return 0;
}


LRESULT CMainFrame::ShowOSrcCode(WPARAM /*wParam*/, LPARAM lParam)
{
    //This is for the Order file
    COrderDoc* pOrderDoc = (COrderDoc*)lParam;
    ASSERT(pOrderDoc);

    CAplDoc* pDoc = ProcessFOForSrcCode(*pOrderDoc); // process the order doc

    if(!pDoc)
        return TRUE;

    POSITION pos = pOrderDoc->GetFirstViewPosition();
    CView* pFormView = pOrderDoc->GetNextView(pos);
    ASSERT(pFormView);
    UNREFERENCED_PARAMETER(pFormView);

    if (pDoc) {
        SetOSourceCode(pDoc);//Set the OSource code
    }
    else {
        //         AfxMessageBox("No Application associated with this order file");
    }

    return 0;
}


CAplDoc* CMainFrame::ProcessFOForSrcCode(CDocument& document)
{
    // returns the application which has this form/order/table spec
    CAplDoc* found_application_document = nullptr;

    // forms
    if( document.IsKindOf(RUNTIME_CLASS(CFormDoc)) )
    {
        CFormDoc& form_doc = assert_cast<CFormDoc&>(document);
        CView* pView = form_doc.GetView();
        CFormChildWnd* pFormChildWnd = ( pView != nullptr ) ? DYNAMIC_DOWNCAST(CFormChildWnd, pView->GetParentFrame()) : nullptr;

        if( pFormChildWnd != nullptr )
        {
            ForeachApplicationDocumentUsingFormFile(form_doc.GetFormFile(),
                [&](CAplDoc& application_document)
                {
                    if( SO::EqualsNoCase(pFormChildWnd->GetApplicationName(), application_document.GetPathName()) )
                    {
                        found_application_document = &application_document;
                        return false;
                    }

                    return true;
                });
        }
    }


    // orders
    else if( document.IsKindOf(RUNTIME_CLASS(COrderDoc)) )
    {
        COrderDoc& order_doc = assert_cast<COrderDoc&>(document);
        POSITION pos = order_doc.GetFirstViewPosition();

        if( pos != nullptr )
        {
            CView* pView = order_doc.GetNextView(pos);
            COrderChildWnd* pOrderChildWnd = DYNAMIC_DOWNCAST(COrderChildWnd, pView->GetParentFrame());
            ASSERT(pOrderChildWnd != nullptr);

            ForeachApplicationDocumentUsingFormFile(order_doc.GetFormFile(),
                [&](CAplDoc& application_document)
                {
                    if( SO::EqualsNoCase(pOrderChildWnd->GetApplicationName(), application_document.GetPathName()) )
                    {
                        found_application_document = &application_document;
                        return false;
                    }

                    return true;
                });
        }
    }


    // tables
    else if( document.IsKindOf(RUNTIME_CLASS(CTabulateDoc)) )
    {
        CTabulateDoc& tab_doc = assert_cast<CTabulateDoc&>(document);

        ForeachDocument<CAplDoc>(
            [&](CAplDoc& application_document)
            {
                for( const std::string& table_spec_file_path : application_document.GetAppObject().GetTableSpecFilePaths() )
                {
                    if( SO::EqualsNoCase(tab_doc.GetPathName(), table_spec_file_path) )
                    {
                        //&&& SAVY fix this later for multiple apps using the same .xts file ?? is it possible?
                        found_application_document = &application_document;
                        return false;
                    }
                }

                return true;
            });
    }


    return found_application_document;
}


template<typename T>
T* CMainFrame::GetNodeIdForSourceCode(T* pNodeId/* = nullptr*/)
{
    const CTreeCtrl& tree = constexpr(std::is_same_v<T, CFormID>) ? static_cast<const CTreeCtrl&>(m_SizeDlgBar.m_FormTree) :
                                                                    static_cast<const CTreeCtrl&>(m_SizeDlgBar.m_OrderTree);

    auto get_node_id = [&](HTREEITEM hItem)
    {
        ASSERT(hItem != nullptr);

        pNodeId = reinterpret_cast<T*>(tree.GetItemData(hItem));
        ASSERT(pNodeId != nullptr);
    };

    if( pNodeId == nullptr )
        get_node_id(tree.GetSelectedItem());


    bool external_logic;
    bool header;

    if constexpr(std::is_same_v<T, CFormID>)
    {
        external_logic = ( pNodeId->GetItemType() == eFFT_EXTERNALCODE );
        header = ( pNodeId->GetTextSource() == nullptr );
    }

    else
    {
        header = pNodeId->IsHeader(AppFileType::Code);
        external_logic = ( header || pNodeId->GetAppFileType() == AppFileType::Code );
    }

    if( external_logic )
    {
        // if clicking on the header, show the entire source code
        if( header )
            get_node_id(tree.GetRootItem());
    }

    else
    {
        bool report;

        if constexpr(std::is_same_v<T, CFormID>)
        {
            report = ( pNodeId->GetItemType() == eFFT_REPORT );
            header = ( pNodeId->GetTextSource() == nullptr );
        }

        else
        {
            header = pNodeId->IsHeader(AppFileType::Report);
            report = ( header || pNodeId->GetAppFileType() == AppFileType::Report );
        }

        if( report )
        {
            // if clicking on the header, show the first report
            if( header )
                get_node_id(tree.GetChildItem(pNodeId->GetHItem()));
        }
    }

    return pNodeId;
}


template<typename T>
int CMainFrame::GetLexerLanguageForSourceCode(const Application& application, const T& app_tree_node) const
{
    if constexpr(std::is_same_v<T, AppTreeNode>)
    {
        const std::optional<AppFileType> app_type_type = app_tree_node.GetAppFileType();

        if( app_type_type.has_value() )
        {
            if( *app_type_type == AppFileType::Code )
            {
                const ExternalCodeAppTreeNode& external_code_app_tree_node = assert_cast<const ExternalCodeAppTreeNode&>(app_tree_node);

                if( external_code_app_tree_node.GetCodeFile().IsJavaScript() )
                    return SCLEX_JAVASCRIPT;
            }

            else if( *app_type_type == AppFileType::Message )
            {
                return Lexers::GetLexer_Message(application);
            }

            else if( *app_type_type == AppFileType::Report )
            {
                const ReportAppTreeNode& report_app_tree_node = assert_cast<const ReportAppTreeNode&>(app_tree_node);
                return Lexers::GetLexer_Report(application, report_app_tree_node.GetReportFile().GetFilePath());
            }
        }
    }

    else
    {
        if( app_tree_node.GetItemType() == eFFT_EXTERNALCODE )
        {
            const FormExternalCodeID& form_external_code_id = assert_cast<const FormExternalCodeID&>(app_tree_node);
            ASSERT(form_external_code_id.GetCodeFile().has_value());

            if( form_external_code_id.GetCodeFile()->IsJavaScript() )
                return SCLEX_JAVASCRIPT;
        }

        else if( app_tree_node.GetItemType() == eFFT_REPORT )
        {
            const FormReportID& form_report_id = assert_cast<const FormReportID&>(app_tree_node);
            return Lexers::GetLexer_Report(application, form_report_id.GetReportFile().GetFilePath());
        }
    }

    return Lexers::GetLexer_Logic(application);
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::SetSourceCode(CAplDoc* pAplDoc)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::SetSourceCode(CAplDoc* pAplDoc)
{
    if( pAplDoc == nullptr )
        return;

    Application* const pApplication = &pAplDoc->GetAppObject();
    ASSERT(pApplication != nullptr && pApplication->GetEngineAppType() == EngineAppType::Entry);

    CFormID* pFormID = GetNodeIdForSourceCode<CFormID>();

    CFormDoc* pFormDoc = pFormID->GetFormDoc();
    CView* pFormView = pFormDoc->GetView();
    ASSERT(pFormView);

    CFormChildWnd* pWnd = (CFormChildWnd*)pFormView->GetParentFrame();
    CFSourceEditView* pView = pWnd->GetSourceView();
    if( pView == nullptr )
        return;

    eNodeType nType = pFormID->GetItemType();

    SharableString source_code;
    bool bAppSrcCode = false;
    const int lexer_language = GetLexerLanguageForSourceCode(*pApplication, *pFormID);

    // external code
    if( nType == eFFT_EXTERNALCODE )
    {
        ASSERT(pFormID->GetTextSource() != nullptr);
        source_code = pFormID->GetTextSource()->GetTextAsSharableString();
    }

    // report
    else if( nType == eFFT_REPORT )
    {
        ASSERT(pFormID->GetTextSource() != nullptr);
        source_code = pFormID->GetTextSource()->GetTextAsSharableString();
    }

    // logic from the main file
    else
    {
        CDEFormBase* pBase = nullptr;

        if(nType == eFTT_GRIDFIELD){
            CDERoster* pRoster = DYNAMIC_DOWNCAST(CDERoster,pFormID->GetItemPtr());
            ASSERT(pRoster);
            pBase = pRoster->GetCol(pFormID->GetColumnIndex())->GetField(pFormID->GetRosterField());
        }
        else {
            pBase = pFormID->GetItemPtr();
        }

        pView->GetEditCtrl()->ClearErrorAndWarningMarkers();
        pWnd->GetLogicDialogBar().GetCompilerOutputTabViewPage()->ClearLogicErrors();

        if(pFormDoc && !pBase) { // if it is a form file show entire source code
            bAppSrcCode = true;
        }

        if(pBase || bAppSrcCode) {
            CString sSymbolName;
            if(pBase && pBase->IsKindOf(RUNTIME_CLASS(CDEField))) {
                sSymbolName = assert_cast<CDEField*>(pBase)->GetItemName();
            }
            else if(pBase && pBase->IsKindOf(RUNTIME_CLASS(CDEGroup))) {
                sSymbolName = assert_cast<CDEGroup*>(pBase)->GetName();
            }
            else if (pBase && pBase->IsKindOf(RUNTIME_CLASS(CDEBlock))) {
                sSymbolName = assert_cast<CDEBlock*>(pBase)->GetName();
            }
            else if(pBase && pBase->IsKindOf(RUNTIME_CLASS(CDELevel))) {
                sSymbolName = assert_cast<CDELevel*>(pBase)->GetName();
            }

            CSourceCode* pSourceCode = pApplication->GetAppSrcCode();
            CStringArray arrProcLines;

            pSourceCode->GetProc(arrProcLines,sSymbolName,CSourceCode_AllEvents); //Get all events for now

            //Get the view and set the text with the proclines
            CString sText;
            if(sSymbolName.IsEmpty()){
                bAppSrcCode = true;
                if(!pSourceCode->IsProcAvailable(_T("GLOBAL"))){
                    sText = _T("PROC GLOBAL\r\n\r\n");
                }
                if(!pSourceCode->IsProcAvailable(pFormDoc->GetFormFile().GetName())){
                    //if the form file is the primary form file
                    CString sFormFName = UTF8_TODO::GetCString(pApplication->GetFormFilePaths().front());//get primary form file
                    if(pFormDoc->GetPathName().CompareNoCase(sFormFName) ==0 ){
                        sText += _T("PROC ")+ pFormDoc->GetFormFile().GetName() + _T("\r\n\r\n");
                    }
                }
            }

            //Get the total memory to allocate
            UINT uAlloc = 0;
            int iNumLines = arrProcLines.GetSize();
            for(int iIndex =0; iIndex <iNumLines ;iIndex++){
                uAlloc  +=  arrProcLines.ElementAt(iIndex).GetLength();
                uAlloc += 2; // for the "\r\n"
            }
            uAlloc++; //for the "\0" @ the end

            uAlloc += sText.GetLength(); //U need to allocate this extra length for the text that is to be appended;
            CString main_source_code;
            LPTSTR pString = main_source_code.GetBufferSetLength(uAlloc);
            _tmemset(pString ,_T('\0'),uAlloc);

            for (int iIndex = 0 ; iIndex < iNumLines ; iIndex++) {
                if(!sText.IsEmpty()){
                    CString sLine = arrProcLines[iIndex];
                    sLine.Trim();
                    if(sLine.Mid(0,4).CompareNoCase(_T("PROC"))==0){
                        int iLength = sText.GetLength();
                        _tmemcpy(pString,sText.GetBuffer(iLength),iLength);
                        pString += iLength;
                        sText.ReleaseBuffer();
                        sText =_T("");
                    }
                }

                CString& csLine = arrProcLines[iIndex];
                int iLength = csLine.GetLength();
                _tmemcpy(pString,csLine.GetBuffer(iLength),iLength);
                pString += iLength;
                csLine.ReleaseBuffer();
                _tmemcpy(pString,_T("\r\n"),2);
                pString += 2;
            }
            main_source_code.ReleaseBuffer();

            if(main_source_code.IsEmpty() && !sSymbolName.IsEmpty()) {
                main_source_code = _T("PROC ") + sSymbolName;
                main_source_code += _T("\r\n");
            }
            if(!sText.IsEmpty()){
                main_source_code += sText;
            }

            source_code = UTF8_TODO::GetUtf8(main_source_code);
        }
    }

    ASSERT(source_code.IsSet());

    bool prevModifiedState = pView->GetEditCtrl()->IsModified(); // 20100708 trying to get rid of superfluous modified statements
    pView->GetEditCtrl()->SetText(source_code.GetString());
    pView->GetEditCtrl()->SetModified(prevModifiedState);

    if( lexer_language != pView->GetEditCtrl()->GetLexer() )
        pView->GetEditCtrl()->InitLogicControl(true, true, lexer_language);

    // fold procs only when viewing PROC GLOBAL
    if( Lexers::CanFoldCode(lexer_language) )
        pView->GetEditCtrl()->SetFolding(bAppSrcCode);

    pWnd->GetLogicDialogBar().UpdateScrollState();
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::SetOSourceCode(CAplDoc* pAplDoc)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::SetOSourceCode(CAplDoc* pAplDoc)
{
    if( pAplDoc == nullptr )
        return;

    Application* pApplication = &pAplDoc->GetAppObject();
    ASSERT(pApplication != nullptr && pApplication->GetEngineAppType() == EngineAppType::Batch);

    AppTreeNode* const app_tree_node = GetNodeIdForSourceCode<AppTreeNode>();
    ASSERT(app_tree_node != nullptr);

    COrderDoc* pOrderDoc = app_tree_node->GetOrderDocument();
    POSITION pos = pOrderDoc->GetFirstViewPosition();
    CView* pOrderView = pOrderDoc->GetNextView(pos);
    ASSERT(pOrderView);

    COrderChildWnd* pWnd = (COrderChildWnd*)pOrderView->GetParentFrame();
    COSourceEditView* pView = pWnd->GetOSourceView();

    SharableString source_code;
    bool bAppSrcCode = false;
    const int lexer_language = GetLexerLanguageForSourceCode(*pApplication, *app_tree_node);

    // external code
    if( app_tree_node->GetAppFileType() == AppFileType::Code )
    {
        ASSERT(app_tree_node->GetTextSource() != nullptr);
        source_code = app_tree_node->GetTextSource()->GetTextAsSharableString();
    }

    // report
    else if( app_tree_node->GetAppFileType() == AppFileType::Report )
    {
        ASSERT(app_tree_node->GetTextSource() != nullptr);
        source_code = app_tree_node->GetTextSource()->GetTextAsSharableString();
    }

    // logic from the main file
    else
    {
        CDEFormBase* form_base = app_tree_node->GetFormBase();

        if(form_base == nullptr) { // if it is a form file show entire source code
            bAppSrcCode = true;
        }

        if(form_base != nullptr || bAppSrcCode) {
            CString sSymbolName;
            if(form_base != nullptr && form_base->IsKindOf(RUNTIME_CLASS(CDEField))) {
                sSymbolName = assert_cast<CDEField*>(form_base)->GetItemName();
            }
            else if(form_base != nullptr && form_base->IsKindOf(RUNTIME_CLASS(CDEGroup))) {
                sSymbolName = assert_cast<CDEGroup*>(form_base)->GetName();
            }
            else if(form_base != nullptr && form_base->IsKindOf(RUNTIME_CLASS(CDEBlock))) {
                sSymbolName = assert_cast<CDEBlock*>(form_base)->GetName();
            }
            else if(form_base != nullptr && form_base->IsKindOf(RUNTIME_CLASS(CDELevel))) {
                sSymbolName = assert_cast<CDELevel*>(form_base)->GetName();
            }

            CSourceCode* pSourceCode = pApplication->GetAppSrcCode();
            CStringArray arrProcLines;

            pSourceCode->GetProc(arrProcLines,sSymbolName,CSourceCode_AllEvents); //Get all events for now

            //Get the view and set the text with the proclines
            CString sText;
            if(sSymbolName.IsEmpty()){
                bAppSrcCode = true;
                if(!pSourceCode->IsProcAvailable(_T("GLOBAL"))){
                    sText = _T("PROC GLOBAL\r\n\r\n");
                }
                if(!pSourceCode->IsProcAvailable(pOrderDoc->GetFormFile().GetName())){
                    //if the order file is not the primary order file
                    CString sOrderFName = UTF8_TODO::GetCString(pApplication->GetFormFilePaths().front());//get primary order file
                    if(pOrderDoc->GetPathName().CompareNoCase(sOrderFName) ==0 ){
                        sText += _T("PROC ")+ pOrderDoc->GetFormFile().GetName() + _T("\r\n\r\n");
                    }
                }
            }

            //Get the total memory to allocate
            UINT uAlloc = 0;
            int iNumLines = arrProcLines.GetSize();
            for(int iIndex =0; iIndex <iNumLines ;iIndex++){
                uAlloc  +=  arrProcLines.ElementAt(iIndex).GetLength();
                uAlloc += 2; // for the "\r\n"
            }
            uAlloc++; //for the "\0" @ the end

            uAlloc += sText.GetLength(); //U need to allocate this extra length for the text that is to be appended;
            CString main_source_code;
            LPTSTR pString = main_source_code.GetBufferSetLength(uAlloc);
            _tmemset(pString ,_T('\0'),uAlloc);

            for (int iIndex = 0 ; iIndex < iNumLines ; iIndex++) {
                if(!sText.IsEmpty()){
                    CString sLine = arrProcLines[iIndex];
                    sLine.Trim();
                    if(sLine.Mid(0,4).CompareNoCase(_T("PROC"))==0){
                        int iLength = sText.GetLength();
                        _tmemcpy(pString,sText.GetBuffer(iLength),iLength);
                        pString += iLength;
                        sText.ReleaseBuffer();
                        sText =_T("");
                    }
                }

                CString& csLine = arrProcLines[iIndex];
                int iLength = csLine.GetLength();
                _tmemcpy(pString,csLine.GetBuffer(iLength),iLength);
                pString += iLength;
                csLine.ReleaseBuffer();
                _tmemcpy(pString,_T("\r\n"),2);
                pString += 2;
            }
            main_source_code.ReleaseBuffer();

            if(main_source_code.IsEmpty() && !sSymbolName.IsEmpty()) {
                main_source_code = _T("PROC ") + sSymbolName;
                main_source_code += _T("\r\n");
            }
            if(!sText.IsEmpty()){
                main_source_code += sText;
            }

            source_code = UTF8_TODO::GetUtf8(main_source_code);
        }
    }

    ASSERT(source_code.IsSet());

    // 20100316 nothing is changed by just loading or changing what proc is displayed
    bool modFlag1 = pOrderDoc->IsModified();
    bool modFlag2 = pView->GetEditCtrl()->IsModified();

    pView->GetEditCtrl()->SetText(source_code.GetString());

    pOrderDoc->SetModifiedFlag(modFlag1); // 20100316
    pView->GetEditCtrl()->SetModified(modFlag2);

    if( lexer_language != pView->GetEditCtrl()->GetLexer() )
        pView->GetEditCtrl()->InitLogicControl(true, true, lexer_language);

    // fold procs only when viewing PROC GLOBAL
    if( Lexers::CanFoldCode(lexer_language) )
        pView->GetEditCtrl()->SetFolding(bAppSrcCode);

    pWnd->GetLogicDialogBar().UpdateScrollState();
}


LRESULT CMainFrame::UpdateSrcCode(WPARAM wParam, LPARAM lParam)
{
    BOOL bForceCompile = (BOOL)wParam;
    CFormID* pFormID = (CFormID*)lParam;
    if(!pFormID)
        return 0;
    CFormDoc* pFormDoc = pFormID->GetFormDoc();

    if(!pFormDoc)
        return 0;

    //SAVY& To Take care when one form is used by multiple applications
    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pFormDoc);

    if (pAplDoc && !pAplDoc->m_bIsClosing) {

        if(!PutSourceCode(pFormID,bForceCompile)) {
            POSITION pos = pFormDoc->GetFirstViewPosition();
            CView* pFormView = pFormDoc->GetNextView(pos);
            ASSERT(pFormView);
            UNREFERENCED_PARAMETER(pFormView);
            return -1L;

        }

    }
    else if(pAplDoc == nullptr){
        //SAVY 07/18/2000 no need to convey it to the user
        AfxMessageBox(_T("No Application associated with this form file"));
    }

    return 0;
}


namespace
{
    template<typename view_type>
    bool ProcessParserMessages(CAplDoc* pAplDoc, CSourceCode* pSourceCode, view_type* pView, LogicDialogBar& logic_dialog_bar,
                               TextSource* external_logic_or_report_text_source = nullptr)
    {
        std::string all_messages;
        bool processed_first_error = false;
        bool has_errors = false;
        std::optional<std::map<std::string, int>> proc_line_number_map;

        CompilerOutputTabViewPage* compiler_output_tab_view_page = logic_dialog_bar.GetCompilerOutputTabViewPage();

        for( Logic::ParserMessage parser_message : CCompiler::GetCurrentSession()->GetParserMessages() )
        {
            // remove any newlines from the message
            NewlineSubstitutor::MakeNewlineToSpace(parser_message.message_text);

            if( parser_message.type == Logic::ParserMessage::Type::Error )
                has_errors = true;

            int line_number_for_display = parser_message.line_number;
            bool use_line_number_for_display_for_bookmark = false;

            // if editing external code or reports, only use the line number for the bookmark if
            // editing the file where the message occurred
            if( external_logic_or_report_text_source != nullptr )
            {
                if( SO::EqualsNoCase(parser_message.compilation_unit_name, external_logic_or_report_text_source->GetFilePath()) )
                    use_line_number_for_display_for_bookmark = true;
            }

            // otherwise only adjust the line number if this is a source-related message not from an external code file or a report
            else if( line_number_for_display != 0 && parser_message.compilation_unit_name.empty() )
            {
                if( !proc_line_number_map.has_value() )
                    proc_line_number_map = pSourceCode->GetProcLineNumberMap();

                auto adjust_line_number_from_proc_lookup = [&](const std::string& proc_name)
                {
                    const auto& line_number_lookup = proc_line_number_map->find(proc_name);

                    if( line_number_lookup != proc_line_number_map->cend() )
                    {
                        line_number_for_display += line_number_lookup->second;
                        return true;
                    }

                    return false;
                };

                if( !adjust_line_number_from_proc_lookup(parser_message.proc_name) )
                {
                    // when compiling user-defined functions, the proc_name is the function name,
                    // so two lookups may be necessary in that case
                    if( pSourceCode->IsCompilingGlobal() )
                        adjust_line_number_from_proc_lookup("GLOBAL");
                }

                use_line_number_for_display_for_bookmark = true;
            }

            // get the line number for the bookmark (-1 because the line numbers are 1-based, or 0 if not in this source view)...
            int line_number_for_bookmark = use_line_number_for_display_for_bookmark ? std::max(0, line_number_for_display - 1) : 0;

            // ...and go to that line if it was an error
            if( !processed_first_error && parser_message.type == Logic::ParserMessage::Type::Error )
            {
                // 20120613 if the proc name existed in the main and external dictionaries, an invalid argument error occurred
                if( line_number_for_bookmark < pView->GetEditCtrl()->GetLineCount() )
                    pView->GetEditCtrl()->GotoLine(line_number_for_bookmark);

                processed_first_error = true;
            }

            const char* const message_type = ( parser_message.type == Logic::ParserMessage::Type::Error )   ? "ERROR" :
                                             ( parser_message.type == Logic::ParserMessage::Type::Warning ) ? "WARNING" :
                                                                                                              "DEPRECATION";

            std::string error_location_and_line_number;

            if( pSourceCode->IsCompilingGlobal() )
            {
                if( std::holds_alternative<CapiLogicLocation>(parser_message.extended_location) )
                {
                    const CapiLogicLocation& capi_logic_location = std::get<CapiLogicLocation>(parser_message.extended_location);

                    error_location_and_line_number = "Question Text, " + parser_message.proc_name;

                    if( capi_logic_location.language_label.has_value() )
                    {
                        error_location_and_line_number.append(", ")
                                                      .append(*capi_logic_location.language_label);
                    }

                    if( capi_logic_location.condition_index > 0 )
                    {
                        error_location_and_line_number.append(", condition #")
                                                      .append(IntToString(capi_logic_location.condition_index + 1));
                    }
                }

                // the compilation unit should only be set when compiling external code files, reports, and message files
                else if( !parser_message.compilation_unit_name.empty() )
                {
                    // use the full filename for message files
                    if( std::holds_alternative<Logic::ParserMessage::MessageFile>(parser_message.extended_location) )
                    {
                        error_location_and_line_number = PortableFunctions::PathGetFilename(parser_message.compilation_unit_name);
                    }

                    // use the name for reports
                    else if( const ReportFile* const report_file = pAplDoc->GetAppObject().GetReportFile(parser_message.compilation_unit_name, false);
                             report_file != nullptr )
                    {
                        error_location_and_line_number = report_file->GetName();
                    }

                    // use the filename (without extension) for external code files
                    else
                    {
                        error_location_and_line_number = Path::GetFilenameWithoutExtension(parser_message.compilation_unit_name);
                    }
                }

                // don't include GLOBAL when there is a non-line related error (e.g., the external code file couldn't be opened)
                else if( line_number_for_display == 0 )
                {
                    ASSERT(SO::EqualsNoCase(parser_message.proc_name, "GLOBAL") || parser_message.proc_name.empty());
                }

                else
                {
                    error_location_and_line_number = parser_message.proc_name;
                }
            }

            if( line_number_for_display > 0 )
                SO::AppendWithSeparator(error_location_and_line_number, IntToString(line_number_for_display), ", ");

            all_messages.append(message_type);

            if( !error_location_and_line_number.empty() )
            {
                all_messages.append("(")
                            .append(error_location_and_line_number)
                            .append(")");
            }

            all_messages.append(": ")
                        .append(parser_message.message_text)
                        .append("\r\n");

            // show on the logic editor where the error or warning is located
            auto add_error_or_warning = [&](const int line_number)
            {
                const bool error = ( parser_message.type == Logic::ParserMessage::Type::Error );
                pView->GetEditCtrl()->AddErrorOrWarningMarker(error, line_number);
            };

            if( external_logic_or_report_text_source != nullptr )
            {
                // when editing external code or reports, mark only errors/warnings in that file
                if( SO::EqualsNoCase(parser_message.compilation_unit_name, external_logic_or_report_text_source->GetFilePath()) )
                {
                    // - 1 because the line numbers are 1-based
                    add_error_or_warning(parser_message.line_number - 1);
                }
            }

            else
            {
                // otherwise mark all errors/warnings that did not come from external code or reports
                if( parser_message.compilation_unit_name.empty() )
                    add_error_or_warning(line_number_for_bookmark);
            }

            compiler_output_tab_view_page->AddLogicError(std::move(parser_message),
                                                         parser_message.compilation_unit_name.empty() ? std::make_optional(line_number_for_bookmark) : std::nullopt);
        }

        if( all_messages.empty() )
            all_messages = "Compile Successful at " + UTF8_TODO::GetUtf8(CTime::GetCurrentTime().Format(_T("%X")));

        compiler_output_tab_view_page->SetReadOnlyText(all_messages);

        logic_dialog_bar.SelectCompilerOutputTab();
        logic_dialog_bar.UpdateScrollState();

        compiler_output_tab_view_page->Invalidate();
        compiler_output_tab_view_page->UpdateWindow();

        if( has_errors )
        {
            AfxMessageBox(L"Compile Failed!");
            return false;
        }

        return true;
    }
}


bool CMainFrame::PutSourceCode(CFormID* pFormID, bool bForceCompile)
{
    pFormID = GetNodeIdForSourceCode<CFormID>(pFormID);
    ASSERT(pFormID != nullptr);

    //Get the active application
    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pFormID->GetFormDoc());
    bool bRet = true;

    if(pAplDoc == nullptr)
        return bRet;

    Application* pApplication = &pAplDoc->GetAppObject();

    CFormDoc* pFormDoc = pFormID->GetFormDoc();
    POSITION pos = pFormDoc->GetFirstViewPosition();
    CView* pFormView = pFormDoc->GetNextView(pos);
    ASSERT(pFormView);

    CFormChildWnd* pWnd = (CFormChildWnd*)pFormView->GetParentFrame();
    CFSourceEditView* pView = pWnd->GetSourceView();

    if( pView == nullptr )
        return bRet;

    CSourceCode* pSourceCode = pApplication->GetAppSrcCode();
    pSourceCode->SetOrder(pAplDoc->GetOrder());

    BOOL bAppSrcCode = FALSE;
    TextSource* external_logic_or_report_text_source = nullptr;
    CString sSymbolName;
    CStringArray arrProcLines;

    // external code and reports...
    if( pFormID->GetItemType() == eFFT_EXTERNALCODE ||
        pFormID->GetItemType() == eFFT_REPORT )
    {
        external_logic_or_report_text_source = pFormID->GetTextSource();
        ASSERT(external_logic_or_report_text_source != nullptr);

        if( pView->GetEditCtrl()->IsModified() )
        {
            std::wstring source_code = UTF8_TODO::GetWide(pView->GetLogicCtrl()->GetText());
            external_logic_or_report_text_source->SetText(UTF8_TODO::GetUtf8(std::move(source_code)));

            pView->GetEditCtrl()->SetModified(FALSE);
        }
    }

    // logic from the main file...
    else
    {
        CDEFormBase* pBase = nullptr;
        if(pFormID->GetItemType() == eFTT_GRIDFIELD){
            CDERoster* pRoster = DYNAMIC_DOWNCAST(CDERoster,pFormID->GetItemPtr());
            ASSERT(pRoster);
            pBase = pRoster->GetCol(pFormID->GetColumnIndex())->GetField(pFormID->GetRosterField());
        }
        else {
            pBase = pFormID->GetItemPtr();
        }

        pView->GetEditCtrl()->ClearErrorAndWarningMarkers();
        pWnd->GetLogicDialogBar().GetCompilerOutputTabViewPage()->ClearLogicErrors();

        if(pBase && pBase->IsKindOf(RUNTIME_CLASS(CDEFormFile))) {
            // if it is a form file show entire source code
            bAppSrcCode = TRUE;
        }

        if( !pBase && !bAppSrcCode )
            return bRet;

        if(pBase && pBase->IsKindOf(RUNTIME_CLASS(CDEField))) {
            sSymbolName = assert_cast<CDEField*>(pBase)->GetItemName();
        }
        else if(pBase && pBase->IsKindOf(RUNTIME_CLASS(CDEGroup))) {
            sSymbolName = assert_cast<CDEGroup*>(pBase)->GetName();
        }
        else if (pBase && pBase->IsKindOf(RUNTIME_CLASS(CDEBlock))) {
            sSymbolName = assert_cast<CDEBlock*>(pBase)->GetName();
        }
        else if(pBase && pBase->IsKindOf(RUNTIME_CLASS(CDELevel))) {
            sSymbolName = assert_cast<CDELevel*>(pBase)->GetName();
        }

        if(sSymbolName.IsEmpty() && !bAppSrcCode)
            return bRet;

        CIMSAString sString = UTF8_TODO::GetCString(pView->GetLogicCtrl()->GetText());
        CString sLine;

        // gsf 23-mar-00: make sure GetToken does not strip off leading quote marks
        arrProcLines.SetSize(0,100); //avoid multiple allocs SAVY 09/27/00
        while(!(sLine = sString.GetToken(_T("\n"), nullptr, TRUE)).IsEmpty()) {
            sLine.TrimRight('\r');sLine.TrimLeft('\r');
            arrProcLines.Add(sLine);
        }
        arrProcLines.FreeExtra(); //Free Extra SAVY 09/27/00

        if(sSymbolName.IsEmpty()) {
            pSourceCode->PutProc(arrProcLines);
        }
        else
        {
            // 20120613 to stop the problem where duplicate procs got added when the name of the proc was changed while editing a proc (rather than editing globally)
            // in these cases, we'll move to the global logic view instead of staying in the changed proc; the error also occurred when two procs were added to the same
            // local (not global) proc section

            if( !m_bRemovingPossibleDuplicateProcs && !pSourceCode->IsOnlyThisProcPresent(arrProcLines,sSymbolName) ) // 20120613
            {
                CFormTreeCtrl * pFTC = pFormDoc->GetFormTreeCtrl();
                CFormNodeID* pRootID = pFTC->GetFormNode(pFormDoc);

                m_bRemovingPossibleDuplicateProcs = true;
                pFTC->SelectItem(pRootID->GetHItem());
                m_bRemovingPossibleDuplicateProcs = false;

                CFormNodeID* pRootIDForRecompilation = (CFormNodeID*)pFTC->GetItemData(pFTC->GetSelectedItem());

                AfxGetMainWnd()->PostMessage(UWM::Form::PutSourceCode, bForceCompile, reinterpret_cast<LPARAM>(pRootIDForRecompilation));
                return FALSE;
            }

            pSourceCode->PutProc(arrProcLines,sSymbolName,CSourceCode_AllEvents); //Get all events for now
        }

        if(pView->GetEditCtrl()->IsModified()) {
            pSourceCode->SetModifiedFlag(true);
            pView->GetEditCtrl()->SetModified(FALSE);
        }
    }


    pView->GetEditCtrl()->ClearErrorAndWarningMarkers();
    pWnd->GetLogicDialogBar().GetCompilerOutputTabViewPage()->ClearLogicErrors();

    if( !bForceCompile )
        return bRet;

    // compile the logic
    try
    {
        DesignerCompiler designer_compiler(pAplDoc);

        if( bAppSrcCode )
        {
            bRet = designer_compiler.CompileAll();
        }

        else if( external_logic_or_report_text_source == nullptr )
        {
            bRet = designer_compiler.CompileProc(sSymbolName, arrProcLines);
        }

        else if( pFormID->GetItemType() == eFFT_EXTERNALCODE )
        {
            bRet = designer_compiler.CompileExternalCode(*assert_cast<FormExternalCodeID&>(*pFormID).GetCodeFile());
        }

        else if( pFormID->GetItemType() == eFFT_REPORT )
        {
            bRet = designer_compiler.CompileReport(assert_cast<FormReportID&>(*pFormID).GetReportFile());
        }

        else
        {
            ASSERT(false);
        }

        if( !ProcessParserMessages(pAplDoc, pSourceCode, pView, pWnd->GetLogicDialogBar(), external_logic_or_report_text_source) )
            return false;

        return bRet;
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return false;
    }
}


bool CMainFrame::CompileAll(CFormDoc* form_doc)
{
    CFormNodeID* form_node = form_doc->GetFormTreeCtrl()->GetFormNode(form_doc);
    if (!form_node)
        return false;

    CAplDoc* pAplDoc = ProcessFOForSrcCode(*form_node->GetFormDoc());

    if( pAplDoc == nullptr )
        return false;

    Application* pApplication = &pAplDoc->GetAppObject();
    CSourceCode* pSourceCode = pApplication->GetAppSrcCode();
    pSourceCode->SetOrder(pAplDoc->GetOrder());

    // compile the logic
    try
    {
        DesignerCompiler designer_compiler(pAplDoc);

        if( designer_compiler.CompileAll() )
            return true;

        // Switch to logic view if there are errors
        form_doc->GetFormTreeCtrl()->Select(form_node->GetHItem(), TVGN_CARET);
        POSITION pos = form_doc->GetFirstViewPosition();
        CView* view = form_doc->GetNextView(pos);
        ASSERT(view);

        CFormChildWnd* parent_frame = (CFormChildWnd*) view->GetParentFrame();
        parent_frame->SendMessage(UWM::Designer::SwitchView, (WPARAM)ViewType::Logic);

        PutSourceCode(form_node, false); // Updates the proc/line number mapping for error messages
        CFSourceEditView* source_view = parent_frame->GetSourceView();
        ProcessParserMessages(pAplDoc, pSourceCode, source_view, parent_frame->GetLogicDialogBar());
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    return false;
}

//Updated to support CSBatch 05/22/00
LRESULT CMainFrame::UpdateOSrcCode(WPARAM wParam, LPARAM lParam)
{
    BOOL bForceCompile = static_cast<BOOL>(wParam);
    AppTreeNode* app_tree_node = reinterpret_cast<AppTreeNode*>(lParam);
    COrderDoc* pOrderDoc = ( app_tree_node != nullptr ) ? app_tree_node->GetOrderDocument() : nullptr;

    if( pOrderDoc == nullptr )
        return 0;

    //SAVY& To Take care when one order is used by multiple applications
    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pOrderDoc);

    if( pAplDoc != nullptr && !pAplDoc->m_bIsClosing )
    {
        if( !PutOSourceCode(app_tree_node, bForceCompile) )
            return -1;
    }

    return 0;
}


//Changed to support CSBatch 05/22/00
bool CMainFrame::PutOSourceCode(AppTreeNode* app_tree_node, bool bForceCompile)
{
    app_tree_node = GetNodeIdForSourceCode<AppTreeNode>(app_tree_node);
    ASSERT(app_tree_node != nullptr);

    CAplDoc* pAplDoc = ProcessFOForSrcCode(*app_tree_node->GetOrderDocument());
    bool bRet = true;

    if( pAplDoc == nullptr )
        return bRet;

    Application* pApplication = &pAplDoc->GetAppObject();

    COrderDoc* pOrderDoc = app_tree_node->GetOrderDocument();
    POSITION pos = pOrderDoc->GetFirstViewPosition();
    CView* pOrderView = pOrderDoc->GetNextView(pos);
    ASSERT(pOrderView);

    COrderChildWnd* pWnd = (COrderChildWnd*)pOrderView->GetParentFrame();
    COSourceEditView* pView = (COSourceEditView*)pOrderView;

    if( pView == nullptr )
        return bRet;

    CSourceCode* pSourceCode = pApplication->GetAppSrcCode();
    pSourceCode->SetOrder(pAplDoc->GetOrder());

    BOOL bAppSrcCode = FALSE;
    TextSource* external_logic_or_report_text_source = nullptr;
    CString sSymbolName;
    CStringArray arrProcLines;

    // external code and reports...
    if( app_tree_node->GetAppFileType() == AppFileType::Code ||
        app_tree_node->GetAppFileType() == AppFileType::Report )
    {
        external_logic_or_report_text_source = app_tree_node->GetTextSource();
        ASSERT(external_logic_or_report_text_source != nullptr);

        if( pView->GetEditCtrl()->IsModified() )
        {
            std::wstring source_code = UTF8_TODO::GetWide(pView->GetLogicCtrl()->GetText());
            external_logic_or_report_text_source->SetText(UTF8_TODO::GetUtf8(std::move(source_code)));

            pView->GetEditCtrl()->SetModified(FALSE);
        }
    }

    // logic from the main file...
    else
    {
        CDEFormBase* pBase = app_tree_node->GetFormBase();

        if(pBase != nullptr && pBase->IsKindOf(RUNTIME_CLASS(CDEFormFile))) {
            // if it is a form file show entire source code
            bAppSrcCode = TRUE;
        }

        if( pBase == nullptr && !bAppSrcCode )
            return bRet;

        if(pBase != nullptr && pBase->IsKindOf(RUNTIME_CLASS(CDEField))) {
            sSymbolName = assert_cast<CDEField*>(pBase)->GetItemName();
        }
        else if(pBase != nullptr && pBase->IsKindOf(RUNTIME_CLASS(CDEGroup))) {
            sSymbolName = assert_cast<CDEGroup*>(pBase)->GetName();
        }
        else if(pBase != nullptr && pBase->IsKindOf(RUNTIME_CLASS(CDEBlock))) {
            sSymbolName = assert_cast<CDEBlock*>(pBase)->GetName();
        }
        else if(pBase != nullptr && pBase->IsKindOf(RUNTIME_CLASS(CDELevel))) {
            sSymbolName = assert_cast<CDELevel*>(pBase)->GetName();
        }

        if(sSymbolName.IsEmpty() && !bAppSrcCode)
            return bRet;

        CIMSAString sString = UTF8_TODO::GetCString(pView->GetLogicCtrl()->GetText());
        CString sLine;

        // gsf 23-mar-00: make sure GetToken does not strip off leading quote marks
        arrProcLines.SetSize(0,100); //AVOID multi allocs SAVY 09/27/00
        while(!(sLine = sString.GetToken(_T("\n"), nullptr, TRUE)).IsEmpty()) {
            sLine.TrimRight('\r');sLine.TrimLeft('\r');
            arrProcLines.Add(sLine);
        }
        arrProcLines.FreeExtra(); //Freeextra SAVY 09/27/00
        if(sSymbolName.IsEmpty()) {
            pSourceCode->PutProc(arrProcLines);
        }
        else
        {
            // 20120613 to stop the problem where duplicate procs got added when the name of the proc was changed while editing a proc (rather than editing globally)
            // in these cases, we'll move to the global logic view instead of staying in the changed proc; the error also occurred when two procs were added to the same
            // local (not global) proc section

            if( !m_bRemovingPossibleDuplicateProcs && !pSourceCode->IsOnlyThisProcPresent(arrProcLines,sSymbolName) ) // 20120613
            {
                COrderTreeCtrl* pOTC = pOrderDoc->GetOrderTreeCtrl();
                const FormOrderAppTreeNode* const form_order_app_tree_node = pOTC->GetFormOrderAppTreeNode(*pOrderDoc);

                m_bRemovingPossibleDuplicateProcs = true;
                pOTC->SelectItem(form_order_app_tree_node->GetHItem());
                m_bRemovingPossibleDuplicateProcs = false;

                AppTreeNode* app_tree_node_for_recompilation = pOTC->GetTreeNode(pOTC->GetSelectedItem());

                AfxGetMainWnd()->PostMessage(UWM::Order::PutSourceCode, bForceCompile, reinterpret_cast<LPARAM>(app_tree_node_for_recompilation));
                return FALSE;
            }

            pSourceCode->PutProc(arrProcLines,sSymbolName,CSourceCode_AllEvents); //Get all events for now
        }

        if(pView->GetEditCtrl()->IsModified()) {
            pSourceCode->SetModifiedFlag(true);
            pView->GetEditCtrl()->SetModified(FALSE);
        }
    }


    pView->GetEditCtrl()->ClearErrorAndWarningMarkers();
    pWnd->GetLogicDialogBar().GetCompilerOutputTabViewPage()->ClearLogicErrors();

    if( !bForceCompile )
        return bRet;

    // compile the logic
    try
    {
        DesignerCompiler designer_compiler(pAplDoc);

        if( bAppSrcCode )
        {
            bRet = designer_compiler.CompileAll();
        }

        else if( external_logic_or_report_text_source == nullptr )
        {
            bRet = designer_compiler.CompileProc(sSymbolName, arrProcLines);
        }

        else if( app_tree_node->GetAppFileType() == AppFileType::Code )
        {
            bRet = designer_compiler.CompileExternalCode(assert_cast<ExternalCodeAppTreeNode&>(*app_tree_node).GetCodeFile());
        }

        else if( app_tree_node->GetAppFileType() == AppFileType::Report )
        {
            bRet = designer_compiler.CompileReport(assert_cast<ReportAppTreeNode&>(*app_tree_node).GetReportFile());
        }

        else
        {
            ASSERT(false);
        }

        if( !ProcessParserMessages(pAplDoc, pSourceCode, pView, pWnd->GetLogicDialogBar(), external_logic_or_report_text_source) )
            return false;

        return bRet;
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return false;
    }
}


LRESULT CMainFrame::OnGetMessageTextSource(const WPARAM wParam, const LPARAM lParam)
{
    CDocument* const pDoc = reinterpret_cast<CDocument*>(wParam);
    std::shared_ptr<TextSourceEditable>& message_text_source = *reinterpret_cast<std::shared_ptr<TextSourceEditable>*>(lParam);

    CAplDoc* const pAplDoc = ProcessFOForSrcCode(*pDoc);

    if( pAplDoc == nullptr )
        return 0;

    message_text_source = pAplDoc->GetMessageTextSource();

    return 1;
}


//SAVY 05/18/00 updated for the CSBatch application
LRESULT CMainFrame::OnRunBatch(WPARAM /*wParam*/, LPARAM lParam)
{
    bool bShiftPressed = GetKeyState(VK_SHIFT) < 0; // 20120510 allow the user to bypass the file associations screen, if possible, by holding down shift

    COrderDoc* pOrder = (COrderDoc*)lParam;

    HANDLE ahEvent = OpenEvent( EVENT_ALL_ACCESS, FALSE, CSPRO_WNDCLASS_BATCHWND);
    if(ahEvent) {
        AfxMessageBox(_T("CSBatch is already running. Please close it before you launch another instance."));
        CloseHandle(ahEvent);
        return 0;
    }

    //Check Applications which has this form as the main one if
    //there are more than one ask the user for which application to
    //run. If there is only one proceed with it.
    CAplDoc* const pDoc = GetApplicationUsingFormFile(TC::ToUtf8(pOrder->GetPathName()));

    if(pDoc) {
        if(pDoc->IsAppModified()) {
            //If application is modified set ask the user to save
            CString sMsg;
            sMsg.FormatMessage(IDS_APPMODIFIED, pDoc->GetPathName().GetString());
            if(AfxMessageBox(sMsg,MB_YESNO) != IDYES) {
                return 0;
            }
            else {
                pDoc->OnSaveDocument(pDoc->GetPathName());
            }
        }

        FormOrderAppTreeNode* const form_order_app_tree_node = pOrder->GetOrderTreeCtrl()->GetFormOrderAppTreeNode(*pOrder);
        BOOL bRun = FALSE;
        if(form_order_app_tree_node != nullptr) {

            if(pOrder->GetOrderTreeCtrl()->Select(form_order_app_tree_node->GetHItem(), TVGN_CARET)){
                if(PutOSourceCode(form_order_app_tree_node, true))
                    bRun = TRUE;
                else
                    return 0;
            }
        }
        if(!bRun){
            AfxMessageBox(_T("Please compile your application before you run"));
            return 0;
        }

        CString filename_to_run = pDoc->GetPathName();

        if( bShiftPressed )
        {
            CString pff_filename = PortableFunctions::PathRemoveFileExtensionCS(filename_to_run) + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Pff));

            if( PortableFunctions::FileIsRegular(pff_filename) )
                filename_to_run = pff_filename;
        }

        CSProExecutables::RunProgramOpeningFile(CSProExecutables::Program::CSBatch, CS2WS(filename_to_run));
    }

    return 0;
}


LRESULT CMainFrame::IsNameUnique(WPARAM wParam, LPARAM lParam)
{
    CString name = (LPCTSTR)wParam;
    auto pForm = (const CFormDoc*)lParam;

    //Check Applications which has this form as the main one if
    //there are more than one ask the user for which application to
    //run. If there is only one proceed with it.
    const CAplDoc* pDoc = GetApplicationUsingFormFile(TC::ToUtf8(pForm->GetPathName()), true);
    bool name_is_unique;

    if( pDoc != nullptr )
    {
        name_is_unique = pDoc->IsNameUnique(pForm, name);
    }

    else
    {
        const CDDTreeCtrl& dictTree = this->GetDlgBar().m_DictTree;
        name_is_unique = pForm->GetFormFile().IsNameUnique(name);

        if( name_is_unique )
        {
            DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(UTF8_TODO::GetUtf8(pForm->GetFormFile().GetDictionaryFilename()));

            if( dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr )
            {
                int iL, iR, iI, iVS;
                name_is_unique = !dictionary_dict_tree_node->GetDDDoc()->GetDict()->LookupName(UTF8_TODO::GetUtf8(name), &iL, &iR, &iI, &iVS);
            }
        }
    }

    if( !name_is_unique )
    {
        AfxMessageBox(FormatText(L"The name '%s' cannot be used as it is not unique in your application.", name.GetString()));
        return 0;
    }

    return 1;
}


LRESULT CMainFrame::IsTabNameUnique(WPARAM wParam, LPARAM lParam)
{
    CTabulateDoc* pTabDoc = (CTabulateDoc*)lParam;
    LPCTSTR sName = (LPCTSTR)wParam;

    //Check Applications which has this tab doc
    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pTabDoc);

    if (pAplDoc)
    {
        if (!pAplDoc->IsNameUnique(pTabDoc, sName))
        {
            CString sMsg;
            sMsg.FormatMessage(_T("%1 is not a unique name. Name already in use."), sName);
            AfxMessageBox(sMsg);
            return 0;
        }
        else {
            return 1;
        }

    }

    return 1;
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OnUpdateKeyOvr
//
/////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnUpdateKeyOvr(CCmdUI* pCmdUI)
{
    CLogicCtrl* logic_ctrl = nullptr;

    //Get the active child window
    //if it is CFormChildWnd and the view is logic
    //Get it from the edit control
    CMDIFrameWnd* pWnd = (CMDIFrameWnd*)MDIGetActive();
    if(pWnd && pWnd->IsKindOf(RUNTIME_CLASS(CFormChildWnd)) && pWnd->GetSafeHwnd()){
        CFormChildWnd* pFormFrame = (CFormChildWnd*)pWnd;
        CView* pView = pFormFrame->GetActiveView();
        if(pView && pView->GetSafeHwnd() && pView->IsKindOf(RUNTIME_CLASS(CFSourceEditView))){
            logic_ctrl = assert_cast<CFSourceEditView*>(pView)->GetLogicCtrl();
        }
    }

    else if(pWnd && pWnd->IsKindOf(RUNTIME_CLASS(COrderChildWnd)) && pWnd->GetSafeHwnd()){
        COrderChildWnd* pFormFrame = (COrderChildWnd*)pWnd;
        CView* pView = pFormFrame->GetActiveView();
        if(pView && pView->GetSafeHwnd() && pView->IsKindOf(RUNTIME_CLASS(COSourceEditView))){
            logic_ctrl = assert_cast<COSourceEditView*>(pView)->GetLogicCtrl();
        }
    }
    //Should not do it for other frames // do it on a need basis
    //   pCmdUI->Enable (::GetKeyState (VK_INSERT) % 2 == 0);
    pCmdUI->Enable(logic_ctrl != nullptr && logic_ctrl->GetOvertype());
}


/////////////////////////////////////////////////////////////////////////////
//
//                      CMainFrame::SetStatusBarPane
//
/////////////////////////////////////////////////////////////////////////////

LRESULT CMainFrame::SetStatusBarPane(WPARAM wParam, LPARAM /*lParam*/)
{
    auto pszText = (const TCHAR*)wParam;
    int iPane = 1;
    ASSERT(indicators[iPane] == ID_STATUS_PANE_INDICATOR);

    CStatusBar* pStatus = (CStatusBar*) GetDescendantWindow (AFX_IDW_STATUS_BAR);
    if (pStatus)  {
        CDC* pDC = pStatus->GetDC();

        if (pszText == nullptr) {
            CMDIFrameWnd* pWnd = (CMDIFrameWnd*)MDIGetActive();
            if(pWnd && pWnd->IsKindOf(RUNTIME_CLASS(CFormChildWnd)) && pWnd->GetSafeHwnd()){
                CFormChildWnd* pFormFrame = (CFormChildWnd*)pWnd;
                CView* pView = pFormFrame->GetActiveView();
                if(pView && pView->GetSafeHwnd() && pView->IsKindOf(RUNTIME_CLASS(CFSourceEditView))){
                    pStatus->SetPaneInfo (iPane, indicators[iPane], SBPS_NORMAL, 100);
                    return 1;
                }
            }
            else if(pWnd && pWnd->IsKindOf(RUNTIME_CLASS(COrderChildWnd)) && pWnd->GetSafeHwnd()){
                pStatus->SetPaneInfo (iPane, indicators[iPane], SBPS_NORMAL, 100);
                return 1;
            }
            pStatus->SetPaneInfo (iPane, indicators[iPane], SBPS_DISABLED, 100);
            pStatus->SetPaneText (iPane, pszText);

        }
        else {
            pDC->SelectObject(pStatus->GetFont()); // 20120301 the text extent isn't correct without this statement
            pStatus->SetPaneInfo (iPane, indicators[iPane], SBPS_NORMAL, pDC->GetTextExtent(pszText, _tcslen(pszText)).cx + 5);
            pStatus->SetPaneText (iPane, pszText);
        }
        pStatus->ReleaseDC (pDC);
    }
    return 1;
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OnIMSASetFocus
//
/////////////////////////////////////////////////////////////////////////////

LRESULT CMainFrame::OnIMSASetFocus (WPARAM /*wParam*/, LPARAM /*lParam*/) {

    CMDlgBar& dlgBar = GetDlgBar();
    dlgBar.SetFocus();
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////
//
//      LRESULT CMainFrame::OnSelChange(WPARAM wParam, LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnSelChange(WPARAM wParam, LPARAM /*lParam*/)
{
    CWnd* pWnd = (CWnd*)wParam;
    if(!pWnd)
        return 0;

    BOOL bProcess = pWnd->IsKindOf(RUNTIME_CLASS(CFSourceEditView)) || pWnd->IsKindOf(RUNTIME_CLASS(COSourceEditView));

    if(!bProcess)
        return 0;


    auto mark_modified = [](auto pID, CAplDoc* pAplDoc)
    {
        if( pID->GetTextSource() != nullptr )
        {
            assert_cast<TextSourceEditable*>(pID->GetTextSource())->SetModified();
        }

        else
        {
            CSourceCode* pSourceCode = pAplDoc->GetAppObject().GetAppSrcCode();
            if(pSourceCode)
                pSourceCode->SetModifiedFlag(true);
        }
    };


    if(pWnd->IsKindOf(RUNTIME_CLASS(CFSourceEditView))){
        CFSourceEditView* pView = (CFSourceEditView*)wParam;
        CFormDoc* pFormDoc = nullptr;

        if(pView->GetDocument()->IsKindOf(RUNTIME_CLASS(CFormDoc))) {
            pFormDoc = (CFormDoc*)pView->GetDocument();
        }

        if(pFormDoc) {
            CAplDoc* pAplDoc = ProcessFOForSrcCode(*pFormDoc);
            if(pAplDoc) {
                CFormID* pID = GetNodeIdForSourceCode<CFormID>();
                if(!pID || pID->GetFormDoc() != pFormDoc )
                    return 0;

                if(pView->GetEditCtrl()->IsModified())
                    mark_modified(pID, pAplDoc);
            }
        }
    }

    else if(pWnd->IsKindOf(RUNTIME_CLASS(COSourceEditView))){
        COSourceEditView* pView = (COSourceEditView*)wParam;
        COrderDoc* pOrderDoc = nullptr;

        if(pView->GetDocument()->IsKindOf(RUNTIME_CLASS(COrderDoc))) {
            pOrderDoc = (COrderDoc*)pView->GetDocument();
        }

        if(pOrderDoc) {
            CAplDoc* pAplDoc = ProcessFOForSrcCode(*pOrderDoc);
            if(pAplDoc) {
                AppTreeNode* app_tree_node = GetNodeIdForSourceCode<AppTreeNode>();
                if(app_tree_node == nullptr || app_tree_node->GetOrderDocument() != pOrderDoc )
                    return 0;

                if(pView->GetEditCtrl()->IsModified())
                    mark_modified(app_tree_node, pAplDoc);
            }
        }
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnEndSession(BOOL bEnding)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnEndSession(BOOL bEnding)
{
    if(bEnding)
        OnClose();
    CMDIFrameWnd::OnEndSession(bEnding);

    // TODO: Add your message handler code here
}


/////////////////////////////////////////////////////////////////////////////////
//
//      LRESULT CMainFrame::OnIsCode(WPARAM wParam, LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnIsCode(WPARAM /*wParam*/, LPARAM lParam)
{
    AppTreeNode* app_tree_node = reinterpret_cast<AppTreeNode*>(lParam);
    COrderDoc* pOrderDoc = ( app_tree_node != nullptr ) ? app_tree_node->GetOrderDocument() : nullptr;
    CAplDoc* pAplDoc = ( pOrderDoc != nullptr ) ? ProcessFOForSrcCode(*pOrderDoc) : nullptr;

    if( pAplDoc == nullptr )
        return 0;

    Application* pApplication = &pAplDoc->GetAppObject();
    CDEFormBase* pBase = app_tree_node->GetFormBase();

    POSITION pos = pOrderDoc->GetFirstViewPosition();
    CView* pOrderView = pOrderDoc->GetNextView(pos);
    ASSERT(pOrderView);

    COSourceEditView* pView = (COSourceEditView*)pOrderView;

    if( pView == nullptr )
        return 0;

    BOOL bAppSrcCode = FALSE;
    if(pOrderDoc && pBase && pBase->IsKindOf(RUNTIME_CLASS(CDEFormFile))) { // if it is a form file show entire source code
        bAppSrcCode = TRUE;
    }

    if(pBase || bAppSrcCode) {
        CString sSymbolName;
        if(pBase && pBase->IsKindOf(RUNTIME_CLASS(CDEField))) {
            sSymbolName = assert_cast<CDEField*>(pBase)->GetItemName();
        }
        else if(pBase && pBase->IsKindOf(RUNTIME_CLASS(CDEGroup))) {
            sSymbolName = assert_cast<CDEGroup*>(pBase)->GetName();
        }
        else if (pBase && pBase->IsKindOf(RUNTIME_CLASS(CDEBlock))) {
            sSymbolName = assert_cast<CDEBlock*>(pBase)->GetName();
        }
        else if(pBase && pBase->IsKindOf(RUNTIME_CLASS(CDELevel))) {
            sSymbolName = assert_cast<CDELevel*>(pBase)->GetName();
        }

        if(sSymbolName.IsEmpty() && !bAppSrcCode)
            return 0;
        CSourceCode* pSourceCode = pApplication->GetAppSrcCode();
        bool bRet = pSourceCode->IsProcAvailable(sSymbolName);
        if(bRet){
            return 1;
        }
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////////
//
//      LRESULT CMainFrame::OnFIsCode(WPARAM wParam, LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnFIsCode(WPARAM /*wParam*/, LPARAM lParam)
{
    CFormID* pFormID = (CFormID*)lParam;
    if(!pFormID)
        return 0;
    CFormDoc* pFormDoc = pFormID->GetFormDoc();

    if(!pFormDoc)
        return 0;

    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pFormID->GetFormDoc());
    if(pAplDoc == nullptr)
        return 0;

    Application* pApplication = &pAplDoc->GetAppObject();
    CDEFormBase* pBase = nullptr;

    eNodeType nType = pFormID->GetItemType();
    if(nType == eFTT_GRIDFIELD){
        CDERoster* pRoster = DYNAMIC_DOWNCAST(CDERoster,pFormID->GetItemPtr());
        ASSERT(pRoster);
        pBase = pRoster->GetCol(pFormID->GetColumnIndex())->GetField(pFormID->GetRosterField());
    }
    else {
        pBase = pFormID->GetItemPtr();
    }


    BOOL bAppSrcCode = FALSE;
    if (pFormDoc && pBase && pBase->IsKindOf(RUNTIME_CLASS(CDEFormFile))) { // if it is a form file show entire source code
        bAppSrcCode = TRUE;
    }

    if(pBase || bAppSrcCode) {
        CString sSymbolName;
        if(pBase && pBase->IsKindOf(RUNTIME_CLASS(CDEField))) {
            sSymbolName = assert_cast<CDEField*>(pBase)->GetItemName();
        }
        else if(pBase && pBase->IsKindOf(RUNTIME_CLASS(CDEGroup))) {
            sSymbolName = assert_cast<CDEGroup*>(pBase)->GetName();
        }
        else if (pBase && pBase->IsKindOf(RUNTIME_CLASS(CDEBlock))) {
            sSymbolName = assert_cast<CDEBlock*>(pBase)->GetName();
        }
        else if(pBase && pBase->IsKindOf(RUNTIME_CLASS(CDELevel))) {
            sSymbolName = assert_cast<CDELevel*>(pBase)->GetName();
        }

        if(sSymbolName.IsEmpty() && !bAppSrcCode)
            return 0;
        CSourceCode* pSourceCode = pApplication->GetAppSrcCode();

        bool bRet = pSourceCode->IsProcAvailable(sSymbolName);
        if(bRet){
            return 1;
        }
    }

    return 0;
}


/////////////////////////////////////////////////////////////////////////////////
//
//      LRESULT CMainFrame::OnUpdateSymbolTblFlag(WPARAM wParam, LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnUpdateSymbolTblFlag(WPARAM wParam, LPARAM /*lParam*/)
{
    CDocument* const pDoc = reinterpret_cast<CDocument*>(wParam);
    ASSERT(pDoc != nullptr);

    ForeachDocument<CAplDoc>(
        [&](CAplDoc& application_document)
        {
            application_document.SetAppObjects(); //Set the objects
            return true;
        });

#ifdef _UNUSED // not sure what the point of all of this was

    //Get the application which has this form as its  form
    CFormDoc* pForm = nullptr;
    COrderDoc* pOrder = nullptr;
    CDDDoc* pDict = nullptr;
    CTabulateDoc* pTabDoc = nullptr;

    if(pDoc->IsKindOf(RUNTIME_CLASS(CFormDoc))) {
        pForm = assert_cast<CFormDoc*>(pDoc);
    }
    else if(pDoc->IsKindOf(RUNTIME_CLASS(COrderDoc))) {
        pOrder = assert_cast<COrderDoc*>(pDoc);
    }
    else if(pDoc->IsKindOf(RUNTIME_CLASS(CDDDoc))) {
        pDict = (CDDDoc*)pDoc;
    }
    else if(pDoc->IsKindOf(RUNTIME_CLASS(CTabulateDoc))) {
        pTabDoc = (CTabulateDoc*)pDoc;
    }

    CCSProApp* pApp = (CCSProApp*)AfxGetApp();
    const CDocTemplate* pTemplate = pApp->GetAppTemplate();

    POSITION pos = pTemplate->GetFirstDocPosition();
    while (pos) {
        bool bFound = false;
        CAplDoc* pAplDoc = (CAplDoc*) pTemplate->GetNextDoc(pos);
        pAplDoc->SetAppObjects(); //Set the objects
        if(pAplDoc->GetEngineAppType() == EngineAppType::Entry) {
            //see if the pForm is same as the applications  form
            for( const auto& pCurFormFile : pAplDoc->GetAppObject().GetRuntimeFormFiles() ) {
                if(pForm) {
                    if(pForm->GetSharedFFSpec() == pCurFormFile){
                        bFound =true;
                        break;
                    }
                }
                else if(pDict) {
                    if(pDict->GetDict() == pCurFormFile->GetDictionary().get()){
                        bFound =true;
                        break;
                    }
                }
            }
        }
        else if(pAplDoc->GetEngineAppType() == EngineAppType::Batch) {
            //see if the pForm is same as the applications  form
            for( const auto& pOrderFile : pAplDoc->GetAppObject().GetRuntimeFormFiles() ) {
                if(pOrder) {
                    if(pOrder->GetSharedOrderSpec() == pOrderFile){
                        bFound =true;
                        break;
                    }
                }
                else if(pDict) {
                    if(pDict->GetDict() == pOrderFile->GetDictionary().get()){
                        bFound =true;
                        break;
                    }
                }
            }
        }
        else if(pAplDoc->GetEngineAppType() == EngineAppType::Tabulation) {
            std::shared_ptr<CTabSet> pTabSet = pAplDoc->GetAppObject().GetTabSpec();
            if(pTabSet == pTabDoc->GetSharedTableSpec()){
                bFound =true;
                break;
            }
        }
        if(pDict && ! bFound) {
            for( const auto& dictionary : pAplDoc->GetAppObject().GetRuntimeExternalDictionaries() )
            {
                if( pDict->GetDict() == dictionary.get() ) {
                    bFound = true;
                    break;
                }
            }
        }
    }
#endif

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::DictMenu
//
/////////////////////////////////////////////////////////////////////////////

HMENU CMainFrame::DictMenu()
{
    m_DictMenu.LoadMenu(IDR_DICT_FRAME);
    m_DictMenu.LoadToolbars(toolbars, 5);

    return(m_DictMenu.Detach());
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::TableMenu
//
/////////////////////////////////////////////////////////////////////////////

HMENU CMainFrame::TableMenu()
{
    m_TableMenu.LoadMenu(IDR_TABLE_FRAME);
    m_TableMenu.LoadToolbars(toolbars, 5);

    return(m_TableMenu.Detach());
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::FormMenu
//
/////////////////////////////////////////////////////////////////////////////

HMENU CMainFrame::FormMenu()
{
    m_FormMenu.LoadMenu(IDR_FORM_FRAME);
    m_FormMenu.LoadToolbars(toolbars, 5);

    return(m_FormMenu.Detach());
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OrderMenu
//
/////////////////////////////////////////////////////////////////////////////

HMENU CMainFrame::OrderMenu()
{
    m_OrderMenu.LoadMenu(IDR_ORDER_FRAME);
    m_OrderMenu.LoadToolbars(toolbars, 5);

    return(m_OrderMenu.Detach());
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::DefaultMenu
//
/////////////////////////////////////////////////////////////////////////////

HMENU CMainFrame::DefaultMenu()
{
    m_DefaultMenu.LoadMenu(IDR_MAINFRAME);
    m_DefaultMenu.LoadToolbar(IDR_MAINFRAME);

    return(m_DefaultMenu.Detach());
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OnMenuChar
//
/////////////////////////////////////////////////////////////////////////////

LRESULT CMainFrame::OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu)
{
    LRESULT lresult;
    if(BCMenu::IsMenu(pMenu)) {
        lresult=BCMenu::FindKeyboardShortcut(nChar, nFlags, pMenu);
    }
    else {
        lresult=CMDIFrameWnd::OnMenuChar(nChar, nFlags, pMenu);
    }
    return(lresult);
}


LRESULT CMainFrame::OnGetDictionaryType(const WPARAM wParam, const LPARAM lParam)
{
    const CDataDict& dictionary = *reinterpret_cast<const CDataDict*>(wParam); // [in] the dictionary object
    DictionaryType& out_dictionary_type = *reinterpret_cast<DictionaryType*>(lParam); // [output] the dictionary type for the dictionary

    std::optional<DictionaryType> dictionary_type;

    ForeachDocument<CAplDoc>(
        [&](const CAplDoc& application_document)
        {
            const DictionaryType this_dictionary_type = application_document.GetAppObject().GetDictionaryType(dictionary);

            // if there are multiple applications using the dictionary, the lowest dictionary type is returned
            if( ( this_dictionary_type != DictionaryType::Unknown ) &&
                ( !dictionary_type.has_value() || static_cast<int>(this_dictionary_type) < static_cast<int>(*dictionary_type) ) )
            {
                dictionary_type = this_dictionary_type;
            }

            return true;
        });

    if( dictionary_type.has_value() )
    {
        out_dictionary_type = *dictionary_type;
        return 1;
    }

    return 0;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  LRESULT CMainFrame::OnDictNameChange(WPARAM wParam, LPARAM lParam)
//  On changing the names of dict/level/recordin the dictionary
/////////////////////////////////////////////////////////////////////////////////

LRESULT CMainFrame::OnDictNameChange(WPARAM wParam, LPARAM /*lParam*/)
{
    CDDDoc* pDDDoc = reinterpret_cast<CDDDoc*>(wParam);
    ASSERT(pDDDoc != nullptr);

    const CDataDict& dictionary = pDDDoc->GetDictionary();

    // do forms
    ForeachDocumentUsingDictionary<CFormDoc>(dictionary,
        [&](CFormDoc& form_doc)
        {
            CDEFormFile* form_file = &form_doc.GetFormFile();

            if( form_file->ReconcileName(dictionary) )
            {
                form_doc.SetModifiedFlag(true);

                // 20120710 reconcile the capi text if necessary

                const DictNamedBase* dict_element = dictionary.GetChangedObject();

                if( dict_element != nullptr && dict_element->GetElementType() == DictElementType::Item )
                {
                    CDEForm* pForm = nullptr;
                    CDEItemBase* pBase = nullptr;
                    form_file->FindField(UTF8_TODO::GetCString(dict_element->GetName()), &pForm, &pBase);

                    CDEField* pField = DYNAMIC_DOWNCAST(CDEField, pBase);

                    if( pField != nullptr )
                    {
                        std::tuple<CDEItemBase*, CString> update(pField, UTF8_TODO::GetCString(dictionary.MakeQualifiedName(dictionary.GetOldName())));
                        SendMessage(WM_IMSA_RECONCILE_QSF_FIELD_NAME, reinterpret_cast<WPARAM>(form_file), reinterpret_cast<LPARAM>(&update));
                    }
                }

                else if( dict_element != nullptr && dict_element->GetElementType() == DictElementType::Dictionary)
                {
                    //dictionary name changed
                    SendMessage(WM_IMSA_RECONCILE_QSF_DICT_NAME, reinterpret_cast<WPARAM>(form_file), reinterpret_cast<LPARAM>(dict_element));
                }
            }

            return true;
        });


    // do orders
    ForeachDocumentUsingDictionary<COrderDoc>(dictionary,
        [&](COrderDoc& order_doc)
        {
            if( order_doc.GetFormFile().ReconcileName(dictionary) )
                order_doc.SetModifiedFlag(true);

            return true;
        });


    // do tables
    ForeachDocumentUsingDictionary<CTabulateDoc>(dictionary,
        [&](CTabulateDoc& tab_doc)
        {
            if( tab_doc.GetTableSpec()->ReconcileName(dictionary) )
                tab_doc.SetModifiedFlag(true);

            return true;
        });

    return 0;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  LRESULT CMainFrame::OnDictValueLabelChange(WPARAM wParam, LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnDictValueLabelChange(WPARAM wParam, LPARAM /*lParam*/)
{
    CDDDoc* pDDDoc = reinterpret_cast<CDDDoc*>(wParam);
    ASSERT(pDDDoc != nullptr);

    ForeachDocumentUsingDictionary<CTabulateDoc>(pDDDoc->GetDictionary(),
        [&](CTabulateDoc& tab_doc)
        {
            if( tab_doc.GetTableSpec()->ReconcileLabel(pDDDoc->GetDictionary()) )
                tab_doc.SetModifiedFlag(true);

            return true;
        });

    return 0;
}


void CMainFrame::OnAbout1()
{
    // CUGExOb exob;exob.CreateUGDialog();
    // 20120524 sorry savy and chris ... but it wasn't working anyway ... i'm only keeping this:
    AfxGetMainWnd()->SetWindowText(_T("CSPro")); // 20110414

}


/////////////////////////////////////////////////////////////////////////////////
//
//  LONG CMainFrame::OnIMSATabConvert(WPARAM /*wParam*/, LPARAM /*lParam*/)
//
/////////////////////////////////////////////////////////////////////////////////
LONG CMainFrame::OnIMSATabConvert(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    //    This function responds to the message WM_IMSA_TABCONVERT, which is sent by other
    //    CSPro modules to invoke a file to be converted from .tab to .tbw.

    /*--- activate ourselves  ---*/
    /* SendMessage(WM_IMSA_SETFOCUS);
    CTabulateDoc* pTabDoc = (CTabulateDoc*)GetActiveDocument();
    pTabDoc->ConvertTABToTBW();*/
    CWnd* pWnd = ((CMainFrame*) AfxGetMainWnd())->GetActiveFrame();
    if (pWnd && pWnd->IsKindOf(RUNTIME_CLASS(CTableChildWnd))) {
        pWnd->SendMessage(WM_IMSA_TABCONVERT);
    }
    else {
        return 0;
    }
    //SAVY &&& TEMP STUFF .
    CTabulateDoc* pTabDoc = (CTabulateDoc*)((CTableChildWnd*)pWnd)->GetActiveDocument();
    CString sPathName = pTabDoc->GetPathName();
    sPathName.ReleaseBuffer();
    PathRemoveFileSpec(sPathName.GetBuffer(MAX_PATH));
    sPathName.ReleaseBuffer();

    CString sPFFName = sPathName + _T("\\CSTab.pff") ;
    CString sAplFile = sPathName + _T("\\CSTab.bch") ;

    CNPifFile pifFile(sPFFName);
    pifFile.SetAppFName(sAplFile);
    CView* pView = ((CTableChildWnd*)pWnd)->GetActiveView();
    if (pView == nullptr) {
        return 0;
    }
    if (pView->IsKindOf(RUNTIME_CLASS(CTabView))) {
        //((CTabView*)pView)->GetGrid()->Update(true);
        //'Cos of area processing we have to do the spect2table stuff
        //after run to tranform from design view to data view
        //so we need to update all not just the data
        ((CTabView*)pView)->GetGrid()->Update();
        ((CTabView*)pView)->GetGrid()->RedrawWindow();
    }
    ///SAVY&&& TEMP STUFF ENDS
    return 0;
}


LRESULT CMainFrame::OnGetApplicationBeingLoaded(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if( m_updateViewsDocument != nullptr &&
        m_updateViewsDocument->IsKindOf(RUNTIME_CLASS(CAplDoc)) )
    {
        return reinterpret_cast<LRESULT>(&assert_cast<CAplDoc*>(m_updateViewsDocument)->GetAppObject());
    }

    return reinterpret_cast<LRESULT>(nullptr);
}


/////////////////////////////////////////////////////////////////////////////////
//
//  LRESULT CMainFrame::OnGetApplication(WPARAM wParam, LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnGetApplication(WPARAM wParam, LPARAM lParam)
{
    Application** ppApplication = reinterpret_cast<Application**>(wParam);
    CDocument* pDoc = reinterpret_cast<CDocument*>(lParam);

    if( pDoc == nullptr )
    {
        CMDIChildWnd* pWnd = this->MDIGetActive();

        if( pWnd != nullptr )
            pDoc = pWnd->GetActiveDocument();

        if( pDoc == nullptr )
            return 0;
    }

    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pDoc);

    if( pAplDoc != nullptr )
    {
        *ppApplication = &pAplDoc->GetAppObject();
        return 1;
    }

    return 0;
}


LRESULT CMainFrame::OnGetFormFileOrDictionary(const WPARAM wParam, LPARAM lParam)
{
    std::variant<std::monostate, std::shared_ptr<const CDEFormFile>, std::shared_ptr<const CDataDict>>& form_file_or_dictionary =
        *reinterpret_cast<std::variant<std::monostate, std::shared_ptr<const CDEFormFile>, std::shared_ptr<const CDataDict>>*>(wParam);
    ASSERT(std::holds_alternative<std::monostate>(form_file_or_dictionary));

    const std::string* const file_path_to_match = reinterpret_cast<const std::string*>(lParam);

    // if not matching a specific file path, return the active form file or dictionary
    if( file_path_to_match == nullptr )
    {
        CMDIChildWnd* const pWnd = MDIGetActive();
        CDocument* const pDoc = ( pWnd != nullptr && pWnd->GetSafeHwnd() != nullptr ) ? pWnd->GetActiveDocument() :
                                                                                        nullptr;

        const FormFileBasedDoc* form_file_based_doc = dynamic_cast<const FormFileBasedDoc*>(pDoc);

        if( form_file_based_doc != nullptr )
        {
            form_file_or_dictionary = form_file_based_doc->GetSharedFormFile();
            ASSERT(std::get<std::shared_ptr<const CDEFormFile>>(form_file_or_dictionary) != nullptr);
            return 1;
        }

        const DictionaryBasedDoc* dictionary_based_doc = dynamic_cast<const DictionaryBasedDoc*>(pDoc);

        if( dictionary_based_doc != nullptr )
        {
            form_file_or_dictionary = dictionary_based_doc->GetSharedDictionary();
            ASSERT(std::get<std::shared_ptr<const CDataDict>>(form_file_or_dictionary) != nullptr);
            return 1;
        }

        return 0;
    }

    // otherwise search all documents for the form file or dictionary
    else
    {
        const std::string extension = PortableFunctions::PathGetFileExtension(*file_path_to_match);

        if( SO::EqualsNoCase(extension, FileExtensions::Dictionary) )
        {
            ForeachDocument<CDDDoc>(
                [&](const CDDDoc& dictionary_doc)
                {
                    if( SO::EqualsNoCase(*file_path_to_match, dictionary_doc.GetPathName()) )
                    {
                        form_file_or_dictionary = dictionary_doc.GetSharedDictionary();
                        return false;
                    }

                    return true;
                });
        }

        else if( SO::EqualsNoCase(extension, FileExtensions::Form) )
        {
            ForeachDocument<CFormDoc>(
                [&](const CFormDoc& form_doc)
                {
                    if( SO::EqualsNoCase(*file_path_to_match, form_doc.GetPathName()) )
                    {
                        form_file_or_dictionary = form_doc.GetSharedFormFile();
                        return false;
                    }

                    return true;
                });
        }

        else
        {
            ASSERT(false);
        }

        return std::holds_alternative<std::monostate>(form_file_or_dictionary) ? 0 : 1;
    }
}


LRESULT CMainFrame::OnCanCodeFileCompilationBeSkipped(const WPARAM wParam, LPARAM /*lParam*/)
{
    const CodeFile* const code_file = reinterpret_cast<const CodeFile*>(wParam);
    ASSERT(code_file != nullptr && code_file->GetFilePath() == code_file->GetTextSource().GetFilePath());

    // as of now, only JavaScript code can be skipped
    if( code_file->IsJavaScript() )
    {
        const TextSource& text_source = code_file->GetTextSource();

        // if the text source is unsaved, always consider that it may not be successfully compiled because
        // the implementation of GetModifiedIteration may not be accurate for unsaved text sources
        if( !text_source.RequiresSave() )
        {
            const auto& lookup = m_codeFileSuccessfulCompilations.find(code_file->GetFilePath());

            if( lookup != m_codeFileSuccessfulCompilations.cend() &&
                std::get<0>(lookup->second) == code_file->GetCodeType() &&
                std::get<1>(lookup->second) == text_source.GetModifiedIteration() )
            {
                return 1;
            }
        }
    }

    return 0;
}


LRESULT CMainFrame::OnSetCodeFileSuccessfullyCompiled(const WPARAM wParam, LPARAM /*lParam*/)
{
    const CodeFile* const code_file = reinterpret_cast<const CodeFile*>(wParam);
    ASSERT(code_file != nullptr);

    m_codeFileSuccessfulCompilations[code_file->GetFilePath()] = std::make_tuple(code_file->GetCodeType(),
                                                                                 code_file->GetTextSource().GetModifiedIteration());

    return 1;
}


LRESULT CMainFrame::OnTokenizeLogic_V0(const WPARAM wParam, const LPARAM lParam)
{
    const SharableString& logic = *reinterpret_cast<const SharableString*>(wParam);
    std::vector<Logic::BasicToken>& basic_tokens = *reinterpret_cast<std::vector<Logic::BasicToken>*>(lParam);

    basic_tokens = Logic::SourceBuffer::Tokenize(logic, LogicSettings::GetOriginalSettings());

    return 1;
}


LRESULT CMainFrame::OnCreateCapiLogicCompiler(const WPARAM wParam, const LPARAM lParam)
{
    std::unique_ptr<DesignerCapiLogicCompiler>& compiler = *reinterpret_cast<std::unique_ptr<DesignerCapiLogicCompiler>*>(wParam);
    Application& application = *reinterpret_cast<Application*>(lParam);

    compiler = std::make_unique<DesignerCapiLogicCompiler>(application);

    return 1;
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

/////////////////////////////////////////////////////////////////////////////////

std::tuple<CAplDoc*, CDEItemBase*> CMainFrame::GetCapiItemDetails(CFormDoc* pFormDoc, CFormID* form_id /* = nullptr*/)
{
    if( pFormDoc != nullptr )
    {
        CAplDoc* pAplDoc = ProcessFOForSrcCode(*pFormDoc);

        if( pAplDoc != nullptr )
        {
            Application* pApplication = &pAplDoc->GetAppObject();
            ASSERT(pApplication->GetEngineAppType() == EngineAppType::Entry);

            CDEItemBase* pBase = nullptr;

            if(form_id == nullptr )
            {
                if( HTREEITEM hItem = m_SizeDlgBar.m_FormTree.GetSelectedItem(); hItem != nullptr )
                    form_id = (CFormID*)m_SizeDlgBar.m_FormTree.GetItemData(hItem);
            }

            if(form_id != nullptr )
            {
                eNodeType nType = form_id->GetItemType();

                if( nType == eFTT_GRIDFIELD )
                {
                    CDERoster* pRoster = DYNAMIC_DOWNCAST(CDERoster, form_id->GetItemPtr());

                    if( pRoster != nullptr )
                        pBase = pRoster->GetCol(form_id->GetColumnIndex())->GetField(form_id->GetRosterField());
                }

                else
                    pBase = DYNAMIC_DOWNCAST(CDEItemBase, form_id->GetItemPtr());
            }

            if( pBase != nullptr && ( pBase->IsKindOf(RUNTIME_CLASS(CDEField)) || pBase->IsKindOf(RUNTIME_CLASS(CDEBlock)) ) )
                return std::make_tuple(pAplDoc, pBase);
        }
    }

    return std::make_tuple(nullptr, nullptr);
}


LRESULT CMainFrame::OnShowCapiText(const WPARAM wParam, LPARAM /*lParam*/)
{
    CFormDoc* const pFormDoc = reinterpret_cast<CFormDoc*>(wParam);
    QSFView* const pQTView = assert_cast<QSFView*>(pFormDoc->GetView(FormViewType::QuestionText));

    if( pQTView == nullptr || !pQTView->IsWindowVisible() )
        return 0;

    CAplDoc* app_doc;
    CDEItemBase* item_base;
    std::tie(app_doc, item_base) = GetCapiItemDetails(pFormDoc);

    SharableString html;

    if( item_base != nullptr )
    {
        ASSERT(app_doc->m_questionManager != nullptr);
        CapiQuestionManager& question_manager = *app_doc->m_questionManager;

        const CapiQuestion* const question = question_manager.GetQuestion(CapiName::Create(item_base));

        if( question != nullptr && !question->GetConditions().empty() )
        {
            // use the currently selected dictionary language where there are multiple languages
            const CDataDict* const dictionary = pFormDoc->GetFormFile().GetDictionary();
            ASSERT(dictionary != nullptr);

            const std::string& language_name = ( dictionary->GetLanguages().size() > 1 ) ? dictionary->GetCurrentLanguage().GetName() :
                                                                                           app_doc->m_questionManager->GetDefaultLanguage().GetName();

            const CapiText* const matched_capi_text = question->GetConditions().front().GetQuestionText(language_name);

            if( matched_capi_text != nullptr )
                html = CreateQuestionTextHtmlPreview(app_doc->GetAppObject(), *matched_capi_text);
        }
    }

    pQTView->SetCapiTextHtml(std::move(html), nullptr);

    return 1;
}


/////////////////////////////////////////////////////////////////////////////////
//
//      LRESULT CMainFrame::OnIsQuestion(WPARAM wParam, LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnIsQuestion(WPARAM /*wParam*/, LPARAM lParam)
{
    CFormID* pFormID = (CFormID*)lParam;
    CFormDoc* pFormDoc = ( pFormID != nullptr ) ? pFormID->GetFormDoc() : nullptr;

    CAplDoc* pAplDoc;
    CDEItemBase* pBase;
    std::tie(pAplDoc, pBase) = GetCapiItemDetails(pFormDoc, pFormID);

    return ( pBase != nullptr && pAplDoc->IsQHAvailable(pBase) ) ? 1 : 0;
}


LRESULT CMainFrame::ProcessLangs(WPARAM wParam, LPARAM lParam)
{
    std::vector<CLangInfo>* pArrInfo = reinterpret_cast<std::vector<CLangInfo>*>(wParam);
    CFormDoc* pFormDoc = (CFormDoc*)lParam;
    ASSERT(pFormDoc);

    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pFormDoc);

    if(pAplDoc == nullptr){
        return 0;
    }
    Application* pApplication = &pAplDoc->GetAppObject();
    ASSERT(pApplication->GetEngineAppType() == EngineAppType::Entry);
    UNREFERENCED_PARAMETER(pApplication);
    pAplDoc->ProcessLangs(*pArrInfo);

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//                      CMainFrame::OnLaunchActiveApp
//
/////////////////////////////////////////////////////////////////////////////

LRESULT CMainFrame::OnLaunchActiveAppAsBch(WPARAM /*wParam*/, LPARAM lParam)
{
    CFormDoc* pForm = (CFormDoc*)lParam;

    //Check Applications which has this form as the main one if
    //there are more than one ask the user for which application to
    //run .If there is only one proceed with it .

    CWnd* pPrevInstance = CWnd::FindWindow(CSPRO_WNDCLASS_ENTRYFRM, nullptr);
    if(pPrevInstance) {
        AfxMessageBox(_T("CSEntry is already running. Please close it before you launch another instance."));
        return 0;
    }

    HANDLE ahEvent = OpenEvent( EVENT_ALL_ACCESS, FALSE, CSPRO_WNDCLASS_BATCHWND);
    if(ahEvent) {
        AfxMessageBox(_T("CSBatch is already running. Please close it before you launch another instance."));
        CloseHandle(ahEvent);
        return 0;
    }


    CAplDoc* const pDoc = GetApplicationUsingFormFile(TC::ToUtf8(pForm->GetPathName()));

    if(pDoc) {
        if (pDoc->AreAplDictsOK()) {            // BMD  28 Jun 00
            if(pDoc->IsAppModified()) {
                //If application is modified set ask the user to save
                CString sMsg;
                sMsg.FormatMessage(IDS_APPMODIFIED, pDoc->GetPathName().GetString());
                if(AfxMessageBox(sMsg,MB_YESNO) != IDYES) {
                    return 0;
                }
                else {
                    pDoc->OnSaveDocument(pDoc->GetPathName());
                }
            }

            CFormNodeID* pNode = pForm->GetFormTreeCtrl()->GetFormNode(pForm);
            BOOL bRun = FALSE;
            CFormChildWnd* pWnd = nullptr;

            if(pNode) {
                if(pForm->GetFormTreeCtrl()->Select(pNode->GetHItem(),TVGN_CARET)){
                    POSITION pos = pForm->GetFirstViewPosition();
                    CView* pView = pForm->GetNextView(pos);
                    ASSERT(pView);
                    pWnd = (CFormChildWnd*)pView->GetParentFrame();
                    if(pWnd->IsFormViewActive()) {
                        pWnd->SendMessage(UWM::Designer::SwitchView, (WPARAM)ViewType::Logic);
                    }
                    if(PutSourceCode(pNode, true))
                        bRun = TRUE;
                    else
                        return 0;
                }
            }

            if(!bRun){
                AfxMessageBox(_T("Please compile your application before you run"));
                return 0;
            }

            if( pWnd != nullptr )
                pWnd->SendMessage(UWM::Designer::SwitchView, (WPARAM)ViewType::Form);

            CopyEnt2Bch(pDoc);

            CString sAppFName = UTF8_TODO::GetCString(PortableFunctions::PathReplaceFilename(UTF8_TODO::GetUtf8(pDoc->GetPathName()), "CSPro_Test.bch"));
            CString sPffFName = PortableFunctions::PathRemoveFileExtensionCS(sAppFName) + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Pff));

            CNPifFile pifFile(sPffFName);

            if( PortableFunctions::FileIsRegular(sPffFName) )
                pifFile.LoadPifFile();

            pifFile.SetAppFName(sAppFName);
            pifFile.SetAppType(BATCH_TYPE);

            bool bSkipStruc = AfxGetApp()->GetProfileInt(_T("Settings"), _T("SkipStructure"), 1) !=0;
            pifFile.SetSkipStructFlag(bSkipStruc);

            bool bChkRanges = AfxGetApp()->GetProfileInt(_T("Settings"), _T("CheckRanges"), 1)!=0;
            pifFile.SetChkRangesFlag(bChkRanges);

            pifFile.Save();

            CSProExecutables::RunProgramOpeningFile(CSProExecutables::Program::CSBatch, CS2WS(sAppFName));
        }
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CMainFrame::CopyEnt2Bch(CAplDoc* pAplDoc)
//
/////////////////////////////////////////////////////////////////////////////////
bool CMainFrame::CopyEnt2Bch(CAplDoc* pAplDoc)
{
    Application application;

    application.SetEngineAppType(EngineAppType::Batch);
    application.SetLogicSettings(pAplDoc->GetAppObject().GetLogicSettings());

    application.SetLabel(pAplDoc->GetAppObject().GetLabel());
    application.SetName(pAplDoc->GetAppObject().GetName());

    // add any code files
    for( const CodeFile& code_file : pAplDoc->GetAppObject().GetCodeFiles() )
        application.AddCodeFile(code_file);

    // add any message files
    for( const AppMessageFile& app_message_file : pAplDoc->GetAppObject().GetMessageFiles() )
        application.AddMessageFile(app_message_file);

    // add any reports
    for( const ReportFile& report_file : pAplDoc->GetAppObject().GetReportFiles() )
        application.AddReport(report_file);

    //Add External dictionary names
    for( const std::string& dictionary_file_path : pAplDoc->GetAppObject().GetExternalDictionaryFilePaths() )
        application.AddExternalDictionary(dictionary_file_path);

    //Copy array of dict desc
    application.SetDictionaryDescriptions(pAplDoc->GetAppObject().GetDictionaryDescriptions());

    //copy the fmf file to ord .
    //set form files to dictionary order as false to have the order files open the spec without reordering
    for( const std::string& form_file_path : pAplDoc->GetAppObject().GetFormFilePaths() ) {
        CFormNodeID* const pID = m_SizeDlgBar.m_FormTree.GetFormNode(form_file_path);
        if (pID != nullptr && pID->GetFormDoc()) {
            pID->GetFormDoc()->GetFormFile().SetDictOrder(false);
            pID->GetFormDoc()->SetModifiedFlag(true);
        }
    }
    pAplDoc->OnSaveDocument(pAplDoc->GetPathName());

    bool bRet = true;

    for(int iIndex = 0; iIndex < static_cast<int>(pAplDoc->GetAppObject().GetFormFilePaths().size()); iIndex++) {
        CString sFormFName = UTF8_TODO::GetCString(pAplDoc->GetAppObject().GetFormFilePaths()[iIndex]);
        CString sOrderFName = sFormFName;

        PathRemoveFileSpec(sOrderFName.GetBuffer(MAX_PATH));
        sOrderFName.ReleaseBuffer();
        CString csPath = sOrderFName;
        if(pAplDoc->GetAppObject().GetFormFilePaths().size() > 1 ) {
            CString sGenName;
            sGenName.Format(_T("CSPro_Test%d.ord"),iIndex);
            sOrderFName = csPath +_T("\\")+ sGenName;
        }
        else {
            sOrderFName = csPath +_T("\\")+ _T("CSPro_Test.ord");
        }

        for( DictionaryDescription& dictionary_description : application.GetDictionaryDescriptions() )
        {
            if( SO::EqualsNoCase(dictionary_description.GetParentFilePath(), sFormFName) )
                dictionary_description.SetParentFilePath(UTF8_TODO::GetUtf8(sOrderFName));
        }

        //copy the  .fmf to .ord
        if(!CopyFile(sFormFName,sOrderFName,FALSE)){
            bRet =false;
        }
        application.AddForm(UTF8_TODO::GetUtf8(sOrderFName));
    }

    //Copy the .ent to .bch
    CString sPath = pAplDoc->GetPathName();
    PathRemoveFileSpec(sPath.GetBuffer(MAX_PATH));
    sPath.ReleaseBuffer();
    sPath = sPath + _T("\\") + _T("CSPro_Test.bch");

    try
    {
        application.Save(sPath);
    }
    catch( const CSProException& )
    {
        bRet = false;
    }

    //reset the form files to dictionary order as true
    for( const std::string& form_file_path : pAplDoc->GetAppObject().GetFormFilePaths() ) {
        CFormNodeID* const pID = m_SizeDlgBar.m_FormTree.GetFormNode(form_file_path);
        if (pID != nullptr && pID->GetFormDoc()) {
            pID->GetFormDoc()->GetFormFile().SetDictOrder(true);
            pID->GetFormDoc()->SetModifiedFlag(true);
        }
    }
    pAplDoc->OnSaveDocument(pAplDoc->GetPathName());
    return bRet;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  LRESULT CMainFrame::UpdateTabSrcCode(WPARAM wParam, LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::UpdateTabSrcCode(WPARAM wParam, LPARAM lParam)
{
    BOOL bForceCompile = static_cast<BOOL>(wParam);
    TableElementTreeNode* table_element_tree_node = reinterpret_cast<TableElementTreeNode*>(lParam);
    ASSERT(table_element_tree_node != nullptr);

    CTabulateDoc* pTabDoc = table_element_tree_node->GetTabDoc();

    if( pTabDoc == nullptr )
        return 0;

    //SAVY& To Take care when one form is used by multiple applications
    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pTabDoc);

    if (pAplDoc && !pAplDoc->m_bIsClosing) {
        bool bCompile = bForceCompile;
        if(!PutTabSourceCode(*table_element_tree_node, bCompile)) {
            POSITION pos = pTabDoc->GetFirstViewPosition();
            CView* pTabView = pTabDoc->GetNextView(pos);
            ASSERT(pTabView);
            UNREFERENCED_PARAMETER(pTabView);
            return -1L;
        }
    }
    else if(pAplDoc == nullptr){
        //SAVY 07/18/2000 no need to convey it to the user
        AfxMessageBox(_T("No Application associated with this tab file"));
        return -1L;
    }

    return 0;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  LRESULT CMainFrame::CheckSyntax4TableLogic(WPARAM wParam, LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::CheckSyntax4TableLogic(WPARAM wParam, LPARAM lParam)
{
    XTABSTMENT_TYPE eXTabStatementType = static_cast<XTABSTMENT_TYPE>(wParam);
    TableElementTreeNode* table_element_tree_node = reinterpret_cast<TableElementTreeNode*>(lParam);
    ASSERT(table_element_tree_node != nullptr);

    CTabulateDoc* pTabDoc = table_element_tree_node->GetTabDoc();

    if( pTabDoc == nullptr )
        return 0;

    //SAVY& To Take care when one form is used by multiple applications
    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pTabDoc);
    if (pAplDoc && !pAplDoc->m_bIsClosing) {
        if(!CheckSyntax4TableLogic(*table_element_tree_node, eXTabStatementType)) {
            return -1L;

        }
        if(pTabDoc->IsModified()){
            pAplDoc->GetAppObject().GetAppSrcCode()->SetModifiedFlag(true);
        }
    }
    else if(pAplDoc == nullptr){
        //SAVY 07/18/2000 no need to convey it to the user
        AfxMessageBox(_T("No Application associated with this tab file"));
        return -1L;
    }
    return 0;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  LRESULT CMainFrame::OnRunTab(WPARAM wParam, LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnRunTab(WPARAM wParam, LPARAM lParam)
{
    CTabulateDoc* pTabDoc = (CTabulateDoc*)lParam;
    int iFlag = (int)wParam;
    ASSERT(iFlag == 1 || iFlag == 2);

    if( pTabDoc == nullptr )
        return 0;

    //SAVY& To Take care when one form is used by multiple applications
    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pTabDoc);

    CString sAplFName = pAplDoc->GetPathName();
    PathRemoveExtension(sAplFName.GetBuffer(_MAX_PATH));
    sAplFName.ReleaseBuffer();
    CString sAplFile = sAplFName + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::TabulationApplication));
    CString sPFFName = sAplFile + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Pff));
    CString sListFile = UTF8_TODO::GetCString(PortableFunctions::PathAppendFileExtension(UTF8_TODO::GetUtf8(sAplFile), FileExtensions::Listing));

    CStringArray arrOldProc;
    bool bReplaceTblCode = false;
    CSourceCode* pSourceCode = pAplDoc->GetAppObject().GetAppSrcCode();

    if (pTabDoc/*->AreAplDictsOK()*/) {            // BMD  28 Jun 00
        TableSpecTabTreeNode* const table_spec_tab_tree_node = pTabDoc->GetTabTreeCtrl()->GetTableSpecTabTreeNode(*pTabDoc);
        BOOL bRun = FALSE;
        if(table_spec_tab_tree_node != nullptr) {
            if(pTabDoc->GetTabTreeCtrl()->Select(table_spec_tab_tree_node->GetHItem(),TVGN_CARET)){
                POSITION pos = pTabDoc->GetFirstViewPosition();
                CView* pView = pTabDoc->GetNextView(pos);
                ASSERT(pView);
                CTableChildWnd* pWnd = (CTableChildWnd*)pView->GetParentFrame();
                pWnd->ActivateFrame();
                //  pWnd->OnViewLogic();
                if(!pWnd->GetSourceView()){
                    pWnd->SendMessage(WM_COMMAND,ID_VIEW_TAB_LOGIC);
                }
                if(PutTabSourceCode(*table_spec_tab_tree_node, false)){//dont compile yet
                    pWnd->SendMessage(WM_COMMAND,ID_VIEW_TABLE);
                    if(!ForceLogicUpdate4Tab(pTabDoc)){//force the logic update
                        return 0;
                    }
                    pWnd->SendMessage(WM_COMMAND,ID_VIEW_TAB_LOGIC);
                    if(PutTabSourceCode(*table_spec_tab_tree_node, true)){//now compile yet
                        bRun = TRUE;
                        pWnd->SendMessage(WM_COMMAND,ID_VIEW_TABLE);
                    }
                    else {
                        return 0;
                    }
                }
                else{
                    return 0;
                }
            }
        }
        if(!ForceLogicUpdate4Tab(pTabDoc)){
            return 0;
        }

        if(!bRun){
            AfxMessageBox(_T("Please compile your application before you run"));
            return 0;
        }
        CString sReconcileSubTblLevels;
        pTabDoc->GetTableSpec()->ConsistencyCheckSubTblNTblLevel(sReconcileSubTblLevels);
    }

    /*if(!ForceLogicUpdate4Tab(pTabDoc)){
        return 0;
    }
    else*/{ //Force Save app logic
        //Remove the logic 4 now in case Generate Logic is false
        CTabSet* pTabSet = pTabDoc->GetTableSpec();
        int iNumTables = pTabSet->GetNumTables();
        //Get Old Proc
        pSourceCode->GetProc(arrOldProc);

        bool bAllTablesExcluded = true;
        for (int iIndex =0 ; iIndex < iNumTables; iIndex++) {
            CTable* pTable = pTabSet->GetTable(iIndex);
            CString sCrossTabStmt;
            if(pTable->IsTableExcluded4Run()){
                pAplDoc->GetAppObject().GetAppSrcCode()->RemoveProc(pTable->GetName(),CSourceCode_AllEvents);
                bReplaceTblCode = true;
            }
            else {
                bAllTablesExcluded = false;
            }

        }
        if(bAllTablesExcluded) {
            AfxMessageBox(_T("All Tables have been excluded from run.\n\nPlease select at least one table to run."));
            return 0;
        }
        if(!bReplaceTblCode){
            arrOldProc.RemoveAll();
        }
        //End code for running table selectively

        pAplDoc->GetAppObject().GetAppSrcCode()->Save();
    }

    if(pAplDoc->IsAppModified()) {
        //If application is modified set ask the user to save
        CString sMsg;
        sMsg.FormatMessage(IDS_APPMODIFIED, pAplDoc->GetPathName().GetString());
        if(AfxMessageBox(sMsg,MB_YESNO) != IDYES) {
            if(bReplaceTblCode){
                pSourceCode->PutProc(arrOldProc);
            }
            return 0;
        }
        else {
            pAplDoc->OnSaveDocument(pAplDoc->GetPathName());
        }
    }

    DeleteFile(sListFile);

    CNPifFile pifFile(sPFFName);
    pifFile.SetAppFName(sAplFile);
    pifFile.SetListingFName(sListFile);

    CString sTabOutPutFName = pifFile.GetTabOutputFName();
    CString sTempTab,sTempTai,sTabTai;
    if (iFlag == 1) { //Run all
        pAplDoc->GetAppObject().SetTabSpec(pTabDoc->GetSharedTableSpec());
        pifFile.SetApplication(pAplDoc->GetSharedAppObject());
        m_eProcess = ALL_STUFF;
        CString sFullPath;
        CFileStatus fStatus;
        if(CFile::GetStatus(sPFFName,fStatus)){
           pifFile.SetPifFileName(sPFFName);
           pifFile.LoadPifFile();
        }

        if(!PreparePieceRun(&pifFile,m_eProcess)){
            if(bReplaceTblCode){
                pSourceCode->PutProc(arrOldProc);
            }
            return 0;
        }
        //Set the inputdata file in the pff
        //      pifFile.SetInputData(sDataFile);
        //SAVY&& for now hard code it engine is not looking at piff input /output args
        sTabOutPutFName = _T("") ;
        if(sTabOutPutFName.IsEmpty()) {
            CString csAplFName = pAplDoc->GetPathName();
            PathRemoveExtension(csAplFName.GetBuffer(_MAX_PATH));
            csAplFName.ReleaseBuffer();
            sTabOutPutFName = csAplFName + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::BinaryTable::Tab));
            sTabTai = csAplFName + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::BinaryTable::TabIndex));
            pifFile.SetTabOutputFName(sTabOutPutFName);
            sTempTab = csAplFName + _T("_precalc") +  UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::BinaryTable::Tab)); // Engine is not looking at the piffile ags
            sTempTai = csAplFName + _T("_precalc") +  UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::BinaryTable::TabIndex));

        }
        if(pifFile.GetCalcInputFNamesArr().empty()) {
            pifFile.GetCalcInputFNamesArr().emplace_back(sTempTab);
        }
        CString sCalcOFName = pifFile.GetCalcOutputFName();
        if(sCalcOFName.IsEmpty()){
            pifFile.SetCalcOutputFName(sTabOutPutFName);
        }
        if(m_eProcess == ALL_STUFF || m_eProcess == CS_PREP){
            pifFile.SetViewListing(ONERROR);
        }
        else {
            pifFile.SetViewListing(ALWAYS);
        }
        pifFile.SetTabProcess(m_eProcess);
        pifFile.SetViewResultsFlag(FALSE);
        pifFile.Save();

    }
    else if (iFlag == 2) {//Run pieces
        pAplDoc->GetAppObject().SetTabSpec(pTabDoc->GetSharedTableSpec());
        pifFile.SetApplication(pAplDoc->GetSharedAppObject());
        m_eProcess  =PROCESS_INVALID;
        if(!PreparePieceRun(&pifFile,m_eProcess)){
            if(bReplaceTblCode){
                pSourceCode->PutProc(arrOldProc);
            }
            return 0;
        }

        if(m_eProcess == ALL_STUFF || m_eProcess == CS_PREP){
            pifFile.SetViewListing(ONERROR);
        }
        else {
            pifFile.SetViewListing(ALWAYS);
        }

        //if(m_eProcess == CS_TAB || m_eProcess == CS_CON){
        pifFile.SetViewResultsFlag(FALSE);
        //}
        pifFile.SetTabProcess(m_eProcess);
        pifFile.Save();
    }

    DoEmulateBCHApp(&pAplDoc->GetAppObject());
    pifFile.SetApplication(pAplDoc->GetSharedAppObject());

    CRunTab runTab;
    runTab.InitRun(&pifFile,&pAplDoc->GetAppObject());

    DeleteFile(pifFile.GetListingFName());
    bool bRet = false;
    {
        EnableWindow(FALSE);
        bRet = runTab.Exec();
        EnableWindow(TRUE);
    }
    if(bRet){
        if(m_eProcess == CS_PREP && pifFile.GetViewResultsFlag()){
            //Here launch .tbw
            CString sTbwFileName;
            sTbwFileName =pifFile.GetPrepOutputFName();
            CFileStatus fStatus;
            BOOL bTBWExists = CFile::GetStatus(sTbwFileName,fStatus);
            if(bTBWExists){
                const std::optional<std::string> tblview_exe =  CSProExecutables::GetExecutablePath(CSProExecutables::Program::TblView);
                if(tblview_exe.has_value()) {
                    IMSASpawnApp(UTF8_TODO::GetWide(*tblview_exe), IMSA_WNDCLASS_TABLEVIEW, sTbwFileName, TRUE);
                }
            }
        }
    }
    if(bReplaceTblCode){
        pSourceCode->PutProc(arrOldProc);
    }
    DoPostRunTabCleanUp(&pifFile);
    if(bRet && (m_eProcess == CS_PREP|| m_eProcess == ALL_STUFF)) {
        CWnd* pWnd = ((CMainFrame*) AfxGetMainWnd())->GetActiveFrame();

        CView* pView = ((CTableChildWnd*)pWnd)->GetActiveView();
        if (pView == nullptr) {
            return 0;
        }
        if (pView->IsKindOf(RUNTIME_CLASS(CTabView))) {
            ((CTableChildWnd*)pView->GetParentFrame())->SetDesignView(false);//force data view
            //((CTabView*)pView)->GetGrid()->Update(true);
            //'Cos of area processing we have to do the spect2table stuff
            //after run to tranform from design view to data view
            //so we need to update all not just the data
            ((CTabView*)pView)->GetGrid()->Update();
            ((CTabView*)pView)->UpdateAreaComboBox(true);
            ((CTabView*)pView)->GetGrid()->RedrawWindow();
        }
    }

    return 1;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CMainFrame::PreparePieceRun(CNPifFile* pPIFFile)
//
/////////////////////////////////////////////////////////////////////////////////
bool CMainFrame::PreparePieceRun(CNPifFile* pPIFFile,PROCESS eProcess)
{
    CRunTab runtab;
    if(runtab.PreparePFF(pPIFFile,eProcess)){
        m_eProcess = eProcess;
    }
    else {
        return false;
    }

    //Then show the appropriate dialog box .

    return true;
}


namespace
{
    std::tuple<bool, CObject*, int> GetTableElementsForCompilation(const TableElementTreeNode& table_element_tree_node)
    {
        bool app_src_code = false;
        CObject* pBase = nullptr;
        int level_num = 0;

        switch( table_element_tree_node.GetTableElementType() )
        {
            case TableElementType::TableSpec:
                app_src_code = true;
                break;

            case TableElementType::Table:
            case TableElementType::RowItem:
            case TableElementType::ColItem:
                pBase = table_element_tree_node.GetTable();
                break;

            case TableElementType::Level:
                pBase = table_element_tree_node.GetLevel();
                level_num = table_element_tree_node.GetLevelNum();
                break;
        }

        return std::make_tuple(app_src_code, pBase, level_num);
    }
}


bool CMainFrame::PutTabSourceCode(const TableElementTreeNode& table_element_tree_node, bool bForceCompile)
{
    CAplDoc* pAplDoc = ProcessFOForSrcCode(*table_element_tree_node.GetTabDoc());
    if(pAplDoc == nullptr)
        return true;

    CTabulateDoc* pTabDoc = table_element_tree_node.GetTabDoc();
    Application* pApplication = &pAplDoc->GetAppObject();

    POSITION pos = pTabDoc->GetFirstViewPosition();
    CView* pTabView = pTabDoc->GetNextView(pos);
    ASSERT(pTabView);
    CTableChildWnd* pWnd = assert_cast<CTableChildWnd*>(pTabView->GetParentFrame());

    CTSourceEditView* pView = pWnd->GetSourceView();

    if( pView == nullptr )
        return true;

    bool bAppSrcCode;
    CObject* pBase;
    int iLevelNum;
    std::tie(bAppSrcCode, pBase, iLevelNum) = GetTableElementsForCompilation(table_element_tree_node);

    if(pBase || bAppSrcCode) {
        CString sSymbolName;
        if(pBase && pBase->IsKindOf(RUNTIME_CLASS(CTable))) {
            sSymbolName = ((CTable*)pBase)->GetName(); // savy && check if this is correct
        }
        else if(pBase && pBase->IsKindOf(RUNTIME_CLASS(CTabLevel))) {
            sSymbolName = UTF8_TODO::GetCString(pTabDoc->GetTableSpec()->GetDict()->GetLevel(iLevelNum).GetName());
        }

        if(sSymbolName.IsEmpty() && !bAppSrcCode)
            return true;
        CSourceCode* pSourceCode = pApplication->GetAppSrcCode();
        pSourceCode->SetOrder(pAplDoc->GetOrder());
        CStringArray arrProcLines;

        CIMSAString sString = UTF8_TODO::GetCString(pView->GetLogicCtrl()->GetText());
        CString sLine;

        // gsf 23-mar-00: make sure GetToken does not strip off leading quote marks
        arrProcLines.SetSize(0,100); //avoid multiple allocs SAVY 09/27/00
        while(!(sLine = sString.GetToken(_T("\n"), nullptr, TRUE)).IsEmpty()) {
            sLine.TrimRight('\r');sLine.TrimLeft('\r');
            arrProcLines.Add(sLine);
        }
        arrProcLines.FreeExtra(); //Free Extra SAVY 09/27/00

        if(sSymbolName.IsEmpty()) {
            pSourceCode->PutProc(arrProcLines);
        }
        else {
            pSourceCode->PutProc(arrProcLines,sSymbolName,CSourceCode_AllEvents); //Get all events for now
        }

        if(true){ //Tabsource can get modified from the interface as well without the edit control mod//(pView->GetEditCtrl()->IsModified()) {
            pSourceCode->SetModifiedFlag(true);
            pView->GetEditCtrl()->SetModified(FALSE);
        }

        //arrProcLines.RemoveAll(); SAVY 09/27/00 Optimize;
        //pSourceCode->GetProc(arrProcLines,sSymbolName); SAVY 09/27/00 Optimize
        pView->GetEditCtrl()->ClearErrorAndWarningMarkers();
        pWnd->GetLogicDialogBar().GetCompilerOutputTabViewPage()->ClearLogicErrors();

        if(!bForceCompile)
            return true;


        // RHF INIC 06/01/2000
        DoEmulateBCHApp(&pAplDoc->GetAppObject());
        CStringArray        arrProcLinesApp;

        if(!bAppSrcCode) {
            CCompiler compiler(pApplication);
            compiler.SetFullCompile( true );
            compiler.SetOptimizeFlowTree(true);
            CString csAppSymb = _T("GLOBAL");

            //Compile the application procedure
            pSourceCode->GetProc( arrProcLinesApp, csAppSymb);
            //Generate CROSSTAB table declaration for all other tables
            for(int iTable =0 ; iTable < pTabDoc->GetTableSpec()->GetNumTables(); iTable++){
                CTable* pCurTable = pTabDoc->GetTableSpec()->GetTable(iTable);
                CString sTableName = pCurTable->GetName();
                if(table_element_tree_node.GetTable() != nullptr && sTableName.CompareNoCase(table_element_tree_node.GetTable()->GetName()) !=0){
                    CString sTableDeclaration;
                    pTabDoc->GetTableSpec()->MakeCrossTabStatement(pCurTable,sTableDeclaration ,XTABSTMENT_BASIC);
                    arrProcLinesApp.Add(sTableDeclaration);
                }
            }

            CCompiler::Result errApp = compiler.Compile(csAppSymb, arrProcLinesApp);

            if(errApp == CCompiler::Result::CantInit || errApp == CCompiler::Result::NoInit) {
                AfxMessageBox(_T("Cannot initialize the compiler"));
                UndoEmulateBCHApp(&pAplDoc->GetAppObject());
                return false;
            }
            if(errApp != CCompiler::Result::NoErrors) {
                AfxMessageBox(_T("Compile Failed. See application procedure"));
                UndoEmulateBCHApp(&pAplDoc->GetAppObject());
                return false;
            }// RHF END 06/01/2000

             // clear any parser messages from the application procedure
            compiler.ClearParserMessages();
        }

        CCompiler compiler(pApplication);
        CCompiler::Result err = CCompiler::Result::NoErrors;

        if(bAppSrcCode) {
            CWaitCursor wait;
            compiler.SetOptimizeFlowTree(true);
            err = compiler.FullCompile(pSourceCode);
        }
        else {
            CWaitCursor wait;
            CStringArray arrOldProc;

            //Get Old Proc
            pSourceCode->GetProc(arrOldProc);

            arrProcLinesApp.Append(arrProcLines);
            pSourceCode->PutProc(arrProcLinesApp);

            compiler.SetFullCompile(false);
            compiler.SetOptimizeFlowTree(true);
            err = compiler.FullCompile(pSourceCode);
            //Replace old proc global
            pSourceCode->PutProc(arrOldProc);
        }


        if(err == CCompiler::Result::CantInit || err == CCompiler::Result::NoInit) {
            AfxMessageBox(_T("Cannot initialize the compiler"));
            UndoEmulateBCHApp(&pAplDoc->GetAppObject());
            return false;
        }

        if( !ProcessParserMessages(pAplDoc, pSourceCode, pView, pWnd->GetLogicDialogBar()) )
            err = CCompiler::Result::SomeErrors;

        UndoEmulateBCHApp(&pAplDoc->GetAppObject());

        return ( err == CCompiler::Result::NoErrors );
    }

    UndoEmulateBCHApp(&pAplDoc->GetAppObject());
    return true;
}



/////////////////////////////////////////////////////////////////////////////////
//
//  LONG CMainFrame::ReplaceLvlProc4Area (UINT, LPARAM)
//
/////////////////////////////////////////////////////////////////////////////////
LONG CMainFrame::ReplaceLvlProc4Area (UINT /*wParam*/, LPARAM lParam)
{
    TBL_PROC_INFO*  pTblProcInfo = (TBL_PROC_INFO*)lParam;
    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pTblProcInfo->pTabDoc);
    if(pAplDoc == nullptr)
        return 1;
    CTabulateDoc* pTabDoc = pTblProcInfo->pTabDoc;
    CTabSet* pTabSet = pTabDoc->GetTableSpec();
    Application* pApplication = &pAplDoc->GetAppObject();
    CSourceCode* pSourceCode = pApplication->GetAppSrcCode();
    if(pTabSet->GetDict()){
        CString sLevelName = UTF8_TODO::GetCString(pTabSet->GetDict()->GetLevel(0).GetName());
        pSourceCode->RemoveProc(sLevelName,CSourceCode_AllEvents);
        CConsolidate* pConsolidate = pTabSet->GetConsolidate();
        CStringArray arrProcLines;
        if(pConsolidate &&  pConsolidate->GetNumAreas() > 0){
            //Remove the level proc for now .To regenerate Break by
            //Think abt code in the LEVEL PROC getting destroyed later
            if(!pSourceCode->IsProcAvailable(sLevelName)){
                //If not Level proc add one
                CStringArray sarrProcLines;
                sarrProcLines.Add(UTF8_TODO::GetCString(pAplDoc->GetAppObject().GetLogicSettings().GetGeneratedCodeTextForTextSource()));
                //Putproc puts the "PROC QUEST"
                pSourceCode->PutProc(sarrProcLines,sLevelName,CSourceCode_AllEvents);
            }
            CString sBreakBy;
            sBreakBy = _T("BREAK BY");
            CStringArray sarrProcLines;
            for(int iArea = 0;  iArea< pConsolidate->GetNumAreas(); iArea++){
                sBreakBy += _T(" ") + pConsolidate->GetArea(iArea)+ _T(",");
            }
            sBreakBy.TrimRight(_T(","));
            sarrProcLines.Add(_T("Preproc"));
            sarrProcLines.Add(sBreakBy + _T(" ;\r\n"));
            pSourceCode->PutProc(sarrProcLines,sLevelName,CSourceCode_PreProc);
            sarrProcLines.RemoveAll();

            ForceLogicUpdate4Tab(pTabDoc); //regenerate table logic when areabreaks change
        }
    }
    return 1;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  LONG CMainFrame::ShowTblSrcCode(WPARAM wParam, LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LONG CMainFrame::ShowTblSrcCode(WPARAM /*wParam*/, LPARAM lParam)
{
    CTabulateDoc* pTabDoc = (CTabulateDoc*)lParam;
    ASSERT(pTabDoc);

    CAplDoc* pDoc = ProcessFOForSrcCode(*pTabDoc);

    if (pDoc) {
        SetTblSourceCode(pDoc);
    }
    else {
        AfxMessageBox(_T("No Application associated with this form file"));
        return -1;
    }

    return 0;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  LONG CMainFrame::ReconcileLinkObj(WPARAM wParam, LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LONG CMainFrame::ReconcileLinkObj(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
#ifndef _LINKING_
    return -1 ;//SAVY&&& revisit this to fix logic problems
#else
    CTabulateDoc* pTabDoc = (CTabulateDoc*)lParam;
    ASSERT(pTabDoc);

    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pTabDoc);
    CArray<CLinkTable*,CLinkTable*> aLinkTables;

    if (pAplDoc) {
        pAplDoc->GetAppObject().SetTabSpec(pTabDoc->GetTableSpec());
        DoEmulateBCHApp(&pAplDoc->GetAppObject());
        CCompiler compiler(&pAplDoc->GetAppObject());
        CErrMsg errMsg;
        CCompiler::Result result = compiler.FullCompile (pAplDoc->GetAppObject().GetAppSrcCode(), &errMsg, true );
        UndoEmulateBCHApp(&pAplDoc->GetAppObject());
        if(result == CCompiler::Result::CantInit || result == CCompiler::Result::NoInit) {
            AfxMessageBox(_T("Cannot initialize the compiler"));
            return -1L;
        }

        compiler.GetLinkTables(aLinkTables);
        CString sMsg;
        if(!pTabDoc->ReconcileLinkTables(aLinkTables,sMsg))
            return -1 ;
    }
    else {
        AfxMessageBox(_T("No Application associated with this form file"));
        return -1;
    }
    return 0;
#endif
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::SetTblSourceCode(CAplDoc* pAplDoc)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::SetTblSourceCode(CAplDoc* pAplDoc)
{
    //return  ;//SAVY&&& revisit this to fix logic problems

    if(pAplDoc == nullptr){
        return;
    }

    Application* pApplication = &pAplDoc->GetAppObject();
    ASSERT(pApplication->GetEngineAppType() == EngineAppType::Tabulation);
    HTREEITEM hItem = m_SizeDlgBar.m_TableTree.GetSelectedItem();
    ASSERT(hItem);

    TableElementTreeNode* table_element_tree_node = m_SizeDlgBar.m_TableTree.GetTreeNode(hItem);
    ASSERT(table_element_tree_node != nullptr);

    bool bAppSrcCode;
    CObject* pBase;
    int iLevelNum;
    std::tie(bAppSrcCode, pBase, iLevelNum) = GetTableElementsForCompilation(*table_element_tree_node);

    CTabulateDoc* pTabDoc = table_element_tree_node->GetTabDoc();
    POSITION pos = pTabDoc->GetFirstViewPosition();
    CView* pTabView = pTabDoc->GetNextView(pos);
    ASSERT(pTabView);

    CTableChildWnd* pWnd = assert_cast<CTableChildWnd*>(pTabView->GetParentFrame());
    CTSourceEditView* pView = pWnd->GetSourceView();

    pAplDoc->GetAppObject().SetTabSpec(pTabDoc->GetSharedTableSpec());

    if(pTabDoc && !pBase) { // if it is a form file show entire source code
        bAppSrcCode = true;
    }
    if(pBase || bAppSrcCode) {
        CString sSymbolName;
        if(pBase && pBase->IsKindOf(RUNTIME_CLASS(CTable))) {
            sSymbolName = ((CTable*)pBase)->GetName(); // savy && check if this is correct
        }
        else if(pBase && pBase->IsKindOf(RUNTIME_CLASS(CTabLevel))) {
            sSymbolName = UTF8_TODO::GetCString(pTabDoc->GetTableSpec()->GetDict()->GetLevel(iLevelNum).GetName());
        }

        CSourceCode* pSourceCode = pApplication->GetAppSrcCode();
        CStringArray arrProcLines;

        pSourceCode->GetProc(arrProcLines,sSymbolName,CSourceCode_AllEvents); //Get all events for now

        //Get the view and set the text with the proclines
        CString sText;
        if(sSymbolName.IsEmpty()){
            if(!pSourceCode->IsProcAvailable(_T("GLOBAL"))){
                sText = _T("PROC GLOBAL\r\n\r\n");
            }
            if(!pSourceCode->IsProcAvailable(pTabDoc->GetTableSpec()->GetName())){
            //        sText += "PROC "+ pTabDoc->GetTableSpec()->GetName() + "\r\n\r\n";
                //SAVY&&& 10/29 engine does not compile with tabspec symbol in the .app report this bug
                //So we are switching off this generation for now
            }
        }
        CString sWindowText;
        //Get the total memory to allocate
        UINT uAlloc = 0;
        int iNumLines = arrProcLines.GetSize();
        for(int iIndex =0; iIndex <iNumLines ;iIndex++){
            uAlloc  +=  arrProcLines.ElementAt(iIndex).GetLength();
            uAlloc += 2; // for the "\r\n"
        }
        uAlloc++; //for the "\0" @ the end

        uAlloc += sText.GetLength(); //U need to allocate this extra length for the text that is to be appended;
        LPTSTR pString = sWindowText.GetBufferSetLength(uAlloc);
        _tmemset(pString ,_T('\0'),uAlloc);

        for (int iIndex = 0 ; iIndex < iNumLines ; iIndex++) {
            if(!sText.IsEmpty()){
                CString sLine = arrProcLines[iIndex];
                sLine.Trim();
                if(sLine.Mid(0,4).CompareNoCase(_T("PROC"))==0){
                    //sWindowText += sText;
                    int iLength = sText.GetLength();
                    _tmemcpy(pString,sText.GetBuffer(iLength),iLength);
                    pString += iLength;
                    sText.ReleaseBuffer();
                    sText =_T("");
                }
            }
            CString& csLine = arrProcLines[iIndex];
            int iLength = csLine.GetLength();
            _tmemcpy(pString,csLine.GetBuffer(iLength),iLength);
            pString += iLength;
            csLine.ReleaseBuffer();
            _tmemcpy(pString,_T("\r\n"),2);
            pString += 2;
        }
        sWindowText.ReleaseBuffer();
        if(sWindowText.IsEmpty() && !sSymbolName.IsEmpty()) {
            sWindowText = _T("PROC ") +  sSymbolName;
            sWindowText += _T("\r\n");
        }
        if(!sText.IsEmpty()){
            sWindowText += sText;
        }

        // 20100316 nothing is changed by just loading or changing what proc is displayed
        bool modFlag1 = pTabDoc->IsModified();
        bool modFlag2 = pView->GetEditCtrl()->IsModified();
        pView->GetEditCtrl()->SetText(UTF8_TODO::GetUtf8(sWindowText));

        pTabDoc->SetModifiedFlag(modFlag1); // 20100316
        pView->GetEditCtrl()->SetModified(modFlag2);

        pWnd->GetLogicDialogBar().UpdateScrollState();
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CMainFrame::DoEmulateBCHApp(Application* pApp)
//
/////////////////////////////////////////////////////////////////////////////////
bool CMainFrame::DoEmulateBCHApp(Application* pApp)
{ // called on a tab app
    auto pDict = pApp->GetTabSpec()->GetSharedDictionary();

    EngineAppType appType = pApp->GetEngineAppType();
    ASSERT(appType == EngineAppType::Tabulation); // call only for TAB_TYPE
    UNREFERENCED_PARAMETER(appType);
    EngineAppType orderType = EngineAppType::Batch;
    pApp->SetEngineAppType(orderType);

    auto pOrder = std::make_shared<CDEFormFile>();
    pOrder->CreateOrderFile(*pDict, true);
    pOrder->SetName(pApp->GetTabSpec()->GetName());
    pOrder->SetDictionary(pDict);
    pApp->AddRuntimeFormFile(pOrder);
    pOrder->UpdatePointers();

    DictionaryDescription dictionary_description(pApp->GetTabSpec()->GetDict()->GetFilePath(), pOrder->GetFilePath(), DictionaryType::Input);
    dictionary_description.SetDictionary(pApp->GetTabSpec()->GetDict());
    pApp->GetDictionaryDescriptions().insert(pApp->GetDictionaryDescriptions().cbegin(), std::move(dictionary_description));

    return true;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CMainFrame::UndoEmulateBCHApp(Application* pApp)
//
/////////////////////////////////////////////////////////////////////////////////
bool CMainFrame::UndoEmulateBCHApp(Application* pApp)
{ // called on a tab app
    if(pApp->GetRuntimeFormFiles().size() == 1 ) {
        pApp->GetRuntimeFormFiles().clear();
    }

    EngineAppType appType = EngineAppType::Tabulation;
    pApp->SetEngineAppType(appType);

    pApp->GetDictionaryDescriptions().erase(pApp->GetDictionaryDescriptions().begin());

    //Delete Temporary files
#ifndef _PROCESSTHIS
    CString sPathName = UTF8_TODO::GetCString(pApp->GetApplicationFilePath());

    sPathName.ReleaseBuffer();
    PathRemoveFileSpec(sPathName.GetBuffer(MAX_PATH));
    sPathName.ReleaseBuffer();


    CString sFile = sPathName + _T("\\CSTab.ord") ; //delete ord
    DeleteFile(sFile);
    sFile = sPathName + _T("\\CSTab") + L"." + UTF8_TODO::GetCString(FileExtensions::Logic); //delete app
    DeleteFile(sFile);
    sFile = sPathName + _T("\\CSTab.pff") ; //delete pff
    DeleteFile(sFile);
    sFile = sPathName + _T("\\CSTab.bch") ; //delete bch
    DeleteFile(sFile);
    sFile = sPathName + _T("\\CSTab.lst") ; //delete lst
    DeleteFile(sFile);
    sFile = sPathName + _T("\\CSTab.mgf") ; //delete mgf
    DeleteFile(sFile);
    sFile = sPathName + _T("\\CSTab.err") ; //delete err
    DeleteFile(sFile);
    //End delete temp files
#endif
    return true;

}


/////////////////////////////////////////////////////////////////////////////////
//
//  LONG CMainFrame::PutTallyProc (UINT wParam, LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LONG CMainFrame::PutTallyProc(UINT /*wParam*/, LPARAM lParam)
{
    //SAVY&&& revisit this stuff later
    TBL_PROC_INFO*  pTblProcInfo = (TBL_PROC_INFO*)lParam;
    if(pTblProcInfo && pTblProcInfo->pTabDoc){
        ForceLogicUpdate4Tab(pTblProcInfo->pTabDoc);
    }
    return 1;
}


LONG CMainFrame::RenameProc(UINT /*wParam*/, LPARAM lParam)
{
    TBL_PROC_INFO*  pTblProcInfo = (TBL_PROC_INFO*)lParam;
    if(pTblProcInfo && pTblProcInfo->pTabDoc){
        CTabulateDoc* pTabDoc =pTblProcInfo->pTabDoc;

        CAplDoc* pAplDoc = ProcessFOForSrcCode(*pTabDoc);
        if(pAplDoc) {
            CSourceCode* pSourceCode = pAplDoc->GetAppObject().GetAppSrcCode();
            CStringArray arrProcLines;
            CTable* pTable = pTblProcInfo->pTable;
            CString sOldSymbolName = pTblProcInfo->sTblLogic; //stored in here
            pSourceCode->GetProc(arrProcLines,sOldSymbolName,CSourceCode_AllEvents); //Get all events for now
            pSourceCode->RemoveProc(sOldSymbolName,CSourceCode_AllEvents);
            for(int iIndex =0; iIndex < arrProcLines.GetSize(); iIndex++){
                CString sLine = arrProcLines[iIndex];
                CIMSAString sTemp(sLine);
                CString sWord;
                while (sTemp.GetLength() > 0) {
                    sWord=sTemp.GetToken();
                    if (sWord.CompareNoCase(sOldSymbolName)==0) {
                        sLine.Replace(sWord,pTable->GetName());
                    }
                }
                arrProcLines[iIndex] =sLine;
            }
            pSourceCode->PutProc(arrProcLines,pTable->GetName(),CSourceCode_AllEvents);
        }
    }

    return 1;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CMainFrame::GetLinkTables(CAplDoc* pAplDoc, CArray<CLinkTable*,CLinkTable*>& aLinkTables , bool bSilent /*=true*/)
//
/////////////////////////////////////////////////////////////////////////////////
bool CMainFrame::GetLinkTables(CTabulateDoc* /*pTabDoc*/, CArray<CLinkTable*,CLinkTable*>& /*aLinkTables */, bool /*bSilent*/ /*=true*/)
{
    bool bRet = false;
#ifdef _LINKING_
    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pTabDoc);

    if (pAplDoc) {
        pAplDoc->GetAppObject().SetTabSpec(pTabDoc->GetTableSpec());
        DoEmulateBCHApp(&pAplDoc->GetAppObject());
        CCompiler compiler(&pAplDoc->GetAppObject());
        CCompiler::Result result = compiler.FullCompile(pAplDoc->GetAppObject().GetAppSrcCode(), true);
        UndoEmulateBCHApp(&pAplDoc->GetAppObject());
        if(result == CCompiler::Result::CantInit || result == CCompiler::Result::NoInit) {
            if(!bSilent) {
                AfxMessageBox(_T("Cannot initialize the compiler"));
            }
            return false;
        }
        bRet = true;

        compiler.GetLinkTables(aLinkTables);
    }
#endif
    return bRet;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  LONG CMainFrame::DeleteTblLogic (UINT wParam, LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LONG CMainFrame::DeleteTblLogic (UINT /*wParam*/, LPARAM lParam)
{
    TBL_PROC_INFO*  pTblProcInfo = (TBL_PROC_INFO*)lParam;
    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pTblProcInfo->pTabDoc);
    if(pAplDoc == nullptr)
        return 1;

    Application* pApplication = &pAplDoc->GetAppObject();
    CSourceCode* pSourceCode = pApplication->GetAppSrcCode();
    pSourceCode->RemoveProc(pTblProcInfo->pTable->GetName());

    return 1;
}


bool CMainFrame::ForceLogicUpdate4Tab(CTabulateDoc* pTabDoc)
{
    bool bRet = false;

    int iIndex =0;
    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pTabDoc);
    if(pAplDoc) {
        CSourceCode* pSourceCode = pAplDoc->GetAppObject().GetAppSrcCode();
        CTabSet* pTabSet = pTabDoc->GetTableSpec();
        int iNumTables = pTabSet->GetNumTables();

        //Put the break by statement in the preproc of level1 . With out this
        // the breaks dont come out right
        if(pTabSet->GetDict() && pTabSet->GetDict()->GetNumLevels() > 0 ){
            CString sLevelName = UTF8_TODO::GetCString(pTabSet->GetDict()->GetLevel(0).GetName());

            CConsolidate* pConsolidate = pTabSet->GetConsolidate();
            if(pConsolidate &&  pConsolidate->GetNumAreas() > 0){
                //Remove the level proc for now .To regenerate Break by
                //Think abt code in the LEVEL PROC getting destroyed later
                if(!pSourceCode->IsProcAvailable(sLevelName)){
                    //If not Level proc add one
                    CStringArray arrProcLines;
                    arrProcLines.Add(UTF8_TODO::GetCString(pAplDoc->GetAppObject().GetLogicSettings().GetGeneratedCodeTextForTextSource()));
                    //Putproc puts the "PROC QUEST"
                    pSourceCode->PutProc(arrProcLines,sLevelName,CSourceCode_AllEvents);
                }
                CString sBreakBy;
                sBreakBy = _T("BREAK  BY");
                CStringArray arrProcLines;
                for(int iArea = 0;  iArea< pConsolidate->GetNumAreas(); iArea++){
                    sBreakBy += _T(" ") + pConsolidate->GetArea(iArea)+ _T(",");
                }
                sBreakBy.TrimRight(_T(","));
                arrProcLines.Add(_T("Preproc"));
                arrProcLines.Add(sBreakBy + _T(" ; \r\n"));
                pSourceCode->PutProc(arrProcLines,sLevelName,CSourceCode_PreProc);
                arrProcLines.RemoveAll();
            }
        }

        for (iIndex =0 ; iIndex < iNumTables; iIndex++) {
            CTable* pTable = pTabSet->GetTable(iIndex);
            CIMSAString sCrossTabStmt;
            if(!pTable->GetGenerateLogic()){
                bRet = true;
                continue;
            }
            pSourceCode->RemoveProc(pTable->GetName(),CSourceCode_AllEvents);
            pTabDoc->MakeCrossTabStatement(pTable,sCrossTabStmt);


            if(!sCrossTabStmt.IsEmpty()){
                CStringArray arrProcLines;
                arrProcLines.Add(UTF8_TODO::GetCString(pAplDoc->GetAppObject().GetLogicSettings().GetGeneratedCodeTextForTextSource()));
                pSourceCode->PutProc(arrProcLines,pTable->GetName(),CSourceCode_AllEvents);

                arrProcLines.RemoveAll();

                CString sLine;
                // gsf 23-mar-00: make sure GetToken does not strip off leading quote marks
                arrProcLines.SetSize(0,100); //avoid multiple allocs SAVY 09/27/00
                while(!(sLine = sCrossTabStmt.GetToken(_T("\n"), nullptr, TRUE)).IsEmpty()) {
                    sLine.TrimRight('\r');sLine.TrimLeft('\r');
                    arrProcLines.Add(sLine);
                }
                arrProcLines.Add(_T(""));
                arrProcLines.FreeExtra(); //Free Extra SAVY 09/27/00
                pSourceCode->PutProc(arrProcLines,pTable->GetName(),CSourceCode_Tally);

                //put postcalc
                if(pTable->GetPostCalcLogic().GetSize() > 0){
                    arrProcLines.RemoveAll();
                    arrProcLines.Append(pTable->GetPostCalcLogic());
                    arrProcLines.Add(_T(""));
                    arrProcLines.FreeExtra();
                    pSourceCode->PutProc(pTable->GetPostCalcLogic(),pTable->GetName(),CSourceCode_PostCalc);
                }

                bRet = true;
            }

        }
    }
    return bRet;
}


bool CMainFrame::CheckSyntax4TableLogic(const TableElementTreeNode& table_element_tree_node, XTABSTMENT_TYPE eXTabStatementType)
{
    CTabulateDoc* pTabDoc = table_element_tree_node.GetTabDoc();
    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pTabDoc);
    if(pAplDoc == nullptr)
        return true;

    CTable* pTable = ( table_element_tree_node.GetTableElementType() == TableElementType::Table ||
                       table_element_tree_node.GetTableElementType() == TableElementType::RowItem ||
                       table_element_tree_node.GetTableElementType() == TableElementType::ColItem ) ? table_element_tree_node.GetTable() :
                                                                                                      nullptr;
    ASSERT(pTable != nullptr);

    Application* pApplication = &pAplDoc->GetAppObject();
    const bool bAppSrcCode = false;

    if(pTable) {
        CString sSymbolName = pTable->GetName();
        CSourceCode* pSourceCode = pApplication->GetAppSrcCode();
        pSourceCode->SetOrder(pAplDoc->GetOrder());
        bool bGenLogic = pTable->GetGenerateLogic();
        CIMSAString sCrossTabStatement;

        pTabDoc->GetTableSpec()->MakeCrossTabStatement(pTable,sCrossTabStatement,eXTabStatementType);
        CStringArray arrProcLines;
        CString sLine;
        // gsf 23-mar-00: make sure GetToken does not strip off leading quote marks
        arrProcLines.SetSize(0,100); //avoid multiple allocs SAVY 09/27/00
        while(!(sLine = sCrossTabStatement.GetToken(_T("\n"), nullptr, TRUE)).IsEmpty()) {
            sLine.TrimRight('\r');sLine.TrimLeft('\r');
            arrProcLines.Add(sLine);
        }
        arrProcLines.FreeExtra(); //Free Extra SAVY 09/27/00

        if(sSymbolName.IsEmpty()) {
            ASSERT(FALSE);
            // pSourceCode->PutProc(arrProcLines);
        }
        else if (bGenLogic){
            CStringArray arrTempProcLines;
            pSourceCode->RemoveProc(sSymbolName,CSourceCode_AllEvents);
            arrTempProcLines.Add(UTF8_TODO::GetCString(pAplDoc->GetAppObject().GetLogicSettings().GetGeneratedCodeTextForTextSource()));

            pSourceCode->PutProc(arrTempProcLines,sSymbolName,CSourceCode_AllEvents);
            pSourceCode->PutProc(arrProcLines,sSymbolName,CSourceCode_Tally); //Get all events for now

            if(eXTabStatementType == XTABSTMENT_POSTCALC_ONLY || eXTabStatementType == XTABSTMENT_ALL){
                if(pTable->GetPostCalcLogic().GetSize() > 0){
                    arrProcLines.RemoveAll();
                    arrProcLines.Append(pTable->GetPostCalcLogic());
                    arrProcLines.Add(_T(""));
                    arrProcLines.FreeExtra();
                    pSourceCode->PutProc(pTable->GetPostCalcLogic(),pTable->GetName(),CSourceCode_PostCalc);
                }
            }
        }

        // RHF INIC 06/01/2000
        pAplDoc->GetAppObject().SetTabSpec(pTabDoc->GetSharedTableSpec());
        DoEmulateBCHApp(&pAplDoc->GetAppObject());
        CStringArray        arrProcLinesApp;
        if(!bAppSrcCode) {
            CCompiler compiler(pApplication);
            compiler.SetFullCompile( true );

            CString csAppSymb = _T("GLOBAL");

            //Compile the application procedure
            pSourceCode->GetProc( arrProcLinesApp, csAppSymb);
            if(!pSourceCode->IsProcAvailable (csAppSymb)){
                arrProcLinesApp.Add(_T("PROC GLOBAL"));//add a blank line to force the engine to build the symbol table
                arrProcLinesApp.Add(UTF8_TODO::GetCString(pAplDoc->GetAppObject().GetLogicSettings().GetGeneratedCodeTextForTextSource()));
            }
            //Generate CROSSTAB table declaration for all other tables
            for(int iTable =0 ; iTable < pTabDoc->GetTableSpec()->GetNumTables(); iTable++){
                CTable* pCurTable = pTabDoc->GetTableSpec()->GetTable(iTable);
                CString sTableName = pCurTable->GetName();
                if(sTableName.CompareNoCase(pTable->GetName()) !=0){
                    CString sTableDeclaration;
                    pTabDoc->GetTableSpec()->MakeCrossTabStatement(pCurTable,sTableDeclaration ,XTABSTMENT_BASIC);
                    arrProcLinesApp.Add(sTableDeclaration);
                }
            }
            compiler.SetOptimizeFlowTree(true);

            CCompiler::Result errApp = compiler.Compile(csAppSymb, arrProcLinesApp);
            if(errApp == CCompiler::Result::CantInit || errApp == CCompiler::Result::NoInit) {
                AfxMessageBox(_T("Cannot initialize the compiler"));
                UndoEmulateBCHApp(&pAplDoc->GetAppObject());
                return false;
            }
            if(errApp != CCompiler::Result::NoErrors) {
                AfxMessageBox(_T("Compile Failed. See application procedure"));
                UndoEmulateBCHApp(&pAplDoc->GetAppObject());
                return false;
            }// RHF END 06/01/2000
        }
        CCompiler compiler(pApplication);
        compiler.SetOptimizeFlowTree(true);
        CCompiler::Result err=CCompiler::Result::CantInit;
        if(bAppSrcCode) {
            ASSERT(FALSE);
        }
        else {//compile only just the table code
            CWaitCursor wait;
            //Begin
            CStringArray arrOldProc;
            arrProcLines.RemoveAll();
            //Get Old Proc
            pSourceCode->GetProc(arrOldProc);
            pSourceCode->GetProc(arrProcLines,sSymbolName);
            arrProcLinesApp.Append(arrProcLines);
            pSourceCode->PutProc(arrProcLinesApp);

            compiler.SetFullCompile(false);
            err = compiler.FullCompile(pSourceCode);

            //Replace old proc global
            pSourceCode->PutProc(arrOldProc);
            //pSourceCode->PutProc(arrProcLines);

            //Done
        }
        if(err == CCompiler::Result::CantInit || err == CCompiler::Result::NoInit) {
            AfxMessageBox(_T("Cannot initialize the compiler"));
            UndoEmulateBCHApp(&pAplDoc->GetAppObject());
            return false;
        }

        if( err != CCompiler::Result::NoErrors )
        {
            std::string all_error_messages;

            for( const Logic::ParserMessage& parser_message : CCompiler::GetCurrentSession()->GetParserMessages() )
            {
                if( parser_message.type == Logic::ParserMessage::Type::Error )
                    SO::AppendWithSeparator(all_error_messages, parser_message.message_text, "\r\n");
            }

            ASSERT(!all_error_messages.empty());

            pTabDoc->SetErrorString(std::move(all_error_messages));

            UndoEmulateBCHApp(&pAplDoc->GetAppObject());
            return false;
        }

        else {
            UndoEmulateBCHApp(&pAplDoc->GetAppObject());
            return true;
        }
    }

    UndoEmulateBCHApp(&pAplDoc->GetAppObject());
    return true;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CMainFrame::OnUpdateAreaComboBox(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateAreaComboBox(CCmdUI* pCmdUI)
{
    CTableChildWnd* pActTabFrame = DYNAMIC_DOWNCAST(CTableChildWnd, GetActiveFrame());
    if (pActTabFrame == nullptr) {
        return; // not a tab frame so toolbar will not show up anyway
    }

    CComboBox* pCombo = (CComboBox*) m_pWndTabTBar->GetDlgItem(ID_AREA_COMBO);
    if (pCombo == nullptr) {
        return; // haven't created toolbar yet
    }

    bool bTabView = pActTabFrame->IsTabViewActive();
    bool bDesign = pActTabFrame->IsDesignView();
    bool bHasArea = pActTabFrame->GetTabView()->GetGrid()->HasArea();
    bool bShowCombo = (bTabView && !bDesign && bHasArea);

    if (bShowCombo) {
        if (!pCombo->IsWindowVisible()) {
            pCombo->ShowWindow(SW_SHOW);
        }
        pCmdUI->Enable(true); // need to check for presence of %areaname% in columns here
    }
    else {
        if (pCombo->IsWindowVisible()) {
            pCombo->ShowWindow(SW_HIDE);
        }
        pCmdUI->Enable(false);
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CMainFrame::OnUpdateZoomComboBox(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateZoomComboBox(CCmdUI* pCmdUI)
{
    CTableChildWnd* pActTabFrame = DYNAMIC_DOWNCAST(CTableChildWnd, GetActiveFrame());

    if ( pActTabFrame == nullptr ) {
        return; // not a tab frame so toolbar will not show up anyway
    }

    CComboBox* pCombo = (CComboBox*) m_pWndTabTBar->GetDlgItem(ID_TAB_ZOOM_COMBO);
    if (pCombo == nullptr) {
        return; // haven't created toolbar yet
    }

    bool bShowCombo = (!pActTabFrame->IsTabViewActive() && !pActTabFrame->IsLogicViewActive());

    if (bShowCombo) {
        if (!pCombo->IsWindowVisible()) {
            pCombo->ShowWindow(SW_SHOW);

            // show zoom buttons and separator on toolbar
            m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_VIEW_ZOOM_IN, FALSE);
            m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_VIEW_ZOOM_OUT, FALSE);
            m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_AREA_COMBO, FALSE);
            m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_PRINTVIEW_CLOSE, FALSE);
            m_printViewCloseButton.ShowWindow(SW_SHOW);
        }
        pCmdUI->Enable(true);
    }
    else {
        if (pCombo->IsWindowVisible()) {
            pCombo->ShowWindow(SW_HIDE);
            // hide magnify button and separator on toolbar
            m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_VIEW_ZOOM_IN, TRUE);
            m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_VIEW_ZOOM_OUT, TRUE);
            m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_AREA_COMBO, TRUE);
            m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_PRINTVIEW_CLOSE, TRUE);
            m_printViewCloseButton.ShowWindow(SW_HIDE);
        }
        pCmdUI->Enable(false);
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CMainFrame::DoPostRunTabCleanUp(CNPifFile* pPifFile)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::DoPostRunTabCleanUp(CNPifFile* pPifFile)
{
    if(pPifFile->GetApplication()){
        UndoEmulateBCHApp(pPifFile->GetApplication());
    }
    return;
}


LRESULT CMainFrame::OnReconcileQsfFieldName(WPARAM wParam, LPARAM lParam) // 20120710 ... modify the field name for qsf text to match the new item name
{
    auto form_file = reinterpret_cast<CDEFormFile*>(wParam);
    auto update = reinterpret_cast<std::tuple<CDEItemBase*, CString>*>(lParam);
    ASSERT(form_file != nullptr && update != nullptr);

    const CDEItemBase* pItem = std::get<0>(*update); // block or field
    const CString& old_name = std::get<1>(*update);

    ForeachApplicationDocumentUsingFormFile(*form_file,
        [&](CAplDoc& application_doc)
        {
            ASSERT(application_doc.GetEngineAppType() == EngineAppType::Entry);
            application_doc.ChangeCapiName(pItem, UTF8_TODO::GetUtf8(old_name));
            return true;
        });

    return 0;
}

LRESULT CMainFrame::OnReconcileQsfDictName(WPARAM wParam, LPARAM lParam)
{
    auto form_file = reinterpret_cast<CDEFormFile*>(wParam);
    auto dictionary = reinterpret_cast<CDataDict*>(lParam);
    ASSERT(form_file != nullptr && dictionary != nullptr);

    ForeachApplicationDocumentUsingFormFile(*form_file,
        [&](CAplDoc& application_doc)
        {
            ASSERT(application_doc.GetEngineAppType() == EngineAppType::Entry);
            application_doc.ChangeCapiDictName(*dictionary);
            return true;
        });

    return 0;
}


void CMainFrame::OnUpdateFrameTitle(BOOL /*bAddToTitle*/)
{
    SetWindowText(m_csWindowText);
}


LRESULT CMainFrame::OnGetApplicationPff(WPARAM wParam, LPARAM /*lParam*/)
{
    UWM::Dictionary::GetApplicationPffParameters& parameters = *reinterpret_cast<UWM::Dictionary::GetApplicationPffParameters*>(wParam);
    bool found = false;

    ForeachDocument<CAplDoc>(
        [&](CAplDoc& application_doc)
        {
            // check that a PFF exists for the application
            CString pff_filename = PortableFunctions::PathRemoveFileExtensionCS(application_doc.GetPathName()) + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Pff));

            if( PortableFunctions::FileIsRegular(pff_filename) )
            {
                parameters.pff = std::make_unique<CNPifFile>(pff_filename);

                if( parameters.pff->LoadPifFile() )
                {
                    parameters.is_input_dictionary = ( application_doc.GetAppObject().GetDictionaryType(parameters.dictionary) == DictionaryType::Input );
                    found = true;
                }
            }

            return !found;
        });

    return found ? 1 : 0;
}


void CMainFrame::OnUpdateIfApplicationIsAvailable(CCmdUI* pCmdUI)
{
    CMDIChildWnd* pWnd = this->MDIGetActive();
    CDocument* const pDoc = pWnd->GetActiveDocument();
    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pDoc);
    pCmdUI->Enable(( pAplDoc == nullptr ) ? FALSE : TRUE);
}


void CMainFrame::OnOptionsProperties()
{
    CMDIChildWnd* pWnd = this->MDIGetActive();
    CDocument* const pDoc = pWnd->GetActiveDocument();
    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pDoc);
    Application& application = pAplDoc->GetAppObject();

    PropertiesDlg properties_dlg(application);

    if( properties_dlg.DoModal() != IDOK )
        return;

    ApplicationProperties new_application_properties = properties_dlg.ReleaseApplicationProperties();
    std::string new_application_properties_file_path = properties_dlg.GetApplicationPropertiesFilePath();

    const bool application_properties_changed = ( application.GetApplicationProperties() != new_application_properties );
    bool application_properties_filename_changed = !SO::EqualsNoCase(application.GetApplicationPropertiesFilePath(), new_application_properties_file_path);
    const bool using_external_properties_file = !new_application_properties_file_path.empty();

    if( application_properties_changed )
        application.SetApplicationProperties(std::move(new_application_properties));

    // save the application properties if they are external and changed, or if they are newly external
    if( using_external_properties_file && ( application_properties_changed || application_properties_filename_changed ) )
    {
        try
        {
            application.GetApplicationProperties().Save(new_application_properties_file_path);
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(FormatText("There was an error saving the application properties to '%s' so they will instead be saved to the application '%s':\n\n%s",
                                             PortableFunctions::PathGetFilename(new_application_properties_file_path).c_str(),
                                             PortableFunctions::PathGetFilename(application.GetApplicationFilePath()).c_str(),
                                             exception.what()));

            new_application_properties_file_path.clear();
            application_properties_filename_changed = true;
        }
    }

    if( application_properties_filename_changed || ( !using_external_properties_file && application_properties_changed ) )
    {
        if( application_properties_filename_changed )
            application.SetApplicationPropertiesFilePath(std::move(new_application_properties_file_path));

        pAplDoc->SetModifiedFlag();
    }

    // some properties are not part of the ApplicationProperties object
    if( application.GetLogicSettings() != properties_dlg.GetLogicSettings() )
    {
        application.SetLogicSettings(properties_dlg.GetLogicSettings());
        pAplDoc->SetModifiedFlag();

        // refresh the Scintilla lexers in case the logic version changed
        auto refresh_lexer = [&](CLogicCtrl* const logic_ctrl)
        {
            if( logic_ctrl != nullptr )
                logic_ctrl->PostMessage(UWM::Edit::RefreshLexer);
        };

        ApplicationChildWnd* application_child_wnd = assert_cast<ApplicationChildWnd*>(pWnd);
        refresh_lexer(application_child_wnd->GetSourceLogicCtrl());
        refresh_lexer(application_child_wnd->GetLogicDialogBar().GetMessageEditCtrl());
    }

    // reset any cached objects (e.g., if the logic version changed, the user message files need to be processed again)
    DesignerApplicationLoader::ResetCachedObjects();
}


LRESULT CMainFrame::OnSetExternalApplicationProperties(const WPARAM wParam, const LPARAM lParam)
{
    const std::string* const application_properties_file_path = reinterpret_cast<const std::string*>(wParam);
    ApplicationProperties* const application_properties = reinterpret_cast<ApplicationProperties*>(lParam);
    ASSERT(application_properties_file_path != nullptr && application_properties != nullptr);
    ASSERT(PortableFunctions::FileIsRegular(*application_properties_file_path));

    CMDIChildWnd* const pWnd = this->MDIGetActive();
    CDocument* const pDoc = pWnd->GetActiveDocument();
    CAplDoc* const pAplDoc = ProcessFOForSrcCode(*pDoc);
    Application& application = pAplDoc->GetAppObject();

    application.SetApplicationPropertiesFilePath(*application_properties_file_path);
    application.SetApplicationProperties(std::move(*application_properties));

    pAplDoc->SetModifiedFlag();

    return 1;
}


LRESULT CMainFrame::OnShowFileProperties(const WPARAM wParam, const LPARAM lParam)
{
    const SharableString path = WindowsDesktopMessage::GetPostedObject<SharableString>(wParam);
    CDocument* const document = reinterpret_cast<CDocument*>(lParam);

    CMDIChildWnd* const active_wnd = MDIGetActive();
    CDocument* const active_document = ( active_wnd != nullptr ) ? active_wnd->GetActiveDocument() :
                                                                   nullptr;

    if( !path.IsSet() || document == nullptr || active_document == nullptr )
        return ReturnProgrammingError(0);

    // only show the Manage Files dialog when the selected path is part of the current application
    const CAplDoc* application_doc;
    bool path_is_part_of_active_application;

    if( document->IsKindOf(RUNTIME_CLASS(CAplDoc)) )
    {
        application_doc = ProcessFOForSrcCode(*active_document);
        path_is_part_of_active_application = ( document == application_doc );
    }

    else
    {
        application_doc = nullptr;
        path_is_part_of_active_application = ( document == active_document );

        if( !path_is_part_of_active_application )
            application_doc = ProcessFOForSrcCode(*active_document);
    }

    if( !path_is_part_of_active_application )
    {
        ErrorMessage::Display(FormatText("'%s' is not part of the active application '%s'. "
                                         "Switch the active application before trying to modify the properties.",
                                         PortableFunctions::PathGetFilename(*path).c_str(),
                                         ( application_doc != nullptr ) ? application_doc->GetAppObject().GetName().c_str() : ""));
        return 0;
    }

    CCSProApp* const cspro_app = assert_cast<CCSProApp*>(AfxGetApp());
    cspro_app->ManageFiles(*path);

    return 1;
}


LRESULT CMainFrame::OnIsReservedWord(const WPARAM wParam, LPARAM /*lParam*/)
{
    const std::string_view text_sv = *reinterpret_cast<const std::string_view*>(wParam);
    return Logic::ReservedWords::IsReservedWord(text_sv) ? 1 : 0;
}


LRESULT CMainFrame::OnCapiMacros(WPARAM /*wParam*/,LPARAM /*lParam*/)
{
    CMDIChildWnd* pWnd = this->MDIGetActive();
    CDocument* const pDoc = pWnd->GetActiveDocument();
    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pDoc);

    CapiMacrosDlg capi_macros_dialog(pAplDoc);
    capi_macros_dialog.DoModal();

    return 0;
}


LRESULT CMainFrame::OnCanAddResources(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    CCSProApp* const cspro_app = assert_cast<CCSProApp*>(AfxGetApp());
    return cspro_app->IsApplicationOpen();
}


LRESULT CMainFrame::OnCopyToResourceDirectory(WPARAM wParam, LPARAM lParam)
{
    const std::string& source_file_path = *reinterpret_cast<const std::string*>(wParam);
    std::string* const destination_file_path = reinterpret_cast<std::string*>(lParam);

    CCSProApp* const cspro_app = assert_cast<CCSProApp*>(AfxGetApp());
    const std::optional<CAplDoc*> active_application_doc = cspro_app->GetActiveApplication(nullptr, L"Select Application to Add Image To");

    if( !active_application_doc.has_value() || *active_application_doc == nullptr )
    {
        ASSERT(!active_application_doc.has_value());
        return 0;
    }

    Application& application = (*active_application_doc)->GetAppObject();

    try
    {
        std::string resources_directory = PortableFunctions::PathReplaceFilename(application.GetApplicationFilePath(), "Resources");
        std::string file_path_in_resources_directory = Path::Combine(resources_directory, PortableFunctions::PathGetFilename(source_file_path));

        FileIO::CreateDirectories(resources_directory);

        // if not already added, add the Resources directory as a resource
        if( application.GetResource(resources_directory) == nullptr )
            cspro_app->AddResourceToApplication(*(*active_application_doc), AppResource(std::move(resources_directory)));

        // copy the file into the Resources directory
        PortableFunctions::FileCopyWithExceptions(source_file_path, file_path_in_resources_directory, FileOverwriteFlag::Different);

        if( destination_file_path != nullptr )
            *destination_file_path = std::move(file_path_in_resources_directory);

        return 1;
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return 0;
    }
}


LRESULT CMainFrame::OnUpdateApplicationExternalities(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    // get all external code and report text sources
    std::set<TextSourceEditable*> text_sources;

    ForeachLogicAndReportTextSource(
        [&](std::shared_ptr<TextSourceEditable> text_source, bool main_logic_file)
        {
            if( !main_logic_file )
                text_sources.insert(text_source.get());

            return true;
        });

    // see which text sources have been modified externally
    static int64_t last_check_timestamp = 0;

    if( !text_sources.empty() )
    {
        std::set<TextSourceEditable*> modified_text_sources;
        std::string reload_message;

        for( const auto& text_source : text_sources )
        {
            const int64_t file_modified_time = PortableFunctions::FileModifiedTime(text_source->GetFilePath());

            if( file_modified_time > last_check_timestamp && text_source->GetModifiedIteration() < file_modified_time )
            {
                modified_text_sources.insert(text_source);
                reload_message.append(FormatText("\"%s\"\n", text_source->GetFilePath().c_str()));
            }
        }

        if( !modified_text_sources.empty() )
        {
            const bool pluralize = ( modified_text_sources.size() > 1 );

            reload_message.append(FormatText("\nThe file%s been modified by another program.\nDo you want to reload %s?",
                                             pluralize ? "s have" : " has",
                                             pluralize ? "them" : "it"));

            if( AfxMessageBox(reload_message, MB_YESNO) == IDYES )
            {
                // reload any text sources modified outside of the application, ignoring any exceptions
                for( TextSourceEditable* const modified_text_source : modified_text_sources )
                {
                    try
                    {
                        modified_text_source->ReloadFromDisk();
                    }

                    catch( const CSProException& ) { ASSERT(false); }
                }

                // if currently viewing external code or reports, refresh the view
                CMDIFrameWnd* pWnd = (CMDIFrameWnd*)MDIGetActive();

                if( pWnd != nullptr )
                {
                    CDocument* const pDoc = pWnd->GetActiveDocument();

                    if( pDoc != nullptr )
                    {
                        auto get_selected_item_data = [](CTreeCtrl* pTreeCtrl) -> void*
                        {
                            HTREEITEM hItem = ( pTreeCtrl != nullptr ) ? pTreeCtrl->GetSelectedItem() : nullptr;
                            return ( hItem != nullptr ) ? (void*)pTreeCtrl->GetItemData(hItem) : nullptr;
                        };

                        auto refresh_view = [&](auto item_data, auto external_logic_item_type, auto report_item_type, auto message)
                        {
                            if( ( item_data != nullptr ) &&
                                ( item_data->GetItemType() == external_logic_item_type || item_data->GetItemType() == report_item_type ) &&
                                ( modified_text_sources.find((TextSourceEditable*)item_data->GetTextSource()) != modified_text_sources.cend() ) )
                            {
                                PostMessage(message, 0, reinterpret_cast<LPARAM>(pDoc));
                            }
                        };

                        if( pDoc->IsKindOf(RUNTIME_CLASS(CFormDoc)) )
                        {
                            auto item_data = (const CFormID*)get_selected_item_data(assert_cast<CFormDoc*>(pDoc)->GetFormTreeCtrl());
                            refresh_view(item_data, eFFT_EXTERNALCODE, eFFT_REPORT, UWM::Form::ShowSourceCode);
                        }

                        else if( pDoc->IsKindOf(RUNTIME_CLASS(COrderDoc)) )
                        {
                            AppTreeNode* app_tree_node = reinterpret_cast<AppTreeNode*>(get_selected_item_data(assert_cast<COrderDoc*>(pDoc)->GetOrderTreeCtrl()));

                            if( ( app_tree_node != nullptr ) &&
                                ( app_tree_node->GetAppFileType() == AppFileType::Code || app_tree_node->GetAppFileType() == AppFileType::Report ) &&
                                ( modified_text_sources.find(reinterpret_cast<TextSourceEditable*>(app_tree_node->GetTextSource())) != modified_text_sources.cend() ) )
                            {
                                PostMessage(UWM::Order::ShowSourceCode, 0, reinterpret_cast<LPARAM>(pDoc));
                            }
                        }
                    }
                }
            }
        }
    }

    last_check_timestamp = GetTimestamp<int64_t>();

    return 0;
}


LRESULT CMainFrame::OnFindOpenTextSourceEditable(const WPARAM wParam, const LPARAM lParam)
{
    // when using an editable text source, if it is open in another application, use the existing object
    // (for now this used only for logic and report files, not message files)
    const std::string& file_path = *reinterpret_cast<const std::string*>(wParam);
    std::shared_ptr<TextSourceEditable>& out_text_source = *reinterpret_cast<std::shared_ptr<TextSourceEditable>*>(lParam);
    ASSERT(out_text_source == nullptr);

    ForeachLogicAndReportTextSource(
        [&](const std::shared_ptr<TextSourceEditable>& text_source, bool /*main_logic_file*/)
        {
            if( SO::EqualsNoCase(text_source->GetFilePath(), file_path) )
            {
                out_text_source = text_source;
                return false;
            }

            return true;
        });

    return ( out_text_source != nullptr ) ? 1 : 0;
}


namespace
{
    template<typename item_data_type>
    bool SelectExternalLogicOrReportNode(CTreeCtrl* pTreeCtrl, const CString& filename)
    {
        HTREEITEM hItemToSelect = nullptr;

        TreeCtrlHelpers::FindInTree(*pTreeCtrl, pTreeCtrl->GetRootItem(), true,
            [&](const HTREEITEM hItem)
            {
                const item_data_type* const item_data = reinterpret_cast<const item_data_type*>(pTreeCtrl->GetItemData(hItem));

                if( item_data->GetTextSource() != nullptr && SO::EqualsNoCase(item_data->GetTextSource()->GetFilePath(), filename) )
                {
                    hItemToSelect = hItem;
                    return true;
                }

                return false;
            });

        if( hItemToSelect != nullptr )
        {
            // select the node if it already isn't selected
            if( pTreeCtrl->GetSelectedItem() != hItemToSelect )
                pTreeCtrl->SelectItem(hItemToSelect);

            return true;
        }

        return false;
    }
}


void CMainFrame::GotoExternalLogicOrReportNode(const CDocument* pDoc, CLogicCtrl* logic_control, NullTerminatedString filename,
                                               bool using_line_number, int line_number_or_position_in_buffer)
{
    bool success = false;

    if( pDoc->IsKindOf(RUNTIME_CLASS(CFormDoc)) )
    {
        success = SelectExternalLogicOrReportNode<CFormID>(assert_cast<const CFormDoc*>(pDoc)->GetFormTreeCtrl(), filename);
    }

    else if( pDoc->IsKindOf(RUNTIME_CLASS(COrderDoc)) )
    {
        success = SelectExternalLogicOrReportNode<AppTreeNode>(assert_cast<const COrderDoc*>(pDoc)->GetOrderTreeCtrl(), filename);
    }

    if( success )
    {
        logic_control->SetFocus();

        if( using_line_number )
        {
            logic_control->GotoLine(line_number_or_position_in_buffer);
        }

        else
        {
            logic_control->GotoPos(line_number_or_position_in_buffer);
        }
    }
}


LRESULT CMainFrame::OnGoToLogicError(WPARAM wParam, LPARAM lParam)
{
    ApplicationChildWnd* application_child_wnd = assert_cast<ApplicationChildWnd*>(reinterpret_cast<CWnd*>(wParam));
    const CompilerOutputTabViewPage::LogicErrorLocation& logic_error_location = *reinterpret_cast<const CompilerOutputTabViewPage::LogicErrorLocation*>(lParam);

    CDocument* const pDoc = assert_cast<CDocument*>(application_child_wnd->GetActiveDocument());

    // if not in an external code file or a report, go to that line number
    if( logic_error_location.parser_message.compilation_unit_name.empty() )
    {
        ASSERT(logic_error_location.adjusted_line_number_for_bookmark.has_value());

        CLogicCtrl* source_logic_ctrl = application_child_wnd->GetSourceLogicCtrl();
        source_logic_ctrl->SetFocus();
        source_logic_ctrl->GotoLine(*logic_error_location.adjusted_line_number_for_bookmark);
    }


    // if a CAPI error, switch to that view
    else if( std::holds_alternative<CapiLogicLocation>(logic_error_location.parser_message.extended_location) )
    {
        const CapiLogicLocation& capi_logic_location = std::get<CapiLogicLocation>(logic_error_location.parser_message.extended_location);

        CFormDoc* pFormDoc = DYNAMIC_DOWNCAST(CFormDoc, pDoc);
        ASSERT(pFormDoc != nullptr);

        CDEItemBase* item_base;
        CDEForm* form;

        if( !pFormDoc->GetFormFile().FindField(UTF8_TODO::GetCString(logic_error_location.parser_message.proc_name), &form, &item_base) )
            return 0;

        pFormDoc->GetFormTreeCtrl()->SelectFTCNode(pFormDoc->GetFormTreeCtrl()->GetFormNode(pFormDoc), item_base->GetFormNum(), item_base);

        POSITION pos = pFormDoc->GetFirstViewPosition();
        CView* form_view = pFormDoc->GetNextView(pos);
        ASSERT(form_view);

        CFormChildWnd* child_frame = DYNAMIC_DOWNCAST(CFormChildWnd, form_view->GetParentFrame());
        child_frame->OnQsfEditor();

        pFormDoc->GetCapiEditorViewModel().SetSelectedConditionIndex(static_cast<int>(capi_logic_location.condition_index));

        if( capi_logic_location.language_label.has_value() )
            child_frame->ShowCapiLanguage(*capi_logic_location.language_label);

        pFormDoc->UpdateAllViews(nullptr, Hint::CapiEditorUpdateQuestion);
    }


    // if an error in the message file, go to the error if the primary file, or open in CSCode otherwise
    else if( std::holds_alternative<Logic::ParserMessage::MessageFile>(logic_error_location.parser_message.extended_location) )
    {
        bool open_in_cscode = true;

        CAplDoc* pAplDoc = ProcessFOForSrcCode(*pDoc);

        if( pAplDoc != nullptr )
        {
            const std::vector<AppMessageFile>& app_message_files = pAplDoc->GetAppObject().GetMessageFiles();

            if( !app_message_files.empty() &&
                SO::EqualsNoCase(app_message_files.front().GetFilePath(), logic_error_location.parser_message.compilation_unit_name) )
            {
                LogicDialogBar& logic_dialog_bar = application_child_wnd->GetLogicDialogBar();
                logic_dialog_bar.SelectMessageEditTab();

                MessageEditCtrl* const message_edit_ctrl = logic_dialog_bar.GetMessageEditCtrl();
                ASSERT(message_edit_ctrl != nullptr);
                ASSERT(logic_error_location.parser_message.line_number > 0);

                message_edit_ctrl->SetFocus();
                message_edit_ctrl->GotoLine(logic_error_location.parser_message.line_number - 1);

                open_in_cscode = false;
            }
        }

        if( open_in_cscode )
            CSProExecutables::RunProgramOpeningFile(CSProExecutables::Program::CSCode, logic_error_location.parser_message.compilation_unit_name);
    }


    // if in an external code file or report, select the file and then go to that line number
    else
    {
        // go to the line number, which is 1-based
        GotoExternalLogicOrReportNode(pDoc, application_child_wnd->GetSourceLogicCtrl(),
                                      UTF8_TODO::GetWide(logic_error_location.parser_message.compilation_unit_name),
                                      true, static_cast<int>(logic_error_location.parser_message.line_number) - 1);
    }


    return 1;
}


LRESULT CMainFrame::OnGetLexerLanguage(const WPARAM wParam, const LPARAM lParam)
{
    const CLogicCtrl* const logic_ctrl = reinterpret_cast<const CLogicCtrl*>(wParam);
    int& lexer_language = *reinterpret_cast<int*>(lParam);

    CMDIChildWnd* const pWnd = this->MDIGetActive();
    CAplDoc* pAplDoc;

    if( pWnd == nullptr || ( pAplDoc = ProcessFOForSrcCode(*pWnd->GetActiveDocument()) ) == nullptr )
        return 0;

    // check if this is the message editor
    std::optional<AppFileType> app_file_type;

    ApplicationChildWnd* const application_child_wnd = dynamic_cast<ApplicationChildWnd*>(pWnd);

    if( application_child_wnd != nullptr &&
        logic_ctrl == application_child_wnd->GetLogicDialogBar().GetMessageEditCtrl() )
    {
        lexer_language = Lexers::GetLexer_Message(pAplDoc->GetAppObject());
    }

    // otherwise use the GetLexerLanguageForSourceCode routine
    else if( pWnd->GetActiveDocument()->IsKindOf(RUNTIME_CLASS(CFormDoc)) )
    {
        CFormID* const pFormID = GetNodeIdForSourceCode<CFormID>();
        ASSERT(pFormID != nullptr);

        lexer_language = GetLexerLanguageForSourceCode(pAplDoc->GetAppObject(), *pFormID);
    }

    else if( pWnd->GetActiveDocument()->IsKindOf(RUNTIME_CLASS(COrderDoc)) )
    {
        const AppTreeNode* const app_tree_node = GetNodeIdForSourceCode<AppTreeNode>();
        ASSERT(app_tree_node != nullptr);

        lexer_language = GetLexerLanguageForSourceCode(pAplDoc->GetAppObject(), *app_tree_node);
    }

    else
    {
        ASSERT(pWnd->GetActiveDocument()->IsKindOf(RUNTIME_CLASS(CTabulateDoc)));
        lexer_language = Lexers::GetLexer_Logic(pAplDoc->GetAppObject());
    }

    return 1;
}


LRESULT CMainFrame::OnGetDesignerIcon(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    // the user of this HICON should call DestroyIcon when done
    return (LRESULT)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 0, 0, LR_CREATEDIBSECTION);
}


LRESULT CMainFrame::OnDisplayErrorMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ErrorMessage::DisplayPostedMessages();
    return 0;
}


LRESULT CMainFrame::OnEngineUI(WPARAM wParam, LPARAM lParam)
{
    // create an instance of the engine UI to process the message
    return EngineUIProcessor(nullptr, false).ProcessMessage(wParam, lParam);
}


LRESULT CMainFrame::OnRedrawPropertyGrid(WPARAM wParam, LPARAM /*lParam*/)
{
    DictBase* pDictBase = (DictBase*)wParam;
    CMDIFrameWnd* pWnd = (CMDIFrameWnd*)MDIGetActive();

    if( pWnd != nullptr && pWnd->IsKindOf(RUNTIME_CLASS(CDictChildWnd)) )
    {
        CDictChildWnd* pDictChildWnd = (CDictChildWnd*)pWnd;
        CDDDoc* pDoc = (CDDDoc*)pDictChildWnd->GetActiveDocument();

        if( pDictChildWnd->GetDictDlgBar()->GetSafeHwnd() && pDictChildWnd->GetDictDlgBar()->GetPropCtrl()->GetSafeHwnd() )
            pDictChildWnd->GetDictDlgBar()->GetPropCtrl()->Initialize(pDoc, pDictBase);
    }

    else
    {
        ASSERT(false);
    }

    return 0;
}

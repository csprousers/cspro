#include "StdAfx.h"
#include "MainFrm.h"
#include "CapiMacrosDlg.h"
#include "Dtypedlg.h"
#include "PropertiesDlg.h"
#include "SelectAppDlg.h"
#include <zToolsO/NewlineSubstitutor.h>
#include <zToolsO/UWM.h>
#include <zUtilO/ConnectionString.h>
#include <zUtilO/TreeCtrlHelpers.h>
#include <zUtilO/UWM.h>
#include <zUtilF/UIThreadRunner.h>
#include <zLogicO/ReservedWords.h>
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
#include <Zsrcmgro/DesignerCompiler.h>
#include <zEngineF/EngineUI.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const int DICTTOOLBARPOS = 1;
const int TABTOOLBARPOS = 2;
const int FORMTOOLBARPOS = 3;
const int ORDERTOOLBARPOS = 4;
const int LANGDBARPOS = 5;

static UINT toolbars[] =
{
    IDR_MAINFRAME,
    IDR_DICT_FRAME,
    IDR_TABLE_FRAME,
    IDR_FORM_FRAME,
    IDR_ORDER_FRAME
};

static void Load24BitColorToolbarImages(CToolBar* pToolBar, UINT nIDResource, CImageList*& imageStorage)
{
    HINSTANCE hInst = AfxFindResourceHandle(MAKEINTRESOURCE(nIDResource),RT_BITMAP);
    HBITMAP hBitmap = (HBITMAP) ::LoadImage(hInst, MAKEINTRESOURCE(nIDResource), IMAGE_BITMAP,
                                            0,0, LR_CREATEDIBSECTION);
    CBitmap bm;
    bm.Attach(hBitmap);
    imageStorage = new CImageList();
    imageStorage->Create(16, 16, ILC_COLOR24 | ILC_MASK, 1, 1);
    imageStorage->Add(&bm, RGB(192,192,192));
    pToolBar->GetToolBarCtrl().SetImageList(imageStorage);
}

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
    ON_COMMAND(ID_DICTTYPE, OnDictType)
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
    ON_MESSAGE(UWM::Designer::GetApplication, OnGetApplication)
    ON_MESSAGE(UWM::Designer::GetFormFileOrDictionary, OnGetFormFileOrDictionary)

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
    ON_MESSAGE(UWM::Form::GetCapiLanguages, GetLangInfo)
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

    ON_MESSAGE(ZEDIT2O_SEL_CHANGE, OnSelChange)
    ON_MESSAGE(ZEDIT2O_LOGIC_REFERENCE, OnLogicReference)
    ON_MESSAGE(WM_IMSA_SYMBOLS_ADDED, OnSymbolsAdded)
    ON_COMMAND(ID_VIEW_TOP_LOGIC, OnViewTopLogic)
    ON_MESSAGE(ZEDIT2O_LOGIC_AUTO_COMPLETE, OnLogicAutoComplete)
    ON_MESSAGE(ZEDIT2O_LOGIC_INSERT_PROC_NAME, OnLogicInsertProcName)

    ON_MESSAGE(WM_IMSA_SET_STATUSBAR_PANE, SetStatusBarPane)
    ON_MESSAGE(WM_IMSA_SETFOCUS, OnIMSASetFocus)

    ON_MESSAGE(WM_IMSA_UPDATE_SYMBOLTBL, OnUpdateSymbolTblFlag)

    ON_MESSAGE(WM_IMSA_TABCONVERT,OnIMSATabConvert)

    ON_MESSAGE(WM_IMSA_RECONCILE_QSF_FIELD_NAME, OnReconcileQsfFieldName)
    ON_MESSAGE(WM_IMSA_RECONCILE_QSF_DICT_NAME, OnReconcileQsfDictName)

    ON_UPDATE_COMMAND_UI(ID_AREA_COMBO, OnUpdateAreaComboBox)
    ON_UPDATE_COMMAND_UI(ID_TAB_ZOOM_COMBO, OnUpdateZoomComboBox)

    ON_UPDATE_COMMAND_UI(ID_OPTIONS_PROPERTIES, OnUpdateIfApplicationIsAvailable)
    ON_COMMAND(ID_OPTIONS_PROPERTIES, OnOptionsProperties)
    ON_MESSAGE(UWM::CSPro::SetExternalApplicationProperties, OnSetExternalApplicationProperties)

    ON_MESSAGE(UWM::UtilO::IsReservedWord, IsReservedWord)

    ON_MESSAGE(UWM::UtilF::CanAddResourceFolder, OnCanAddResourceFolder)
    ON_MESSAGE(UWM::UtilF::CreateResourceFolder, OnCreateResourceFolder)

    ON_MESSAGE(UWM::CSPro::CreateUniqueName, OnCreateUniqueName)
    ON_MESSAGE(UWM::CSPro::UpdateApplicationExternalities, OnUpdateApplicationExternalities)
    ON_MESSAGE(UWM::Designer::FindOpenTextSourceEditable, OnFindOpenTextSourceEditable)

    ON_MESSAGE(UWM::Designer::GoToLogicError, OnGoToLogicError)

    ON_MESSAGE(UWM::Edit::GetLexerLanguage, OnGetLexerLanguage)

    ON_MESSAGE(UWM::Designer::EditReportProperties, OnEditReportProperties)
    ON_COMMAND(ID_VIEW_REPORT_PREVIEW, OnViewReportPreview)
    ON_UPDATE_COMMAND_UI(ID_VIEW_REPORT_PREVIEW, OnUpdateViewReportPreview)

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
{
    // TODO: add member initialization code here
    m_pWndFormTBar = nullptr;
    m_pWndOrderTBar = nullptr;
    m_pWndTabTBar = nullptr;
    m_pWndDictTBar = nullptr;
    m_pWndToolBar = nullptr;

    m_bDictToolbar = FALSE;
    m_bTabToolbar = FALSE;
    m_bFormToolbar = FALSE;
    m_bOrderToolbar = FALSE;

    m_pszClassName = nullptr;

    m_bRemovingPossibleDuplicateProcs = false;
}

CMainFrame::~CMainFrame()
{
    delete m_pWndToolBar;
    delete m_pWndToolBarImages;
    delete m_pWndTabTBar;
    delete m_pWndDictTBar;
    delete m_pWndFormTBar;
    delete m_pWndOrderTBar;
    delete m_pWndOrderTBarImages;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    // Create CSPro tool bar
    m_pWndToolBar = new CToolBar();

    if (!m_pWndToolBar->CreateEx(this,TBSTYLE_FLAT,WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP,CRect(0,0,0,0), AFX_IDW_TOOLBAR )||
        !m_pWndToolBar->LoadToolBar(IDR_MAINFRAME))
    {
        TRACE0("Failed to create CSPro toolbar\n");
        return -1;      // fail to create
    }

    Load24BitColorToolbarImages(m_pWndToolBar, IDR_MAINFRAME, m_pWndToolBarImages);

    for( unsigned tools_id = ID_TOOLS_DATAVIEWER; tools_id <= ID_TOOLS_TEXTCONVERTER; ++tools_id )
        m_pWndToolBar->GetToolBarCtrl().HideButton(tools_id);

    // Create Dictionary tool bar
    m_pWndDictTBar = new CToolBar();

    if (!m_pWndDictTBar->CreateEx(this,TBSTYLE_FLAT,WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP,CRect(0,0,0,0),996)||
        !m_pWndDictTBar->LoadToolBar(IDR_DICT_FRAME))
    {
        TRACE0("Failed to create dict toolbar\n");
        return -1;      // fail to create
    }

    // Create Tabulation tool bar
    m_pWndTabTBar = new CToolBar();

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
    m_tabAreaComboBox.SetParent(m_pWndTabTBar); // parent is toolbar but messages get sent to CMainFrame

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
    m_tabZoomComboBox.SetParent(m_pWndTabTBar); // parent is toolbar but messages get sent to CMainFrame

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
    m_printViewCloseButton.SetParent(m_pWndTabTBar); // parent is toolbar but messages get sent to CMainFrame

    // Create Form tool bar
    m_pWndFormTBar = CFormChildWnd::CreateFormToolBar(this);

    if( m_pWndFormTBar == nullptr )
    {
        TRACE0("Failed to create form toolbar\n");
        return -1;      // fail to create
    }

    // Create Order tool bar
    m_pWndOrderTBar = new CToolBar();

    if (!m_pWndOrderTBar->CreateEx(this,TBSTYLE_FLAT,WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP,CRect(0,0,0,0), 999)||
        !m_pWndOrderTBar->LoadToolBar(IDR_ORDER_FRAME))
    {
        TRACE0("Failed to create form toolbar\n");
        return -1;      // fail to create
    }

    Load24BitColorToolbarImages(m_pWndOrderTBar, IDR_ORDER_FRAME, m_pWndOrderTBarImages);

    //Create language bar
    if (!m_wndLangDlgBar.Create(this, IDD_LANGDLGBAR, CBRS_ALIGN_TOP | CBRS_TOOLTIPS | CBRS_FLYBY, IDD_LANGDLGBAR))
    {
        TRACE0("Failed to create language dialogbar \n");
        return -1;      // fail to create
    }

    // Create rebar
    if (!m_wndReBar.Create(this) ||
        !m_wndReBar.AddBar(m_pWndToolBar) ||
        !m_wndReBar.AddBar(m_pWndDictTBar) ||
        !m_wndReBar.AddBar(m_pWndTabTBar) ||
        !m_wndReBar.AddBar(m_pWndFormTBar) ||
        !m_wndReBar.AddBar(m_pWndOrderTBar) ||
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


    m_csWindowText = Versioning::GetVersionString(true);
    SetWindowText(m_csWindowText);

    return 0;
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
    pTemplate = GetDocTemplate(_T(".ent;.xtb;.bch"));
    if (pTemplate)
        pTemplate->CloseAllDocuments(FALSE);


    //Get the forms and close
    pTemplate = GetDocTemplate(FileExtensions::WithDot::Form);
    if(pTemplate)
        pTemplate->CloseAllDocuments(FALSE);

    //Get the orders and close
    pTemplate = GetDocTemplate(FileExtensions::WithDot::Order);
    if(pTemplate)
        pTemplate->CloseAllDocuments(FALSE);

    //Get the tables and close
    pTemplate = GetDocTemplate(FileExtensions::WithDot::TableSpec);
    if(pTemplate)
        pTemplate->CloseAllDocuments(FALSE);

    //Get the dictionaries and close
    pTemplate = GetDocTemplate(FileExtensions::WithDot::Dictionary);
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


CDocTemplate* CMainFrame::GetDocTemplate(wstring_view extension)
{
    const CCSProApp* cspro_app = assert_cast<const CCSProApp*>(AfxGetApp());
    POSITION pos = cspro_app->GetFirstDocTemplatePosition();

    while( pos != nullptr )
    {
        CDocTemplate* doc_template = cspro_app->GetNextDocTemplate(pos);
        CString doc_extension;

        doc_template->GetDocString(doc_extension, CDocTemplate::filterExt);

        if( SO::EqualsNoCase(doc_extension, extension) )
            return doc_template;
    }

    return nullptr;
}


template<typename DocumentType, typename CF>
void CMainFrame::ForeachDocument(CF callback_function)
{
    const CDocTemplate* doc_template;

    if constexpr(std::is_same_v<DocumentType, CAplDoc>)
    {
        const CCSProApp* cspro_app = assert_cast<const CCSProApp*>(AfxGetApp());
        doc_template = cspro_app->GetAppTemplate();
    }

    else
    {
        doc_template = GetDocTemplate(DocumentType::GetExtensionWithDot());

        if( doc_template == nullptr )
        {
            ASSERT(false);
            return;
        }
    }

    POSITION pos = doc_template->GetFirstDocPosition();

    while( pos != nullptr )
    {
        DocumentType* document = assert_cast<DocumentType*>(doc_template->GetNextDoc(pos));

        if( !callback_function(*document) )
            return;
    }
}


template<typename DocumentType, typename CF>
void CMainFrame::ForeachDocumentUsingDictionary(const CDataDict& dictionary, CF callback_function)
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
void CMainFrame::ForeachApplicationDocumentUsingFormFile(const CDEFormFile& form_file, CF callback_function)
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
void CMainFrame::ForeachLogicAndReportTextSource(CF callback_function)
{
    ForeachDocument<CAplDoc>(
        [&](CAplDoc& application_doc)
        {
            Application& application = application_doc.GetAppObject();

            auto process_text_source = [&](auto text_source, bool main_logic_file)
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
            for( auto& report_named_text_source : application.GetReportNamedTextSources() )
            {
                if( process_text_source(report_named_text_source->text_source, false) )
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
    auto update_dictionary_view = [&](const CString& dictionary_filename)
    {
        DictionaryDictTreeNode* dictionary_dict_tree_node = m_SizeDlgBar.m_DictTree.GetDictionaryTreeNode(dictionary_filename);

        if( dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDocument() != nullptr )
            dictionary_dict_tree_node->GetDocument()->UpdateAllViews(nullptr, Hint::LanguageChanged);
    };

    // orders
    if( dictionary_based_doc->IsKindOf(RUNTIME_CLASS(COrderDoc)) )
    {
        COrderDoc* pOrderDoc = assert_cast<COrderDoc*>(dictionary_based_doc);
        CDEFormFile& order_file = pOrderDoc->GetFormFile();

        update_dictionary_view(order_file.GetDictionaryFilename());
    }

    // forms
    else if( dictionary_based_doc->IsKindOf(RUNTIME_CLASS(CFormDoc)) )
    {
        CFormDoc* pFormDoc = assert_cast<CFormDoc*>(dictionary_based_doc);
        CDEFormFile& form_file = pFormDoc->GetFormFile();

        update_dictionary_view(form_file.GetDictionaryFilename());

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
        SendMessage(UWM::Form::ShowCapiText, reinterpret_cast<WPARAM>(dictionary_based_doc));
    }

    return 1;
}


LRESULT CMainFrame::OnGetCurrentLanguageName(WPARAM wParam, LPARAM lParam)
{
    const CDocument& document = *reinterpret_cast<CDocument*>(wParam);
    std::wstring& language_name = *reinterpret_cast<std::wstring*>(lParam);

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

    CAplDoc* pDoc = GetApplicationUsingFormFile(pForm->GetPathName());

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
                CString pff_filename = PortableFunctions::PathRemoveFileExtension<CString>(filename_to_run) + FileExtensions::WithDot::Pff;

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

    CAplDoc* pDoc = GetApplicationUsingFormFile(pForm->GetPathName());

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
                    CString sBinFileName = pDoc->GetPathName();
                    PathRemoveExtension(sBinFileName.GetBuffer());
                    sBinFileName.ReleaseBuffer();
                    sBinFileName += FileExtensions::WithDot::BinaryEntryPen;
                    if (PortableFunctions::FileExists(sBinFileName)) {
                        DeleteFile(sBinFileName); // 20140311 deleting the file instead of recycling it
                    }
                }
            }

            if (!CompileAll(pForm)) {
                return 0;
            }

            CString sBinName = PortableFunctions::PathRemoveFileExtension<CString>(pDoc->GetPathName()) + FileExtensions::WithDot::BinaryEntryPen;

            CFileDialog dlgFile(FALSE,
                                FileExtensions::WithDot::BinaryEntryPen,
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

            const std::optional<std::wstring> csentry_exe = CSProExecutables::GetExecutablePath(CSProExecutables::Program::CSEntry);

            if( !csentry_exe.has_value() )
                return 0;

            CString command_line = FormatText(_T("\"%s\" \"%s\" /pen /binaryName \"%s\""), csentry_exe->c_str(), pDoc->GetPathName().GetString(), sBinName.GetString());

            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            ZeroMemory( &si, sizeof(si) );
            si.cb = sizeof(si);
            ZeroMemory( &pi, sizeof(pi) );

            BOOL bRes = ::CreateProcess(csentry_exe->c_str(),     // app name
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

    CAplDoc* pDoc = GetApplicationUsingFormFile(pForm->GetPathName());

    if (pDoc) {
        CString pffPath = PortableFunctions::PathRemoveFileExtension<CString>(pDoc->GetPathName()) + FileExtensions::WithDot::Pff;
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
                    CString sBinFileName = pDoc->GetPathName();
                    PathRemoveExtension(sBinFileName.GetBuffer());
                    sBinFileName.ReleaseBuffer();
                    sBinFileName += FileExtensions::WithDot::BinaryEntryPen;
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

            const std::optional<std::wstring> csdeploy_exe = CSProExecutables::GetExecutablePath(CSProExecutables::Program::CSDeploy);

            if( !csdeploy_exe.has_value() )
                return 0;

            CString command_line = FormatText(_T("\"%s\" \"%s\""), csdeploy_exe->c_str(), pffPath.GetString());

            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));

            BOOL bRes = ::CreateProcess(csdeploy_exe->c_str(), // app name
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
        FileTreeNode* file_tree_node = ObjTree.GetFileTreeNode(hItem);
        ASSERT(file_tree_node);
        bool bProcess = false;
        CDocument* pDoc = file_tree_node->GetDocument();
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
            sMsg += _T(" is modified. Do you want to save it ?");
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


CAplDoc* CMainFrame::GetApplicationUsingFormFile(wstring_view form_filename, bool silent/* = false*/)
{
    std::vector<CAplDoc*> application_docs;

    ForeachDocument<CAplDoc>(
        [&](CAplDoc& application_doc)
        {
            for( const CString& this_form_filename : application_doc.GetAppObject().GetFormFilenames() )
            {
                if( SO::EqualsNoCase(form_filename, this_form_filename) )
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
            AfxMessageBox(FormatText(_T("The file %s does not belong to any application."), std::wstring(form_filename).c_str()));
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
                for( const CString& tab_spec_filename : application_document.GetAppObject().GetTabSpecFilenames() )
                {
                    if( SO::EqualsNoCase(tab_doc.GetPathName(), tab_spec_filename) )
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


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::SetSourceCode(CAplDoc* pAplDoc)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::SetSourceCode(CAplDoc* pAplDoc)
{
    if( pAplDoc == nullptr )
        return;

    Application* pApplication = &pAplDoc->GetAppObject();
    ASSERT(pApplication->GetEngineAppType() == EngineAppType::Entry);

    CFormID* pFormID = GetNodeIdForSourceCode<CFormID>();

    CFormDoc* pFormDoc = pFormID->GetFormDoc();
    CView* pFormView = pFormDoc->GetView();
    ASSERT(pFormView);

    CFormChildWnd* pWnd = (CFormChildWnd*)pFormView->GetParentFrame();
    CFSourceEditView* pView = pWnd->GetSourceView();
    if( pView == nullptr )
        return;

    eNodeType nType = pFormID->GetItemType();

    std::wstring source_code_ws;
    bool bAppSrcCode = false;
    int lexer_language = Lexers::GetLexer_Logic(*pApplication);

    // external code
    if( nType == eFFT_EXTERNALCODE )
    {
        ASSERT(pFormID->GetTextSource() != nullptr);
        source_code_ws = pFormID->GetTextSource()->GetText();
    }

    // report
    else if( nType == eFFT_REPORT )
    {
        ASSERT(pFormID->GetTextSource() != nullptr);
        source_code_ws = pFormID->GetTextSource()->GetText();
        lexer_language = Lexers::GetLexer_Report(*pApplication, FileExtensions::IsFilenameHtml(pFormID->GetTextSource()->GetFilename()));
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
                    CString sFormFName = pApplication->GetFormFilenames().front();//get primary form file
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
            CString source_code;
            LPTSTR pString = source_code.GetBufferSetLength(uAlloc);
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
            source_code.ReleaseBuffer();

            if(source_code.IsEmpty() && !sSymbolName.IsEmpty()) {
                source_code = _T("PROC ") + sSymbolName;
                source_code += _T("\r\n");
            }
            if(!sText.IsEmpty()){
                source_code += sText;
            }

            source_code_ws = CS2WS(source_code);
        }
    }

    bool prevModifiedState = pView->GetEditCtrl()->IsModified(); // 20100708 trying to get rid of superfluous modified statements
    pView->GetEditCtrl()->SetText(source_code_ws);
    pView->GetEditCtrl()->SetModified(prevModifiedState);

    pView->GetEditCtrl()->ToggleLexer(lexer_language);

    if( Lexers::CanFoldCode(lexer_language) )
    {
        // fold procs only when viewing PROC GLOBAL
        pView->GetEditCtrl()->SetFolding(bAppSrcCode);
    }

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
    ASSERT(pApplication->GetEngineAppType() == EngineAppType::Batch);

    AppTreeNode* app_tree_node = GetNodeIdForSourceCode<AppTreeNode>();

    COrderDoc* pOrderDoc = app_tree_node->GetOrderDocument();
    POSITION pos = pOrderDoc->GetFirstViewPosition();
    CView* pOrderView = pOrderDoc->GetNextView(pos);
    ASSERT(pOrderView);

    COrderChildWnd* pWnd = (COrderChildWnd*)pOrderView->GetParentFrame();
    COSourceEditView* pView = pWnd->GetOSourceView();

    std::wstring source_code_ws;
    bool bAppSrcCode = false;
    int lexer_language = Lexers::GetLexer_Logic(*pApplication);

    // external code
    if( app_tree_node->GetAppFileType() == AppFileType::Code )
    {
        ASSERT(app_tree_node->GetTextSource() != nullptr);
        source_code_ws = app_tree_node->GetTextSource()->GetText();
    }

    // report
    else if( app_tree_node->GetAppFileType() == AppFileType::Report )
    {
        ASSERT(app_tree_node->GetTextSource() != nullptr);
        source_code_ws = app_tree_node->GetTextSource()->GetText();
        lexer_language = Lexers::GetLexer_Report(*pApplication, FileExtensions::IsFilenameHtml(app_tree_node->GetTextSource()->GetFilename()));
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
                    CString sOrderFName = pApplication->GetFormFilenames().front();//get primary order file
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
            CString source_code;
            LPTSTR pString = source_code.GetBufferSetLength(uAlloc);
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
            source_code.ReleaseBuffer();

            if(source_code.IsEmpty() && !sSymbolName.IsEmpty()) {
                source_code = _T("PROC ") + sSymbolName;
                source_code += _T("\r\n");
            }
            if(!sText.IsEmpty()){
                source_code += sText;
            }

            source_code_ws = CS2WS(source_code);
        }
    }

    // 20100316 nothing is changed by just loading or changing what proc is displayed
    bool modFlag1 = pOrderDoc->IsModified();
    bool modFlag2 = pView->GetEditCtrl()->IsModified();

    pView->GetEditCtrl()->SetText(source_code_ws);

    pOrderDoc->SetModifiedFlag(modFlag1); // 20100316
    pView->GetEditCtrl()->SetModified(modFlag2);

    pView->GetEditCtrl()->ToggleLexer(lexer_language);

    if( Lexers::IsCSProLogic(lexer_language) )
    {
        // fold procs only when viewing PROC GLOBAL
        pView->GetEditCtrl()->SetFolding(bAppSrcCode);
    }

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
    void ProcessParserMessages(CAplDoc* pAplDoc, CSourceCode* pSourceCode, view_type* pView, LogicDialogBar& logic_dialog_bar,
                               TextSource* external_logic_or_report_text_source = nullptr)
    {
        std::wstring all_messages;
        bool processed_first_error = false;
        bool has_errors = false;
        std::optional<std::map<std::wstring, int>> proc_line_number_map;

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
                if( SO::EqualsNoCase(parser_message.compilation_unit_name, external_logic_or_report_text_source->GetFilename()) )
                    use_line_number_for_display_for_bookmark = true;
            }

            // otherwise only adjust the line number if this is a source-related message not from an external code file or a report
            else if( line_number_for_display != 0 && parser_message.compilation_unit_name.empty() )
            {
                if( !proc_line_number_map.has_value() )
                    proc_line_number_map = pSourceCode->GetProcLineNumberMap();

                auto adjust_line_number_from_proc_lookup = [&](const std::wstring& proc_name)
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
                        adjust_line_number_from_proc_lookup(_T("GLOBAL"));
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

            const TCHAR* message_type = ( parser_message.type == Logic::ParserMessage::Type::Error )   ? _T("ERROR") :
                                        ( parser_message.type == Logic::ParserMessage::Type::Warning ) ? _T("WARNING") :
                                                                                                         _T("DEPRECATION");

            std::wstring error_location_and_line_number;

            if( pSourceCode->IsCompilingGlobal() )
            {
                if( std::holds_alternative<CapiLogicLocation>(parser_message.extended_location) )
                {
                    const CapiLogicLocation& capi_logic_location = std::get<CapiLogicLocation>(parser_message.extended_location);

                    error_location_and_line_number = FormatTextCS2WS(_T("CAPI Text, %s"), parser_message.proc_name.c_str());

                    if( capi_logic_location.language_label.has_value() )
                    {
                        SO::Append(error_location_and_line_number, _T(", "),
                                   *capi_logic_location.language_label);
                    }

                    if( capi_logic_location.condition_index > 0 )
                    {
                        SO::Append(error_location_and_line_number, _T(", condition #"),
                                   IntToString(capi_logic_location.condition_index + 1));
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
                    else if( const NamedTextSource* report_named_text_source = pAplDoc->GetAppObject().GetReportNamedTextSource(parser_message.compilation_unit_name, false);
                             report_named_text_source != nullptr )
                    {
                        error_location_and_line_number = report_named_text_source->name;
                    }

                    // use the filename (without extension) for external code files
                    else
                    {
                        error_location_and_line_number = PortableFunctions::PathGetFilenameWithoutExtension(parser_message.compilation_unit_name);
                    }
                }

                // don't include GLOBAL when there is a non-line related error (e.g., the external code file couldn't be opened)
                else if( line_number_for_display == 0 )
                {
                    ASSERT(SO::EqualsNoCase(parser_message.proc_name, _T("GLOBAL")));
                }

                else
                {
                    error_location_and_line_number = parser_message.proc_name;
                }
            }

            if( line_number_for_display > 0 )
                SO::AppendWithSeparator(error_location_and_line_number, IntToString(line_number_for_display), _T(", "));

            std::wstring formatted_message = message_type;

            if( !error_location_and_line_number.empty() )
                SO::AppendFormat(formatted_message, _T("(%s)"), error_location_and_line_number.c_str());

            SO::AppendFormat(formatted_message, _T(": %s\r\n"), parser_message.message_text.c_str());

            all_messages.append(formatted_message);

            // show on the logic editor where the error or warning is located
            auto add_error_or_warning = [&](int line_number)
            {
                const bool error = ( parser_message.type == Logic::ParserMessage::Type::Error );
                pView->GetEditCtrl()->AddErrorOrWarningMarker(error, line_number);
            };

            if( external_logic_or_report_text_source != nullptr )
            {
                // when editing external code or reports, mark only errors/warnings in that file
                if( SO::EqualsNoCase(parser_message.compilation_unit_name, external_logic_or_report_text_source->GetFilename()) )
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
            all_messages = FormatTextCS2WS(_T("Compile Successful at %s"), CTime::GetCurrentTime().Format(_T("%X")).GetString());

        compiler_output_tab_view_page->SetReadOnlyText(all_messages);

        logic_dialog_bar.SelectCompilerOutputTab();
        logic_dialog_bar.UpdateScrollState();

        compiler_output_tab_view_page->Invalidate();
        compiler_output_tab_view_page->UpdateWindow();

        if( has_errors )
            AfxMessageBox(_T("Compile Failed!"));
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
            std::wstring source_code = pView->GetLogicCtrl()->GetText();
            external_logic_or_report_text_source->SetText(std::move(source_code));

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

        CIMSAString sString = WS2CS(pView->GetLogicCtrl()->GetText());
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
            bRet = designer_compiler.CompileReport(*assert_cast<FormReportID&>(*pFormID).GetNamedTextSource());
        }

        else
        {
            ASSERT(false);
        }

        ProcessParserMessages(pAplDoc, pSourceCode, pView, pWnd->GetLogicDialogBar(), external_logic_or_report_text_source);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        bRet = false;
    }

    return bRet;
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
            std::wstring source_code = pView->GetLogicCtrl()->GetText();
            external_logic_or_report_text_source->SetText(std::move(source_code));

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

        CIMSAString sString = WS2CS(pView->GetLogicCtrl()->GetText());
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
                const FormOrderAppTreeNode* form_order_app_tree_node = pOTC->GetFormOrderAppTreeNode(*pOrderDoc);

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
            bRet = designer_compiler.CompileReport(assert_cast<ReportAppTreeNode&>(*app_tree_node).GetNamedTextSource());
        }

        else
        {
            ASSERT(false);
        }

        ProcessParserMessages(pAplDoc, pSourceCode, pView, pWnd->GetLogicDialogBar(), external_logic_or_report_text_source);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        bRet = false;
    }

    return bRet;
}


LRESULT CMainFrame::OnGetMessageTextSource(WPARAM wParam, LPARAM lParam)
{
    CDocument* pDoc = reinterpret_cast<CDocument*>(wParam);
    std::shared_ptr<TextSourceEditable>& message_text_source = *reinterpret_cast<std::shared_ptr<TextSourceEditable>*>(lParam);

    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pDoc);

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
    CAplDoc* pDoc = GetApplicationUsingFormFile(pOrder->GetPathName());

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

        FormOrderAppTreeNode* form_order_app_tree_node = pOrder->GetOrderTreeCtrl()->GetFormOrderAppTreeNode(*pOrder);
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
            CString pff_filename = PortableFunctions::PathRemoveFileExtension<CString>(filename_to_run) + FileExtensions::WithDot::Pff;

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
    const CAplDoc* pDoc = GetApplicationUsingFormFile(pForm->GetPathName(), true);
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
            DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(pForm->GetFormFile().GetDictionaryFilename());

            if( dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr )
            {
                int iL, iR, iI, iVS;
                name_is_unique = !dictionary_dict_tree_node->GetDDDoc()->GetDict()->LookupName(name, &iL, &iR, &iI, &iVS);
            }
        }
    }

    if( !name_is_unique )
    {
        AfxMessageBox(FormatText(_T("The name '%s' cannot be used as it is not unique in your application."), name.GetString()));
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
//      void CMainFrame::OnDictType()
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnDictType()
{
    //Get the objtreectrl
    CObjTreeCtrl& ObjTree = m_SizeDlgBar.m_ObjTree;

    //Get the application file name
    //Get the parent form file
    //Get the dictionary file name
    CString sAplFileName;
    CString sDictFName;
    CString sParentFName;

    if(!ObjTree.GetDictTypeArgs(sAplFileName,sDictFName,sParentFName))
        return;
    FileTreeNode* file_tree_node = ObjTree.FindNode(sAplFileName);
    ASSERT(file_tree_node && file_tree_node->GetDocument());
    //Get the DictionaryDescription
    CAplDoc* pAplDoc = DYNAMIC_DOWNCAST(CAplDoc, file_tree_node->GetDocument());
    ASSERT(pAplDoc);
    Application& application = pAplDoc->GetAppObject();

    if(pAplDoc->GetEngineAppType() == EngineAppType::Entry || pAplDoc->GetEngineAppType() == EngineAppType::Batch || pAplDoc->GetEngineAppType() == EngineAppType::Tabulation ) {
        CDictTypeDlg dictTypeDlg;

        BOOL bSpecialOutPut = FALSE;
        BOOL bMain = FALSE;

        DictionaryDescription* dictionary_description = application.GetDictionaryDescription(sDictFName, sParentFName);

        if( dictionary_description == nullptr )
        {
            dictionary_description = application.AddDictionaryDescription(DictionaryDescription(CS2WS(sDictFName), CS2WS(sParentFName),
                ( pAplDoc->GetEngineAppType() == EngineAppType::Tabulation ) ? DictionaryType::Working : DictionaryType::External));

            pAplDoc->SetModifiedFlag(TRUE);
        }

        if(pAplDoc->GetEngineAppType() == EngineAppType::Batch){
            bSpecialOutPut = TRUE;
        }
        //check if this dict is the main dict
        if(!sParentFName.IsEmpty()){
            if(application.GetFormFilenames().front().CompareNoCase(sParentFName)==0){
                //Check if the the first dictionary
                //For now since we know that .fmf / .ord contain only one dict we can go  through
                //later change it to support multiple dicts if it comes to that
                //SAVY&&&
                bMain =TRUE;
                dictionary_description->SetDictionaryType(DictionaryType::Input);
            }
        }

        dictTypeDlg.m_bMain = bMain;
        dictTypeDlg.m_bSpecialOutPut = bSpecialOutPut;
        switch(dictionary_description->GetDictionaryType()){
        case DictionaryType::Input:
            dictTypeDlg.m_iDType = 0;
            break;
        case DictionaryType::External:
            dictTypeDlg.m_iDType = 1;
            break;
        case DictionaryType::Working:
            dictTypeDlg.m_iDType = 2;
            break;
        case DictionaryType::Output:
            dictTypeDlg.m_iDType = 3;
            break;
        default:
            break;
        }

        int iOldDictType = dictTypeDlg.m_iDType;

        if( dictTypeDlg.DoModal() != IDOK || iOldDictType == dictTypeDlg.m_iDType )
            return;

        switch(dictTypeDlg.m_iDType)
        {
        case 0:
            dictionary_description->SetDictionaryType(DictionaryType::Input);
            break;
        case 1:
            dictionary_description->SetDictionaryType(DictionaryType::External);
            break;
        case 2:
            dictionary_description->SetDictionaryType(DictionaryType::Working);
            break;
        case 3:
            dictionary_description->SetDictionaryType(DictionaryType::Output);
            break;
        default:
            break;
        }

        pAplDoc->SetModifiedFlag(TRUE);
    }
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
    CDocument* pDoc = reinterpret_cast<CDocument*>(wParam);
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


LRESULT CMainFrame::OnGetDictionaryType(WPARAM wParam, LPARAM lParam)
{
    const CDataDict* dictionary = reinterpret_cast<const CDataDict*>(wParam); // [in] the dictionary object
    DictionaryType* out_dictionary_type = reinterpret_cast<DictionaryType*>(lParam); // [output] the dictionary type for the dictionary
    ASSERT(dictionary != nullptr && out_dictionary_type != nullptr);

    std::optional<DictionaryType> dictionary_type;

    ForeachDocument<CAplDoc>(
        [&](const CAplDoc& application_document)
        {
            DictionaryType this_dictionary_type = application_document.GetAppObject().GetDictionaryType(*dictionary);

            // if there are multiple applications using the dictionary, the lowest dictionary type is returned
            if( ( this_dictionary_type != DictionaryType::Unknown ) &&
                ( !dictionary_type.has_value() || (int)this_dictionary_type < (int)*dictionary_type ) )
            {
                dictionary_type = this_dictionary_type;
            }

            return true;
        });

    if( dictionary_type.has_value() )
    {
        *out_dictionary_type = *dictionary_type;
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
                    form_file->FindField(dict_element->GetName(), &pForm, &pBase);

                    CDEField* pField = DYNAMIC_DOWNCAST(CDEField, pBase);

                    if( pField != nullptr )
                    {
                        std::tuple<CDEItemBase*, CString> update(pField, dictionary.MakeQualifiedName(dictionary.GetOldName()));
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
    }

    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pDoc);

    if( pAplDoc != nullptr )
    {
        *ppApplication = &pAplDoc->GetAppObject();
        return 1;
    }

    return 0;
}


LRESULT CMainFrame::OnGetFormFileOrDictionary(WPARAM wParam, LPARAM /*lParam*/)
{
    std::variant<std::monostate, std::shared_ptr<const CDEFormFile>, std::shared_ptr<const CDataDict>>& form_file_or_dictionary =
        *reinterpret_cast<std::variant<std::monostate, std::shared_ptr<const CDEFormFile>, std::shared_ptr<const CDataDict>>*>(wParam);

    CMDIChildWnd* pWnd = MDIGetActive();
    CDocument* pDoc = ( pWnd != nullptr && pWnd->GetSafeHwnd() != nullptr ) ? pWnd->GetActiveDocument() : nullptr;

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

/////////////////////////////////////////////////////////////////////////////////

LRESULT CMainFrame::OnCreateUniqueName(WPARAM wParam, LPARAM lParam)
{
    // creates a name that is unique to the application
    CString& name = *(CString*)wParam;
    const CAplDoc* pAplDoc = (const CAplDoc*)lParam;

    CMDIChildWnd* pWnd = MDIGetActive();
    const CDocument* pDoc = ( pWnd != nullptr && pWnd->GetSafeHwnd() != nullptr ) ? pWnd->GetActiveDocument() : nullptr;

    name = WS2CS(CIMSAString::CreateUnreservedName(name,
            [&](const std::wstring& name_candidate)
            {
                return pAplDoc->IsNameUnique(pDoc, WS2CS(name_candidate));
            }));

    return 0;
}


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


LRESULT CMainFrame::OnShowCapiText(WPARAM wParam, LPARAM /*lParam*/)
{
    CFormDoc* pFormDoc = reinterpret_cast<CFormDoc*>(wParam);
    QSFView* pQTView = (QSFView*)pFormDoc->GetView(FormViewType::QuestionText);

    if (pQTView != nullptr && pQTView->IsWindowVisible())
    {
        CAplDoc* pAplDoc;
        CDEItemBase* pBase;
        std::tie(pAplDoc, pBase) = GetCapiItemDetails(pFormDoc);

        CString csQuestionText;

        if( pBase != nullptr )
        {
            // use the currently selected dictionary language if possible
            const auto& pDataDict = pFormDoc->GetFormFile().GetDictionary();

            if( pDataDict->GetLanguages().size() > 1 )
                csQuestionText = pAplDoc->GetCapiTextForFirstCondition(pBase, pDataDict->GetCurrentLanguage().GetName());

            else
                csQuestionText = pAplDoc->GetCapiTextForFirstCondition(pBase);
        }

        pQTView->SetText(csQuestionText);
    }

    return 0;
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

LRESULT CMainFrame::GetLangInfo(WPARAM wParam, LPARAM lParam)
{
    CArray<CLangInfo,CLangInfo&>* pArrInfo = (CArray<CLangInfo,CLangInfo&>*)(wParam);
    CFormDoc* pFormDoc = (CFormDoc*)lParam;
    ASSERT(pFormDoc);

    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pFormDoc);

    if(pAplDoc == nullptr){
        return 0;
    }
    Application* pApplication = &pAplDoc->GetAppObject();
    ASSERT(pApplication->GetEngineAppType() == EngineAppType::Entry);
    UNREFERENCED_PARAMETER(pApplication);
    pAplDoc->GetLangInfo(*pArrInfo);

    return 0;
}

LRESULT CMainFrame::ProcessLangs(WPARAM wParam, LPARAM lParam)
{
    CArray<CLangInfo,CLangInfo&>* pArrInfo = (CArray<CLangInfo,CLangInfo&>*)(wParam);
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


    CAplDoc* pDoc = GetApplicationUsingFormFile(pForm->GetPathName());

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

            CString sAppFName = PortableFunctions::PathAppendToPath(PortableFunctions::PathGetDirectory<CString>(pDoc->GetPathName()), _T("CSPro_Test.bch"));
            CString sPffFName = PortableFunctions::PathRemoveFileExtension<CString>(sAppFName) + FileExtensions::WithDot::Pff;

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
    for( const auto& message_text_source : pAplDoc->GetAppObject().GetMessageTextSources() )
        application.AddMessageFile(std::make_shared<TextSource>(message_text_source->GetFilename()));

    // add any reports
    for( const auto& report_named_text_source : pAplDoc->GetAppObject().GetReportNamedTextSources() )
        application.AddReport(report_named_text_source->name, std::make_shared<TextSource>(report_named_text_source->text_source->GetFilename()));

    //Add External dictionary names
    for( const auto& dictionary_filename : pAplDoc->GetAppObject().GetExternalDictionaryFilenames() )
        application.AddExternalDictionaryFilename(dictionary_filename);

    //Copy array of dict desc
    application.SetDictionaryDescriptions(pAplDoc->GetAppObject().GetDictionaryDescriptions());

    //copy the fmf file to ord .
    //set form files to dictionary order as false to have the order files open the spec without reordering
    for( const auto& sFormFName : pAplDoc->GetAppObject().GetFormFilenames() ) {
        CFormNodeID* pID = m_SizeDlgBar.m_FormTree.GetFormNode(sFormFName);
        if (pID != nullptr && pID->GetFormDoc()) {
            pID->GetFormDoc()->GetFormFile().SetDictOrder(false);
            pID->GetFormDoc()->SetModifiedFlag(true);
        }
    }
    pAplDoc->OnSaveDocument(pAplDoc->GetPathName());

    bool bRet = true;

    for(int iIndex = 0; iIndex < (int)pAplDoc->GetAppObject().GetFormFilenames().size(); iIndex++) {
        CString sFormFName = pAplDoc->GetAppObject().GetFormFilenames()[iIndex];
        CString sOrderFName = sFormFName;

        PathRemoveFileSpec(sOrderFName.GetBuffer(MAX_PATH));
        sOrderFName.ReleaseBuffer();
        CString csPath = sOrderFName;
        if(pAplDoc->GetAppObject().GetFormFilenames().size() > 1 ) {
            CString sGenName;
            sGenName.Format(_T("CSPro_Test%d.ord"),iIndex);
            sOrderFName = csPath +_T("\\")+ sGenName;
        }
        else {
            sOrderFName = csPath +_T("\\")+ _T("CSPro_Test.ord");
        }

        for( DictionaryDescription& dictionary_description : application.GetDictionaryDescriptions() )
        {
            if( SO::EqualsNoCase(dictionary_description.GetParentFilename(), sFormFName) )
                dictionary_description.SetParentFilename(CS2WS(sOrderFName));
        }

        //copy the  .fmf to .ord
        if(!CopyFile(sFormFName,sOrderFName,FALSE)){
            bRet =false;
        }
        application.AddFormFilename(sOrderFName);
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
    for( const auto& sFormFName : pAplDoc->GetAppObject().GetFormFilenames() ) {
        CFormNodeID* pID = m_SizeDlgBar.m_FormTree.GetFormNode(sFormFName);
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
    CString sAplFile = sAplFName + FileExtensions::WithDot::TabulationApplication;
    CString sPFFName = sAplFile + FileExtensions::WithDot::Pff;
    CString sListFile = sAplFile + FileExtensions::WithDot::Listing;

    CStringArray arrOldProc;
    bool bReplaceTblCode = false;
    CSourceCode* pSourceCode = pAplDoc->GetAppObject().GetAppSrcCode();

    if (pTabDoc/*->AreAplDictsOK()*/) {            // BMD  28 Jun 00
        TableSpecTabTreeNode* table_spec_tab_tree_node = pTabDoc->GetTabTreeCtrl()->GetTableSpecTabTreeNode(*pTabDoc);
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
            sTabOutPutFName = csAplFName+ FileExtensions::BinaryTable::WithDot::Tab;
            sTabTai = csAplFName + FileExtensions::BinaryTable::WithDot::TabIndex;
            pifFile.SetTabOutputFName(sTabOutPutFName);
            sTempTab = csAplFName + _T("_precalc") +  FileExtensions::BinaryTable::WithDot::Tab; // Engine is not looking at the piffile ags
            sTempTai = csAplFName + _T("_precalc") +  FileExtensions::BinaryTable::WithDot::TabIndex;

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
                const std::optional<std::wstring> tblview_exe =  CSProExecutables::GetExecutablePath(CSProExecutables::Program::TblView);
                if(tblview_exe.has_value()) {
                    IMSASpawnApp(*tblview_exe, IMSA_WNDCLASS_TABLEVIEW, sTbwFileName, TRUE);
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
            sSymbolName = pTabDoc->GetTableSpec()->GetDict()->GetLevel(iLevelNum).GetName();
        }

        if(sSymbolName.IsEmpty() && !bAppSrcCode)
            return true;
        CSourceCode* pSourceCode = pApplication->GetAppSrcCode();
        pSourceCode->SetOrder(pAplDoc->GetOrder());
        CStringArray arrProcLines;

        CIMSAString sString = WS2CS(pView->GetLogicCtrl()->GetText());
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

        ProcessParserMessages(pAplDoc, pSourceCode, pView, pWnd->GetLogicDialogBar());

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
        CString sLevelName = pTabSet->GetDict()->GetLevel(0).GetName();
        pSourceCode->RemoveProc(sLevelName,CSourceCode_AllEvents);
        CConsolidate* pConsolidate = pTabSet->GetConsolidate();
        CStringArray arrProcLines;
        if(pConsolidate &&  pConsolidate->GetNumAreas() > 0){
            //Remove the level proc for now .To regenerate Break by
            //Think abt code in the LEVEL PROC getting destroyed later
            if(!pSourceCode->IsProcAvailable(sLevelName)){
                //If not Level proc add one
                CStringArray sarrProcLines;
                sarrProcLines.Add(WS2CS(pAplDoc->GetAppObject().GetLogicSettings().GetGeneratedCodeTextForTextSource()));
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
            sSymbolName = pTabDoc->GetTableSpec()->GetDict()->GetLevel(iLevelNum).GetName();
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
        pView->GetEditCtrl()->SetText(sWindowText);

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

    DictionaryDescription dictionary_description(CS2WS(pApp->GetTabSpec()->GetDict()->GetName()), CS2WS(pOrder->GetName()), DictionaryType::Input);
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
    CString sPathName = pApp->GetApplicationFilename();

    sPathName.ReleaseBuffer();
    PathRemoveFileSpec(sPathName.GetBuffer(MAX_PATH));
    sPathName.ReleaseBuffer();


    CString sFile = sPathName + _T("\\CSTab.ord") ; //delete ord
    DeleteFile(sFile);
    sFile = sPathName + _T("\\CSTab") + FileExtensions::WithDot::Logic; //delete app
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
            CString sLevelName = pTabSet->GetDict()->GetLevel(0).GetName();

            CConsolidate* pConsolidate = pTabSet->GetConsolidate();
            if(pConsolidate &&  pConsolidate->GetNumAreas() > 0){
                //Remove the level proc for now .To regenerate Break by
                //Think abt code in the LEVEL PROC getting destroyed later
                if(!pSourceCode->IsProcAvailable(sLevelName)){
                    //If not Level proc add one
                    CStringArray arrProcLines;
                    arrProcLines.Add(WS2CS(pAplDoc->GetAppObject().GetLogicSettings().GetGeneratedCodeTextForTextSource()));
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
                arrProcLines.Add(WS2CS(pAplDoc->GetAppObject().GetLogicSettings().GetGeneratedCodeTextForTextSource()));
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
            arrTempProcLines.Add(WS2CS(pAplDoc->GetAppObject().GetLogicSettings().GetGeneratedCodeTextForTextSource()));

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
                arrProcLinesApp.Add(WS2CS(pAplDoc->GetAppObject().GetLogicSettings().GetGeneratedCodeTextForTextSource()));
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
            std::wstring all_error_messages;

            for( const Logic::ParserMessage& parser_message : CCompiler::GetCurrentSession()->GetParserMessages() )
            {
                if( parser_message.type == Logic::ParserMessage::Type::Error )
                    SO::AppendWithSeparator(all_error_messages, parser_message.message_text, _T("\r\n"));
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
            application_doc.ChangeCapiName(pItem, old_name);
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
            CString pff_filename = PortableFunctions::PathRemoveFileExtension<CString>(application_doc.GetPathName()) + FileExtensions::WithDot::Pff;

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
    CDocument* pDoc = pWnd->GetActiveDocument();
    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pDoc);
    pCmdUI->Enable(( pAplDoc == nullptr ) ? FALSE : TRUE);
}


void CMainFrame::OnOptionsProperties()
{
    CMDIChildWnd* pWnd = this->MDIGetActive();
    CDocument* pDoc = pWnd->GetActiveDocument();
    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pDoc);
    Application& application = pAplDoc->GetAppObject();

    PropertiesDlg properties_dlg(application);

    if( properties_dlg.DoModal() != IDOK )
        return;

    ApplicationProperties new_application_properties = properties_dlg.ReleaseApplicationProperties();
    std::wstring new_application_properties_filename = properties_dlg.GetApplicationPropertiesFilename();

    const bool application_properties_changed = ( application.GetApplicationProperties() != new_application_properties );
    bool application_properties_filename_changed = !SO::EqualsNoCase(application.GetApplicationPropertiesFilename(), new_application_properties_filename);
    const bool using_external_properties_file = !new_application_properties_filename.empty();

    if( application_properties_changed )
        application.SetApplicationProperties(std::move(new_application_properties));

    // save the application properties if they are external and changed, or if they are newly external
    if( using_external_properties_file && ( application_properties_changed || application_properties_filename_changed ) )
    {
        try
        {
            application.GetApplicationProperties().Save(new_application_properties_filename);
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(FormatText(_T("There was an error saving the application properties to '%s' so they will instead be saved to the application '%s':\n\n%s"),
                                  PortableFunctions::PathGetFilename(new_application_properties_filename),
                                  PortableFunctions::PathGetFilename(application.GetApplicationFilename()),
                                  exception.GetErrorMessage().c_str()));

            new_application_properties_filename.clear();
            application_properties_filename_changed = true;
        }
    }

    if( application_properties_filename_changed || ( !using_external_properties_file && application_properties_changed ) )
    {
        if( application_properties_filename_changed )
            application.SetApplicationPropertiesFilename(std::move(new_application_properties_filename));

        pAplDoc->SetModifiedFlag();
    }

    // some properties are not part of the ApplicationProperties object
    if( application.GetLogicSettings() != properties_dlg.GetLogicSettings() )
    {
        application.SetLogicSettings(properties_dlg.GetLogicSettings());
        pAplDoc->SetModifiedFlag();

        // refresh the Scintilla lexers in case the logic version changed
        auto refresh_lexer = [&](CLogicCtrl* logic_ctrl)
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


LRESULT CMainFrame::OnSetExternalApplicationProperties(WPARAM wParam, LPARAM lParam)
{
    const std::wstring* application_properties_filename = reinterpret_cast<const std::wstring*>(wParam);
    ApplicationProperties* application_properties = reinterpret_cast<ApplicationProperties*>(lParam);
    ASSERT(application_properties_filename != nullptr && application_properties != nullptr);
    ASSERT(PortableFunctions::FileIsRegular(*application_properties_filename));

    CMDIChildWnd* pWnd = this->MDIGetActive();
    CDocument* pDoc = pWnd->GetActiveDocument();
    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pDoc);
    Application& application = pAplDoc->GetAppObject();

    application.SetApplicationPropertiesFilename(*application_properties_filename);
    application.SetApplicationProperties(std::move(*application_properties));

    pAplDoc->SetModifiedFlag();

    return 1;
}


LRESULT CMainFrame::IsReservedWord(WPARAM wParam, LPARAM /*lParam*/)
{
    wstring_view* text = reinterpret_cast<wstring_view*>(wParam);
    return Logic::ReservedWords::IsReservedWord(*text) ? 1 : 0;
}


LRESULT CMainFrame::OnCapiMacros(WPARAM /*wParam*/,LPARAM /*lParam*/)
{
    CMDIChildWnd* pWnd = this->MDIGetActive();
    CDocument* pDoc = pWnd->GetActiveDocument();
    CAplDoc* pAplDoc = ProcessFOForSrcCode(*pDoc);

    CapiMacrosDlg capi_macros_dialog(pAplDoc);
    capi_macros_dialog.DoModal();

    return 0;
}


LRESULT CMainFrame::OnCanAddResourceFolder(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    CCSProApp* cspro_app = assert_cast<CCSProApp*>(AfxGetApp());
    return cspro_app->IsApplicationOpen();
}

LRESULT CMainFrame::OnCreateResourceFolder(WPARAM wParam, LPARAM lParam)
{
    auto resource_folder_name = reinterpret_cast<const CString*>(wParam);
    auto resource_folder_directory = reinterpret_cast<CString*>(lParam);

    CCSProApp* cspro_app = assert_cast<CCSProApp*>(AfxGetApp());
    std::optional<CAplDoc*> pAplDoc = cspro_app->GetActiveApplication(nullptr, _T("Select Application to Add Image To"));

    if( !pAplDoc.has_value() || pAplDoc == nullptr )
    {
        ASSERT(!pAplDoc.has_value());
        return 0;
    }

    CString application_directory = PortableFunctions::PathGetDirectory<CString>((*pAplDoc)->GetAppObject().GetApplicationFilename());
    *resource_folder_directory = PortableFunctions::PathAppendToPath<CString>(application_directory, *resource_folder_name);

    cspro_app->AddResourceFolderToApp(*pAplDoc, *resource_folder_directory);

    return 1;
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
        CString reload_message;

        for( const auto& text_source : text_sources )
        {
            int64_t file_modified_time = PortableFunctions::FileModifiedTime(text_source->GetFilename());

            if( file_modified_time > last_check_timestamp && text_source->GetModifiedIteration() < file_modified_time )
            {
                modified_text_sources.insert(text_source);
                reload_message.AppendFormat(_T("\"%s\"\n"), text_source->GetFilename().c_str());
            }
        }

        if( !modified_text_sources.empty() )
        {
            bool pluralize = ( modified_text_sources.size() > 1 );

            reload_message.AppendFormat(_T("\nThe file%s been modified by another program.\nDo you want to reload %s?"),
                pluralize ? _T("s have") : _T(" has"),
                pluralize ? _T("them") : _T("it"));

            if( AfxMessageBox(reload_message, MB_YESNO) == IDYES )
            {
                // reload any text sources modified outside of the application, ignoring any exceptions
                for( TextSourceEditable* modified_text_source : modified_text_sources )
                {
                    try
                    {
                        modified_text_source->ReloadFromDisk();
                    }

                    catch( const CSProException& ) { }
                }

                // if currently viewing external code or reports, refresh the view
                CMDIFrameWnd* pWnd = (CMDIFrameWnd*)MDIGetActive();

                if( pWnd != nullptr )
                {
                    CDocument* pDoc = pWnd->GetActiveDocument();

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


LRESULT CMainFrame::OnFindOpenTextSourceEditable(WPARAM wParam, LPARAM lParam)
{
    // when using an editable text source, if it is open in another application, use the existing object
    // (for now this used only for logic and report files, not message files)
    const std::wstring& filename = *reinterpret_cast<const std::wstring*>(wParam);
    std::shared_ptr<TextSourceEditable>& out_text_source = *reinterpret_cast<std::shared_ptr<TextSourceEditable>*>(lParam);
    ASSERT(out_text_source == nullptr);

    ForeachLogicAndReportTextSource(
        [&](const std::shared_ptr<TextSourceEditable>& text_source, bool /*main_logic_file*/)
        {
            if( SO::EqualsNoCase(text_source->GetFilename(), filename) )
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

        TreeCtrlHelpers::FindInTree(*pTreeCtrl, pTreeCtrl->GetRootItem(),
            [&](HTREEITEM hItem)
            {
                const item_data_type* item_data = reinterpret_cast<const item_data_type*>(pTreeCtrl->GetItemData(hItem));

                if( item_data->GetTextSource() != nullptr && SO::EqualsNoCase(item_data->GetTextSource()->GetFilename(), filename) )
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

    CDocument* pDoc = assert_cast<CDocument*>(application_child_wnd->GetActiveDocument());

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

        if( !pFormDoc->GetFormFile().FindField(WS2CS(logic_error_location.parser_message.proc_name), &form, &item_base) )
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
            const std::vector<std::shared_ptr<TextSource>>& message_text_sources = pAplDoc->GetAppObject().GetMessageTextSources();

            if( !message_text_sources.empty() &&
                SO::EqualsNoCase(message_text_sources.front()->GetFilename(), logic_error_location.parser_message.compilation_unit_name) )
            {
                LogicDialogBar& logic_dialog_bar = application_child_wnd->GetLogicDialogBar();
                logic_dialog_bar.SelectMessageEditTab();

                MessageEditCtrl* message_edit_ctrl = logic_dialog_bar.GetMessageEditCtrl();
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
                                      logic_error_location.parser_message.compilation_unit_name,
                                      true, static_cast<int>(logic_error_location.parser_message.line_number) - 1);
    }


    return 1;
}


LRESULT CMainFrame::OnGetLexerLanguage(WPARAM wParam, LPARAM lParam)
{
    const CLogicCtrl* logic_ctrl = reinterpret_cast<const CLogicCtrl*>(wParam);
    int& logic_language = *reinterpret_cast<int*>(lParam);

    CMDIChildWnd* pWnd = this->MDIGetActive();
    CAplDoc* pAplDoc;

    if( pWnd == nullptr || ( pAplDoc = ProcessFOForSrcCode(*pWnd->GetActiveDocument()) ) == nullptr )
        return 0;

    // check if this is a message file
    ApplicationChildWnd* application_child_wnd = dynamic_cast<ApplicationChildWnd*>(pWnd);

    if( application_child_wnd != nullptr &&
        application_child_wnd->GetLogicDialogBar().GetMessageEditCtrl() != nullptr &&
        logic_ctrl == application_child_wnd->GetLogicDialogBar().GetMessageEditCtrl() )
    {
        logic_language = Lexers::GetLexer_Message(pAplDoc->GetAppObject());
        return 1;
    }

    // otherwise check if this is a report
    bool is_report = false;
    bool is_report_html_type = false;

    auto set_report_flags = [&](const TextSource* text_source)
    {
        is_report = true;
        is_report_html_type = FileExtensions::IsFilenameHtml(text_source->GetFilename());
    };

    if( pWnd->GetActiveDocument()->IsKindOf(RUNTIME_CLASS(CFormDoc)) )
    {
        CFormID* pFormID = GetNodeIdForSourceCode<CFormID>();

        if( pFormID->GetItemType() == eFFT_REPORT )
            set_report_flags(pFormID->GetTextSource());
    }

    else if( pWnd->GetActiveDocument()->IsKindOf(RUNTIME_CLASS(COrderDoc)) )
    {
        const AppTreeNode* app_tree_node = GetNodeIdForSourceCode<AppTreeNode>();

        if( app_tree_node->GetAppFileType() == AppFileType::Report )
            set_report_flags(app_tree_node->GetTextSource());
    }

    logic_language = is_report ? Lexers::GetLexer_Report(pAplDoc->GetAppObject(), is_report_html_type) :
                                 Lexers::GetLexer_Logic(pAplDoc->GetAppObject());

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

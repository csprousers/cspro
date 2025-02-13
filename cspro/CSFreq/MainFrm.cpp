// MainFrm.cpp : implementation of the CMainFrame class
//

#include "StdAfx.h"
#include "MainFrm.h"
#include "CSFreq.h"
#include <zInterfaceF/UWM.h>


/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    //{{AFX_MSG_MAP(CMainFrame)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    ON_WM_CREATE()
    ON_WM_MENUCHAR()
    //}}AFX_MSG_MAP
    // Global help commands
    ON_COMMAND(ID_HELP_FINDER, OnHelpFinder)
    ON_COMMAND(ID_HELP, OnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, OnContextHelp)
    ON_COMMAND(ID_DEFAULT_HELP, OnHelpFinder)
    ON_MESSAGE(UWM::Interface::SelectLanguage, OnSelectLanguage)
    ON_MESSAGE(UWM::Edit::GetLexerLanguage, OnGetLexerLanguage)
END_MESSAGE_MAP()

static UINT indicators[] =
{
    ID_SEPARATOR,           // status line indicator
    ID_INDICATOR_CAPS,
    ID_INDICATOR_NUM,
    ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP) ||
        !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
    {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }

    //Create language bar
    if (!m_wndLangDlgBar.Create(this, IDD_LANGDLGBAR, CBRS_ALIGN_TOP, IDD_LANGDLGBAR))
    {
        TRACE0("Failed to create language dialogbar \n");
        return -1;      // fail to create
    }

    // Create rebar
    if (!m_wndReBar.Create(this) ||
        !m_wndReBar.AddBar(&m_wndToolBar) ||
        !m_wndReBar.AddBar(&m_wndLangDlgBar))
    {
        /*
            These ints are positions of the bars defined at the top of this file. I
            While adding a bar make sure that the positions are specified corrrectly.
            const int TOOLBARPOS = 0;
            const int LANGTOOLBARPOS = 1;
        */
        TRACE0("Failed to create rebar\n");
        return -1;      // fail to create
    }

    //  Places band 1 (Language Bar) flush next to toolbar.
    m_wndReBar.GetReBarCtrl().RestoreBand(1);

    //  default the language bar off.
    m_wndReBar.GetReBarCtrl().ShowBand(1, FALSE);

    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators,
          sizeof(indicators)/sizeof(UINT)))
    {
        TRACE0("Failed to create status bar\n");
        return -1;      // fail to create
    }

    // TODO: Remove this if you don't want tool tips
    m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
        CBRS_TOOLTIPS | CBRS_FLYBY);

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

/////////////////////////////////////////////////////////////////////////////////
//
//  BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
//
/////////////////////////////////////////////////////////////////////////////////
BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
    BOOL bRet = FALSE;
    UNREFERENCED_PARAMETER(lpcs);

    // Create splitter window
    VERIFY(m_wndSplitter.CreateStatic(this,1,2));

    VERIFY(m_wndSplitter.CreateView(0,0,RUNTIME_CLASS(CSFreqView), CSize(0,0), pContext));
    VERIFY(m_wndSplitter.CreateView(0,1,RUNTIME_CLASS(CFrqOptionsView), CSize(0,0), pContext));


    //Set the splitter pane size
    CRect r;
    GetClientRect(&r);
    int w1 = (int)(0.25*r.Width());
    int w2 = (int)(0.75*r.Width());
    m_wndSplitter.SetColumnInfo( 0, w1, 0 );
    m_wndSplitter.SetColumnInfo( 1, w2, 0 );
    m_wndSplitter.RecalcLayout();
    // Splitter pane sizing done .

    bRet = TRUE;

    return bRet;
}



/////////////////////////////////////////////////////////////////////////////////
//
//  CSFreqView* CMainFrame::GetFreqTreeView()
//
/////////////////////////////////////////////////////////////////////////////////
CSFreqView* CMainFrame::GetFreqTreeView()
{
    return ((CSFreqView*) m_wndSplitter.GetPane(0,0));
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CFrqOptionsView* CMainFrame::GetFrqOptionsView()
//
/////////////////////////////////////////////////////////////////////////////////
CFrqOptionsView* CMainFrame::GetFrqOptionsView()
{
    if(m_wndSplitter.GetSafeHwnd()){
        return ((CFrqOptionsView*) m_wndSplitter.GetPane(0,1));
    }
    else {
        return NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

LRESULT CMainFrame::OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu)
{
  LRESULT lresult;
  if(BCMenu::IsMenu(pMenu))
    lresult=BCMenu::FindKeyboardShortcut(nChar, nFlags, pMenu);
    else
    lresult=CFrameWnd::OnMenuChar(nChar, nFlags, pMenu);
  return(lresult);
}


void CMainFrame::OnUpdateFrameTitle(BOOL /*bAddToTitle*/)
{
    CString csAppName;
    csAppName.Format(AFX_IDS_APP_TITLE);

    CSFreqDoc* pDoc = (CSFreqDoc*)GetActiveDocument();
    CString csDocumentTitle = pDoc->GetDocumentWindowTitle();

    CString csTitle;
    csTitle.Format(_T("%s%s%s"), (LPCTSTR)csDocumentTitle, csDocumentTitle.IsEmpty() ? _T("") : _T(" - "), (LPCTSTR)csAppName);

    SetWindowText(csTitle);
}


LRESULT CMainFrame::OnSelectLanguage(WPARAM wParam, LPARAM /*lParam*/)
{
    const CSFreqDoc* pDoc = assert_nullable_cast<const CSFreqDoc*>(GetActiveDocument());
    if (pDoc)
    {
        const CDataDict* pDataDict = pDoc->GetDataDict();
        if (pDataDict)
        {
            pDataDict->SetCurrentLanguage(wParam);
            // redraw the tree.
            CSFreqView* pFrqTreeView = GetFreqTreeView();
            pFrqTreeView->RefreshTree();
        }
    }

    return 1;
}


LRESULT CMainFrame::OnGetLexerLanguage(WPARAM /*wParam*/, LPARAM lParam)
{
    int& logic_language = *reinterpret_cast<int*>(lParam);

    const CSFreqDoc* pDoc = assert_cast<const CSFreqDoc*>(GetActiveDocument());
    logic_language = Lexers::GetLexer_Logic(pDoc->GetLogicSettings());

    return 1;
}

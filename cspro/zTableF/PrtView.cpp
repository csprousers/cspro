// todo:
// - figure out the MISSING constant (search for 1.0e50)
// - change
//    for (int iFmtType=(int)FMT_ID_SPANNER ; iFmtType<=(int)FMT_ID_TBLPRINT ; iFmtType++) {
// to
//    for (int iFmtType=(int)FMT_ID_FIRST ; iFmtType<=(int)FMT_ID_LAST ; iFmtType++) {
//
// HTML output:
// - use try/catch for putline; nuke iStatus


//

// PrtView.cpp : implementation file
//

#include "StdAfx.h"
#include "TabDoc.h"
#include "PrtView.h"
#include "facing.h"
#include "PrtView.h"
#include "TblPFmtD.h"
#include "PrtVDlg.h"
#include "TblPrtDg.h"
#include "TabChWnd.h"
#include "FlashMsg.h"
#include <zUtilO/BCMenu.h>
#include <zUtilO/Pgsetup.h>
#include <zUToolO/TreePrps.h>
#include <zGridO/Ugcell.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabPrtView
#define TABTOOLBAR     997         //this is defined in the CSPro CMainFrame class create
#define SAMPTOOLBAR    1000       //this is defined in the CSPro CMainFrame class create //Savy (R) sampling app 20081231

IMPLEMENT_DYNCREATE(CTabPrtView, CView)
IMPLEMENT_DYNAMIC(CPgOb, CObject)

static const int   OUTSIDE_BORDER = 15;   // device units (screen only) (note that the outside border might be larger than this, if the page(s) are centered due to aspect ratios)
static const int   INSIDE_BORDER = 10;    // device units (screen only)
static const int   THREE_D_BORDER = 4;    // device units (screen only)
static const int   STUB_INDENT = (int)(TWIPS_PER_INCH/4);  // logical units -- 1/4"

static const int   RESIZE_BAR_WIDTH_LP = 40;
static const int   SCROLL_LINE_SIZE = 20;     // how much to increment when scrolling line by line (that is, client area width or height / SCROLL_LINE_SIZE)
static const int   CELL_PADDING_LEFT=36;      // whitespace between a pgob's contents and its left border; expressed in TWIPS
static const int   CELL_PADDING_TOP=18;       // whitespace between a pgob's contents and its top border; expressed in TWIPS
static const int   CELL_PADDING_RIGHT=36;     // whitespace between a pgob's contents and its right border; expressed in TWIPS
static const int   CELL_PADDING_BOTTOM=18;    // whitespace between a pgob's contents and its bottom border; expressed in TWIPS

/////////////////////////////////////////////////////////////////////////////////
//
//  BOOL Dbg_Dump_Tree(CRowColPgOb* pRCPgOb, int iIndent=0)
//  void Dbg_Dump_Page(const CPgLayout& pl)
//  void Dbg_Dump_FmtReg(const CFmtReg* pFmtReg)
//
/////////////////////////////////////////////////////////////////////////////////
CString Dbg_Get_Type(PAGEOB_TYPE type)
{
    CString sType;
    switch (type) {
    case PGOB_STUBHEAD:
        sType=_T("stubhead");
        break;
    case PGOB_STUB:
        sType=_T("stub");
        break;
    case PGOB_STUB_RIGHT:
        sType=_T("stub");
        break;
    case PGOB_READER_BREAK:
        sType=_T("reader break");
        break;
    case PGOB_CAPTION:
        sType=_T("caption");
        break;
    case PGOB_COLHEAD:
        sType=_T("colhead");
        break;
    case PGOB_SPANNER:
        sType=_T("spanner");
        break;
    case PGOB_ROOT:
        sType=_T("root");
        break;
    case PGOB_HEADER_LEFT:
        sType=_T("left header");
        break;
    case PGOB_HEADER_CENTER:
        sType=_T("center header");
        break;
    case PGOB_HEADER_RIGHT:
        sType=_T("right header");
        break;
    case PGOB_FOOTER_LEFT:
        sType=_T("left footer");
        break;
    case PGOB_FOOTER_CENTER:
        sType=_T("center footer");
        break;
    case PGOB_FOOTER_RIGHT:
        sType=_T("right footer");
        break;
    case PGOB_TITLE:
        sType=_T("title");
        break;
    case PGOB_SUBTITLE:
        sType=_T("sub title");
        break;
    case PGOB_PAGENOTE:
        sType=_T("page note");
        break;
    case PGOB_ENDNOTE:
        sType=_T("end note");
        break;
    case PGOB_NOTINCLUDED:
        sType=_T("excluded");
        break;
    case PGOB_DATACELL:
        sType=_T("data cell");
        break;
    default:
        ASSERT(FALSE);
    }
    return sType;
}

BOOL Dbg_Dump_Tree(CRowColPgOb* pRCPgOb, int iIndent=0, bool bIncludeHidden=false)
{
    ASSERT(pRCPgOb->GetType()!=PGOB_UNDETERMINED);

    bool bHidden=false;

    if (pRCPgOb->GetFmt()) {
        bHidden=(pRCPgOb->GetFmt()->GetHidden()==HIDDEN_YES);
    }

    if (bIncludeHidden || !bHidden) {
        CIMSAString sPad(SPACE, iIndent);
        CIMSAString sText, sHidden;
        if (pRCPgOb->GetType()==PGOB_ROOT) {
            sText=_T("***ROOT***");
        }
        else if (pRCPgOb->GetType()==PGOB_NOTINCLUDED) {
            sText=_T("(excluded)");
        }
        else {
            sText=pRCPgOb->GetText();
        }
        CRect rc = pRCPgOb->GetClientRectLP();
        if (bHidden) {
            sHidden=_T("(HIDDEN)");
        }

        CIMSAString sFmt(_T("NULL"));
        if (pRCPgOb->GetFmt()) {
            sFmt=pRCPgOb->GetFmt()->GetIDString();
        }

        TRACE(_T("%s%d  (%d %d %d %d, w=%d, h=%d) ==> %s (%s %s, HORZPG=%d vertpg=%d)  (extra=%d %d, %s)  %p %s\n"),
            (LPCTSTR)sPad, pRCPgOb->GetLevel(), rc.left, rc.top, rc.right, rc.bottom, rc.Width(), rc.Height(),
            (LPCTSTR)sText, (LPCTSTR)Dbg_Get_Type(pRCPgOb->GetType()), (LPCTSTR)sFmt, pRCPgOb->GetHPage(), pRCPgOb->GetVPage(),
            pRCPgOb->GetExtraLP().cx, pRCPgOb->GetExtraLP().cy, (pRCPgOb->IsCustom()?_T("Yes"):_T("No")), pRCPgOb, (LPCTSTR)sHidden);
    }

    for (int i=0 ; i<pRCPgOb->GetNumChildren() ; i++) {
        Dbg_Dump_Tree(pRCPgOb->GetChild(i), iIndent+4);
    }
    return TRUE;
}

void Dbg_Dump_Page(const CPgLayout& pl)
{
    for (int iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
        const CPgOb& pgob=pl.GetPgOb(iPgOb);
        TRACE(_T("%d: %s\n"),iPgOb, (LPCTSTR)Dbg_Get_Type(pgob.GetType()));
    }
}

void Dbg_Dump_FmtReg(const CFmtReg* pFmtReg)
{
    pFmtReg->Debug();
}

/////////////////////////////////////////////////////////////////////////////
//
//        GetZoomScaleFactor
//
/////////////////////////////////////////////////////////////////////////////
float CTabPrtView::GetZoomScaleFactor() const
{
    float f=0.00f;
    switch(GetZoomState()) {
    case ZOOM_STATE_4H_4V:
    case ZOOM_STATE_4H_3V:
        f=1.00f;
        break;
    case ZOOM_STATE_3H_3V:
    case ZOOM_STATE_3H_2V:
        f=1.00f;
        break;
    case ZOOM_STATE_2H_2V:
    case ZOOM_STATE_2H_1V:
        f=1.00f;
        break;
    case ZOOM_STATE_100_PERCENT:
        f=1.00f;
        break;
    case ZOOM_STATE_125_PERCENT:
        f=1.25f;
        break;
    case ZOOM_STATE_150_PERCENT:
        f=1.50f;
        break;
    case ZOOM_STATE_175_PERCENT:
        f=1.75f;
        break;
    case ZOOM_STATE_200_PERCENT:
        f=2.00f;
        break;
    case ZOOM_STATE_225_PERCENT:
        f=2.25f;
        break;
    case ZOOM_STATE_250_PERCENT:
        f=2.50f;
        break;
    case ZOOM_STATE_275_PERCENT:
        f=2.75f;
        break;
    case ZOOM_STATE_300_PERCENT:
        f=3.00f;
        break;
    default:
        ASSERT(FALSE);
    }
    return f;
}

CPrtViewNavigationBar& CTabPrtView::GetNavBar()
{
	if(GetParentFrame()->IsKindOf(RUNTIME_CLASS(CTableChildWnd))){
		return ((CTableChildWnd*)GetParentFrame())->m_wndNavDlgBar;
	}
	else if(this->m_pNavBar){
		return *(this->m_pNavBar);
	}
	else // we should not end up here
	{
#ifdef _DEBUG
		abort();
#endif
		return *(this->m_pNavBar);
	}
}

/////////////////////////////////////////////////////////////////////////////
//
//        CTabPrtView ctor
//
/////////////////////////////////////////////////////////////////////////////
CTabPrtView::CTabPrtView() : m_szPagesToView(1,1), m_szSavePagesToView(1,1)
{
	m_pNavBar = NULL;				//Savy (R) sampling app 20081230
    m_iCurrPrintPg = NONE;         // uninitialized
    m_aiViewPg.Add(0);             // start off showing the first page
    m_pgMgr.RemoveAllPages();
    m_pHSBar = new CScrollBar();
    m_pVSBar = new CScrollBar();
    m_bInitialized = false;
    m_eZoomState=ZOOM_STATE_100_PERCENT;
    m_bBookLayout=true;
    m_aSelected.RemoveAll();
    m_eResizeOrientation=RESIZE_INACTIVE;
    m_ResizeOb.SetPg(NONE);
    m_ResizeOb.SetPgOb(NONE);
    m_bApplyAcrossPanels=false;
    m_bAutoFitColumns=true;
    m_pPrtDC=NULL;
    m_iLogPixelsY=NONE;
    m_bForceRemeasure=false;
	m_pTabSet = NULL;
}

CTabPrtView::CTabPrtView(CTabSet* pTabSet): m_szPagesToView(1,1), m_szSavePagesToView(1,1)
{
	m_pNavBar = NULL;				//Savy (R) sampling app 20081230
	 m_iCurrPrintPg = NONE;         // uninitialized
    m_aiViewPg.Add(0);             // start off showing the first page
    m_pgMgr.RemoveAllPages();
    m_pHSBar = new CScrollBar();
    m_pVSBar = new CScrollBar();
    m_bInitialized = false;
    m_eZoomState=ZOOM_STATE_100_PERCENT;
    m_bBookLayout=true;
    m_aSelected.RemoveAll();
    m_eResizeOrientation=RESIZE_INACTIVE;
    m_ResizeOb.SetPg(NONE);
    m_ResizeOb.SetPgOb(NONE);
    m_bApplyAcrossPanels=false;
    m_bAutoFitColumns=true;
    m_pPrtDC=NULL;
    m_iLogPixelsY=NONE;
    m_bForceRemeasure=false;
	m_pTabSet = pTabSet;
}//Today SAvy changes 20081224
/////////////////////////////////////////////////////////////////////////////
//
//        ~CTabPrtView dtor
//
/////////////////////////////////////////////////////////////////////////////
CTabPrtView::~CTabPrtView()
{
    AfxGetApp()->WriteProfileInt(_T("Settings"),_T("ScaleFactor"),m_iScaleFactor);
    AfxGetApp()->WriteProfileInt(_T("Settings"),_T("ViewPagesHorz"),m_szPagesToView.cx);
    AfxGetApp()->WriteProfileInt(_T("Settings"),_T("ViewPagesVert"),m_szPagesToView.cy);
    AfxGetApp()->WriteProfileInt(_T("Settings"),_T("BookLayout"),m_bBookLayout);
    AfxGetApp()->WriteProfileInt(_T("Settings"),_T("ApplyAcrossPanels"),m_bApplyAcrossPanels);
    AfxGetApp()->WriteProfileInt(_T("Settings"),_T("AutoFitColumns"),m_bAutoFitColumns);

    ASSERT_VALID(m_pVSBar);
    ASSERT_VALID(m_pHSBar);
    delete m_pVSBar;
    delete m_pHSBar;

    ASSERT_VALID(m_pPrtDC);
    m_pPrtDC->Detach();
    SAFE_DELETE(m_pPrtDC);
}


BEGIN_MESSAGE_MAP(CTabPrtView, CView)
    //{{AFX_MSG_MAP(CTabPrtView)
    ON_WM_SIZE()
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_WM_ERASEBKGND()
    ON_WM_PAINT()
    ON_WM_KEYDOWN()
    ON_WM_SETCURSOR()
    ON_COMMAND(ID_VIEW_FACING, OnViewFacing)
    ON_COMMAND(ID_VIEW_BOOKLAYOUT, OnViewBookLayout)
    ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOM, OnUpdateViewZoom)
    ON_UPDATE_COMMAND_UI(ID_VIEW_FACING, OnUpdateViewFacing)
    ON_UPDATE_COMMAND_UI(ID_VIEW_BOOKLAYOUT, OnUpdateViewBooklayout)
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEWHEEL()
    //}}AFX_MSG_MAP
    // Standard printing commands
    ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)

    // Other commands
    ON_COMMAND(ID_EDIT_TBL_PRINTFMT, OnEditTablePrintFmt)
    ON_COMMAND(ID_EDIT_COL_BREAK, OnEditColBreak)
    ON_COMMAND(ID_EDIT_STUB_BREAK, OnEditStubBreak)
    ON_COMMAND(ID_EDIT_APPLY_ACROSS_PANELS, OnEditApplyAcrossPanels)
    ON_COMMAND(ID_EDIT_AUTOFIT_COLUMNS, OnEditAutoFitColumns)
    ON_COMMAND(ID_EDIT_RESTORE_PRTVIEW_DEFAULTS, OnRestorePrtViewDefaults)
    ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
    ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
    ON_COMMAND(ID_VIEW_FIRST_PAGE, OnViewFirstPage)
    ON_COMMAND(ID_VIEW_LAST_PAGE, OnViewLastPage)
    ON_COMMAND(ID_VIEW_PREV_PAGE, OnViewPrevPage)
    ON_COMMAND(ID_VIEW_NEXT_PAGE, OnViewNextPage)
    ON_COMMAND(ID_EDIT_GOTO, OnGoto)
    ON_UPDATE_COMMAND_UI(ID_VIEW_FIRST_PAGE, OnUpdateViewFirstPage)
    ON_UPDATE_COMMAND_UI(ID_VIEW_LAST_PAGE, OnUpdateViewLastPage)
    ON_UPDATE_COMMAND_UI(ID_VIEW_PREV_PAGE, OnUpdateViewPrevPage)
    ON_UPDATE_COMMAND_UI(ID_VIEW_NEXT_PAGE, OnUpdateViewNextPage)
    ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
    ON_MESSAGE(UWM::Table::Zoom, OnZoom)
    ON_WM_CONTEXTMENU()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_CBN_SELCHANGE(ID_TAB_ZOOM_COMBO, OnSelChangeZoomComboBox)
    ON_COMMAND(ID_VIEW_ZOOM_IN, OnViewZoomIn)
    ON_COMMAND(ID_VIEW_ZOOM_OUT, OnViewZoomOut)
    ON_COMMAND(ID_PRINTVIEW_CLOSE, OnPrintViewClose)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
//
//        OnInitialUpdate
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnInitialUpdate()
{
    ((CMDIFrameWnd*)AfxGetMainWnd())->RecalcLayout();

    CView::OnInitialUpdate();

    // fill initial values for zoom combo box
    InitZoomCombo();

    // set up the scroll bars (use dummy values; these change dyanmically via OnSize)
    CRect rc;
    GetClientRect(&rc);
    m_pVSBar->Create(WS_CHILD|WS_VISIBLE|SBS_VERT|SBS_RIGHTALIGN, rc, this, 101);
    m_pHSBar->Create(WS_CHILD|WS_VISIBLE|SBS_HORZ|SBS_BOTTOMALIGN, rc, this, 102);

    m_pVSBar->GetWindowRect(&rc);
    m_szSBar.cx = rc.Width();
    m_pHSBar->GetWindowRect(&rc);
    m_szSBar.cy = rc.Height();
    ASSERT(m_szSBar == CSize(::GetSystemMetrics(SM_CXVSCROLL), ::GetSystemMetrics(SM_CYHSCROLL)));

    GetClientRect(&rc);

    // get number of concurrent pages from registry
    m_szPagesToView.cx = AfxGetApp()->GetProfileInt(_T("Settings"),_T("ViewPagesHorz"),1);
    m_szPagesToView.cy = AfxGetApp()->GetProfileInt(_T("Settings"),_T("ViewPagesVert"),1);

    if (m_szPagesToView.cx==0) {
        m_szPagesToView.cx=1;
    }
    if (m_szPagesToView.cy==0) {
        m_szPagesToView.cy=1;
    }
    SetZoomState(m_szPagesToView);

    // get book layout setting from registry
    SetBookLayout(AfxGetApp()->GetProfileInt(_T("Settings"),_T("BookLayout"),0)?true:false);

    // get "apply changes across panels" setting from registry
    SetApplyAcrossPanels(AfxGetApp()->GetProfileInt(_T("Settings"),_T("ApplyAcrossPanels"),0)?true:false);

    // get "auto fit coumns" setting from registry
    SetAutoFitColumns(AfxGetApp()->GetProfileInt(_T("Settings"),_T("AutoFitColumns"),1)?true:false);

    // get scaling factor from registry
    // scaling precision; 1=very quick (nice fonts, very fast, poor layout) ... 10=a lot (awkward font rendering, slow, perfect layout)
    m_iScaleFactor = AfxGetApp()->GetProfileInt(_T("Settings"),_T("ScaleFactor"),5);
    if (m_iScaleFactor<1 || m_iScaleFactor>10)  {
        AfxMessageBox(_T("Scaling factor is out of range; value must be between 1 and 10.\n\nSetting scaling factor=5 instead."), MB_ICONEXCLAMATION);
        m_iScaleFactor=5;
    }

    m_undoStack.ClearUndo();
    m_undoStack.ClearRedo();

    if (!PreparePrinterDC()) {
        // error ... we can't render
        return;
    }

    ASSERT(GetDC()->GetDeviceCaps(LOGPIXELSY)==GetDC()->GetDeviceCaps(LOGPIXELSY));
    m_iLogPixelsY=GetDC()->GetDeviceCaps(LOGPIXELSY);

    //////////////////////////////////////////////////////////////////////////////////////////////
    // build the layout
    m_bInitialized = true;
    Build();

    // show current page number (should be page 1) on the navigation bar
    GetNavBar().SetPageInfo(GetCurrFirstViewPg()+1, m_pgMgr.GetNumPages());
}


/////////////////////////////////////////////////////////////////////////////
//
//        PrepareMapMode
//
//  Prepares map mode for window (logical units) and viewport (device units),
//  using TWIPS and the paper size/type for page iPage.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::PrepareMapMode(CDC* pDC, int iPage)
{
    if (m_pgMgr.GetNumPages()==0) {
        // not fully initialized yet
        return;
    }

    float fPgWidthInches = m_pgMgr.GetPgLayout(iPage).GetPgWidthInches();
    float fPgHgtInches = m_pgMgr.GetPgLayout(iPage).GetPgHgtInches();

    // set up mapping system so that (0,0) is in upper left corner, +x --> right, +y down (like MM_TEXT)
    pDC->SetMapMode(MM_ISOTROPIC);
    pDC->SetWindowOrg(0,0);
    pDC->SetWindowExt((int)(TWIPS_PER_INCH * fPgWidthInches), (int)(TWIPS_PER_INCH * fPgHgtInches));  // window is sized to table's paper (logical)
    pDC->SetViewportOrg(0,0);
    pDC->SetViewportExt((int)(pDC->GetDeviceCaps(LOGPIXELSX) * fPgWidthInches), (int)(pDC->GetDeviceCaps(LOGPIXELSY) * fPgHgtInches));  // viewport is also sized to tbl's paper (device)

    CSize szW = pDC->GetWindowExt();
    CSize szV = pDC->GetViewportExt();

    // prepare DC differently if we are printing, as opposed to displaying on the screen
    //    if displaying on screen: scale viewport to accommodate current client rect
    //    if printing, account for physical page margins

    if (pDC->IsPrinting())  {
        // printing ... adjust viewport org to account for non-printable area
        CPoint ptPhysOffset(pDC->GetDeviceCaps(PHYSICALOFFSETX), pDC->GetDeviceCaps(PHYSICALOFFSETY));
        pDC->SetViewportOrg(-ptPhysOffset);
    }
    else {
        // map mode scaling is handled in OnDraw, for each page being viewed concurrently

        // do nothing
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        PreparePrinterDC
//
//  Prepares printer device context
//
/////////////////////////////////////////////////////////////////////////////
bool CTabPrtView::PreparePrinterDC()
{
    SAFE_DELETE(m_pPrtDC);

    m_pPrtDC=new CDC();

    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());

    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}

    const CFmtReg& fmtReg=pSet->GetFmtReg();
    CTblPrintFmt* pTblPrintFmt=DYNAMIC_DOWNCAST(CTblPrintFmt,fmtReg.GetFmt(FMT_ID_TBLPRINT,0));

    CString sPrinterDevice(pTblPrintFmt->GetPrinterDevice());
    if (sPrinterDevice.IsEmpty()) {
        AfxMessageBox(_T("No printer declared!"));
        return false;
    }

    HGLOBAL hDevMode=NULL;
    HGLOBAL hDevNames=NULL;
    TCHAR* pszPrinterDevice=sPrinterDevice.GetBuffer(sPrinterDevice.GetLength());
    sPrinterDevice.ReleaseBuffer();
    if (GetPrinterDevice(pszPrinterDevice, &hDevNames, &hDevMode)) {
        AfxGetApp()->SelectPrinter(hDevNames, hDevMode);
        if(!AfxGetApp()->CreatePrinterDC(*m_pPrtDC)){
            AfxMessageBox(_T("Error attaching to printer ")+sPrinterDevice);
            return false;
        }
    }
    else {
        AfxMessageBox(_T("Error attaching to printer ")+sPrinterDevice);
        return false;
    }
    return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//        OnPrepareDC
//
//  Called to prepare DC when we are printing.  Note that this function
//  is not used for drawing on the screen (that's done through PrepareMapMode
//  for each page being viewed).
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
    if (pDC->IsPrinting()) {
        // we are printing ...
        ASSERT(NULL!=pInfo);
        if (m_aSelTblsPages.GetCount() > 0) {
            // printing selected tables, get next page from list of pages to print
            ASSERT(pInfo->m_nCurPage - 1  < (UINT) m_aSelTblsPages.GetCount());
            m_iCurrPrintPg = m_aSelTblsPages[pInfo->m_nCurPage - 1];
        }
        else {
            // printing a consecutive page range, use the PrintInfo page but number
            // from zero, not 1.
            m_iCurrPrintPg = pInfo->m_nCurPage - 1;
        }
        ASSERT(m_iCurrPrintPg>=0 && m_iCurrPrintPg<m_pgMgr.GetNumPages());
        PrepareMapMode(pDC, m_iCurrPrintPg);
    }
    else {
        // we are displaying on screen, nothing to do since this is handled for each page in OnDraw()
        //
        // note that we have to do it this way because multiple pages might be
        // displayed on screen concurrently, and each page gets its DC prepared separately
        ASSERT(FALSE);
    }
    CView::OnPrepareDC(pDC, pInfo);
}


/////////////////////////////////////////////////////////////////////////////
//
//        CTabPrtView::OnPaint
//
//  Handles WM_PAINT, draws us on the screen.
//
//  This derived function does not call OnPrepareDC; instead, OnPrepareDC
//  is called individually for each page in OnDraw().
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnPaint()
{
    CPaintDC dc(this); // device context for painting
    OnDraw(&dc);
}


/////////////////////////////////////////////////////////////////////////////
//
//        CTabPrtView::OnDraw
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnDraw(CDC* pDC)
{
    if (m_pgMgr.GetNumPages()==0) {
        // not fully initialized yet...
        return;
    }

    if (!pDC->IsPrinting())  {
        // not printing ...

        // fill in the spot where the 2 scroll bars intersect (bottom right corner of client)
        CRect rcClient;
        GetClientRect(&rcClient);
        rcClient.BottomRight() -= m_szSBar;

        COLORREF c = ::GetSysColor(COLOR_SCROLLBAR);
        CBrush brushScroll(c);
        CPen penScroll(PS_SOLID, 1, c);
        CBrush* pOldBrush = pDC->SelectObject(&brushScroll);
        CPen* pOldPen = pDC->SelectObject(&penScroll);
        CRect rcSBar(0,0,m_szSBar.cx, m_szSBar.cy);
        rcSBar.OffsetRect(rcClient.BottomRight());
        pDC->DPtoLP(&rcSBar);
        pDC->Rectangle(rcSBar);

        pDC->ExcludeClipRect(rcSBar);
        pDC->SelectObject(pOldBrush);
        pDC->SelectObject(pOldPen);

        // identify number of horizontal and vertical pages to view
        int iHorzPgs = GetNumHorzPgs();   // pages to display concurrently across
        int iVertPgs = GetNumVertPgs();   // pages to display concurrently down

        // remove minimal outside border area from client rect
        CSize szBorderOutside((int)(OUTSIDE_BORDER*GetZoomScaleFactor()), (int)(OUTSIDE_BORDER*GetZoomScaleFactor()));
        rcClient.right -= szBorderOutside.cx * 2;
        rcClient.bottom -= szBorderOutside.cy * 2;

        // remove inside gaps between each horizontal and vertical page
        CSize szBorderInside(INSIDE_BORDER, INSIDE_BORDER);
        rcClient.right -= szBorderInside.cx * (iHorzPgs-1);
        rcClient.bottom -= szBorderInside.cy * (iVertPgs-1);

        // calculate each page's portion of the client rect
        CSize szPgClient = CSize(rcClient.Width() / iHorzPgs, rcClient.Height() / iVertPgs);

        // draw each page that is currently visible ...
        int iPg = 0;    // for looping through pages
        for (int iPgDown=0 ; iPgDown<iVertPgs ; iPgDown++) {
            for (int iPgAcross=0 ; iPgAcross<iHorzPgs ; iPgAcross++) {
                // get page number to display
                int iPage=m_aiViewPg[iPg++];

                if (iPage==NONE) {
                    // this chunk of the screen does not have a page to display;
                    // it is a blank placeholder
                }
                else {
                    // prep the page's DC, without consideration for the number of currently displayed pages
                    PrepareMapMode(pDC, iPage);  // preps the DC for this particular page (page size, type, etc taken into account)

                    // scale the viewport so that this page occupies the proper chunk of the screen ...
                    CSize szScrV = pDC->GetViewportExt();                               // total viewport; note that this viewport might already be scaled
                    pDC->ScaleViewportExt((int)((float)szPgClient.cx * GetZoomScaleFactor()), szScrV.cx, (int)((float)szPgClient.cy * GetZoomScaleFactor()), szScrV.cy);
//                    pDC->ScaleViewportExt((int)((float)szPgClient.cx), szScrV.cx, (int)((float)szPgClient.cy), szScrV.cy);
                    szScrV = pDC->GetViewportExt();    // get new, updated viewport

                    // if not zooming: offset the viewport origin for this page, based on its screen position; in some cases we shift the page toward the center of the screne to improve readability
                    // if zooming: offset the viewport origin to account for scroll position
                    ASSERT(pDC->GetViewportOrg()==CPoint(0,0));

                    CPoint ptPgScrOrg = CPoint(szPgClient.cx * iPgAcross, szPgClient.cy * iPgDown) + szBorderOutside;
                    ptPgScrOrg.x += szBorderInside.cx * iPgAcross;
                    ptPgScrOrg.y += szBorderInside.cy * iPgDown;

                    // shift page horizontally towards center if currently showing 2 pages wide and aspect ratios are amenable
                    if (iHorzPgs<3) {
                        if (szScrV.cx < szPgClient.cx && iPgAcross==0)  {
                            // left facing page ... shift it to the right
                            ptPgScrOrg.x += (szPgClient.cx - szScrV.cx)/(3-iHorzPgs);       // 2 horz pages --> shift whole distance over (ie, /1) and meet the right page in the middle
                        }                                                                   // 1 horz page  --> shift half of the the distance (ie, /2) and center ourselves horizontally
                    }

                    // shift page vertically towards center if currently showing 2 pages heigh and aspect ratios are amenable
                    if (iVertPgs<3) {
                        if (szScrV.cy < szPgClient.cy && iPgDown==0)  {
                            // top page ... shift it down
                            ptPgScrOrg.y += (szPgClient.cy - szScrV.cy)/(3-iVertPgs);       // 2 vert pages --> shift whole distance down (ie, /1) and meet the bottom page in the middle
                        }                                                                   // 1 vert page  --> shift half of the the distance (ie, /2) and center ourselves vertically
                    }


                    ////////////////////////////////////////////////////////////////////////////////////////////////
                    // only if we're zooming: adjust window org to account for current scroll position
                    if ((int)GetZoomState()>(int)ZOOM_STATE_100_PERCENT) {
                        CPoint ptScrollPos = CPoint(m_pHSBar->GetScrollPos(), m_pVSBar->GetScrollPos());
                        pDC->DPtoLP(&ptScrollPos);
                        pDC->SetWindowOrg(ptScrollPos);
                    }

                    // set viewport origin
                    pDC->SetViewportOrg(ptPgScrOrg);

                    // draw resizing bar, if applicable
                    if (IsResizing() && GetResizeHitOb().GetPg()==iPage) {
                        CRect rcResize;

                        CRect rcPage(m_pgMgr.GetPgLayout(iPage).GetUserAreaLP());
                        ASSERT(GetResizeOrientation()!=RESIZE_INACTIVE);
                        switch(GetResizeOrientation()) {
                        case RESIZE_COL:
                            rcResize.SetRect(m_iCurResizePosDP,rcClient.top,m_iCurResizePosDP+1,rcClient.bottom);
                            pDC->DPtoLP(&rcResize);
                            rcResize.left -= RESIZE_BAR_WIDTH_LP/2;
                            rcResize.right += RESIZE_BAR_WIDTH_LP/2;
                            rcResize.top=rcPage.top;
                            rcResize.bottom=rcPage.bottom;
                            break;
                        case RESIZE_ROW:
                            rcResize.SetRect(rcClient.left, m_iCurResizePosDP, rcClient.right, m_iCurResizePosDP+1);
                            pDC->DPtoLP(&rcResize);
                            rcResize.top -= RESIZE_BAR_WIDTH_LP/2;
                            rcResize.bottom += RESIZE_BAR_WIDTH_LP/2;
                            rcResize.left=rcPage.left;
                            rcResize.right=rcPage.right;
                            break;
                        default:
                            ASSERT(FALSE);
                        }

                        // fire up GDI objects
                        CBrush brush(GetSysColor(COLOR_3DSHADOW));
                        CPen pen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
                        pDC->SelectObject(&brush);
                        pDC->SelectObject(&pen);

                        // remove scroll bar shafts from the area where the resize bar draws
                        CRect rcHSBarLP(0,rcSBar.top, rcSBar.right, rcSBar.bottom);
                        CRect rcVSBarLP(rcSBar.left, 0, rcSBar.right, rcSBar.bottom);
                        pDC->DPtoLP(&rcHSBarLP);
                        pDC->DPtoLP(&rcVSBarLP);
                        rcResize.bottom=__min(rcResize.bottom, rcHSBarLP.top);
                        rcResize.right=__min(rcResize.right, rcVSBarLP.left);

                        // draw it!
                        pDC->Rectangle(&rcResize);
                        pDC->ExcludeClipRect(rcResize);

                        // clean up
                        pDC->SelectStockObject(BLACK_PEN);
                        pDC->SelectStockObject(BLACK_BRUSH);
                    }

                    ///////////////////////////////////////////////////////////////
                    // finally, render the page on the screen
                    DrawPage(pDC, iPage);

                }
            }
        }
    }
    else {

        // printing ... just spit out the current page
        PrintPage(pDC, m_iCurrPrintPg);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        DrawPage
//
// Renders a page on the screen, layed out as close as possible to how
// the page would look if printed.
//
// This "print preview" display requires drawing the text into a printer-
// compatible DC, caputuring it in a bitmap, then displaying the bitmap
// on the screen (using the screen's DC).
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::DrawPage(CDC* pDC, int iPage)
{
    CPgLayout& pl = m_pgMgr.GetPgLayout(iPage);

    // memory DC (for bitmap output), based on printer DC
    CDC dcMem;

    // draw page outline
    ASSERT_VALID(m_pPrtDC);
    CRect rcPgOutline(pl.GetUserAreaLP());
    CPen penPgOutline(PS_SOLID, 2, GetSysColor(COLOR_WINDOWFRAME));
    pDC->SelectObject(&penPgOutline);
    pDC->SelectStockObject(WHITE_BRUSH);
    pDC->Rectangle(rcPgOutline);   // page outline, white-filled

    // draw page border (w/ "3D effect")
    pDC->SelectStockObject(BLACK_BRUSH);
    pDC->SelectStockObject(BLACK_PEN);
    CSize szBorder(THREE_D_BORDER,THREE_D_BORDER);
    pDC->DPtoLP(&szBorder);
    CRect rcRightBorder(rcPgOutline.right+1, rcPgOutline.top+szBorder.cy, rcPgOutline.right+szBorder.cx, rcPgOutline.bottom+szBorder.cy);
    CRect rcBottomBorder(rcPgOutline.left+szBorder.cx, rcPgOutline.bottom+1, rcPgOutline.right+szBorder.cx, rcPgOutline.bottom+szBorder.cy);
    pDC->Rectangle(rcRightBorder);
    pDC->Rectangle(rcBottomBorder);

    // info on screen and its DC
    CSize szScrV = pDC->GetViewportExt();
    CSize szScrW = pDC->GetWindowExt();

    // prep the printer DC (note: pDC will have already been prepped before coming here)
    m_pPrtDC->m_bPrinting = TRUE;
    PrepareMapMode(m_pPrtDC, iPage);

    // create and prep memory DC (based on printer DC)
    dcMem.CreateCompatibleDC(m_pPrtDC);
    dcMem.SetBkMode(TRANSPARENT);
    bool bMono=(dcMem.GetDeviceCaps(NUMCOLORS)==2);

    // prep screen DC
    pDC->SelectStockObject(NULL_BRUSH);
    pDC->SelectStockObject(BLACK_PEN);

    // get clip box, so we skip past objects that won't be rendered at all
    CRect rcClip;
    pDC->GetClipBox(&rcClip);

    // loop through all the objects on this page, and render each into the printer DC BMP, then copy it to the screen DC
    for (int iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
        CFmtFont fmtFont;   // for scaling fonts
        CBitmap bmp;  // for use with mem DC

        // get object and its area
        CPgOb& pgob = pl.GetPgOb(iPgOb);
        CRect rcOb = pgob.GetClientRectLP();

		/*TODO Get the "spanned" width for the joined cells
		//If it is the begnning of the join spanner add all the widths of the
		//subsequent spanners to this spanner rcOb

		if(pgob.GetFmt()->GetHidden() == HIDDEN_NO && pgob.GetFmt()->GetID() == FMT_ID_SPANNER){
			((CTabVar*)pgob.GetTblBase() )->GetF
			rcOb.right =rcOb.right +  (0.5)* rcOb.Width();
		}*/
        CFmt* pFmt=pgob.GetFmt();

        // store this object's client rect (in device units), for use with hit testing later
        CPoint ptTopLeft(rcOb.TopLeft());
        CRect rcDP(ptTopLeft, rcOb.Size());
        pDC->LPtoDP(&rcDP);
        pgob.SetClientRectDP(rcDP);

        if (rcOb.right<rcClip.left || rcOb.left>rcClip.right) {
            // object is entirely to the left or right of the clip rect
            continue;
        }
        if (rcOb.bottom<rcClip.top|| rcOb.top>rcClip.bottom) {
            // object is entirely above or below the clip rect
            continue;
        }

        // determine scaled object dimensions; note: as scale factor increases:
        // - speed decreases,
        // - font rendering becomes more accurate (ie, more printer-like and thus more awkward when put on the screen)
        // - layout becomes more accurate
        // examples: m_iScaleFactor=1  --> fast performance, nice fonts, poor layout
        //           m_iScaleFactor=10 --> slower performance, awkward font rendering, almost perfect layout
        const CSize szScaledOb = CSize(MulDiv(rcOb.Width(),szScrV.cx*m_iScaleFactor,szScrW.cx), MulDiv(rcOb.Height(),szScrV.cy*m_iScaleFactor,szScrW.cy));
        const CSize szDescaledOb = CSize(MulDiv(rcOb.Width(),szScrW.cx,szScrV.cx*m_iScaleFactor), MulDiv(rcOb.Height(),szScrW.cy,szScrV.cy*m_iScaleFactor));

        // prep bitmap for object's output
        bmp.CreateCompatibleBitmap(m_pPrtDC, szScaledOb.cx, szScaledOb.cy);
        CBitmap* pOldBmp = dcMem.SelectObject(&bmp);                 // load the bmp
        dcMem.PatBlt(0,0, szScaledOb.cx, szScaledOb.cy, WHITENESS);  // make sure background is white

        // shift object to (0,0) so that it draws into the bitmap properly
        rcOb.OffsetRect(-ptTopLeft);

        // retrieve logfont for this fmt
        LOGFONT lf;
        ASSERT(pFmt->GetFont()!=NULL);
        pFmt->GetFont()->GetLogFont(&lf);

        // scale logfont height
        lf.lfHeight = MulDiv(lf.lfHeight,szScrV.cx*m_iScaleFactor,szScrW.cx);

        // get a font with this new height (only calls CFont::CreateFontIndirect if the font hasn't already been created)
        fmtFont.SetFont(&lf);

        // load the CFont
        CFont* pFont=fmtFont.GetFont();
        dcMem.SelectObject(pFont);

        // sanity checks (the CMap was giving us trouble at one point)
        #ifdef _DEBUG
            LOGFONT lfFoo;
            pFont->GetLogFont(&lfFoo);
            ASSERT(lf.lfHeight==lfFoo.lfHeight);
        #endif

        // prep colors
        COLORREF rgbText, rgbFill;
		byte* pBits = NULL;
        if (bMono) {

            // note that setting a different text color doesn't work well with scaling, but we do it anyway

            // convert RGB to greyscale; see Programming Windows 95 with MFC, Part VIII: Printing and Print Previewing; MSDN 4/1996
            BYTE greyText=(BYTE)(GetRValue(pFmt->GetTextColor().m_rgb)*0.30+
                                 GetGValue(pFmt->GetTextColor().m_rgb)*0.59+
                                 GetBValue(pFmt->GetTextColor().m_rgb)*0.11);
            BYTE greyFill=(BYTE)(GetRValue(pFmt->GetFillColor().m_rgb)*0.30+
                                 GetGValue(pFmt->GetFillColor().m_rgb)*0.59+
                                 GetBValue(pFmt->GetFillColor().m_rgb)*0.11);
            rgbText=RGB(greyText,greyText,greyText);
            rgbFill=RGB(greyFill,greyFill,greyFill);
        }
        else {
            // color printer, que sorte!
            rgbText=pFmt->GetTextColor().m_rgb;
            rgbFill=pFmt->GetFillColor().m_rgb;
        }

        // handle text color (see below for fill color)
        dcMem.SetTextColor(rgbText);

        // get rect and deflate for indentation (padding)
        CRect rcDraw(rcOb);
        CRect rcIndent = CRect(CELL_PADDING_LEFT,CELL_PADDING_TOP,CELL_PADDING_RIGHT,CELL_PADDING_BOTTOM);
        rcIndent.left += pFmt->GetIndent(LEFT);
        rcIndent.right += pFmt->GetIndent(RIGHT);
        rcDraw.DeflateRect(rcIndent);

        // scale drawing rect
        rcDraw.left = MulDiv(rcDraw.left,szScrV.cx*m_iScaleFactor,szScrW.cx);
        rcDraw.top = MulDiv(rcDraw.top,szScrV.cy*m_iScaleFactor,szScrW.cy);
        rcDraw.right = MulDiv(rcDraw.right,szScrV.cx*m_iScaleFactor,szScrW.cx);
        rcDraw.bottom = MulDiv(rcDraw.bottom,szScrV.cy*m_iScaleFactor,szScrW.cy);

        CString sText=pgob.GetText();
        UINT uFormat=pgob.GetDrawFormatFlags();

        // mirror horz alignment for right-side stubs
        if (pgob.GetType()==PGOB_STUB_RIGHT && pFmt->GetHorzAlign()==HALIGN_LEFT) {
            uFormat ^= DT_LEFT;
            uFormat |= DT_RIGHT;
        }

        // expand date/time/page/filename codes in headers and footers ...
        // note that iPage is the index in m_pgMgr, which is not the same as the printed page number (pl.GetPrintedPageNum()) that gets printed
        if (pgob.GetType()==PGOB_HEADER_LEFT||pgob.GetType()==PGOB_HEADER_CENTER||pgob.GetType()==PGOB_HEADER_RIGHT||pgob.GetType()==PGOB_FOOTER_LEFT||pgob.GetType()==PGOB_FOOTER_CENTER||pgob.GetType()==PGOB_FOOTER_RIGHT) {
            CIMSAString sFileName(GetDocument()->GetPathName());
            PathStripPath(sFileName.GetBuffer(MAX_PATH));
            sFileName.ReleaseBuffer();
            sText=CFolio::ExpandText(sText, sFileName, pl.GetPrintedPageNum());
        }

        // handle vertical alignment manually ... necessary for DT_BOTTOM and DT_VCENTER flags
        AlignVertical(sText, rcDraw, uFormat, &dcMem);

        // draw object's text into the bitmap
        int iDrawHgt=dcMem.DrawText(sText, &rcDraw, uFormat);

        // draw reader breaks, if needed
        PutLeadering(&dcMem, pgob, sText, rcDraw, iDrawHgt);

        // normalize rect
        rcOb.OffsetRect(ptTopLeft);

        // put the bitmap and fill color out to the screen ... for efficiency, we do this differently depending on whether or not we need to fill
        if (rgbFill!=rgbWhite) {
            // handle fill color (this goes straight to the screen)
            CBrush brush(rgbFill);
            pDC->FillRect(rcOb,&brush);  // expensive operation

			//Converting DDB to DIB
			//http://win32-framework.sourceforge.net/Tutorials/tutorial9.htm
			// Fill the BITMAPINFOHEADER structure
			BITMAPINFOHEADER bi = {0};
			bi.biSize = sizeof(BITMAPINFOHEADER);
			bi.biHeight =szScaledOb.cy;
			bi.biWidth =  szScaledOb.cx;
			bi.biPlanes = 1;
			bi.biBitCount =  24;
			bi.biCompression = BI_RGB;

			// Note: BITMAPINFO and BITMAPINFOHEADER are the same for 24 bit bitmaps
			// Get the size of the image data
			GetDIBits(dcMem, bmp, 0, szScaledOb.cy, NULL, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

			// Retrieve the image data
			pBits = new byte[bi.biSizeImage];
			GetDIBits(dcMem, bmp, 0, szScaledOb.cy, pBits, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

			// Use StretchDIBits to scale the bitmap and maintain its original proportions
			StretchDIBits(pDC->m_hDC, ptTopLeft.x, ptTopLeft.y,rcOb.Width(), rcOb.Height(), 0, 0,  szScaledOb.cx, szScaledOb.cy, pBits,
				(BITMAPINFO*)&bi, DIB_RGB_COLORS, SRCAND);
        }
        else {
			//Converting DDB to DIB
			//http://win32-framework.sourceforge.net/Tutorials/tutorial9.htm
			// Fill the BITMAPINFOHEADER structure
			BITMAPINFOHEADER bi = {0};
			bi.biSize = sizeof(BITMAPINFOHEADER);
			bi.biHeight =szScaledOb.cy;
			bi.biWidth =  szScaledOb.cx;
			bi.biPlanes = 1;
			bi.biBitCount =  24;
			bi.biCompression = BI_RGB;

			// Note: BITMAPINFO and BITMAPINFOHEADER are the same for 24 bit bitmaps
			// Get the size of the image data
			GetDIBits(dcMem, bmp, 0, szScaledOb.cy, NULL, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

			// Retrieve the image data
			pBits = new byte[bi.biSizeImage];
			GetDIBits(dcMem, bmp, 0, szScaledOb.cy, pBits, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

			// Use StretchDIBits to scale the bitmap and maintain its original proportions
			StretchDIBits(pDC->m_hDC, ptTopLeft.x, ptTopLeft.y,rcOb.Width(), rcOb.Height(), 0, 0,  szScaledOb.cx, szScaledOb.cy, pBits,
				(BITMAPINFO*)&bi, DIB_RGB_COLORS, SRCCOPY);
        }

        // put borders directly onto the screen DC, to avoid scaling inconsistencies
        PutBorders(pDC, pFmt, rcOb);

        // unload font
        dcMem.SelectStockObject(OEM_FIXED_FONT);

        // unload bitmap
        dcMem.SelectObject(pOldBmp);
        bmp.DeleteObject();
		if(pBits){
			delete []pBits;
		}
    }

    // store the page's rect (user area) in device units, for use with hit testing later
    pDC->LPtoDP(&rcPgOutline);   // page outline, white-filled
    pl.SetUserAreaDP(&rcPgOutline);

    // clean up
    dcMem.DeleteDC();
}



/////////////////////////////////////////////////////////////////////////////
//
//        PrintPage
//
// Renders a page on the printer.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::PrintPage(CDC* pDC, int iPage)
{
    const CPgLayout& pl = m_pgMgr.GetPgLayout(iPage);
    pDC->Rectangle(pl.GetUserAreaLP());

    for (int iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {

        // get object
        const CPgOb& pgob = pl.GetPgOb(iPgOb);
        CFmt* pFmt=pgob.GetFmt();

        // load font
        ASSERT_VALID(pFmt->GetFont());
        pDC->SelectObject(pFmt->GetFont());

        // get the bounding rect and border it, if needed
        const CRect& rcOb = pgob.GetClientRectLP();

        // set colors
        pDC->SetTextColor(pFmt->GetTextColor().m_rgb);
        pDC->FillSolidRect(rcOb, pFmt->GetFillColor().m_rgb);

        // print the object's borders
        PutBorders(pDC, pFmt, rcOb);

        // get rect and deflate for indentation (padding)
        CRect rcDraw(rcOb);
        CRect rcIndent(CELL_PADDING_LEFT,CELL_PADDING_TOP,CELL_PADDING_RIGHT,CELL_PADDING_BOTTOM);
        rcIndent.left += pFmt->GetIndent(LEFT);
        rcIndent.right += pFmt->GetIndent(RIGHT);
        rcDraw.DeflateRect(rcIndent);

        // init
        UINT uFormat=pgob.GetDrawFormatFlags();
        CIMSAString sText=pgob.GetText();

        // mirror horz alignment for right-side stubs
        if (pgob.GetType()==PGOB_STUB_RIGHT && pFmt->GetHorzAlign()==HALIGN_LEFT) {
            uFormat ^= DT_LEFT;
            uFormat |= DT_RIGHT;
        }

        // expand date/time/page/filename codes in headers and footers ...
        // note that iPage is the index in m_pgMgr, which is not the same as the printed page number (pl.GetPrintedPageNum()) that gets printed
        if (pgob.GetType()==PGOB_HEADER_LEFT||pgob.GetType()==PGOB_HEADER_CENTER||pgob.GetType()==PGOB_HEADER_RIGHT||pgob.GetType()==PGOB_FOOTER_LEFT||pgob.GetType()==PGOB_FOOTER_CENTER||pgob.GetType()==PGOB_FOOTER_RIGHT) {
            CIMSAString sFileName(GetDocument()->GetPathName());
            PathStripPath(sFileName.GetBuffer(MAX_PATH));
            sFileName.ReleaseBuffer();
            sText=CFolio::ExpandText(sText, sFileName, pl.GetPrintedPageNum());
        }

        // handle vertical alignment manually ... necessary for DT_BOTTOM and DT_VCENTER flags
        AlignVertical(sText, rcDraw, uFormat, pDC);

        // draw object's text
        int iDrawHgt=pDC->DrawText(sText, &rcDraw, uFormat);

        // draw reader breaks, if needed
        PutLeadering(pDC, pgob, sText, rcDraw, iDrawHgt);

        // unload font
        pDC->SelectStockObject(OEM_FIXED_FONT);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        AlignVertical
//
// Handles vertical alignment of text, when rendering to either
// the printer or screen.  We have to do this manually because
// CDC::DrawText() doesn't let us mix DT_WORDBREAK and (DT_VCENTER or
// DT_BOTTOM).
//
// Returns vertical offset amount.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::AlignVertical(const CString& sText, CRect& rcDraw, UINT& uFormat, CDC* pDC)
{
    // handle vertical alignment manually ... necessary for DT_BOTTOM and DT_VCENTER flags
    bool bVCenter=((uFormat|DT_VCENTER)==uFormat);
    bool bBottom=((uFormat|DT_BOTTOM)==uFormat);
    int iVertOffset=0; // vertical offset to rcDraw.top, to accommodate vertical alignment

    if (bVCenter || bBottom) {

        // vertical centered or bottom; see how much space we actually need to draw the text...
        CRect rcVert(rcDraw);
        rcVert.bottom=rcVert.top+1;
        uFormat ^= DT_VCENTER;

        pDC->DrawText(sText, &rcVert, uFormat|DT_CALCRECT);
        if (bVCenter) {
            // center vertically
            iVertOffset = (rcDraw.Height() - rcVert.Height())/2;
            uFormat ^= DT_VCENTER;
        }
        else {
            // vertical bottom
            iVertOffset = rcDraw.Height() - rcVert.Height();
            uFormat ^= DT_BOTTOM;
        }
        uFormat |= DT_TOP;
        rcDraw.top += iVertOffset;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        PutLeadering
//
// Adds leadering to stubs, if needed.  Leadering characters (dots and dashes)
// are rendered as small rectanges.
//
// iDrawHgt helps determine the baseline.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::PutLeadering(CDC* pDC, const CPgOb& pgob, const CIMSAString& /*sText*/, const CRect& rcDraw, const int iDrawHgt)
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
   // CTabSet* pSet = pDoc->GetTableSpec();
	//SAvy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    CTable* pTbl=pSet->GetTable(pgob.GetTbl());
    const CFmtReg& fmtReg=pSet->GetFmtReg();

    if (pgob.GetType()!=PGOB_STUB && pgob.GetType()!=PGOB_STUB_RIGHT)  {
        // leadering only happens on stubs
        return;
    }

    // table format (for leadering) ...
    CTblFmt fmtTbl;
    if (NULL!=pTbl->GetDerFmt()) {
        fmtTbl=*pTbl->GetDerFmt();
        fmtTbl.CopyNonDefaultValues(DYNAMIC_DOWNCAST(CTblFmt,fmtReg.GetFmt(FMT_ID_TABLE,0)));
    }
    else {
        fmtTbl=*DYNAMIC_DOWNCAST(CTblFmt,fmtReg.GetFmt(FMT_ID_TABLE,0));
    }

    // bail out if we're not doing leadering
    LEADERING leadering=(pgob.GetType()==PGOB_STUB?fmtTbl.GetLeadering(LEFT):fmtTbl.GetLeadering(RIGHT));
    if (leadering==LEADERING_NONE) {
        return;
    }

    // table print format
    CTblPrintFmt fmtTblPrint;
    if (NULL!=pTbl->GetTblPrintFmt()) {
        fmtTblPrint=*pTbl->GetTblPrintFmt();
        fmtTblPrint.CopyNonDefaultValues(DYNAMIC_DOWNCAST(CTblPrintFmt,fmtReg.GetFmt(FMT_ID_TBLPRINT,0)));
    }
    else {
        fmtTblPrint=*DYNAMIC_DOWNCAST(CTblPrintFmt,fmtReg.GetFmt(FMT_ID_TBLPRINT,0));
    }

    // leadering is not applicable for right-aligned text
    CFmt* pFmt=pgob.GetFmt();
    if (pFmt->GetHorzAlign()==HALIGN_RIGHT && pgob.GetType()==PGOB_STUB) {
        return;
    }

    // init GDI stuff
    CBrush brush(RGB(0,0,0));
    CBrush* pOldBrush=pDC->SelectObject(&brush);

    // determine the size of a dot and a dash, using current font & scaling
    int iDotWidth;
    pDC->GetCharWidth((UINT)_T('.'),(UINT)_T('.'),&iDotWidth);

    // for fixed width fonts use 1/2 the dot size since GetCharWidth gives width
    // of largest char for fixed width font
    CFont* pFont = pDC->GetCurrentFont();
    LOGFONT lfCurrentFont;
    pFont->GetLogFont(&lfCurrentFont);
    if (lfCurrentFont.lfPitchAndFamily & FIXED_PITCH) {
        iDotWidth = iDotWidth/2;
    }
    CSize szLeaderingDot(iDotWidth/3,iDotWidth/3);
    CSize szLeaderingDash(iDotWidth,iDotWidth/4);

    // measure stuff
    TEXTMETRIC tm;
    pDC->GetTextMetrics(&tm);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // figure out where the last line of the wrapped stub is positioned, since leadering will
    // appear to the right of that point.

    // determine line height
    CPgOb obTmp(pgob);
    obTmp.SetDrawFormatFlags(DT_SINGLELINE|DT_TOP|DT_LEFT);
    int iLineHgt=CalcDrawHeight(obTmp, *pDC) - (CELL_PADDING_TOP + CELL_PADDING_BOTTOM);
    CRect rcSingleLine(obTmp.GetClientRectLP());

    rcSingleLine.bottom=rcSingleLine.top+iLineHgt;
    obTmp.SetClientRectLP(rcSingleLine);
    obTmp.SetDrawFormatFlags(DT_WORDBREAK|DT_TOP|DT_LEFT);

    // iterate through the stub, extracting a line at a time. A line is what can be displayed until word wrapping is needed.
    CIMSAString sRemaining(obTmp.GetText());
    CIMSAString sCurLine;
    int iWordWrapPoint;
    while (!sRemaining.IsEmpty()) {
        obTmp.SetText(sRemaining);
        CalcDrawHeight(obTmp, *pDC, &iWordWrapPoint);
        sCurLine=sRemaining.Left(iWordWrapPoint);
        //sCurLine.Trim();
        sRemaining=sRemaining.Mid(iWordWrapPoint);
    }

    // calcuate vertical placement (baseline)
    int iBaseLine=rcDraw.top+iDrawHgt-tm.tmDescent-szLeaderingDot.cy;

    // calculate horz spacing
    int iHorzSpacing;
    switch(leadering) {
    case LEADERING_DOT:
        iHorzSpacing=szLeaderingDot.cx*3;
        break;
    case LEADERING_DOT_SPACE:
        iHorzSpacing=szLeaderingDot.cx*5;
        break;
    case LEADERING_DASH:
        iHorzSpacing=szLeaderingDash.cx+szLeaderingDot.cx;
        break;
    case LEADERING_DASH_SPACE:
        iHorzSpacing=szLeaderingDash.cx+szLeaderingDot.cx*3;
        break;
    default:
        ASSERT(FALSE);
    }

    // detect if sizing is so small that leaders can't be output
    if (iHorzSpacing==0) {
        // avoids divide by 0 burp
        return;
    }

    // calculate horz starting placement for leadering
    int iTextEnd=pDC->GetTextExtent(sCurLine).cx;  // the end of the last line of text in the stub
    if (pFmt->GetHorzAlign()==HALIGN_CENTER) {
        // need to account for the left-side whitespace because of centering
        iTextEnd += (rcDraw.Width()-iTextEnd)/2;
    }

    // leadering is lined up in columns, similar to tab stops; shift over to the next column
    int iXLeaderStart, iXLeaderEnd;
    if (pgob.GetType()==PGOB_STUB) {
        // left side stub
        iXLeaderStart = iTextEnd + rcDraw.left + iHorzSpacing - (iTextEnd%iHorzSpacing);
        iXLeaderEnd = rcDraw.right;
    }
    else {
        // right side stub
        iXLeaderStart = rcDraw.left;
        iXLeaderEnd = rcDraw.right - iTextEnd - iHorzSpacing + (iTextEnd%iHorzSpacing);
    }

    // draw the leadering rectangles
    for (int iX=iXLeaderStart; iX<iXLeaderEnd ; iX+=iHorzSpacing) {
        switch(leadering) {
        case LEADERING_DOT:
        case LEADERING_DOT_SPACE:
            pDC->Rectangle(iX,iBaseLine,iX+szLeaderingDot.cx,iBaseLine+szLeaderingDot.cy);
            break;
        case LEADERING_DASH:
        case LEADERING_DASH_SPACE:
            pDC->Rectangle(iX,iBaseLine,iX+szLeaderingDash.cx,iBaseLine+szLeaderingDash.cy);
            break;
        default:
            ASSERT(FALSE);
        }
    }
    pDC->SelectObject(pOldBrush);
}

/////////////////////////////////////////////////////////////////////////////
//
//        PutBorders
//
// Renders an objects borders into a DC, based on the object's formatting
// specifications.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::PutBorders(CDC* pDC, CFmt* pFmt, const CRect& rcOb)
{
        CPen penThinLine(PS_SOLID, 1, RGB(0,0,0));
        CPen penThickLine(PS_SOLID, 30, RGB(0,0,0));

        CPoint ptTopLeft(rcOb.TopLeft());
        if (pFmt->GetLineLeft()==LINE_THIN) {
            // left border, thin line
            pDC->SelectObject(&penThinLine);
            pDC->MoveTo(ptTopLeft);
            pDC->LineTo(ptTopLeft+CPoint(0, rcOb.Height()));
        }
        if (pFmt->GetLineLeft()==LINE_THICK) {
            // left border, thick line
            pDC->SelectObject(&penThickLine);
            pDC->MoveTo(ptTopLeft);
            pDC->LineTo(ptTopLeft+CPoint(0, rcOb.Height()));
        }
        if (pFmt->GetLineTop()==LINE_THIN) {
            // top border, thin line
            pDC->SelectObject(&penThinLine);
            pDC->MoveTo(ptTopLeft);
            pDC->LineTo(ptTopLeft+CPoint(rcOb.Width(),0));
        }
        if (pFmt->GetLineTop()==LINE_THICK) {
            // top border, thick line
            pDC->SelectObject(&penThickLine);
            pDC->MoveTo(ptTopLeft);
            pDC->LineTo(ptTopLeft+CPoint(rcOb.Width(),0));
        }
        if (pFmt->GetLineRight()==LINE_THIN) {
            pDC->SelectObject(&penThinLine);
            // right border, thin line
            pDC->MoveTo(ptTopLeft+CPoint(rcOb.Width(),0));
            pDC->LineTo(ptTopLeft+rcOb.Size());
        }
        if (pFmt->GetLineRight()==LINE_THICK) {
            // right border, thick line
            pDC->SelectObject(&penThickLine);
            pDC->MoveTo(ptTopLeft+CPoint(rcOb.Width(),0));
            pDC->LineTo(ptTopLeft+rcOb.Size());
        }
        if (pFmt->GetLineBottom()==LINE_THIN) {
            // bottom border, thin line
            pDC->SelectObject(&penThinLine);
            pDC->MoveTo(ptTopLeft+CPoint(0,rcOb.Height()));
            pDC->LineTo(ptTopLeft+rcOb.Size());
        }
        if (pFmt->GetLineBottom()==LINE_THICK) {
            // bottom border, thick line
            pDC->SelectObject(&penThickLine);
            pDC->MoveTo(ptTopLeft+CPoint(0,rcOb.Height()));
            pDC->LineTo(ptTopLeft+rcOb.Size());
        }

        // unload pen
        pDC->SelectStockObject(BLACK_PEN);
}


/////////////////////////////////////////////////////////////////////////////
//
//        OnPrint
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnPrint(CDC* pDC, CPrintInfo* pInfo)
{
    CView::OnPrint(pDC, pInfo);
}


/////////////////////////////////////////////////////////////////////////////
//
//        OnPreparePrinting
//
/////////////////////////////////////////////////////////////////////////////
BOOL CTabPrtView::OnPreparePrinting(CPrintInfo* pInfo)
{
    pInfo->SetMaxPage(1);
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    CTable* pTbl = pSet->GetTable(0);
    const CFmtReg& fmtReg=pSet->GetFmtReg();

    // table print format ...
    CTblPrintFmt fmtTblPrint;
    if (NULL!=pTbl->GetTblPrintFmt()) {
        fmtTblPrint=*pTbl->GetTblPrintFmt();
        fmtTblPrint.CopyNonDefaultValues(DYNAMIC_DOWNCAST(CTblPrintFmt,fmtReg.GetFmt(FMT_ID_TBLPRINT,0)));
    }
    else {
        fmtTblPrint=*DYNAMIC_DOWNCAST(CTblPrintFmt,fmtReg.GetFmt(FMT_ID_TBLPRINT,0));
    }

    // prep the print setup dialog ...

    // setup custom print dialog that adds our own radio buttons
	delete pInfo->m_pPD;
    CTablePrintDlg* pPrintDlg = new CTablePrintDlg(FALSE, pSet);
    const int iCurrTbl = m_pgMgr.GetPgLayout(GetCurrFirstViewPg()).GetPgOb(0).GetTbl();
    pPrintDlg->m_aSelectedTables.Add(iCurrTbl);
	pInfo->m_pPD = pPrintDlg;
	pInfo->m_pPD->m_pd.nMinPage = 1;
	pInfo->m_pPD->m_pd.nMaxPage = 1;

    CPrintDialog dlg(TRUE);
    CString sPrinterDevice=fmtTblPrint.GetPrinterDevice();
    TCHAR* pszPrinterDevice=sPrinterDevice.GetBuffer(sPrinterDevice.GetLength());
    if (GetPrinterDevice(pszPrinterDevice, &dlg.m_pd.hDevNames, &dlg.m_pd.hDevMode)) {
        DEVMODE FAR* pDevMode=(DEVMODE FAR*)::GlobalLock(dlg.m_pd.hDevMode);

        // set orientation ...
        switch (fmtTblPrint.GetPageOrientation()) {
        case PAGE_ORIENTATION_PORTRAIT:
            pDevMode->dmOrientation=DMORIENT_PORTRAIT;
            break;
        case PAGE_ORIENTATION_LANDSCAPE:
            pDevMode->dmOrientation=DMORIENT_LANDSCAPE;
            break;
        default:
            ASSERT(FALSE);
            break;
        }

        // set paper type ...
        switch (fmtTblPrint.GetPaperType()) {
        case PAPER_TYPE_A4:
            pDevMode->dmPaperSize=DMPAPER_A4;
            break;
        case PAPER_TYPE_A3:
            pDevMode->dmPaperSize=DMPAPER_A3;
            break;
        case PAPER_TYPE_LETTER:
            pDevMode->dmPaperSize=DMPAPER_LETTER;
            break;
        case PAPER_TYPE_LEGAL:
            pDevMode->dmPaperSize=DMPAPER_LEGAL;
            break;
        case PAPER_TYPE_TABLOID:
            pDevMode->dmPaperSize=DMPAPER_TABLOID;
            break;
        default:
            ASSERT(FALSE);
            break;
        }

        // update printer
        ::GlobalUnlock(dlg.m_pd.hDevMode);

        AfxGetApp()->SelectPrinter(dlg.m_pd.hDevNames, dlg.m_pd.hDevMode);
    }
    else {
        AfxMessageBox(_T("Internal error preparing printer %s")+sPrinterDevice, MB_ICONEXCLAMATION);
    }
    sPrinterDevice.ReleaseBuffer();

    // set min/max pages
    pInfo->SetMinPage(1);
    pInfo->SetMaxPage(m_pgMgr.GetNumPages());

    // clear out selected tables page range
    m_aSelTblsPages.RemoveAll();

	::GlobalUnlock(dlg.m_pd.hDevNames);
    ::GlobalUnlock(dlg.m_pd.hDevMode);

    BOOL bRet = DoPreparePrinting(pInfo);

    if (bRet)  {

        // update page range for current page and current table options in custom print dlg
        switch (((CTablePrintDlg*) pInfo->m_pPD)->m_pageButtState) {
            case CTablePrintDlg::PG_CURR_PAGE:
                {
                int iCurrPage = GetCurrFirstViewPg() + 1;
                pInfo->m_pPD->m_pd.nFromPage = iCurrPage;
                pInfo->m_pPD->m_pd.nToPage = iCurrPage;
                }
                break;
            case CTablePrintDlg::PG_CURR_TABLE:
                {
                    int iCurrTbl = m_pgMgr.GetPgLayout(GetCurrFirstViewPg()).GetPgOb(0).GetTbl();
                    int iStart, iEnd;
                    GetPageRangeForTable(iCurrTbl, iStart, iEnd);
                    pInfo->m_pPD->m_pd.nFromPage = iStart + 1;
                    pInfo->m_pPD->m_pd.nToPage = iEnd + 1;
                }
                break;
            case CTablePrintDlg::PG_RANGE:
                // this is either page range or all, in either case range is set correctly
                break;
            case CTablePrintDlg::PG_SELECTED_TABLES:
                {
                // selected tables is not always consecutive page range so we need a small hack
                // we tell MFC to print from page 1 to the total number of pages in all selected tables
                // and then we map the pages in that range to the actual pages we want to print
                // using the array m_aSelTblsPages.
                ASSERT(m_aSelTblsPages.GetCount() == 0); // should be cleared initially
                for (int iSelTbl = 0; iSelTbl < pPrintDlg->m_aSelectedTables.GetCount(); ++iSelTbl) {
                    int iStart, iEnd;
                    GetPageRangeForTable(pPrintDlg->m_aSelectedTables[iSelTbl], iStart, iEnd);
                    for (int iPg = iStart; iPg <= iEnd; ++iPg) {
                        m_aSelTblsPages.Add(iPg);
                    }
                }
                pInfo->m_pPD->m_pd.nFromPage = 1;
                pInfo->m_pPD->m_pd.nToPage = m_aSelTblsPages.GetCount();
                }
                break;
            default:
                ASSERT(FALSE);
        }

        // update printer settings (see also CTableChildWnd::OnFilePrintSetup)

        CTblPrintFmt* pTblPrintFmt=pTbl->GetTblPrintFmt();
        CTblPrintFmt* pDefaultTblPrintFmt=DYNAMIC_DOWNCAST(CTblPrintFmt, fmtReg.GetFmt(FMT_ID_TBLPRINT,0));
        bool bPrintSettingsChanged=false;

        // retrieve printer device info ...
		//this call return after DoPreparePrinting will return invalid handle error in VS 2010 /MFC
        /*DEVNAMES FAR *pDevNames=(DEVNAMES FAR *)::GlobalLock(dlg.m_pd.hDevNames);*/
		DEVNAMES FAR *pDevNames = (DEVNAMES FAR *)::GlobalLock(pInfo->m_pPD->m_pd.hDevNames); // this should be the correct call anyway
		//DWORD dWordError = GetLastError(); //uncomment this to check the if anyerror in global lock
        CString sDriver((LPTSTR)pDevNames + pDevNames->wDriverOffset);
        CString sDevice((LPTSTR)pDevNames + pDevNames->wDeviceOffset);
        CString sOutput((LPTSTR)pDevNames + pDevNames->wOutputOffset);
        if (sDevice!=pTblPrintFmt->GetPrinterDevice()) {
            bPrintSettingsChanged=true;
        }
        pTblPrintFmt->SetPrinterDevice(sDevice);
        pTblPrintFmt->SetPrinterDriver(sDriver);
        pTblPrintFmt->SetPrinterOutput(sOutput);
        pDefaultTblPrintFmt->SetPrinterDevice(sDevice);
        pDefaultTblPrintFmt->SetPrinterDriver(sDriver);
        pDefaultTblPrintFmt->SetPrinterOutput(sOutput);

        // retrieve page orientation and paper type ...
		//this call return after DoPreparePrinting will return invalid handle error in VS 2010 /MFC
       /* DEVMODE FAR* pDevMode=(DEVMODE FAR*)::GlobalLock(dlg.m_pd.hDevMode);*/  //this does not work in VS2010 /MFC
		DEVMODE FAR* pDevMode=(DEVMODE FAR*)::GlobalLock(pInfo->m_pPD->m_pd.hDevMode);
        switch(pDevMode->dmPaperSize) {
        case DMPAPER_A4:
            pTblPrintFmt->SetPaperType(PAPER_TYPE_A4);
            break;
        case DMPAPER_LETTER:
            pTblPrintFmt->SetPaperType(PAPER_TYPE_LETTER);
            break;
        case DMPAPER_LEGAL:
            pTblPrintFmt->SetPaperType(PAPER_TYPE_LEGAL);
            break;
        case DMPAPER_A3:
            pTblPrintFmt->SetPaperType(PAPER_TYPE_A3);
            break;
        case DMPAPER_TABLOID:
            pTblPrintFmt->SetPaperType(PAPER_TYPE_TABLOID);
            break;
        default:
            // we'll just use the previous value
            CIMSAString sMsg, sCurrPaperType;
            switch(pTblPrintFmt->GetPaperType()) {
            case PAPER_TYPE_A4:
                sCurrPaperType=_T("A4");
                break;
            case PAPER_TYPE_A3:
                sCurrPaperType=_T("A3");
                break;
            case PAPER_TYPE_LETTER:
                sCurrPaperType=_T("letter");
                break;
            case PAPER_TYPE_LEGAL:
                sCurrPaperType=_T("legal");
                break;
            case PAPER_TYPE_TABLOID:
                sCurrPaperType=_T("tabloid");
                break;
            default:
                ASSERT(FALSE);
                break;
            }
            sMsg.Format(_T("Sorry, paper type [%s] is not supported.  Using [%s] instead."), (LPCTSTR)pDevMode->dmFormName, (LPCTSTR)sCurrPaperType);
            AfxMessageBox(sMsg,MB_ICONINFORMATION);
            break;
        }
        pDefaultTblPrintFmt->SetPaperType(pTblPrintFmt->GetPaperType());

        switch(pDevMode->dmOrientation) {
        case DMORIENT_PORTRAIT:
            pTblPrintFmt->SetPageOrientation(PAGE_ORIENTATION_PORTRAIT);
            break;
        case DMORIENT_LANDSCAPE:
            pTblPrintFmt->SetPageOrientation(PAGE_ORIENTATION_LANDSCAPE);
            break;
        default:
            // we'll just use the previous value
            CIMSAString sMsg, sCurrPageOrientation;
            switch(pTblPrintFmt->GetPageOrientation()) {
            case PAGE_ORIENTATION_PORTRAIT:
                sCurrPageOrientation=_T("portrait");
                break;
            case PAGE_ORIENTATION_LANDSCAPE:
                sCurrPageOrientation=_T("landscape");
                break;
            default:
                ASSERT(FALSE);
                break;
            }
            sMsg.Format(_T("Sorry, that page orientation is not supported.  Using %s instead."), (LPCTSTR)sCurrPageOrientation);
            AfxMessageBox(sMsg,MB_ICONINFORMATION);
            break;
        }
        pDefaultTblPrintFmt->SetPageOrientation(pTblPrintFmt->GetPageOrientation());

        if (pDefaultTblPrintFmt->GetPaperType()!=fmtTblPrint.GetPaperType() || pDefaultTblPrintFmt->GetPageOrientation()!=fmtTblPrint.GetPageOrientation()) {
            bPrintSettingsChanged=true;
        }



		::GlobalUnlock(pInfo->m_pPD->m_pd.hDevNames);
        ::GlobalUnlock(pInfo->m_pPD->m_pd.hDevMode);


        // process page layout or printer selection changes ...
        if (bPrintSettingsChanged) {

            // let the user know ...
            CFlashMsgDlg dlg;
            dlg.m_iSeconds=3;
            dlg.m_sFlashMsg=_T("Table layout being adjusted to correspond to your changed printer settings.");
            dlg.DoModal();

            // update printer DC (needed if the user changed printers)
            if (!PreparePrinterDC()) {
                AfxMessageBox(_T("CTabPrtView::OnPreparePrinting() -- error changing printer DC"), MB_ICONEXCLAMATION);
            }
            PRINTDLG prtdlg;  // this will retrieve devmode, devnames for the new printer
            AfxGetApp()->GetPrinterDeviceDefaults(&prtdlg);

            // update printinfo so that the new printer gets used (on screen and on paper)
            pInfo->m_pPD->m_pd.hDevMode=prtdlg.hDevMode;
            pInfo->m_pPD->m_pd.hDevNames=prtdlg.hDevNames;

            // signal the update, so that it will be incorporated into the next page build ...
            pSet->SetSelectedPrinterChanged();
            Build(true);
        }

        // we're dirty now!
        pDoc->SetModifiedFlag(TRUE);
    }
    return bRet;
}


/////////////////////////////////////////////////////////////////////////////
//
//        OnBeginPrinting
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo)
{
    CView::OnBeginPrinting(pDC, pInfo);
}


/////////////////////////////////////////////////////////////////////////////
//
//        OnEndPrinting
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnEndPrinting(CDC* pDC, CPrintInfo* pInfo)
{
    m_iCurrPrintPg=NONE;
    // clear out selected tables page range
    m_aSelTblsPages.RemoveAll();

    CView::OnEndPrinting(pDC, pInfo);
}




/////////////////////////////////////////////////////////////////////////////
//
//        OnEraseBkgnd
//
//  Fills the view background.
//
/////////////////////////////////////////////////////////////////////////////
BOOL CTabPrtView::OnEraseBkgnd(CDC* pDC)
{
    ASSERT_VALID(pDC);

    // Fill background with APPWORKSPACE
    CBrush backBrush(GetSysColor(COLOR_APPWORKSPACE));
    CBrush* pOldBrush = pDC->SelectObject(&backBrush);
    CRect rect;
    pDC->GetClipBox(&rect);     // Erase the area needed

    pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(), PATCOPY);
    pDC->SelectObject(pOldBrush);

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//        OnSetCursor
//
/////////////////////////////////////////////////////////////////////////////
BOOL CTabPrtView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    CPoint ptDPTest;

    if (nHitTest==HTCLIENT && pWnd==this)  {


        // determine cursor position (device units)
        GetCursorPos(&ptDPTest);

        // convert cursor position to logical units, which are compatible with the CPageOb layout rects
        ScreenToClient(&ptDPTest);

        CTblPrtViewHitOb obHit;
        PRTVIEW_HITTEST ht = HitTest(ptDPTest, obHit);

        switch(ht) {
        case PRTVIEW_HIT_DEAD_SPACE:  {
            // show horz resize cursor if we're close to the right side of a col header ...
            // show vert resize cursor if we're close to the bottom of a stub or caption ...
            // otherwise, just show the magnifying glass cursor
            // note: this code block is similar to what is used in OnLButtonDown!
            bool bCloseToColHead=false;
            bool bCloseToStub=false;
            const CPgLayout& pl=m_pgMgr.GetPgLayout(obHit.GetPg());
            for (int iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
                const CPgOb& pgob = pl.GetPgOb(iPgOb);
                if (pgob.GetType()==PGOB_COLHEAD) {
                    const CRect& rcClient=pgob.GetClientRectDP();
                    int iDistanceRightSide=ptDPTest.x - rcClient.right;
                    if (rcClient.top<=ptDPTest.y && rcClient.bottom>=ptDPTest.y
                    && iDistanceRightSide>=0 && iDistanceRightSide<GetHitTestBorder()) {
                        bCloseToColHead=true;
                        break;
                    }
                }
                if (pgob.GetType()==PGOB_STUB || pgob.GetType()==PGOB_STUB_RIGHT || pgob.GetType()==PGOB_CAPTION || pgob.GetType()==PGOB_READER_BREAK) {
                    const CRect& rcClient=pgob.GetClientRectDP();
                    int iDistanceBottom=ptDPTest.y - rcClient.bottom;
                    if (rcClient.left<=ptDPTest.x && rcClient.right>=ptDPTest.x
                    && iDistanceBottom>=0 && iDistanceBottom<GetHitTestBorder()) {
                        bCloseToStub=true;
                        break;
                    }
                }
            }
            if (bCloseToColHead) {
                ::SetCursor(::LoadCursor(NULL, IDC_SIZEWE));
                return TRUE;
            }
            else if (bCloseToStub) {
                ::SetCursor(::LoadCursor(NULL, IDC_SIZENS));
                return TRUE;
            }
            }  break;

        case PRTVIEW_HIT_HEADER:
        case PRTVIEW_HIT_FOOTER:
        case PRTVIEW_HIT_TITLE:
        case PRTVIEW_HIT_SUBTITLE:
        case PRTVIEW_HIT_CELL:
        case PRTVIEW_HIT_PAGENOTE:
        case PRTVIEW_HIT_ENDNOTE:
        case PRTVIEW_HIT_STUB:
        case PRTVIEW_HIT_CAPTION:
        case PRTVIEW_HIT_COLHEAD:
        case PRTVIEW_HIT_SPANNER:
        case PRTVIEW_HIT_STUBHEAD:
            break;

        case PRTVIEW_HIT_HBORDER_HEADER:
        case PRTVIEW_HIT_HBORDER_FOOTER:
        case PRTVIEW_HIT_HBORDER_TITLE:
        case PRTVIEW_HIT_HBORDER_SUBTITLE:
        case PRTVIEW_HIT_HBORDER_CELL:
        case PRTVIEW_HIT_HBORDER_PAGENOTE:
        case PRTVIEW_HIT_HBORDER_ENDNOTE:
        case PRTVIEW_HIT_HBORDER_SPANNER:
        case PRTVIEW_HIT_HBORDER_COLHEAD:
        case PRTVIEW_HIT_HBORDER_STUBHEAD:
            break;

        case PRTVIEW_HIT_HBORDER_STUB:
        case PRTVIEW_HIT_HBORDER_CAPTION: {
            // show north/south resize, unless user is just below the stubhead
            const CPgLayout& pl=m_pgMgr.GetPgLayout(obHit.GetPg());
            const CPgOb& pgob=pl.GetPgOb(obHit.GetPgOb());
            int iDistanceTopSide=abs(ptDPTest.y - pgob.GetClientRectDP().top);
            int iDistanceBottomSide=abs(ptDPTest.y - pgob.GetClientRectDP().bottom);
            if (iDistanceTopSide<iDistanceBottomSide) {
                // we've hit just above this stub/caption, which means we're really resizing the object above us
                obHit.SetPgOb(obHit.GetPgOb()-1);
                const CPgOb& pgobTop=pl.GetPgOb(obHit.GetPgOb());
                if (pgobTop.GetType()!=PGOB_STUB && pgobTop.GetType()!=PGOB_STUB_RIGHT && pgobTop.GetType()!=PGOB_CAPTION && pgobTop.GetType()!=PGOB_READER_BREAK) {
                    // we're actually resizing the stubhead, but that's not allowed currently; just magnify
                    break;
                }
            }

            // otherwise, show vert resize cursor ...
            ::SetCursor(::LoadCursor(NULL, IDC_SIZENS));
            return TRUE;
            break;  }

        case PRTVIEW_HIT_VBORDER_HEADER:
        case PRTVIEW_HIT_VBORDER_FOOTER:
        case PRTVIEW_HIT_VBORDER_TITLE:
        case PRTVIEW_HIT_VBORDER_SUBTITLE:
        case PRTVIEW_HIT_VBORDER_CELL:
        case PRTVIEW_HIT_VBORDER_PAGENOTE:
        case PRTVIEW_HIT_VBORDER_ENDNOTE:
        case PRTVIEW_HIT_VBORDER_STUB:
        case PRTVIEW_HIT_VBORDER_SPANNER:
        case PRTVIEW_HIT_VBORDER_CAPTION:
            break;

        case PRTVIEW_HIT_VBORDER_STUBHEAD:  {
            // show horz resize cursor unless we're on the left side of the primary stubhead
            const CPgLayout& pl=m_pgMgr.GetPgLayout(obHit.GetPg());
            const CPgOb& pgobStubhead=pl.GetPgOb(obHit.GetPgOb());

            // figure out if we're working on the primary or secondary stubhead (if one is present)    // we're resizing pgob (the hit ob) ... exception: rightmost colhead really resizes secondary stubhead, if one is present
            bool bResizingPrimaryStubhead=false;
            for (int iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
                const CPgOb& pgob=pl.GetPgOb(iPgOb);
                if (pgob.GetType()==PGOB_STUBHEAD) {
                    // first stubhead is always the primary
                    bResizingPrimaryStubhead=(iPgOb==obHit.GetPgOb());
                    break;
                }
            }

            int iDistanceLeftSide=abs(ptDPTest.x - pgobStubhead.GetClientRectDP().left);
            int iDistanceRightSide=abs(ptDPTest.x - pgobStubhead.GetClientRectDP().right);
            // left<right means that we're on the left border of the stubhead, otherwise on the right border

            // do nothing if we are on the left vertical border of the primary stubhead (not a resizable spot!)
            if (!bResizingPrimaryStubhead || iDistanceLeftSide>=iDistanceRightSide) {
                // show resize cursor
                ::SetCursor(::LoadCursor(NULL, IDC_SIZEWE));
                return TRUE;
            }
            break;  }

        case PRTVIEW_HIT_VBORDER_COLHEAD:
            ::SetCursor(::LoadCursor(NULL, IDC_SIZEWE));
            return TRUE;
            break;

        case PRTVIEW_NO_HIT:
            // no hit, fall through and call base class
            break;

        default:
            ASSERT(FALSE);
        }
    }
    return CView::OnSetCursor(pWnd, nHitTest, message);
}


/////////////////////////////////////////////////////////////////////////////
//
//        HitTest
//
// ptDPTest is a point to test in device units

// obHit is the object that was hit (contains the page # and pg ob #; to get to
//      the object do      m_pgMgr.GetPgLayout(obHit.GetPg()).GetPgOb(obHit.GetPgOb())   )
// returns true if hit
// note: (0,0) is dead space btwn header and stub
//       (3,0) is 2nd stub row
//       (5,2) is 4th data row, 1st data col
// possible return codes:
//
// NO_HIT           didn't hit anything (on corner between scrollbars, on area to right of header or below stubs)
// HIT_CELL         hit inside a data cell (but not on a data field)
// HIT_CELLFIELD    hit on a data field (inside a cell)
// HIT_CELLBOX      hit on a box (inside a cell)
// HIT_CELLTEXT     hit on a text object (inside a cell)
// HIT_STUB         hit inside a stub cell
// HIT_HEADER       hit inside a header cell
// HIT_CORNER       hit the corner between stubs and headers (deadspace area)
// BORDER_STUBH     hit a stub (horiz side), but within GRIDCELL_BORDER pixels of that stub's edge
// BORDER_STUBV     hit a stub (vert side), but within GRIDCELL_BORDER pixels of that stub's edge
// BORDER_HEADERH   hit a header (horiz side), but within GRIDCELL_BORDER pixels of that header's edge
// BORDER_HEADERV   hit a header (vert side), but within GRIDCELL_BORDER pixels of that header's edge
// BORDER_CELLH     hit a cell (horiz side), but within GRIDCELL_BORDER pixels of that cell's edge
// BORDER_CELLV     hit a cell (vert side), but within GRIDCELL_BORDER pixels of that cell's edge
// BORDER_CORNERH   hit the corner deadspace area (horiz side), but within GRIDCELL_BORDER pixels from its edge
// BORDER_CORNERV   hit the corner deadspace area (vert side), but within GRIDCELL_BORDER pixels from its edgs
//
///////////////////////////////////////////////////////////////////////////////////////////////////

//
/////////////////////////////////////////////////////////////////////////////
PRTVIEW_HITTEST CTabPrtView::HitTest(const CPoint& ptDPTest, CTblPrtViewHitOb& obHit) const
{
    // see if it hit any of the objects on the screen ...

    // loop through each page being displayed ...
    for (int iPg=0 ; iPg<m_aiViewPg.GetSize() ; iPg++) {
        int iPage=m_aiViewPg[iPg];
        if (iPage==NONE)  {
            // blank placeholder page
            continue;
        }
        const CPgLayout& pl = m_pgMgr.GetPgLayout(iPage);

        // see if we hit somewhere on this page ...
        if (pl.GetUserAreaDP().PtInRect(ptDPTest)) {
            obHit.SetPg(iPage);
            for (int iPgOb=0 ; iPgOb<pl.GetNumPgObs(); iPgOb++) {
                const CPgOb& ob = pl.GetPgOb(iPgOb);
                CRect& rcOb = ob.GetClientRectDP();

                if (rcOb.PtInRect(ptDPTest)) {
                    obHit.SetPgOb(iPgOb);
                    switch(ob.GetType()) {

                    case PGOB_HEADER_LEFT:
                    case PGOB_HEADER_CENTER:
                    case PGOB_HEADER_RIGHT:
                        if (ptDPTest.x-rcOb.left<GetHitTestBorder() || rcOb.right-ptDPTest.x<GetHitTestBorder()) {
                            return PRTVIEW_HIT_VBORDER_HEADER;
                        }
                        if (ptDPTest.y-rcOb.top<GetHitTestBorder() || rcOb.bottom-ptDPTest.y<GetHitTestBorder()) {
                            return PRTVIEW_HIT_HBORDER_HEADER;
                        }
                        return PRTVIEW_HIT_HEADER;

                    case PGOB_FOOTER_LEFT:
                    case PGOB_FOOTER_CENTER:
                    case PGOB_FOOTER_RIGHT:
                        if (ptDPTest.x-rcOb.left<GetHitTestBorder() || rcOb.right-ptDPTest.x<GetHitTestBorder()) {
                            return PRTVIEW_HIT_VBORDER_FOOTER;
                        }
                        if (ptDPTest.y-rcOb.top<GetHitTestBorder() || rcOb.bottom-ptDPTest.y<GetHitTestBorder()) {
                            return PRTVIEW_HIT_HBORDER_FOOTER;
                        }
                        return PRTVIEW_HIT_FOOTER;

                    case PGOB_TITLE:
                        if (ptDPTest.x-rcOb.left<GetHitTestBorder() || rcOb.right-ptDPTest.x<GetHitTestBorder()) {
                            return PRTVIEW_HIT_VBORDER_TITLE;
                        }
                        if (ptDPTest.y-rcOb.top<GetHitTestBorder() || rcOb.bottom-ptDPTest.y<GetHitTestBorder()) {
                            return PRTVIEW_HIT_HBORDER_TITLE;
                        }
                        return PRTVIEW_HIT_TITLE;

                    case PGOB_SUBTITLE:
                        if (ptDPTest.x-rcOb.left<GetHitTestBorder() || rcOb.right-ptDPTest.x<GetHitTestBorder()) {
                            return PRTVIEW_HIT_VBORDER_SUBTITLE;
                        }
                        if (ptDPTest.y-rcOb.top<GetHitTestBorder() || rcOb.bottom-ptDPTest.y<GetHitTestBorder()) {
                            return PRTVIEW_HIT_HBORDER_SUBTITLE;
                        }
                        return PRTVIEW_HIT_SUBTITLE;

                    case PGOB_PAGENOTE:
                        if (ptDPTest.x-rcOb.left<GetHitTestBorder() || rcOb.right-ptDPTest.x<GetHitTestBorder()) {
                            return PRTVIEW_HIT_VBORDER_PAGENOTE;
                        }
                        if (ptDPTest.y-rcOb.top<GetHitTestBorder() || rcOb.bottom-ptDPTest.y<GetHitTestBorder()) {
                            return PRTVIEW_HIT_HBORDER_PAGENOTE;
                        }
                        return PRTVIEW_HIT_PAGENOTE;

                    case PGOB_ENDNOTE:
                        if (ptDPTest.x-rcOb.left<GetHitTestBorder() || rcOb.right-ptDPTest.x<GetHitTestBorder()) {
                            return PRTVIEW_HIT_VBORDER_ENDNOTE;
                        }
                        if (ptDPTest.y-rcOb.top<GetHitTestBorder() || rcOb.bottom-ptDPTest.y<GetHitTestBorder()) {
                            return PRTVIEW_HIT_HBORDER_ENDNOTE;
                        }
                        return PRTVIEW_HIT_ENDNOTE;

                    case PGOB_STUBHEAD:
                        if (ptDPTest.x-rcOb.left<GetHitTestBorder() || rcOb.right-ptDPTest.x<GetHitTestBorder()) {
                            return PRTVIEW_HIT_VBORDER_STUBHEAD;
                        }
                        if (ptDPTest.y-rcOb.top<GetHitTestBorder() || rcOb.bottom-ptDPTest.y<GetHitTestBorder()) {
                            return PRTVIEW_HIT_HBORDER_STUBHEAD;
                        }
                        return PRTVIEW_HIT_STUBHEAD;

                    case PGOB_DATACELL:
        //todo: if I hit left V border, it should be a stub hit
                        if (ptDPTest.x-rcOb.left<GetHitTestBorder() || rcOb.right-ptDPTest.x<GetHitTestBorder()) {
                            return PRTVIEW_HIT_VBORDER_CELL;
                        }
                        if (ptDPTest.y-rcOb.top<GetHitTestBorder() || rcOb.bottom-ptDPTest.y<GetHitTestBorder()) {
                            return PRTVIEW_HIT_HBORDER_CELL;
                        }
                        return PRTVIEW_HIT_CELL;

                    case PGOB_COLHEAD:
                        if (ptDPTest.x-rcOb.left<GetHitTestBorder() || rcOb.right-ptDPTest.x<GetHitTestBorder()) {
                            return PRTVIEW_HIT_VBORDER_COLHEAD;
                        }
                        if (ptDPTest.y-rcOb.top<GetHitTestBorder() || rcOb.bottom-ptDPTest.y<GetHitTestBorder()) {
                            return PRTVIEW_HIT_HBORDER_COLHEAD;
                        }
                        return PRTVIEW_HIT_COLHEAD;

                    case PGOB_SPANNER:
                        if (ptDPTest.x-rcOb.left<GetHitTestBorder() || rcOb.right-ptDPTest.x<GetHitTestBorder()) {
                            return PRTVIEW_HIT_VBORDER_SPANNER;
                        }
                        if (ptDPTest.y-rcOb.top<GetHitTestBorder() || rcOb.bottom-ptDPTest.y<GetHitTestBorder()) {
                            return PRTVIEW_HIT_HBORDER_SPANNER;
                        }
                        return PRTVIEW_HIT_SPANNER;

                    case PGOB_STUB:
                    case PGOB_STUB_RIGHT:
                    case PGOB_READER_BREAK:
                        if (ptDPTest.x-rcOb.left<GetHitTestBorder() || rcOb.right-ptDPTest.x<GetHitTestBorder()) {
                            return PRTVIEW_HIT_VBORDER_STUB;
                        }
                        if (ptDPTest.y-rcOb.top<GetHitTestBorder() || rcOb.bottom-ptDPTest.y<GetHitTestBorder()) {
                            return PRTVIEW_HIT_HBORDER_STUB;
                        }
                        return PRTVIEW_HIT_STUB;

                    case PGOB_CAPTION:
                        if (ptDPTest.x-rcOb.left<GetHitTestBorder() || rcOb.right-ptDPTest.x<GetHitTestBorder()) {
                            return PRTVIEW_HIT_VBORDER_CAPTION;
                        }
                        if (ptDPTest.y-rcOb.top<GetHitTestBorder() || rcOb.bottom-ptDPTest.y<GetHitTestBorder()) {
                            return PRTVIEW_HIT_HBORDER_CAPTION;
                        }
                        return PRTVIEW_HIT_CAPTION;

                    default:
                        ASSERT(FALSE);
                    }
                }
            }

            // no hit on any page objects; see if we're positioned somewhere on the page
            // that is outside of the page objects (in the margins, for example)
            if (pl.GetUserAreaDP().PtInRect(ptDPTest)) {
                return PRTVIEW_HIT_DEAD_SPACE;
            }
        }
    }
    return PRTVIEW_NO_HIT;
}



/////////////////////////////////////////////////////////////////////////////
//
//        OnSize
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnSize(UINT nType, int cx, int cy)
{
    CView::OnSize(nType, cx, cy);
    ResizeScrollBars();
    UpdateScrollBarRanges();
}



/////////////////////////////////////////////////////////////////////////////
//
//        ResizeScrollBars
//
// Positions scroll bar control windows in the client window.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::ResizeScrollBars()
{
    if (!m_bInitialized) {
        return;
    }

    ASSERT(m_szSBar != CSize(0,0));

    // calc client area, taking scroll bar sizes into account
    CRect rcClient;
    GetClientRect(&rcClient);
    rcClient.BottomRight() -= m_szSBar;

    // calc window positions for horz and vert scroll bars
    CRect rcVSB(0,0,m_szSBar.cx, rcClient.Height());
    rcVSB.OffsetRect(rcClient.Width(), 0);
    CRect rcHSB(0, 0, rcClient.Width(), m_szSBar.cy);
    rcHSB.OffsetRect(0, rcClient.Height());
    CRect rcSizeBox(rcVSB.left, rcHSB.top, rcVSB.right, rcHSB.bottom);

    // place scroll bars
    ASSERT_VALID(m_pVSBar);
    ASSERT_VALID(m_pHSBar);
    m_pVSBar->MoveWindow(rcVSB);
    m_pHSBar->MoveWindow(rcHSB);

    // reposition navigation bar
    GetNavBar().CenterToolBar();
}


/////////////////////////////////////////////////////////////////////////////
//
//        UpdateScrollBarRanges
//
// Sets horz and vert scroll bar ranges.  Note that these can have
// different properties depending on zoom status.
//
// if not zooming (m_fZoomFactor==NO_ZOOM), then horz bar is off and
//     vert bar has range of 0..max pages
//
// if zooming (m_fZoomFactor!=NO_ZOOM), then horz and vert correspond
//     to limits of the single zoomed page
//
// Note that this function does ***not*** set scroll position, just the ranges!!!
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::UpdateScrollBarRanges()
{
    if (!m_bInitialized) {
        return;
    }

    ASSERT(m_szSBar != CSize(0,0));

    m_pHSBar->EnableWindow();
    m_pHSBar->EnableScrollBar();

    m_pVSBar->EnableWindow();
    m_pVSBar->EnableScrollBar();

    // not zooming ==> horz is off, vert corresponds to number of pages in table set
    if ((int)GetZoomState()<=(int)ZOOM_STATE_100_PERCENT) {
        // set vertical scrollbar ranges
        // vertical bar: set scroll info, page size, etc.
        SCROLLINFO siV;
        siV.cbSize=sizeof(SCROLLINFO);
        siV.fMask=SIF_PAGE | SIF_RANGE;
        siV.nMin= 0;
        siV.nMax= m_pgMgr.GetNumPages()-1;
        if (IsBookLayout()) {
            siV.nMax++; // extra blank for first page
        }

        // add space for filler blank pages on last screen
        siV.nMax += GetPagesPerScreen() - 1;
        siV.nPage=GetPagesPerScreen();
        m_pVSBar->SetScrollInfo(&siV);
        m_pVSBar->EnableScrollBar(ESB_ENABLE_BOTH);

        // horz bars are off, but visible
        m_pHSBar->SetScrollRange(0,0);
        m_pHSBar->EnableScrollBar(ESB_DISABLE_BOTH);
    }
    else {
        // zooming ==> horz and vert correspond to the single page being viewed
        ASSERT(m_aiViewPg.GetSize()==1); // must be just 1 page being viewed

        // get total scroll size (device units) for the single table being displayed
        CSize szBorderOutside = CSize((int)(OUTSIDE_BORDER*GetZoomScaleFactor()), (int)(OUTSIDE_BORDER*GetZoomScaleFactor()));
        int iPage = GetCurrFirstViewPg();
        CSize szPage = m_pgMgr.GetPgLayout(iPage).GetUserAreaDP().Size();
        CSize szTotScroll = szBorderOutside + szBorderOutside + szPage;

        CRect rcClient;
        GetClientRect(&rcClient);
        rcClient.BottomRight() -= m_szSBar;

        if (szPage.cx>0) {
            // horizontal bar: set scroll info, page size, etc.
            SCROLLINFO siH;
            siH.cbSize=sizeof(SCROLLINFO);
            siH.fMask=SIF_PAGE | SIF_RANGE;
            siH.nMin=0;
            siH.nMax=szTotScroll.cx;
            siH.nPage=std::min(rcClient.Width(), (int) szTotScroll.cx);
            m_pHSBar->SetScrollInfo(&siH);
            if ((int)siH.nPage==siH.nMax) {
                m_pHSBar->EnableScrollBar(ESB_DISABLE_BOTH);
            }
        }
        else {
            // too small to show
            m_pHSBar->EnableWindow(FALSE);
        }

        if (szPage.cy>0) {
            // vertical bar: set scroll info, page size, etc.
            SCROLLINFO siV;
            siV.cbSize=sizeof(SCROLLINFO);
            siV.fMask=SIF_PAGE | SIF_RANGE;
            siV.nMin=0;
            siV.nMax=szTotScroll.cy;
            siV.nPage=std::min(rcClient.Height(), (int) szTotScroll.cy);
            m_pVSBar->SetScrollInfo(&siV);
            if ((int)siV.nPage==siV.nMax) {
                m_pVSBar->EnableScrollBar(ESB_DISABLE_BOTH);
            }
        }
        else {
            // too small to show
            m_pVSBar->EnableWindow(FALSE);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        OnHScroll
//
//  Handles horizontal scrolling.  Note that horz scrolling is only
//  available when we're in zoom mode.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnHScroll(UINT uSBCode, UINT uPos, CScrollBar* pScrollBar)
{
    ASSERT((int)GetZoomState()>=(int)ZOOM_STATE_100_PERCENT); // horz scrolling is only active if we are zooming

    int iHScrollPos, iPrevHScrollPos;
    SCROLLINFO siH;
    int iLineSize;   // scroll amount for line left/right
    CRect rcClient;

    siH.cbSize=sizeof(SCROLLINFO);
    m_pHSBar->GetScrollInfo(&siH, SIF_PAGE | SIF_RANGE);

    GetClientRect(&rcClient);
    rcClient.BottomRight() -= m_szSBar;

    iHScrollPos = iPrevHScrollPos = m_pHSBar->GetScrollPos();

    iLineSize = rcClient.Width() / SCROLL_LINE_SIZE;

    switch (uSBCode)  {
    case SB_TOP:
        iHScrollPos=0;
        break;
    case SB_BOTTOM:
        iHScrollPos=siH.nMax;
        break;
    case SB_LINELEFT:
        iHScrollPos -= iLineSize;
        break;
    case SB_PAGELEFT:
        iHScrollPos -= siH.nPage;
        break;
    case SB_LINERIGHT:
        iHScrollPos += iLineSize;
        break;
    case SB_PAGERIGHT:
        iHScrollPos += siH.nPage;
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        iHScrollPos = uPos;
        break;
    case SB_ENDSCROLL:
        // no action
        return;
    }

    if (iHScrollPos<siH.nMin) {
        // fix us on the left
        iHScrollPos=0;
    }

    if (iHScrollPos+(int)siH.nPage>siH.nMax) {
        // user has tried to scroll too far; fix us at the right
        iHScrollPos = siH.nMax - siH.nPage;
    }

    ScrollWindow(iPrevHScrollPos - iHScrollPos, 0, &rcClient, &rcClient);
    m_pHSBar->SetScrollPos(iHScrollPos);

    CView::OnHScroll(uSBCode, uPos, pScrollBar);
}


/////////////////////////////////////////////////////////////////////////////
//
//        OnVScroll
//
//  Handles vertical scrolling.  This can work in one of two ways:
//
//  - if in zoom mode, scrolls within a single page (the scroll extent is
//    the zoomed page);
//  - if not in zoom mode, scrolls between pages
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnVScroll(UINT uSBCode, UINT uPos, CScrollBar* pScrollBar)
{
    if (!m_pVSBar->IsWindowEnabled())  {
        return;
    }

    int iVScrollPos, iPrevVScrollPos;
    iVScrollPos = iPrevVScrollPos = m_pVSBar->GetScrollPos();
    SCROLLINFO siV;
    siV.cbSize=sizeof(SCROLLINFO);
    m_pVSBar->GetScrollInfo(&siV, SIF_ALL);
    iVScrollPos = iPrevVScrollPos = siV.nPos;

    // if viewing multiple pages concurrently, then scrolling works to move up or down among pages
    if ((int)GetZoomState()<=(int)ZOOM_STATE_100_PERCENT) {

        //-------------------------------------------------------------------------------
        // multi-page view
        //-------------------------------------------------------------------------------

        // determine number of pages concurrently displayed ...
        int iHorzPgs = GetNumHorzPgs();
        int iVertPgs = GetNumVertPgs();
        int iTotPgs = iHorzPgs * iVertPgs;   // total number of pages being shown right now
        ASSERT(iTotPgs>0 && iHorzPgs>0 && iVertPgs>0);

        const int iMin = siV.nMin;
        const int iMax = siV.nMax;
        const int iPageSz = siV.nPage;

        switch (uSBCode)  {
        case SB_TOP:
            iVScrollPos=iMin;
            break;
        case SB_BOTTOM:
            iVScrollPos= iMax;
            break;
        case SB_LINEUP:
            if (IsBookLayout()) {
                iVScrollPos -= iTotPgs; // skip by twos for book layout so we don't
                                        // show odd pages in first panel
            }
            else {
                iVScrollPos -= 1;
            }
            break;
        case SB_PAGEUP:
            iVScrollPos -= iTotPgs;
            break;
        case SB_LINEDOWN:
            if (IsBookLayout()) {
                iVScrollPos += iTotPgs; // skip by twos for book layout so we don't
                                        // show odd pages in first panel
            }
            else {
                iVScrollPos += 1;
            }
            break;
        case SB_PAGEDOWN:
            iVScrollPos += iTotPgs;
            break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            iVScrollPos = uPos;
            if (IsBookLayout()) {
                if (uPos % 2 != 0) {
                    iVScrollPos = uPos - 1; // book layout always on even page
                }
            }
            break;
        case SB_ENDSCROLL:
            // no action
            return;
        }

        if (iVScrollPos<iMin) {
            // fix us at the top
            iVScrollPos=iMin;
        }

        if (iVScrollPos>iMax) {
            // user has tried to scroll too far; do nothing
            iVScrollPos=iMax;
        }

        // scroll to the new position

        // convert scroll pos to page number
        int iPage = iVScrollPos;
        if (IsBookLayout()) {
            iPage--; // blank first page
        }
        if (iPage < 0) {
            iPage = 0;
        }
        if (iPage > m_pgMgr.GetNumPages()-1) {
            iPage = m_pgMgr.GetNumPages()-1;
        }

        GotoPage(iPage);
    }
    else {

        //-------------------------------------------------------------------------------
        // single-page view (possibly zoomed) ... scroll bars work for the currently displayed single page...
        //-------------------------------------------------------------------------------
        SCROLLINFO siV;
        int iLineSize;   // scroll amount for line up/down
        CRect rcClient;

        siV.cbSize=sizeof(SCROLLINFO);
        m_pVSBar->GetScrollInfo(&siV, SIF_PAGE | SIF_RANGE);

        GetClientRect(&rcClient);
        rcClient.BottomRight() -= m_szSBar;

        iLineSize = rcClient.Height() / SCROLL_LINE_SIZE;

        switch (uSBCode)  {
        case SB_TOP:
            iVScrollPos=0;
            break;
        case SB_BOTTOM:
            iVScrollPos=siV.nMax;
            break;
        case SB_LINEUP:
            iVScrollPos -= iLineSize;
            break;
        case SB_PAGEUP:
            iVScrollPos -= siV.nPage;
            break;
        case SB_LINEDOWN:
            iVScrollPos += iLineSize;
            break;
        case SB_PAGEDOWN:
            iVScrollPos += siV.nPage;
            break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            iVScrollPos = uPos;
            break;
        case SB_ENDSCROLL:
            // no action
            return;
        }

        if (iVScrollPos<siV.nMin) {
            // fix us at the top
            iVScrollPos=0;
        }

        if (iVScrollPos+(int)siV.nPage>siV.nMax) {
            // user has tried to scroll too far; fix us at the bottom
            iVScrollPos = siV.nMax - siV.nPage;
        }

        ScrollWindow(0, iPrevVScrollPos - iVScrollPos, &rcClient, &rcClient);
        m_pVSBar->SetScrollPos(iVScrollPos);
    }

    // show current page number (should be page 1) on the navigation bar
    GetNavBar().SetPageInfo(GetCurrFirstViewPg()+1, m_pgMgr.GetNumPages());

    CView::OnVScroll(uSBCode, uPos, pScrollBar);
}


/////////////////////////////////////////////////////////////////////////////
//
//        OnMouseWheel
//
/////////////////////////////////////////////////////////////////////////////
BOOL CTabPrtView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    bool bCtrl = (GetKeyState(VK_CONTROL)<0);
    int iLines = zDelta/WHEEL_DELTA;
    int iCount;
    ASSERT(iLines!=0);

    if (bCtrl) {
        ZOOM_STATE eNewZoomState=ZOOM_STATE_100_PERCENT;

        // figure out if we're zooming in or out...
        if (iLines<0) {
            // ctrl+wheel down --> zoom in
            if (GetZoomState()==ZOOM_STATE_300_PERCENT) {
                // can't zoom in any more
                eNewZoomState=ZOOM_STATE_300_PERCENT;
            }
            else {
                eNewZoomState=(ZOOM_STATE)((int)GetZoomState() + 1);
            }
        }
        else {
            // ctrl+wheel up --> zoom out
            if (GetZoomState()==ZOOM_STATE_4H_4V) {
                // can't zoom out any more
                eNewZoomState=ZOOM_STATE_4H_4V;
            }
            else {
                eNewZoomState=(ZOOM_STATE)((int)GetZoomState() - 1);
            }
        }

        // do the zooming ...
        if (eNewZoomState!=GetZoomState()) {
            SendMessage(UWM::Table::Zoom, (WPARAM)eNewZoomState);
        }
    }
    else {
        if (iLines<0) {
            // wheel down --> scroll down by iLines
            for (iCount=0 ; iCount<-iLines ; iCount++) {
                SendMessage(WM_VSCROLL,SB_LINEDOWN);
            }
        }
        else {
            // wheel up --> scroll up by iLines
            for (iCount=0 ; iCount<iLines ; iCount++) {
                SendMessage(WM_VSCROLL,SB_LINEUP);
            }
        }
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//        OnKeyDown
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    switch(nChar) {
    case VK_UP:
        SendMessage (WM_VSCROLL, SB_LINEUP);
        break;
    case VK_DOWN:
        SendMessage (WM_VSCROLL, SB_LINEDOWN);
        break;
    case VK_HOME:
        OnViewFirstPage();
        break;
    case VK_END:
        OnViewLastPage();
        break;
    case VK_PRIOR:
        SendMessage (WM_VSCROLL, SB_PAGEUP);
        break;
    case VK_NEXT:
        SendMessage (WM_VSCROLL, SB_PAGEDOWN);
        break;
    case VK_LEFT:
        OnViewPrevPage();
        break;
    case VK_RIGHT:
        OnViewNextPage();
        break;
    case VK_ESCAPE:
        if (IsResizing()) {
            EndResize();
        }
        break;
    }
    CView::OnKeyDown(nChar, nRepCnt, nFlags);

    // show current page number (should be page 1) on the navigation bar
    GetNavBar().SetPageInfo(GetCurrFirstViewPg()+1, m_pgMgr.GetNumPages());
}


/////////////////////////////////////////////////////////////////////////////
//
//        PreTranslateMessage
//
// Handles Esc key
//
/////////////////////////////////////////////////////////////////////////////
BOOL CTabPrtView::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message==WM_KEYDOWN && pMsg->wParam==VK_ESCAPE) {
        // so it doesn't get trapped by accelerators (maps to ID_DESELECTALL)
        SendMessage(WM_KEYDOWN, VK_ESCAPE);
    }
    return CView::PreTranslateMessage(pMsg);
}


/////////////////////////////////////////////////////////////////////////////
//
//        SetZoomState
//
// Sets zoom state to the most applicable level, based on a specified
// number of pages to view concurrently (horz/vert)
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::SetZoomState(const CSize& szPagesToView)
{
    // set zoom state based on horz/vert pages to view ...
    if (szPagesToView==CSize(1,1)) {
        SetZoomState(ZOOM_STATE_100_PERCENT);
    }
    else {
        switch(szPagesToView.cx) {
        case 1:
            switch(szPagesToView.cy) {
            case 1: //1H 1V
                SetZoomState(ZOOM_STATE_100_PERCENT);
                break;
            case 2: //1H 2V
                SetZoomState(ZOOM_STATE_2H_2V);
                break;
            case 3: //1H 3V
                SetZoomState(ZOOM_STATE_3H_3V);
                break;
            case 4: //1H 4V
                SetZoomState(ZOOM_STATE_4H_4V);
                break;
            default: //1H >4V
                SetZoomState(ZOOM_STATE_4H_4V);
                break;
            }
            break;
        case 2:
            switch(szPagesToView.cy) {
            case 1: //2H 1V
                SetZoomState(ZOOM_STATE_2H_1V);
                break;
            case 2: //2H 2V
                SetZoomState(ZOOM_STATE_2H_2V);
                break;
            case 3: //2H 3V
                SetZoomState(ZOOM_STATE_3H_3V);
                break;
            case 4: //2H 4V
                SetZoomState(ZOOM_STATE_4H_4V);
                break;
            default: //2H >4V
                SetZoomState(ZOOM_STATE_4H_4V);
                break;
            }
            break;
        case 3:
            switch(szPagesToView.cy) {
            case 1: //3H 1V
                SetZoomState(ZOOM_STATE_3H_3V);
                break;
            case 2: //3H 2V
                SetZoomState(ZOOM_STATE_3H_2V);
                break;
            case 3: //3H 3V
                SetZoomState(ZOOM_STATE_3H_3V);
                break;
            case 4: //3H 4V
                SetZoomState(ZOOM_STATE_4H_4V);
                break;
            default: //3H >4V
                SetZoomState(ZOOM_STATE_4H_4V);
                break;
            }
            break;
        case 4:
            switch(szPagesToView.cy) {
            case 1: //4H 1V
                SetZoomState(ZOOM_STATE_3H_3V);
                break;
            case 2: //4H 2V
                SetZoomState(ZOOM_STATE_3H_2V);
                break;
            case 3: //4H 3V
                SetZoomState(ZOOM_STATE_3H_3V);
                break;
            case 4: //4H 4V
                SetZoomState(ZOOM_STATE_4H_4V);
                break;
            default: //4H >4V
                SetZoomState(ZOOM_STATE_4H_4V);
                break;
            }
            break;
        default:
            switch(szPagesToView.cy) {
            case 1: //>4H 1V
                SetZoomState(ZOOM_STATE_3H_3V);
                break;
            case 2: //>4H 2V
                SetZoomState(ZOOM_STATE_3H_2V);
                break;
            case 3: //>4H 3V
                SetZoomState(ZOOM_STATE_3H_3V);
                break;
            case 4: //>4H 4V
                SetZoomState(ZOOM_STATE_4H_3V);
                break;
            default: //>4H >4V
                SetZoomState(ZOOM_STATE_4H_4V);
                break;
            }
            break;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// CTabPrtView diagnostics

#ifdef _DEBUG
void CTabPrtView::AssertValid() const
{
    CView::AssertValid();
}

void CTabPrtView::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}
#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
//
//        GetAlignmentFlags
//
// Returns the appropriate formatting flags for horizontal and vertical
// justification of a page object.  These flags are for use with CDC::DrawText.
//
/////////////////////////////////////////////////////////////////////////////
UINT CTabPrtView::GetAlignmentFlags(const CFmt* pFmt) const
{
    UINT uFormat=0;

    switch(pFmt->GetHorzAlign()) {
    case HALIGN_LEFT:
        uFormat |= DT_LEFT;
        break;
    case HALIGN_CENTER:
        uFormat |= DT_CENTER;
        break;
    case HALIGN_RIGHT:
        uFormat |= DT_RIGHT;
        break;
    default:
        ASSERT(FALSE);
    }
    switch(pFmt->GetVertAlign()) {
    case VALIGN_TOP:
        uFormat |= DT_TOP;
        break;
    case VALIGN_MID:
        uFormat |= DT_VCENTER;
        break;
    case VALIGN_BOTTOM:
        uFormat |= DT_BOTTOM;
        break;
    default:
        ASSERT(FALSE);
    }
    return uFormat;
}


/////////////////////////////////////////////////////////////////////////////
//
//        CalcDrawHeight
//
// Determines the height necessary to draw a given CPgOb within a specified
// width rectangle.
//
// Returns the height needed, in logical units loaded into the DC.
//
// piLengthDrawn optionally gives the number of characters drawn.
//
/////////////////////////////////////////////////////////////////////////////
int CTabPrtView::CalcDrawHeight(const CPgOb& pgob, CDC& dc, int* piLengthDrawn /*=NULL*/) const
{
    const CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt, pgob.GetFmt()); //pTblOb->GetFmt());
    ASSERT_VALID(pFmt);

    // back up DC
    int iDC=dc.SaveDC();
    ASSERT(iDC>0);

    // load font
    ASSERT_VALID(pFmt->GetFont());
    dc.SelectObject(pFmt->GetFont());

    // deflate horizontally into a tmp rect, to allow for indentation (whitespace padding) (tmp rect just to find bounding text rect)
    CRect rcDraw(pgob.GetClientRectLP());
    CRect rcIndent(CELL_PADDING_LEFT,CELL_PADDING_TOP,CELL_PADDING_RIGHT,CELL_PADDING_BOTTOM);
    rcDraw.DeflateRect(rcIndent.left, rcIndent.top, rcIndent.right, -rcIndent.top);  // don't deflate vertically, because we haven't yet measured

    if (NULL==piLengthDrawn) {
        // no need to know how many characters were drawn; do a straight-up call to CDC::DrawText(), using DT_CALCRECT
        dc.DrawText(pgob.GetText(), &rcDraw, pgob.GetDrawFormatFlags()|DT_CALCRECT);
    }
    else {
        // we're interested in knowing how many characters fit into the specified rectangle.  For this, we cannot use
        // DT_CALCRECT (since that would adjust the drawing rect to fit the text); instead, we create a compatible
        // DC and draw into it using ::DrawTextEx and the DRAWTEXTPARAMS parameter.

        // set params for retrieving length drawn...
        DRAWTEXTPARAMS dtp;
        dtp.cbSize=sizeof(dtp);     // size of structure
        dtp.iTabLength=0;           // size of each tab stop
        dtp.iLeftMargin=0;          // left margin
        dtp.iRightMargin=0;         // right margin
        dtp.uiLengthDrawn=0;        // receives number of characters processed

        // since we're not using DT_CALCRECT, we need to draw it somewhere other than the screen, so we'll create a compatible DC...
        CDC dcMem;
        dcMem.CreateCompatibleDC(&dc);

        // load font
        ASSERT_VALID(pFmt->GetFont());
        dcMem.SelectObject(pFmt->GetFont());

        // calc bounding rectangle for this string, based on formatting; store result in a tmp rect so we can adjust
        DrawTextEx(dcMem.GetSafeHdc(), (LPTSTR)(const TCHAR*)pgob.GetText(), -1, &rcDraw, pgob.GetDrawFormatFlags(), &dtp);

        // retrieve number of characters drawn
        *piLengthDrawn=dtp.uiLengthDrawn;

        // nuke our special DC
        dcMem.SelectStockObject(OEM_FIXED_FONT);
        dcMem.DeleteDC();
    }

    // deselect font
    dc.SelectStockObject(OEM_FIXED_FONT);

    // restore DC
    BOOL bRestore=dc.RestoreDC(iDC);
    ASSERT(bRestore);

    // return height of the calculated draw rectangle
    return rcDraw.Height();
}


/////////////////////////////////////////////////////////////////////////////
//
//        BuildHeaders
//
// Lays out headers for a single page.  Note that headers comprise 3 CPgObs:
// left, middle, and right.  The header objects are added to the passed CPgLayout
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::BuildHeaders(CPgLayout& pl, int iTbl, CDC& dc)
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    const CFmtReg& fmtReg = pSet->GetFmtReg();
    CTable* pTbl = pSet->GetTable(iTbl);

    //////////////////////////////////////////////////////////////////////////////////
    // get format attributes for headers and table printing (need to resolve any defaults)

    // header format (a CFmt)... will not contain any defaults
    CFmt *pHeaderFmt[3];
    for (int iHdr=0 ; iHdr<3 ; iHdr++) {
        pHeaderFmt[iHdr]=new CFmt;
        if (NULL!=pTbl->GetHeader(iHdr)->GetDerFmt()) {
            *pHeaderFmt[iHdr]=*pTbl->GetHeader(iHdr)->GetDerFmt();
            pHeaderFmt[iHdr]->CopyNonDefaultValues(DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(pHeaderFmt[iHdr]->GetID(),0)));
        }
        else {
            FMT_ID id;
            switch(iHdr) {
                case 0:
                    id=FMT_ID_HEADER_LEFT;
                    break;
                case 1:
                    id=FMT_ID_HEADER_CENTER;
                    break;
                case 2:
                    id=FMT_ID_HEADER_RIGHT;
                    break;
                default:
                    ASSERT(FALSE);
            }
            *pHeaderFmt[iHdr]=*DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(id,0));
        }
        LPToPoints(pHeaderFmt[iHdr], GetLogPixelsY());
        PointsToTwips(pHeaderFmt[iHdr]);
    }

    // table print format ...
    CTblPrintFmt fmtTblPrint;
    if (NULL!=pTbl->GetTblPrintFmt()) {
        fmtTblPrint=*pTbl->GetTblPrintFmt();
        fmtTblPrint.CopyNonDefaultValues(DYNAMIC_DOWNCAST(CTblPrintFmt,fmtReg.GetFmt(FMT_ID_TBLPRINT,0)));
    }
    else {
        fmtTblPrint=*DYNAMIC_DOWNCAST(CTblPrintFmt,fmtReg.GetFmt(FMT_ID_TBLPRINT,0));
    }

    //////////////////////////////////////////////////////////////////////////////////
    // determine header area, and place objects there;
    //    header is divided into left/ctr/right, and each gets a third of the available width
    //    don't worry about height yet; we still have to measure

    int iMaxHgt=0;  // tallest header
    CPgOb obHdr[3];

    CRect rcClient;
    rcClient.TopLeft() = dc.GetWindowOrg();
    rcClient.BottomRight() = dc.GetWindowExt();
    rcClient.DeflateRect(fmtTblPrint.GetPageMargin());

    // determine placements (reasonable default or persisted layout)
    CSize szThird(rcClient.Width()/3, 0);
    int iLeftPos=rcClient.left; // left side for the header we're working on
    for (int iHdr=0 ; iHdr<3 ; iHdr++) {

        // get * to the header's format/style info (a CFmt*)
        CTblOb* pHdrOb=pTbl->GetHeader(iHdr);
        ASSERT_VALID(pHdrOb);
        ASSERT_VALID(pHeaderFmt[iHdr]);

        // attach CTblOb to the header (it has the text)
        obHdr[iHdr].SetTblBase(pHdrOb);
        obHdr[iHdr].SetFmt(pHeaderFmt[iHdr]);
        obHdr[iHdr].SetTbl(iTbl);
        switch(iHdr) {
        case 0:
            obHdr[iHdr].SetType(PGOB_HEADER_LEFT);
            break;
        case 1:
            obHdr[iHdr].SetType(PGOB_HEADER_CENTER);
            break;
        case 2:
            obHdr[iHdr].SetType(PGOB_HEADER_RIGHT);
            break;
        default:
            ASSERT(FALSE);
        }

        // determine justification
        UINT uFormat = DT_WORDBREAK | GetAlignmentFlags(pHeaderFmt[iHdr]);

        // set formatting flag
        obHdr[iHdr].SetDrawFormatFlags(uFormat);

        // set text
        if (obHdr[iHdr].GetFmt()->IsTextCustom()) {
			// 20090915 GHM added functionality for the &I command
			CIMSAString headerText;
			headerText = obHdr[iHdr].GetFmt()->GetCustom().m_sCustomText;

			int strPos;
			while ((strPos = headerText.Find(_T("&I"))) != -1) {;
			        headerText = headerText.Left(strPos) + pSet->GetInputDataFilename() + headerText.Mid(strPos + 2);
				}

			obHdr[iHdr].SetText(headerText);
        }
        else {
            obHdr[iHdr].SetText(pHdrOb->GetText());
        }

        // calc drawing rect if it hasn't been done already...
        CRect rcHdr;
        bool bRecalcLayout=(pHdrOb->GetPrtViewInfoSize()==0);
        if (!bRecalcLayout) {
            if (!obHdr[iHdr].IsCustom()) {
                bRecalcLayout=true;
            }
        }
        if (m_bForceRemeasure) {
            bRecalcLayout=true;
        }
        if (!bRecalcLayout) {
            // use previous layout
            iMaxHgt=std::max(iMaxHgt, (int) pHdrOb->GetPrtViewInfo(0).GetCurrSize().cy);
            rcHdr=CRect(CPoint(iLeftPos,rcClient.top), pHdrOb->GetPrtViewInfo(0).GetCurrSize());
            rcHdr.right+=pHeaderFmt[iHdr]->GetIndent(LEFT)+pHeaderFmt[iHdr]->GetIndent(RIGHT)+CELL_PADDING_LEFT+CELL_PADDING_RIGHT;
        }
        else if (!obHdr[iHdr].GetClientRectLP().IsRectNull()) {
            // use previous calc
            rcHdr=obHdr[iHdr].GetClientRectLP();
        }
        else {
            // calc and track tallest header
            obHdr[iHdr].SetClientRectLP(CRect(0,0,szThird.cx,0));
            iMaxHgt= std::max(iMaxHgt, CalcDrawHeight(obHdr[iHdr], dc));
            rcHdr=CRect(CPoint(iLeftPos,rcClient.top), szThird);
        }

        obHdr[iHdr].SetClientRectLP(rcHdr);
        iLeftPos+=rcHdr.Width();
    }

    // give all headers the same height, corresponding to the largest of the 3
    for (int iHdr=0 ; iHdr<3 ; iHdr++) {
        // adjust rect to accommodate the drawing height
        int iBottom=rcClient.top + iMaxHgt + CELL_PADDING_TOP+CELL_PADDING_BOTTOM;
        obHdr[iHdr].GetClientRectLP().bottom = iBottom;

        // add header to array of page objects
        pl.AddPgOb(obHdr[iHdr]);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        BuildFooters
//
// Lays out footers for a single page.  Note that footers comprise 3 CPgObs:
// left, middle, and right.  The footer objects are added to the passed CPgLayout
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::BuildFooters(CPgLayout& pl, int iTbl, CDC& dc)
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    const CFmtReg& fmtReg = pSet->GetFmtReg();
    CTable* pTbl = pSet->GetTable(iTbl);

    //////////////////////////////////////////////////////////////////////////////////
    // get format attributes for footers and table printing (need to resolve any defaults)

    // header format (a CFmt)... will not contain any defaults
    CFmt *pFooterFmt[3];
    for (int iFtr=0 ; iFtr<3 ; iFtr++) {
        pFooterFmt[iFtr]=new CFmt;
        if (NULL!=pTbl->GetFooter(iFtr)->GetDerFmt()) {
            *pFooterFmt[iFtr]=*pTbl->GetFooter(iFtr)->GetDerFmt();
            pFooterFmt[iFtr]->CopyNonDefaultValues(DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(pFooterFmt[iFtr]->GetID(),0)));
        }
        else {
            FMT_ID id;
            switch(iFtr) {
                case 0:
                    id=FMT_ID_FOOTER_LEFT;
                    break;
                case 1:
                    id=FMT_ID_FOOTER_CENTER;
                    break;
                case 2:
                    id=FMT_ID_FOOTER_RIGHT;
                    break;
                default:
                    ASSERT(FALSE);
            }
            *pFooterFmt[iFtr]=*DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(id,0));
        }
        LPToPoints(pFooterFmt[iFtr], GetLogPixelsY());
        PointsToTwips(pFooterFmt[iFtr]);
    }

    // table print format ...
    CTblPrintFmt fmtTblPrint;
    if (NULL!=pTbl->GetTblPrintFmt()) {
        fmtTblPrint=*pTbl->GetTblPrintFmt();
        fmtTblPrint.CopyNonDefaultValues(DYNAMIC_DOWNCAST(CTblPrintFmt,fmtReg.GetFmt(FMT_ID_TBLPRINT,0)));
    }
    else {
        fmtTblPrint=*DYNAMIC_DOWNCAST(CTblPrintFmt,fmtReg.GetFmt(FMT_ID_TBLPRINT,0));
    }

    //////////////////////////////////////////////////////////////////////////////////
    // determine footer area, and place objects there;
    //    footer is divided into left/ctr/right, and each gets a third of the available width
    //    don't worry about height yet; we still have to measure

    int iMaxHgt=0;  // tallest footer
    CPgOb obFtr[3];

    CRect rcClient;
    rcClient.TopLeft() = dc.GetWindowOrg();
    rcClient.BottomRight() = dc.GetWindowExt();
    rcClient.DeflateRect(fmtTblPrint.GetPageMargin());

    // determine placements (reasonable default or persisted layout)
    CSize szThird(rcClient.Width()/3, 0);
    int iLeftPos=rcClient.left; // left side for the footer we're working on
    for (int iFtr=0 ; iFtr<3 ; iFtr++) {

        // get * to the footer's format/style info (a CFmt*)
        CTblOb* pFtrOb = pTbl->GetFooter(iFtr);
        ASSERT_VALID(pFtrOb);
//        const CFmt* pFtrFmt = DYNAMIC_DOWNCAST(CFmt, pFtrOb->GetFmt());
        ASSERT_VALID(pFooterFmt[iFtr]);

        // attach CTblOb to the footer (it has the text)
        obFtr[iFtr].SetTblBase(pFtrOb);
        obFtr[iFtr].SetFmt(pFooterFmt[iFtr]);
        obFtr[iFtr].SetTbl(iTbl);
        switch(iFtr) {
        case 0:
            obFtr[iFtr].SetType(PGOB_FOOTER_LEFT);
            break;
        case 1:
            obFtr[iFtr].SetType(PGOB_FOOTER_CENTER);
            break;
        case 2:
            obFtr[iFtr].SetType(PGOB_FOOTER_RIGHT);
            break;
        default:
            ASSERT(FALSE);
        }

        // determine justification
        UINT uFormat = DT_WORDBREAK | GetAlignmentFlags(pFooterFmt[iFtr]);

        // set formatting flag
        obFtr[iFtr].SetDrawFormatFlags(uFormat);

        // set text
        if (obFtr[iFtr].GetFmt()->IsTextCustom()) {
			// 20090915 GHM added functionality for the &I command
			CIMSAString footerText;
			footerText = obFtr[iFtr].GetFmt()->GetCustom().m_sCustomText;

			int strPos;
			while ((strPos = footerText.Find(_T("&I"))) != -1) {;
			        footerText = footerText.Left(strPos) + pSet->GetInputDataFilename() + footerText.Mid(strPos + 2);
				}

			obFtr[iFtr].SetText(footerText);

		}
        else {
            obFtr[iFtr].SetText(pFtrOb->GetText());
        }

        // calc drawing rect if it hasn't been done already...
        CRect rcFtr;
        bool bRecalcLayout=(pFtrOb->GetPrtViewInfoSize()==0);
        if (!bRecalcLayout) {
            if (!obFtr[iFtr].IsCustom()) {
                bRecalcLayout=true;
            }
        }
        if (m_bForceRemeasure) {
            bRecalcLayout=true;
        }
        if (!bRecalcLayout) {
            // use previous layout
            iMaxHgt= std::max(iMaxHgt, (int) pFtrOb->GetPrtViewInfo(0).GetCurrSize().cy);
            rcFtr=CRect(CPoint(iLeftPos,rcClient.bottom), pFtrOb->GetPrtViewInfo(0).GetCurrSize());
            rcFtr.OffsetRect(0,-pFtrOb->GetPrtViewInfo(0).GetCurrSize().cy);
            rcFtr.right+=pFooterFmt[iFtr]->GetIndent(LEFT)+pFooterFmt[iFtr]->GetIndent(RIGHT)+CELL_PADDING_LEFT+CELL_PADDING_RIGHT;
        }
        else if (!obFtr[iFtr].GetClientRectLP().IsRectNull()) {
            // use previous calc
            rcFtr=obFtr[iFtr].GetClientRectLP();
        }
        else {
            // calc and track tallest footer
            obFtr[iFtr].SetClientRectLP(CRect(0,0,szThird.cx,0));
            iMaxHgt= std::max(iMaxHgt, CalcDrawHeight(obFtr[iFtr], dc));
            rcFtr=CRect(CPoint(iLeftPos,rcClient.bottom), szThird);
        }

        obFtr[iFtr].SetClientRectLP(rcFtr);
        iLeftPos+=rcFtr.Width();
    }

    // give all footers the same height, corresponding to the largest of the 3
    for (int iFtr=0 ; iFtr<3 ; iFtr++) {
//        CTblOb* pFtrOb = pTbl->GetFooter(iFtr);
//        const CFmt* pFtrFmt = DYNAMIC_DOWNCAST(CFmt, pFtrOb->GetFmt());
//        ASSERT_VALID(pFtrFmt);

        // adjust rect to accommodate the drawing height
        int iTop=rcClient.bottom - iMaxHgt -  (CELL_PADDING_TOP + CELL_PADDING_BOTTOM);
        obFtr[iFtr].GetClientRectLP().top=iTop;

        // add footer to array of page objects
        pl.AddPgOb(obFtr[iFtr]);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        BuildTitles
//
// Lays out title and subtitle, which are then added to the CPgLayout.  The
// title continuation string is included when determining layouts.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::BuildTitles(CPgLayout& pl, int iTbl, CDC& dc, int iPage /*=1*/)
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    const CFmtReg& fmtReg = pSet->GetFmtReg();
    CTable* pTbl = pSet->GetTable(iTbl);
    CTblOb* pTitleOb = pTbl->GetTitle();
    CTblOb* pSubTitleOb = pTbl->GetSubTitle();

    //////////////////////////////////////////////////////////////////////////////////
    // get format attributes for titles and table printing (need to resolve any defaults)

    // title format (a CFmt)... will not contain any defaults
    CFmt fmtTitle;
    if (NULL!=pTitleOb->GetDerFmt()) {
        fmtTitle=*pTitleOb->GetDerFmt();
        fmtTitle.CopyNonDefaultValues(DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(fmtTitle.GetID(),0)));
    }
    else {
        fmtTitle=*DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_TITLE,0));
    }
    LPToPoints(&fmtTitle, GetLogPixelsY());
    PointsToTwips(&fmtTitle);

    // subtitle format (a CFmt)... will not contain any defaults
    CFmt fmtSubTitle;
    if (NULL!=pSubTitleOb->GetDerFmt()) {
        fmtSubTitle=*pSubTitleOb->GetDerFmt();
        fmtSubTitle.CopyNonDefaultValues(DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(fmtSubTitle.GetID(),0)));
    }
    else {
        fmtSubTitle=*DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_SUBTITLE,0));
    }
    LPToPoints(&fmtSubTitle, GetLogPixelsY());
    PointsToTwips(&fmtSubTitle);

    // table print format ...
    CTblPrintFmt fmtTblPrint;
    if (NULL!=pTbl->GetTblPrintFmt()) {
        fmtTblPrint=*pTbl->GetTblPrintFmt();
        fmtTblPrint.CopyNonDefaultValues(DYNAMIC_DOWNCAST(CTblPrintFmt,fmtReg.GetFmt(FMT_ID_TBLPRINT,0)));
    }
    else {
        fmtTblPrint=*DYNAMIC_DOWNCAST(CTblPrintFmt,fmtReg.GetFmt(FMT_ID_TBLPRINT,0));
    }

    const CTabSetFmt* pTabSetFmt=DYNAMIC_DOWNCAST(CTabSetFmt,fmtReg.GetFmt(FMT_ID_TABSET,0));

    // figure out where headers end vertically
    CRect rcHdr;
    int iPgOb=0 ;
    for (iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
        if (pl.GetPgOb(iPgOb).GetType()==PGOB_HEADER_LEFT || pl.GetPgOb(iPgOb).GetType()==PGOB_HEADER_CENTER || pl.GetPgOb(iPgOb).GetType()==PGOB_HEADER_RIGHT)  {
            rcHdr = pl.GetPgOb(iPgOb).GetClientRectLP();
            break;
        }
    }
    ASSERT(iPgOb<pl.GetNumPgObs());  // if not, there have been no header objects laid out

    // initialize drawing rect
    CRect rcClient;
    rcClient.TopLeft() = dc.GetWindowOrg();
    rcClient.BottomRight() = dc.GetWindowExt();
    rcClient.DeflateRect(fmtTblPrint.GetPageMargin());


    //////////////////////////////////////////////////////////////////////////////////
    // First, build the titles.  We store either 2 or 4 titles in the page template:
    //
    // for layouts LAYOUT_LEFT_STANDARD and LAYOUT_BOTH_STANDARD, store 2 titles:
    //      #0: title without continuation string
    //      #1: title with continuation string
    //
    // for layouts LAYOUT_LEFT_FACING and LAYOUT_BOTH_FACING, store 4 titles:
    //      #0: left-side title without continuation string
    //      #1: right-side title without continuation string
    //      #2: left-side title with continuation string
    //      #3: right-side title with continuation string
    //      If the title is centered, then left-side titles become right
    //      justified and right-side titles become left justified.
    //
    // These titles are stored consecutively in each CPageLayout.
    //
    // note that ALL table objects have the same size, which means that whitespace will
    // sometimes appear when they are rendered.  But this makes the table layout similar
    // across pages.
    //////////////////////////////////////////////////////////////////////////////////
    CArray<CPgOb, CPgOb&> aTitle;
    int iNumTitles=2;
    bool bFacingLayout=false;
    if (fmtTblPrint.GetTblLayout()==TBL_LAYOUT_LEFT_FACING || fmtTblPrint.GetTblLayout()==TBL_LAYOUT_BOTH_FACING) {
        iNumTitles += 2;
        bFacingLayout=true;
    }
    for (int iTitle=0 ; iTitle<iNumTitles ; iTitle++) {
        CPgOb obTitle;
        aTitle.Add(obTitle);
    }

    // titles go just below headers ...
    CRect rcTitle(rcClient.left, rcHdr.bottom, rcClient.right, rcHdr.bottom);

    // initialize titles
    for (int iTitle=0 ; iTitle<aTitle.GetSize() ; iTitle++) {
        CPgOb& obTitle=aTitle[iTitle];

        // set starting rect
        obTitle.SetClientRectLP(rcTitle);  // don't assign vertical area; we still need to calc that

        // set type
        obTitle.SetType(PGOB_TITLE);

        // set format
        CFmt* pTitleFmt=new CFmt(fmtTitle);
        obTitle.SetFmt(pTitleFmt);

        // set text
        if (pTitleFmt->IsTextCustom()) {
            obTitle.SetText(pTitleFmt->GetCustom().m_sCustomText);
        }
        else {
            obTitle.SetText(pTitleOb->GetText());
        }

        // add continuation string to titles that have it, just for calculating rect
        if ((bFacingLayout && iTitle>1) || (!bFacingLayout && iTitle==1)) {
            // yes, continuation string
            obTitle.SetText(obTitle.GetText()+pTabSetFmt->GetContinuationStr());
        }

        // attach the ob's CTblOb
        obTitle.SetTblBase(pTitleOb);
        obTitle.SetTbl(iTbl);

        // set formats...
        if (bFacingLayout && pTitleFmt->GetHorzAlign()==HALIGN_CENTER) {
            // these use the scrap fmts
            if ((iTitle%2)==0) {
                pTitleFmt->SetHorzAlign(HALIGN_RIGHT);
            }
            else {
                pTitleFmt->SetHorzAlign(HALIGN_LEFT);
            }
        }
        obTitle.SetDrawFormatFlags(DT_WORDBREAK | GetAlignmentFlags(obTitle.GetFmt()));
    }

    int iTitleHgt=0;
    if (bFacingLayout) {
        for (int iTitle=0 ; iTitle<aTitle.GetSize() ; iTitle+=2) {  // this will get us titles #0 and #2

            // LAYOUT_LEFT_FACING and LAYOUT_BOTH_FACING need special treatment for titles.  These layouts cause
            // the table title to be spread across 2 pages, for example:
            //     left page (1,3,5,...)            right page (2,4,6,...)
            //     Now is the time for              all good men to
            //     come to the aid                  of their party.
            // In these cases, we'll have 4 title objects on each page layout (see comments above).
            // Later, we'll pluck them out and assign them to pages left, right, left, right, etc.

            CPgOb& obTitleLeft=aTitle[iTitle];
            CPgOb& obTitleRight=aTitle[iTitle+1];
            CIMSAString sRemaining;  // remaining title text to allocate
            CIMSAString sCurLine;    // current line we're working on
            CFmt* pFmt;               // title format
            int iLineHgt;             // line height
            bool bAddLeft=true;       // true when the next line will be added to the left title (false means that next line will be added to the right title)
            CPgOb obTmp;              // temp object, for calcs
            int iWordWrapPoint;

            // init
            ASSERT(obTitleLeft.GetText()==obTitleRight.GetText());
            ASSERT(obTitleLeft.GetTblBase()->GetFmt()==obTitleRight.GetTblBase()->GetFmt());
            sRemaining=obTitleLeft.GetText();
            pFmt=obTitleLeft.GetFmt();
            obTitleLeft.SetText(_T(""));
            obTitleRight.SetText(_T(""));
            obTmp=obTitleLeft;

            // determine line height
            obTmp.SetDrawFormatFlags(DT_SINGLELINE|DT_TOP|DT_LEFT);
            obTmp.SetText(sRemaining);
            iLineHgt=CalcDrawHeight(obTmp, dc) - (CELL_PADDING_TOP + CELL_PADDING_BOTTOM);
            CRect rcSingleLine(obTmp.GetClientRectLP());

            rcSingleLine.bottom=rcSingleLine.top+iLineHgt;
            obTmp.SetClientRectLP(rcSingleLine);
            obTmp.SetDrawFormatFlags(DT_WORDBREAK|GetAlignmentFlags(pFmt));

            // iterate through the title, extracting a line at a time. A line is what can be displayed until word wrapping is needed
            while (!sRemaining.IsEmpty()) {
                obTmp.SetText(sRemaining);
                CalcDrawHeight(obTmp, dc, &iWordWrapPoint);
                sCurLine=sRemaining.Left(iWordWrapPoint);
                sCurLine.Trim();
                if (bAddLeft) {
                    if (!obTitleLeft.GetText().IsEmpty()) {
                        // force a line break
                        sCurLine=_T("\n")+sCurLine;
                    }
                    obTitleLeft.SetText(obTitleLeft.GetText()+sCurLine);
                }
                else {
                    if (!obTitleRight.GetText().IsEmpty()) {
                        // force a line break
                        sCurLine=_T("\n")+sCurLine;
                    }
                    obTitleRight.SetText(obTitleRight.GetText()+sCurLine);
                }
                sRemaining=sRemaining.Mid(iWordWrapPoint);
                bAddLeft=!bAddLeft;
            }

            // calc drawing rect if it hasn't been done already...
            bool bRecalcLayout=(obTitleLeft.GetTblBase()->GetPrtViewInfoSize()==0);
            if (m_bForceRemeasure) {
                bRecalcLayout=true;
            }
            if (!bRecalcLayout) {
                // use persisted layout
                iTitleHgt=__max(iTitleHgt,obTitleLeft.GetTblBase()->GetPrtViewInfo(0).GetCurrSize().cy);
            }
            else if (obTitleLeft.GetClientRectLP().Height()>0) {
                // use previous calcs, if possible
                iTitleHgt=__max(iTitleHgt,obTitleLeft.GetClientRectLP().Height());
            }
            else {
                int iLeftRightMax=__max(CalcDrawHeight(obTitleLeft, dc), CalcDrawHeight(obTitleRight, dc));
                iTitleHgt = __max(iTitleHgt, iLeftRightMax);
            }
        }
    }
    else {
        // LAYOUT_LEFT_STANDARD or LAYOUT_BOTH_STANDARD layout
        for (int iTitle=0 ; iTitle<aTitle.GetSize() ; iTitle++) {  // this will get us titles #0 and #1
            CPgOb& obTitle=aTitle[iTitle];

            // calc drawing rect if it hasn't been done already...
            bool bRecalcLayout=(obTitle.GetTblBase()->GetPrtViewInfoSize()==0);
            if (m_bForceRemeasure) {
                bRecalcLayout=true;
            }
            if (!bRecalcLayout) {
                // use persisted layout
                iTitleHgt=__max(iTitleHgt,obTitle.GetTblBase()->GetPrtViewInfo(0).GetCurrSize().cy);
            }
            else if (obTitle.GetClientRectLP().Height()>0) {
                // use previous calcs, if possible
                iTitleHgt=__max(iTitleHgt,obTitle.GetClientRectLP().Height() - (CELL_PADDING_TOP + CELL_PADDING_BOTTOM));
            }
            else {
                iTitleHgt = __max(iTitleHgt, CalcDrawHeight(obTitle, dc));
            }
        }
    }

    // all title objects are now measured and prepped.  We now make them all the same height, and add
    // them to the page template...
    for (int iTitle=0 ; iTitle<aTitle.GetSize() ; iTitle++) {
        CPgOb& obTitle = aTitle[iTitle];
        CFmt* pFmt=obTitle.GetFmt();

        // set size to highest title seen, and add vertical indentation (padding)
        rcTitle=obTitle.GetClientRectLP();
        rcTitle.bottom = rcTitle.top + iTitleHgt + CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
        obTitle.SetClientRectLP(rcTitle);

        // add title to array of page objects
        pl.AddPgOb(obTitle);
    }


    //////////////////////////////////////////////////////////////////////////////////
    // Next, build the subtitles.  We store either 1 or 2 subtitles in the page template,
    // similar to how titles work but with no use of continuation strings.
    //
    // for layouts LAYOUT_LEFT_STANDARD and LAYOUT_BOTH_STANDARD, store 2 subtitles:
    //      #0: subtitle
    //
    // for layouts LAYOUT_LEFT_FACING and LAYOUT_BOTH_FACING, store 4 subtitles:
    //      #0: left-side subtitle
    //      #1: right-side subtitle
    //      If the title is centered, then left-side subtitles become right
    //      justified and right-side subtitles become left justified.
    //
    // These subtitles are stored consecutively in each CPageLayout.
    //
    // note that ALL table objects have the same size, which means that whitespace will
    // sometimes appear when they are rendered.  But this makes the table layout similar
    // across pages.
    //////////////////////////////////////////////////////////////////////////////////
    CArray<CPgOb, CPgOb&> aSubTitle;
    int iNumSubTitles=1;
    if (bFacingLayout) {
        iNumSubTitles++;
    }
    for (int iSubTitle=0 ; iSubTitle<iNumSubTitles ; iSubTitle++) {
        CPgOb obSubTitle;
        aSubTitle.Add(obSubTitle);
    }

    // subtitles go just below titles...
    CRect rcSubTitle(rcClient.left, rcTitle.bottom, rcClient.right, rcTitle.bottom);

    // initialize subtitles
    for (int iSubTitle=0 ; iSubTitle<aSubTitle.GetSize() ; iSubTitle++) {
        CPgOb& obSubTitle=aSubTitle[iSubTitle];

        // set starting rect
        obSubTitle.SetClientRectLP(rcSubTitle);  // don't assign vertical area; we still need to calc that

        // set type
        obSubTitle.SetType(PGOB_SUBTITLE);

        // set format
        CFmt* pSubTitleFmt=new CFmt(fmtSubTitle);
        obSubTitle.SetFmt(pSubTitleFmt);

        // set text
        if (pSubTitleFmt->IsTextCustom()) {
            obSubTitle.SetText(pSubTitleFmt->GetCustom().m_sCustomText);
        }
        else {
            obSubTitle.SetText(pSubTitleOb->GetText());
        }

        // note that no continuation strings are used for subtitles

        // attach the ob's CTblOb
        obSubTitle.SetTblBase(pSubTitleOb);
        obSubTitle.SetTbl(iTbl);

        // set formats...
        if (bFacingLayout && pSubTitleFmt->GetHorzAlign()==HALIGN_CENTER) {
            // these use the scrap fmts
            if ((iSubTitle%2)==0) {
                pSubTitleFmt->SetHorzAlign(HALIGN_RIGHT);
            }
            else {
                pSubTitleFmt->SetHorzAlign(HALIGN_LEFT);
            }
        }
        obSubTitle.SetDrawFormatFlags(DT_WORDBREAK | GetAlignmentFlags(obSubTitle.GetFmt()));
    }

    int iSubTitleHgt=0;
    if (bFacingLayout) {
        ASSERT(aSubTitle.GetSize()==2);

        // LAYOUT_LEFT_FACING and LAYOUT_BOTH_FACING need special treatment for subtitles.  These layouts cause
        // the table subtitle to be spread across 2 pages, for example:
        //     left page (1,3,5,...)            right page (2,4,6,...)
        //     Now is the time for              all good men to
        //     come to the aid                  of their party.
        // In these cases, we'll have 2 subtitle objects on each page layout (see comments above).
        // Later, we'll pluck them out and assign them to pages left, right, left, right, etc.

        CPgOb& obSubTitleLeft=aSubTitle[0];
        CPgOb& obSubTitleRight=aSubTitle[1];
        CIMSAString sRemaining;   // remaining subtitle text to allocate
        CIMSAString sCurLine;     // current line we're working on
        CFmt* pFmt;               // subtitle format
        int iLineHgt;             // line height
        bool bAddLeft=true;       // true when the next line will be added to the left subtitle (false means that next line will be added to the right subtitle)
        CPgOb obTmp;              // temp object, for calcs
        int iWordWrapPoint;

        // init
        ASSERT(obSubTitleLeft.GetText()==obSubTitleRight.GetText());
        ASSERT(obSubTitleLeft.GetTblBase()->GetFmt()==obSubTitleRight.GetTblBase()->GetFmt());
        sRemaining=obSubTitleLeft.GetText();
        pFmt=obSubTitleLeft.GetFmt();
        obSubTitleLeft.SetText(_T(""));
        obSubTitleRight.SetText(_T(""));
        obTmp=obSubTitleLeft;

        // determine line height
        obTmp.SetDrawFormatFlags(DT_SINGLELINE|DT_TOP|DT_LEFT);
        obTmp.SetText(sRemaining);
        iLineHgt=CalcDrawHeight(obTmp, dc) - (CELL_PADDING_TOP + CELL_PADDING_BOTTOM);
        CRect rcSingleLine(obTmp.GetClientRectLP());

        rcSingleLine.bottom=rcSingleLine.top+iLineHgt;
        obTmp.SetClientRectLP(rcSingleLine);
        obTmp.SetDrawFormatFlags(DT_WORDBREAK|GetAlignmentFlags(pFmt));

        // iterate through the subtitle, extracting a line at a time. A line is what can be displayed until word wrapping is needed
        while (!sRemaining.IsEmpty()) {
            obTmp.SetText(sRemaining);
            CalcDrawHeight(obTmp, dc, &iWordWrapPoint);
            sCurLine=sRemaining.Left(iWordWrapPoint);
            sCurLine.Trim();
            if (bAddLeft) {
                if (!obSubTitleLeft.GetText().IsEmpty()) {
                    // force a line break
                    sCurLine=_T("\n")+sCurLine;
                }
                obSubTitleLeft.SetText(obSubTitleLeft.GetText()+sCurLine);
            }
            else {
                if (!obSubTitleRight.GetText().IsEmpty()) {
                    // force a line break
                    sCurLine=_T("\n")+sCurLine;
                }
                obSubTitleRight.SetText(obSubTitleRight.GetText()+sCurLine);
            }
            sRemaining=sRemaining.Mid(iWordWrapPoint);
            bAddLeft=!bAddLeft;
        }

        // calc drawing rect if it hasn't been done already...
        bool bRecalcLayout=(obSubTitleLeft.GetTblBase()->GetPrtViewInfoSize()==0);
        if (m_bForceRemeasure) {
            bRecalcLayout=true;
        }
        if (!bRecalcLayout) {
            // use persisted layout
            iSubTitleHgt=__max(iSubTitleHgt,obSubTitleLeft.GetTblBase()->GetPrtViewInfo(0).GetCurrSize().cy);
        }
        else if (obSubTitleLeft.GetClientRectLP().Height()>0) {
            // use previous calcs, if possible
            iSubTitleHgt=__max(iSubTitleHgt,obSubTitleLeft.GetClientRectLP().Height());
        }
        else {
            int iLeftRightMax=__max(CalcDrawHeight(obSubTitleLeft, dc), CalcDrawHeight(obSubTitleRight, dc));
            iSubTitleHgt = __max(iSubTitleHgt, iLeftRightMax);
        }
    }
    else {
        // LAYOUT_LEFT_STANDARD or LAYOUT_BOTH_STANDARD layout
        ASSERT(aSubTitle.GetSize()==1);
        CPgOb& obSubTitle=aSubTitle[0];

        // calc drawing rect if it hasn't been done already...
        bool bRecalcLayout=(obSubTitle.GetTblBase()->GetPrtViewInfoSize()==0);
        if (m_bForceRemeasure) {
            bRecalcLayout=true;
        }
        if (!bRecalcLayout) {
            // use persisted layout
            iSubTitleHgt=__max(iSubTitleHgt,obSubTitle.GetTblBase()->GetPrtViewInfo(0).GetCurrSize().cy);
        }
        else if (obSubTitle.GetClientRectLP().Height()>0) {
            // use previous calcs, if possible
            iSubTitleHgt=__max(iSubTitleHgt,obSubTitle.GetClientRectLP().Height() - (CELL_PADDING_TOP + CELL_PADDING_BOTTOM));
        }
        else {
            iSubTitleHgt = __max(iSubTitleHgt, CalcDrawHeight(obSubTitle, dc));
        }
    }

    // all subtitle objects are now measured and prepped.  We now make them all the same height, and add
    // them to the page template...
    for (int iSubTitle=0 ; iSubTitle<aSubTitle.GetSize() ; iSubTitle++) {
        CPgOb& obSubTitle = aSubTitle[iSubTitle];
        CFmt* pFmt=obSubTitle.GetFmt();

        // set size to highest subtitle seen, and add vertical indentation (padding)
        rcSubTitle=obSubTitle.GetClientRectLP();
        rcSubTitle.bottom = rcSubTitle.top + iSubTitleHgt + CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
        obSubTitle.SetClientRectLP(rcSubTitle);

        // add subtitle to array of page objects
        pl.AddPgOb(obSubTitle);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        BuildNotes
//
// Lays out page note and end note, if present.  (Page notes appear on every
// page, below the cells and above the footer.  End notes appear on the last
// vert page{s} of a table, below the page note and above the footer.)
//
// Facing page layouts have 2 notes, standard layouts have one:
//
// for layouts LAYOUT_LEFT_STANDARD and LAYOUT_BOTH_STANDARD:
//      #0: page/end note for all pages
//
// for layouts LAYOUT_LEFT_FACING and LAYOUT_BOTH_FACING:
//      #0: left-side page/end note
//      #1: right-side page/end note
//      If a page or end note is centered, then the left-side note becomes right
//      justified and the right-side note becomes left justified.
//      These are stored consecutively in the page layout.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::BuildNotes(CPgLayout& pl, int iTbl, CDC& dc, int iPage /*=1*/)
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    const CFmtReg& fmtReg = pSet->GetFmtReg();
    CTable* pTbl = pSet->GetTable(iTbl);
    CTblOb* pPageNoteOb = DYNAMIC_DOWNCAST(CTblOb,pTbl->GetPageNote());
    CTblOb* pEndNoteOb = DYNAMIC_DOWNCAST(CTblOb,pTbl->GetEndNote());

    // page note object format (will not contain any defaults)
    CFmt fmtPageNote;
    if (NULL!=pPageNoteOb->GetDerFmt()) {
        fmtPageNote=*pPageNoteOb->GetDerFmt();
        fmtPageNote.CopyNonDefaultValues(DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(fmtPageNote.GetID(),0)));
    }
    else {
        fmtPageNote=*DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_PAGENOTE,0));
    }
    LPToPoints(&fmtPageNote, GetLogPixelsY());
    PointsToTwips(&fmtPageNote);

    // table print format ...
    CTblPrintFmt fmtTblPrint;
    if (NULL!=pTbl->GetTblPrintFmt()) {
        fmtTblPrint=*pTbl->GetTblPrintFmt();
        fmtTblPrint.CopyNonDefaultValues(DYNAMIC_DOWNCAST(CTblPrintFmt,fmtReg.GetFmt(FMT_ID_TBLPRINT,0)));
    }
    else {
        fmtTblPrint=*DYNAMIC_DOWNCAST(CTblPrintFmt,fmtReg.GetFmt(FMT_ID_TBLPRINT,0));
    }

    bool bFacingLayout = (fmtTblPrint.GetTblLayout()==TBL_LAYOUT_LEFT_FACING || fmtTblPrint.GetTblLayout()==TBL_LAYOUT_BOTH_FACING);

    //////////////////////////////////////////////////////////////////////////////////
    // Build the page note, if there is one present
    if (fmtPageNote.IsTextCustom() || !pPageNoteOb->GetText().IsEmpty()) {
        CPgOb obPageNote;

        // figure out where footers start vertically
        CRect rcFtr;
        int iPgOb=0 ;
        for (iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
            if (pl.GetPgOb(iPgOb).GetType()==PGOB_FOOTER_LEFT || pl.GetPgOb(iPgOb).GetType()==PGOB_FOOTER_CENTER || pl.GetPgOb(iPgOb).GetType()==PGOB_FOOTER_RIGHT)  {
                rcFtr = pl.GetPgOb(iPgOb).GetClientRectLP();
                break;
            }
        }
        ASSERT(iPgOb<pl.GetNumPgObs());  // if not, there have been no header objects laid out

        // initialize drawing rect
        CRect rcClient;
        rcClient.TopLeft() = dc.GetWindowOrg();
        rcClient.BottomRight() = dc.GetWindowExt();
        rcClient.DeflateRect(fmtTblPrint.GetPageMargin());

        // we'll calculate a placement for the page note, but it might get moved later depending on how the stubs lay out.
        CRect rcPageNote(rcClient.left, rcFtr.top, rcClient.right, rcFtr.top);
        obPageNote.SetClientRectLP(rcPageNote);  // don't assign vertical area; we still need to calc that

        // set type
        obPageNote.SetType(PGOB_PAGENOTE);

        // attach the ob's CTblOb
        obPageNote.SetTblBase(pPageNoteOb);
        obPageNote.SetTbl(iTbl);

        // create and attach format
        CFmt* pFmt=new CFmt(fmtPageNote);
        obPageNote.SetFmt(pFmt);

        // set text
        if (pFmt->IsTextCustom()) {
            obPageNote.SetText(pFmt->GetCustom().m_sCustomText);
        }
        else  {
            obPageNote.SetText(pTbl->GetPageNote()->GetText());
        }

        if (bFacingLayout) {
            CPgOb obPageNote2(obPageNote);   // create a new page note, for right side pages
            CIMSAString sRemaining;  // remaining note text to allocate
            CIMSAString sCurLine;    // current note we're working on
            CFmt* pFmt;               // note format
            int iLineHgt;             // line height
            bool bAddLeft=true;       // true when the next line will be added to the left note (false means that next line will be added to the right note)
            CPgOb obTmp;              // temp object, for calcs
            int iWordWrapPoint;

            // init
            ASSERT(obPageNote.GetText()==obPageNote2.GetText());
            ASSERT(obPageNote.GetTblBase()->GetFmt()==obPageNote2.GetTblBase()->GetFmt());
            sRemaining=obPageNote.GetText();
            pFmt=new CFmt(fmtPageNote);
            obPageNote2.SetFmt(pFmt);

            obPageNote.SetText(_T(""));
            obPageNote2.SetText(_T(""));
            obTmp=obPageNote;

            // determine line height
            obTmp.SetDrawFormatFlags(DT_SINGLELINE|DT_TOP|DT_LEFT);
            obTmp.SetText(sRemaining);
            iLineHgt=CalcDrawHeight(obTmp, dc) - (CELL_PADDING_TOP + CELL_PADDING_BOTTOM);
            CRect rcSingleLine(obTmp.GetClientRectLP());

            rcSingleLine.bottom=rcSingleLine.top+iLineHgt;
            obTmp.SetClientRectLP(rcSingleLine);
            obTmp.SetDrawFormatFlags(DT_WORDBREAK|GetAlignmentFlags(pFmt));

            // iterate through the note, extracting a line at a time. A line is what can be displayed until word wrapping is needed
            while (!sRemaining.IsEmpty()) {
                obTmp.SetText(sRemaining);
                CalcDrawHeight(obTmp, dc, &iWordWrapPoint);
                sCurLine=sRemaining.Left(iWordWrapPoint);
                sCurLine.Trim();
                if (bAddLeft) {
                    if (!obPageNote.GetText().IsEmpty()) {
                        // force a line break
                        sCurLine=_T("\n")+sCurLine;
                    }
                    obPageNote.SetText(obPageNote.GetText()+sCurLine);
                }
                else {
                    if (!obPageNote2.GetText().IsEmpty()) {
                        // force a line break
                        sCurLine=_T("\n")+sCurLine;
                    }
                    obPageNote2.SetText(obPageNote2.GetText()+sCurLine);
                }
                sRemaining=sRemaining.Mid(iWordWrapPoint);
                bAddLeft=!bAddLeft;
            }

            // use the tallest page note, and add vertical indentation (padding)
            bool bRecalcLayout=(obPageNote.GetTblBase()->GetPrtViewInfoSize()==0);
            int iHgt;
            if (m_bForceRemeasure) {
                bRecalcLayout=true;
            }
            if (bRecalcLayout) {
                iHgt=__max(CalcDrawHeight(obPageNote, dc), CalcDrawHeight(obPageNote2, dc));
            }
            else {
                // use previous layout
                iHgt=obPageNote.GetTblBase()->GetPrtViewInfo(0).GetCurrSize().cy;
            }

            rcPageNote.top = rcPageNote.bottom - iHgt - (CELL_PADDING_BOTTOM + CELL_PADDING_TOP);
            obPageNote.SetClientRectLP(rcPageNote);
            obPageNote2.SetClientRectLP(rcPageNote);

            // set formats for left/right pages ...
            if (obPageNote.GetFmt()->GetHorzAlign()==HALIGN_CENTER) {
                // these use the scrap fmts
                obPageNote.GetFmt()->SetHorzAlign(HALIGN_RIGHT);
                obPageNote2.GetFmt()->SetHorzAlign(HALIGN_LEFT);
            }

            // set format flags
            obPageNote.SetDrawFormatFlags(DT_WORDBREAK | GetAlignmentFlags(obPageNote.GetFmt()));
            obPageNote2.SetDrawFormatFlags(DT_WORDBREAK | GetAlignmentFlags(obPageNote2.GetFmt()));

            // add page notes to array of page objects
            pl.AddPgOb(obPageNote);
            pl.AddPgOb(obPageNote2);
        }
        else {
            // calc drawing rect, if necessary
            bool bRecalcLayout=(obPageNote.GetTblBase()->GetPrtViewInfoSize()==0);
            int iHgt;
            if (m_bForceRemeasure) {
                bRecalcLayout=true;
            }
            if (bRecalcLayout) {
                iHgt=CalcDrawHeight(obPageNote, dc);
            }
            else {
                // use previous layout
                iHgt=obPageNote.GetTblBase()->GetPrtViewInfo(0).GetCurrSize().cy;
            }

            // add vertical indentation (padding)
            rcPageNote.top = rcPageNote.bottom - iHgt - (CELL_PADDING_BOTTOM + CELL_PADDING_TOP);
            obPageNote.SetClientRectLP(rcPageNote);

            // set format flags
            obPageNote.SetDrawFormatFlags(DT_WORDBREAK | GetAlignmentFlags(obPageNote.GetFmt()));

            // add page note to array of page objects
            pl.AddPgOb(obPageNote);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////
    // Build the end note, if there is one present

    // end note object format (will not contain any defaults)
    CFmt fmtEndNote;
    if (NULL!=pEndNoteOb->GetDerFmt()) {
        fmtEndNote=*pEndNoteOb->GetDerFmt();
        fmtEndNote.CopyNonDefaultValues(DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(fmtEndNote.GetID(),0)));
    }
    else {
        fmtEndNote=*DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_ENDNOTE,0));
    }
    LPToPoints(&fmtEndNote, GetLogPixelsY());
    PointsToTwips(&fmtEndNote);

    if (fmtEndNote.IsTextCustom() || !pEndNoteOb->GetText().IsEmpty()) {
        CPgOb obEndNote;

        // figure out where footers start vertically
        CRect rcFtr;
        int iPgOb=0 ;
        for (iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
            if (pl.GetPgOb(iPgOb).GetType()==PGOB_FOOTER_LEFT || pl.GetPgOb(iPgOb).GetType()==PGOB_FOOTER_CENTER || pl.GetPgOb(iPgOb).GetType()==PGOB_FOOTER_RIGHT)  {
                rcFtr = pl.GetPgOb(iPgOb).GetClientRectLP();
                break;
            }
        }
        ASSERT(iPgOb<pl.GetNumPgObs());  // if not, there have been no header objects laid out

        // initialize drawing rect
        CRect rcClient;
        rcClient.TopLeft() = dc.GetWindowOrg();
        rcClient.BottomRight() = dc.GetWindowExt();
        rcClient.DeflateRect(fmtTblPrint.GetPageMargin());

        // we'll calculate a placement for the end note, but it might get moved later depending on how the stubs lay out.
        CRect rcEndNote(rcClient.left, rcFtr.top, rcClient.right, rcFtr.top);
        obEndNote.SetClientRectLP(rcEndNote);  // don't assign vertical area; we still need to calc that

        // set type
        obEndNote.SetType(PGOB_ENDNOTE);

        // create and attach fmt
        CFmt* pFmt=new CFmt(fmtEndNote);
        obEndNote.SetFmt(pFmt);

        // set text
        if (pFmt->IsTextCustom()) {
            obEndNote.SetText(pFmt->GetCustom().m_sCustomText);
        }
        else {
            obEndNote.SetText(pTbl->GetEndNote()->GetText());
        }

        // attach the ob's CTblOb
        obEndNote.SetTblBase(pEndNoteOb);
        obEndNote.SetTbl(iTbl);

        obEndNote.SetDrawFormatFlags(DT_WORDBREAK | GetAlignmentFlags(pFmt));

        if (bFacingLayout) {
            CPgOb obEndNote2(obEndNote);   // create a new end note, for right side pages
            CIMSAString sRemaining;  // remaining note text to allocate
            CIMSAString sCurLine;    // current note we're working on
            CFmt* pFmt;               // note format
            int iLineHgt;             // line height
            bool bAddLeft=true;       // true when the next line will be added to the left note (false means that next line will be added to the right note)
            CPgOb obTmp;              // temp object, for calcs
            int iWordWrapPoint;

            // init
            ASSERT(obEndNote.GetText()==obEndNote2.GetText());
            ASSERT(obEndNote.GetTblBase()->GetFmt()==obEndNote2.GetTblBase()->GetFmt());
            sRemaining=obEndNote.GetText();
//            pFmt= DYNAMIC_DOWNCAST(CFmt,obEndNote.GetTblBase()->GetFmt());
            pFmt=new CFmt(fmtEndNote);
            obEndNote2.SetFmt(pFmt);

            obEndNote.SetText(_T(""));
            obEndNote2.SetText(_T(""));
            obTmp=obEndNote;

            // determine line height
            obTmp.SetDrawFormatFlags(DT_SINGLELINE|DT_TOP|DT_LEFT);
            obTmp.SetText(sRemaining);
//            iLineHgt=CalcDrawHeight(obTmp, dc) - pFmt->GetIndent().top - pFmt->GetIndent().bottom;  CHRISFOO
            iLineHgt=CalcDrawHeight(obTmp, dc) - (CELL_PADDING_TOP+CELL_PADDING_BOTTOM);
            CRect rcSingleLine(obTmp.GetClientRectLP());

            rcSingleLine.bottom=rcSingleLine.top+iLineHgt;
            obTmp.SetClientRectLP(rcSingleLine);
            obTmp.SetDrawFormatFlags(DT_WORDBREAK|GetAlignmentFlags(pFmt));

            // iterate through the note, extracting a line at a time. A line is what can be displayed until word wrapping is needed
            while (!sRemaining.IsEmpty()) {
                obTmp.SetText(sRemaining);
                CalcDrawHeight(obTmp, dc, &iWordWrapPoint);
                sCurLine=sRemaining.Left(iWordWrapPoint);
                sCurLine.Trim();
                if (bAddLeft) {
                    if (!obEndNote.GetText().IsEmpty()) {
                        // force a line break
                        sCurLine=_T("\n")+sCurLine;
                    }
                    obEndNote.SetText(obEndNote.GetText()+sCurLine);
                }
                else {
                    if (!obEndNote2.GetText().IsEmpty()) {
                        // force a line break
                        sCurLine=_T("\n")+sCurLine;
                    }
                    obEndNote2.SetText(obEndNote2.GetText()+sCurLine);
                }
                sRemaining=sRemaining.Mid(iWordWrapPoint);
                bAddLeft=!bAddLeft;
            }

            // use the tallest end note, and add vertical indentation (padding)
            bool bRecalcLayout=(obEndNote.GetTblBase()->GetPrtViewInfoSize()==0);
            int iHgt;
            if (m_bForceRemeasure) {
                bRecalcLayout=true;
            }
            if (bRecalcLayout) {
                iHgt=__max(CalcDrawHeight(obEndNote, dc), CalcDrawHeight(obEndNote2, dc));
            }
            else {
                // use previous layout
                iHgt=obEndNote.GetTblBase()->GetPrtViewInfo(0).GetCurrSize().cy;
            }

            rcEndNote.top = rcEndNote.bottom - iHgt - (CELL_PADDING_TOP+CELL_PADDING_BOTTOM);
            obEndNote.SetClientRectLP(rcEndNote);
            obEndNote2.SetClientRectLP(rcEndNote);

            // set formats for left/right pages ...
            if (obEndNote.GetFmt()->GetHorzAlign()==HALIGN_CENTER) {
                // these use the scrap fmts
                obEndNote.GetFmt()->SetHorzAlign(HALIGN_RIGHT);
                obEndNote2.GetFmt()->SetHorzAlign(HALIGN_LEFT);
            }

            // set format flags
            obEndNote.SetDrawFormatFlags(DT_WORDBREAK | GetAlignmentFlags(obEndNote.GetFmt()));
            obEndNote2.SetDrawFormatFlags(DT_WORDBREAK | GetAlignmentFlags(obEndNote2.GetFmt()));

            // add end notes to array of page objects
            pl.AddPgOb(obEndNote);
            pl.AddPgOb(obEndNote2);
        }
        else {
            // calc drawing rect, if necessary
            bool bRecalcLayout=(obEndNote.GetTblBase()->GetPrtViewInfoSize()==0);
            int iHgt;
            if (m_bForceRemeasure) {
                bRecalcLayout=true;
            }
            if (bRecalcLayout) {
                iHgt=CalcDrawHeight(obEndNote, dc);
            }
            else {
                // use previous layout
                iHgt=obEndNote.GetTblBase()->GetPrtViewInfo(0).GetCurrSize().cy;
            }

            // add vertical indentation (padding)
            rcEndNote.top = rcEndNote.bottom - iHgt - (CELL_PADDING_TOP + CELL_PADDING_BOTTOM);
            obEndNote.SetClientRectLP(rcEndNote);

            // set format flags
            obEndNote.SetDrawFormatFlags(DT_WORDBREAK | GetAlignmentFlags(obEndNote.GetFmt()));

            // add end note to array of page objects
            pl.AddPgOb(obEndNote);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        BuildRowColTree
//
// Builds a tree of CRowColPgOb structures, for either stubs or columns
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::BuildRowColTree(CTabVar* pTabVar, CRowColPgOb* pRCParent, const CFmtReg& fmtReg, bool bColumn, int iLevel /*=0*/)
{
    //Recursive routine for adding child cols
    pRCParent->SetLevel(iLevel);
    if(!pTabVar->IsRoot()) {
        CRowColPgOb* pRCChild = new CRowColPgOb;
        pRCChild->SetTblBase(pTabVar);
        pRCChild->SetText(pTabVar->GetText());
        pRCChild->SetLevel(iLevel+1);
        pRCChild->SetType(bColumn?PGOB_SPANNER:PGOB_CAPTION);
        pRCChild->SetTbl(pRCParent->GetTbl());

        CFmt* pFmt=new CFmt;
        if (pTabVar->GetDerFmt()!=NULL) {
            // we have customized formats
            pFmt->Assign(*pTabVar->GetDerFmt());
            pFmt->CopyNonDefaultValues(DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(bColumn?FMT_ID_SPANNER:FMT_ID_CAPTION,0)));
            if(bColumn && pTabVar->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) ==0){
                pFmt->SetHidden(HIDDEN_YES);
            }
        }
        else {
            // we use default formats
            *pFmt=*DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(bColumn?FMT_ID_SPANNER:FMT_ID_CAPTION,0));
            if(bColumn && pTabVar->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) ==0){
                pFmt->SetHidden(HIDDEN_YES);
            }
        }
		//if the current tabVar is part of the a join then set the fmt of the tabvar as hiddden
		CTabVar* pParentTabVar = pTabVar->GetParent();
		int iJoinPos = 0;
		int iNumJoinSpanners =0;
		bool bHideJoinSpanner = false;
		for(int iChildVar =0 ; iChildVar < pParentTabVar->GetNumChildren(); iChildVar++){
			CTabVar* pCurrTabVar = pParentTabVar->GetChild(iChildVar);
			if(pCurrTabVar->GetFmt()){
				iJoinPos = 0;
				iNumJoinSpanners = ((CDataCellFmt*)pCurrTabVar->GetFmt())->GetNumJoinSpanners();
				//the current var is the start of a join. We can skip this
				continue;
			}
			if(iNumJoinSpanners > 0){
				iJoinPos++;
			}
			if(pTabVar == pCurrTabVar){

				//if we are part of a join spanner enable hide
				(iJoinPos > 0 && iJoinPos <= iNumJoinSpanners) && iNumJoinSpanners > 0  ? bHideJoinSpanner = true : bHideJoinSpanner = false;
				//we are done !
				break;
			}

		}
		if(bHideJoinSpanner){
			pFmt->SetHidden(HIDDEN_YES);
		}

        ASSERT_VALID(pFmt);
        if (pFmt->IsTextCustom()) {
            pRCChild->SetText(pFmt->GetCustom().m_sCustomText);
            if(pRCChild->GetText().IsEmpty()&& bColumn){
                pRCChild->SetText(_T(" "));//insert a blank space to fix josh's datacell not showing bug when customtext is empty
            }
        }

        LPToPoints(pFmt, GetLogPixelsY());
        PointsToTwips(pFmt);
        pRCChild->SetFmt(pFmt);
        pRCChild->SetParent(pRCParent);
        pRCParent->AddChild(pRCChild);

        //for each tab value
        int iNumValues = pTabVar->GetNumValues();
        for(int iIndex=0 ; iIndex<iNumValues ; iIndex++) {
            CTabValue* pTabVal = pTabVar->GetValue(iIndex);

            CRowColPgOb* pRCChildVal = new CRowColPgOb;
            pRCChildVal->SetTbl(pRCParent->GetTbl());
            pRCChildVal->SetTblBase(pTabVal);
            pRCChildVal->SetText(pTabVal->GetText());
            pRCChildVal->SetLevel(iLevel+2);

            CFmt* pFmt=NULL; // really a CDataCellFmt if we're a colhead/stub, or a CFmt if we're a spanner/caption
            if (pTabVar->GetNumChildren()>0) {
                // tabvar is crossed with another variable, therefore these values are captions/spanners
                if (bColumn) {
                    pRCChildVal->SetType(PGOB_SPANNER);
                }
                else {
                    if (pTabVal->GetTabValType()==RDRBRK_TABVAL) {
                        pRCChildVal->SetType(PGOB_READER_BREAK);
                    }
                    else {
                        pRCChildVal->SetType(PGOB_CAPTION);
                    }
                }

                // apply formats
                pFmt=new CFmt;
                if (pTabVal->GetDerFmt()!=NULL) {
                    // we have customized formats
//                    *pFmt=*pTabVal->GetDerFmt();
                    pFmt->Assign(*pTabVal->GetDerFmt());
                    pFmt->CopyNonDefaultValues(DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(bColumn?FMT_ID_SPANNER:FMT_ID_CAPTION,0)));
                }
                else {
                    // we use default formats
                    *pFmt=*DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(bColumn?FMT_ID_SPANNER:FMT_ID_CAPTION,0));
                }
            }
            else {
                // no cross variables, so these values are stubs/colheads
                if (bColumn) {
                    pRCChildVal->SetType(PGOB_COLHEAD);
                }
                else {
                    if (pTabVal->GetTabValType()==RDRBRK_TABVAL) {
                        pRCChildVal->SetType(PGOB_READER_BREAK);
                    }
                    else {
                        pRCChildVal->SetType(PGOB_STUB);
                    }
                }

                // apply formats
                pFmt=new CDataCellFmt;
                if (pTabVal->GetDerFmt()!=NULL) {
                    // we have customized formats
//                    *pFmt=*pTabVal->GetDerFmt();
                    pFmt->Assign(*pTabVal->GetDerFmt());
                    pFmt->CopyNonDefaultValues(DYNAMIC_DOWNCAST(CDataCellFmt,fmtReg.GetFmt(bColumn?FMT_ID_COLHEAD:FMT_ID_STUB,0)));
                }
                else {
                    // we use default formats
                    *DYNAMIC_DOWNCAST(CDataCellFmt,pFmt)=*DYNAMIC_DOWNCAST(CDataCellFmt,fmtReg.GetFmt(bColumn?FMT_ID_COLHEAD:FMT_ID_STUB,0));
                }
            }

            ASSERT_VALID(pFmt);
            if (pFmt->IsTextCustom()) {
                pRCChildVal->SetText(pFmt->GetCustom().m_sCustomText);
                if(pRCChildVal->GetText().IsEmpty()&& bColumn){
                    pRCChildVal->SetText(_T(" "));//insert a blank space to fix josh's datacell not showing bug when customtext is empty
                }
            }

            LPToPoints(pFmt, GetLogPixelsY());
            PointsToTwips(DYNAMIC_DOWNCAST(CFmt,pFmt));
            pRCChildVal->SetFmt(DYNAMIC_DOWNCAST(CFmt,pFmt));
            pRCChildVal->SetParent(pRCChild);
            pRCChild->AddChild(pRCChildVal);

            //Now for each TabVar do the same (don't do this for reader break vals though)
            if (pRCChildVal->GetType()!=PGOB_READER_BREAK) {
                for (int iCVar=0; iCVar<pTabVar->GetNumChildren() ; iCVar++) {
                    CTabVar* pChildTabVar = pTabVar->GetChild(iCVar);
                    BuildRowColTree(pChildTabVar, pRCChildVal, fmtReg, bColumn, iLevel+2);
                }
            }
        }
    }
    else {
        //Now for each TabVar do the same
        if(!bColumn && pTabVar->IsRoot() ){
            CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
			//CTabSet* pSet = pDoc->GetTableSpec();
			//Savy (R) sampling app 20081224
			CTabSet* pSet = NULL;

			if(!pDoc){
				ASSERT(m_pTabSet);
				pSet = m_pTabSet;
			}
			else{
				pSet = pDoc->GetTableSpec();
			}
            CTable* pTbl = pSet->GetTable(pRCParent->GetTbl());
            ASSERT(pTbl);
            bool bHasAreaCaption = false;
            bHasAreaCaption = pSet->GetConsolidate()->GetNumAreas() > 0?bHasAreaCaption = true:bHasAreaCaption=false;
            if(bHasAreaCaption){
                //Add area caption here
                CRowColPgOb* pRCChild = new CRowColPgOb;
                pRCChild->SetTbl(pRCParent->GetTbl());
                pTbl->GetAreaCaption()->SetText(AREA_TOKEN);
                pRCChild->SetTblBase(pTbl->GetAreaCaption());
                pRCChild->SetText(pTbl->GetAreaCaption()->GetText());
                pRCChild->SetLevel(iLevel+1);
                pRCChild->SetType(PGOB_CAPTION);//see if you need to create special enum for area

                CFmt* pFmt=new CFmt;
                if (pTabVar->GetDerFmt()!=NULL) {
                    // we have customized formats
                    pFmt->Assign(*pTabVar->GetDerFmt());
                    pFmt->CopyNonDefaultValues(DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_AREA_CAPTION,0)));
                }
                else {
                    // we use default formats
                    if(pTbl->GetAreaCaption()->GetDerFmt()){
                        pFmt->Assign(*pTbl->GetAreaCaption()->GetDerFmt());
                        pFmt->CopyNonDefaultValues(DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_AREA_CAPTION,0)));
                    }
                    else {
                        *pFmt=*DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_AREA_CAPTION,0));
                    }
                }
                ASSERT_VALID(pFmt);
                if (pFmt->IsTextCustom()) {
                    pRCChild->SetText(pFmt->GetCustom().m_sCustomText);
                    if(pRCChild->GetText().IsEmpty()&& bColumn){
                        pRCChild->SetText(_T(" "));//insert a blank space to fix josh's datacell not showing bug when customtext is empty
                    }
                }
                if(ForceHideAreaCaptionInOneRowTable(pTbl)){
                    pFmt->SetHidden(HIDDEN_YES);
                }
                LPToPoints(pFmt, GetLogPixelsY());
                PointsToTwips(pFmt);
                pRCChild->SetFmt(pFmt);
                pRCChild->SetParent(pRCParent);
                pRCParent->AddChild(pRCChild);
            }
        }
        int iNumCrossVars = pTabVar->GetNumChildren();
        for (int iCVar=0 ; iCVar < iNumCrossVars ; iCVar++) {
            CTabVar* pChildTabVar = pTabVar->GetChild(iCVar);
            BuildRowColTree(pChildTabVar, pRCParent, fmtReg, bColumn, iLevel+1);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        ProcessAreaTokensinRows
//
// Puts area name text into stubs/captions where the area name escape
// code is present.  Similar to CTblGrid::ProcessAreaTokensinRows().
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::ProcessAreaTokensinRows(CTable* pTbl, CArray<CRowColPgOb*, CRowColPgOb*>& aStub)
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    CArray<CTabData*, CTabData*>& aTabData=pTbl->GetTabDataArray();
    CMapStringToString& areaLabelLookup = pDoc->GetAreaLabelLookup();

    int iNumSlices=aTabData.GetSize();
	if(iNumSlices == 0){
		return;
	}
    ASSERT(iNumSlices>0);
    int iStubsPerSlice=aStub.GetSize() / iNumSlices;

    // sanity checks ... each slice must have the same number of data cells ...
    for (int iDbg=0 ; iDbg<pTbl->GetTabDataArray().GetSize() ; iDbg++) {
        ASSERT(aTabData[iDbg]->GetCellArray().GetSize()==aTabData[0]->GetCellArray().GetSize());
    }
//    ASSERT(aStub.GetSize()==iNumSlices*iStubsPerSlice);

    for (int iIndex=0, iSlice=0 ; iSlice<iNumSlices ; iSlice++){
        CTabData* pTabData = aTabData[iSlice];
        CIMSAString sBreakKey = pTabData->GetBreakKey();
        sBreakKey.Remove(';');
        sBreakKey.Replace(_T("-"),_T(" "));
        sBreakKey.MakeUpper();
        CIMSAString sAreaLabel;

        if (((CTableChildWnd*)GetParent())->IsViewer()) {
            // in viewer we don't have areaLabelLookup so use
            // area label in CTabData if there is one
            if (!pTabData->GetAreaLabel().IsEmpty()) {
                sBreakKey = pTabData->GetAreaLabel();
            }
        }
        else if (areaLabelLookup.Lookup(sBreakKey,sAreaLabel)){
            sBreakKey = sAreaLabel;
        }

        CArray<double, double&>& aCells = pTabData->GetCellArray();
        int iNumCells = aCells.GetSize();
        int iTempStubsPerSlice = iStubsPerSlice;
        bool bProcessedSlice = false;
        for (int iStub=0 ; iStub<iTempStubsPerSlice ; iStub++) {
            //ASSERT(iIndex<aStub.GetSize());
            if(iIndex>=aStub.GetSize()){
                break;
            }
            CRowColPgOb* pStub=aStub[iIndex++];

            CIMSAString sStubOrCaption=pStub->GetText();
            CIMSAString sTemp(sStubOrCaption);
            CIMSAString sWord;
            while (sTemp.GetLength() > 0) {
                sWord=sTemp.GetToken();
                if (sWord.CompareNoCase(AREA_TOKEN)==0) {
                    sStubOrCaption.Replace(sWord,sBreakKey);
                    bProcessedSlice = true;
                }
            }
            pStub->SetText(sStubOrCaption);
            if(!bProcessedSlice){//assumption is atleast one token per slice is present
                //and it hasnt been processed yet . So we continue to go down through the stubs
                iTempStubsPerSlice++;
                if(iTempStubsPerSlice < aStub.GetSize()){
                    continue;
                }
                else {
                    break;
                }
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        SetDrawFormatFlags
//
// Calls CPgOb::SetDrawFormatFlags() recursively
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::SetDrawFormatFlags(CRowColPgOb* pOb)
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
   // CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    const CFmtReg& fmtReg = pSet->GetFmtReg();

    if (pOb->GetType()!=PGOB_ROOT) {
        CFmt* pFmt = pOb->GetFmt();
        ASSERT_VALID(pFmt);
        pOb->SetDrawFormatFlags(DT_WORDBREAK | GetAlignmentFlags(pFmt));
    }
    for (int iChild=0 ; iChild<pOb->GetNumChildren() ; iChild++) {
        SetDrawFormatFlags(pOb->GetChild(iChild));
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        BuildRowsAndCols
//
// Lays out rows and columns, following a hueristic
//
// plTemplate  -- template page of this table, has headers/footers/title already
//                positioned (read only!)
// pgMgr       -- page manager (array of CPgLayout objects) into which we add pages
//                for this table.  pgMgr should be empty to start with.  The added
//                tables will contain objects from plTemplate, along with rows and
//                columns as needed.
// iTbl        -- tbl number we're laying out
// dc          -- table's dc
//
// If a table requires more than 1 page, we can add additional pages to
// pgMgr.  When doing this, though, be sure to copy the headers/footers/title
// from plTemplate onto any new page.
//
// Returns true if the build was successful, false if the current measurements
// make it impossible to complete the build (for example, if stubs are very very
// wide and the user switches to "both standard" tbl layout).
//
/////////////////////////////////////////////////////////////////////////////
bool CTabPrtView::BuildRowsAndCols(const CPgLayout& plTemplate, CPgMgr& pgMgr, int iTbl, CDC& dc)
{
    ASSERT(pgMgr.GetNumPages()==0);      // pgMgr must be empty to start with

    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    const CFmtReg& fmtReg = pSet->GetFmtReg();
    CTable* pTbl = pSet->GetTable(iTbl);
    CTblOb* pTitleOb = DYNAMIC_DOWNCAST(CTblOb,pTbl->GetTitle());
    int iNumVPages, iNumHPages;   // total number of vert/horz pages needed to render this table
    int iVPage, iHPage;           // looping variables for vert/horz pages
    {
        CTabVar* pRowRootTabVar = pTbl->GetRowRoot();
        bool bIsOneRowVar = pRowRootTabVar->GetNumChildren() == 1 && pRowRootTabVar->GetChild(0)->GetNumChildren() ==0;

        if(bIsOneRowVar){
            bool bHasAreaCaption = false;
            bHasAreaCaption = pSet->GetConsolidate()->GetNumAreas() > 0?bHasAreaCaption = true:bHasAreaCaption=false;

            if(bHasAreaCaption){
                CTabVar* pTabVar = pTbl->GetRowRoot()->GetChild(0);
                if(pTabVar && pTabVar->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) == 0){
                    pTabVar->GetValue(0)->SetText(AREA_TOKEN);
                }
            }
            else {
                CTabVar* pTabVar = pTbl->GetRowRoot()->GetChild(0);
                ASSERT(pTabVar);
                if(pTabVar && pTabVar->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) == 0){
                    pTabVar->GetValue(0)->SetText(TOTAL_LABEL);
                }
            }
        }
    }

    // table print format ...
    CTblPrintFmt fmtTblPrint;
    if (NULL!=pTbl->GetTblPrintFmt()) {
        fmtTblPrint=*pTbl->GetTblPrintFmt();
        fmtTblPrint.CopyNonDefaultValues(DYNAMIC_DOWNCAST(CTblPrintFmt,fmtReg.GetFmt(FMT_ID_TBLPRINT,0)));
    }
    else {
        fmtTblPrint=*DYNAMIC_DOWNCAST(CTblPrintFmt,fmtReg.GetFmt(FMT_ID_TBLPRINT,0));
    }

    // table format ...
    CTblFmt fmtTbl;
    if (NULL!=pTbl->GetDerFmt()) {
        fmtTbl=*pTbl->GetDerFmt();
        fmtTbl.CopyNonDefaultValues(DYNAMIC_DOWNCAST(CTblFmt,fmtReg.GetFmt(FMT_ID_TABLE,0)));
    }
    else {
        fmtTbl=*DYNAMIC_DOWNCAST(CTblFmt,fmtReg.GetFmt(FMT_ID_TABLE,0));
    }

    // tab set format
    const CTabSetFmt* pTabSetFmt=DYNAMIC_DOWNCAST(CTabSetFmt,fmtReg.GetFmt(FMT_ID_TABSET,0));

    //////////////////////////////////////////////////////////////////////////////////
    // stubhead objects
    CRowColPgOb pgobStubhead, pgobSecondaryStubhead;

    // create primary stubhead pgob and format
    CFmt* pStubheadFmt=new CFmt;
    if (NULL!=pTbl->GetStubhead(0)->GetDerFmt()) {
        *pStubheadFmt=*pTbl->GetStubhead(0)->GetDerFmt();
        pStubheadFmt->CopyNonDefaultValues(DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_STUBHEAD,0)));
    }
    else {
        *pStubheadFmt=*DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_STUBHEAD,0));
    }
    LPToPoints(pStubheadFmt, GetLogPixelsY());
    PointsToTwips(pStubheadFmt);

    pgobStubhead.SetType(PGOB_STUBHEAD);
    pgobStubhead.SetTblBase(pTbl->GetStubhead(0));
    pgobStubhead.SetFmt(pStubheadFmt);
    pgobStubhead.SetDrawFormatFlags(DT_WORDBREAK | GetAlignmentFlags(pStubheadFmt));
    if (pStubheadFmt->IsTextCustom()) {
        pgobStubhead.SetText(pStubheadFmt->GetCustom().m_sCustomText);
    }
    else {
        pgobStubhead.SetText(pTbl->GetStubhead(0)->GetText());
    }

    // create secondary stubhead pgob and format
    CFmt* pSecStubheadFmt=new CFmt;
    if (NULL!=pTbl->GetStubhead(1)->GetDerFmt()) {
        *pSecStubheadFmt=*pTbl->GetStubhead(1)->GetDerFmt();
        pSecStubheadFmt->CopyNonDefaultValues(DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_STUBHEAD_SEC,0)));
    }
    else {
        *pSecStubheadFmt=*DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_STUBHEAD_SEC,0));
    }
    LPToPoints(pSecStubheadFmt, GetLogPixelsY());
    PointsToTwips(pSecStubheadFmt);

    if (pSecStubheadFmt->IsTextCustom()) {
        pgobStubhead.SetText(pSecStubheadFmt->GetCustom().m_sCustomText);
    }
    else {
        if (pTbl->GetStubhead(1)->GetText().IsEmpty()) {
            // no secondary stubhead text, so use the primary stubhead text instead
            pgobSecondaryStubhead.SetText(pTbl->GetStubhead(0)->GetText());
        }
        else {
            pgobSecondaryStubhead.SetText(pTbl->GetStubhead(1)->GetText());
        }
    }

    pgobSecondaryStubhead.SetType(PGOB_STUBHEAD);
    pgobSecondaryStubhead.SetTblBase(pTbl->GetStubhead(1));
    pgobSecondaryStubhead.SetFmt(pSecStubheadFmt);
    pgobSecondaryStubhead.SetDrawFormatFlags(DT_WORDBREAK | GetAlignmentFlags(pSecStubheadFmt));

    pgobStubhead.CalcMinSizes(dc, m_bForceRemeasure);
    pgobSecondaryStubhead.CalcMinSizes(dc, m_bForceRemeasure);

    //////////////////////////////////////////////////////////////////////////////////
    // get client rect, based on prepared screen origin and extent
    CRect rcClient;
    rcClient.TopLeft() = dc.GetWindowOrg();
    rcClient.BottomRight() = dc.GetWindowExt();
    rcClient.DeflateRect(fmtTblPrint.GetPageMargin());

    /////////////////////////////////////////////////////////////////////////////////////
    // construct row and column trees, incorporating nesting and concatenation as needed
    CRowColPgOb rcpgobColRoot, rcpgobRowRoot;
    rcpgobRowRoot.SetTbl(iTbl);

    // columns ...
    rcpgobColRoot.SetType(PGOB_ROOT);
    BuildRowColTree(pTbl->GetColRoot(), &rcpgobColRoot, fmtReg, true);
    FixLineFmt(&rcpgobColRoot,fmtReg, true);

    // rows need to account for area processing ... (a "slice" is an area instance)
    bool bAreaProcessing=(pTbl->GetTabDataArray().GetSize()>1);

    // build the first slice (or only slice if no data is present)
    rcpgobRowRoot.SetType(PGOB_ROOT);
    BuildRowColTree(pTbl->GetRowRoot(), &rcpgobRowRoot, fmtReg, false);
    FixLineFmt(&rcpgobRowRoot,fmtReg, false);

    // handle slices #2 onwards...
    for (int iSlice=1 ; iSlice<pTbl->GetTabDataArray().GetSize() ; iSlice++) {
        CRowColPgOb rcpgobRowSlice;  // a tree structure for this slice
        rcpgobRowSlice.SetTbl(iTbl);
        // build the slice
        BuildRowColTree(pTbl->GetRowRoot(), &rcpgobRowSlice, fmtReg, false);
        rcpgobRowSlice.SetType(PGOB_ROOT);
        bool bRemoveTabVar =false;//when only one tabvar in rows . then remove the caption (special case)
        if (pTbl->GetRowRoot() && pTbl->GetRowRoot()->GetNumChildren()==1 && pTbl->GetRowRoot()->GetChild(0)->GetNumChildren()==0 && pTbl->GetColRoot()->GetNumChildren()!=0)  {
            bRemoveTabVar=true;
        }
        // in this case the caption is shown in the stubhead and NOT in rows area ...
        if (iSlice>0) {
            // splice the slice onto the main row tree (combine below the root node)

            for (int iSliceChild=0 ; iSliceChild<rcpgobRowSlice.GetNumChildren() ; iSliceChild++) {
                CRowColPgOb* pChild = new CRowColPgOb(*rcpgobRowSlice.GetChild(iSliceChild));
                rcpgobRowRoot.AddChild(pChild);
                if(bRemoveTabVar && iSliceChild == 1 ){
                    //0 is AREA CAPTION #1 is for tabVar
                    pChild->SetType(PGOB_NOTINCLUDED);
                    continue; //dont add the tabvar
                }
            }
        }
    }

//Dbg_Dump_Tree(&rcpgobRowRoot);

    /////////////////////////////////////////////////////////////////////////////////////
    // build array of column heads and stubs (cols and rows that have data), so we can easily loop through them
    int iColHead, iStub;
    CArray<CRowColPgOb*, CRowColPgOb*> aColHead;
    CArray<CRowColPgOb*, CRowColPgOb*> aStub;
    rcpgobColRoot.GetLeaves(aColHead);         // note: aColHead does *not* include spanners, since we have to deal with those recursively
    rcpgobRowRoot.GetLeavesAndNodes(aStub);    // note: aStub does include captions, since those line up without needing recursion
    // also note that neither aColHead nor aStub contain hidden colheads/stubs


    // embed area names into stubs if we're doing area processing ...
	bool bHasAreaCaption = pSet->GetConsolidate()->GetNumAreas() > 0?bHasAreaCaption = true:bHasAreaCaption=false;
   // if (bAreaProcessing) {// savy Feb 07 changed this as the number of slices in the tabdata is only one when the con is Total
	if(bHasAreaCaption){
        ProcessAreaTokensinRows(pTbl, aStub);
    }

//Dbg_Dump_Tree(&rcpgobRowRoot);

    /////////////////////////////////////////////////////////////////////////////////////
    // abort build process if there are no rows or columns ...
    if (rcpgobRowRoot.GetNumChildren()==0 && rcpgobColRoot.GetNumChildren()==0)  {
        // add the page layout template to the page mgr (it has headers, footers, titles, etc.)
        CPgLayout pl(plTemplate);   // copy objects already on the first page into this new page

        bool bGotTitle=false;         // include the 1st title object only
        bool bGotPageNote=false;      // include the 1st page note object only
        bool bGotEndNote=false;       // include the 1st end note object only
        for (int iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
            CPgOb& pgob=pl.GetPgOb(iPgOb);

            if (pgob.GetType()==PGOB_TITLE) {
                if (bGotTitle) {
                    pl.RemovePgObAt(iPgOb--);
                }
                bGotTitle=true;
            }
            if (pgob.GetType()==PGOB_PAGENOTE) {
                if (bGotPageNote) {
                    pl.RemovePgObAt(iPgOb--);
                }
                bGotPageNote=true;
            }
            if (pgob.GetType()==PGOB_ENDNOTE) {
                if (bGotEndNote) {
                    pl.RemovePgObAt(iPgOb--);
                }
                bGotEndNote=true;
            }
        }
        pgMgr.AddPgLayout(pl);
        return true;
    }

    // we don't show the first caption if have A*B in the rows
    if (pTbl->GetRowRoot() && pTbl->GetRowRoot()->GetNumChildren()==1 && pTbl->GetRowRoot()->GetChild(0)->GetNumChildren()==0 && pTbl->GetColRoot()->GetNumChildren()!=0)  {
        // in this case the caption is shown in the stubhead and NOT in rows area ...
        bool bHasAreaCaption = false;
        bHasAreaCaption = pSet->GetConsolidate()->GetNumAreas() > 0?bHasAreaCaption = true:bHasAreaCaption=false;
        if(bHasAreaCaption){
            rcpgobRowRoot.GetChild(1)->SetType(PGOB_NOTINCLUDED);    // don't include this object in the layout
            //ASSERT(aStub[1]->GetType()==PGOB_NOTINCLUDED);
            aStub.RemoveAt(1);
        }
        else {
            rcpgobRowRoot.GetChild(0)->SetType(PGOB_NOTINCLUDED);    // don't include this object in the layout
            //ASSERT(aStub[0]->GetType()==PGOB_NOTINCLUDED);
            aStub.RemoveAt(0);
        }
    }

Dbg_Dump_Tree(&rcpgobRowRoot);

    // assign formats
    SetDrawFormatFlags(&rcpgobRowRoot);
    SetDrawFormatFlags(&rcpgobColRoot);

    /////////////////////////////////////////////////////////////////////////////////////
    // align column heads so they are all on the same level
    int iColDepth=rcpgobColRoot.GetDepth();
    for (iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
        CRowColPgOb* pColHead = aColHead[iColHead];
        ASSERT(pColHead->GetLevel()<=iColDepth);
        if (pColHead->GetLevel()!=iColDepth) {
            pColHead->SetLevel(iColDepth);
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // calc min sizes of column heads and stubs

    // replace column head measurements with persisted information, if it is available
    for (iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
        CRowColPgOb* pColHead = aColHead[iColHead];

        if (pColHead->GetTblBase()->GetPrtViewInfoSize()==0) {
            // no persisted info available ...
            continue;
        }

        // if we're remeasuring, then nuke any stored sizing info ...
        if (m_bForceRemeasure) {
            pColHead->GetTblBase()->RemoveAllPrtViewInfo();
            continue;
        }

        // this col head has persisted information; figure out which occurrence (panel) within the prtviewinfo we are...
        int iOcc=0;
        for (int iColHead2=0 ; iColHead2<iColHead ; iColHead2++) {
            if (aColHead[iColHead2]->GetTblBase()==pColHead->GetTblBase()) {
                iOcc++;
            }
        }

        const CPrtViewInfo& pvi=pColHead->GetTblBase()->GetPrtViewInfo(iOcc);
        CFmt* pFmt=pColHead->GetFmt();
        CRect rcCurr(CPoint(0,0), pvi.GetCurrSize());
        CSize szExtra(pvi.GetExtraSize());
        CSize szMin(pvi.GetMinSize());
        CSize szMax(pvi.GetMaxSize());
        rcCurr.right += pFmt->GetIndent(LEFT) + pFmt->GetIndent(RIGHT)+CELL_PADDING_LEFT+CELL_PADDING_RIGHT;
        rcCurr.bottom += CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
        szMin.cx += pFmt->GetIndent(LEFT) + pFmt->GetIndent(RIGHT) + CELL_PADDING_LEFT + CELL_PADDING_RIGHT;
        szMin.cy += CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
        szMax.cx += pFmt->GetIndent(LEFT) + pFmt->GetIndent(RIGHT) + CELL_PADDING_LEFT + CELL_PADDING_RIGHT;
        szMax.cy += CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
        pColHead->SetMinResize(szMin);
        pColHead->SetMaxResize(szMax);
        pColHead->SetExtraLP(szExtra);
        pColHead->SetClientRectLP(rcCurr);
        pColHead->SetCustom(pvi.IsCustom());
        pColHead->SetPageBreak(pvi.IsPageBreak());
    }

    // replace stub measurements with persisted information, if it is available
    for (iStub=0 ; iStub<aStub.GetSize() ; iStub++) {
        CRowColPgOb* pStub = aStub[iStub];

        if (pStub->GetTblBase()->GetPrtViewInfoSize()==0) {
            // no persisted info available ...
            continue;
        }

        // if we're remeasuring, then nuke any stored sizing info ...
        if (m_bForceRemeasure) {
            pStub->GetTblBase()->RemoveAllPrtViewInfo();
            continue;
        }

        // this stub has persisted information; figure out which occurrence (panel) within the prtviewinfo we are...
        int iOcc=0;
        for (int iStub2=0 ; iStub2<iStub ; iStub2++) {
            if (aStub[iStub2]->GetTblBase()==pStub->GetTblBase()) {
                iOcc++;
            }
        }

        if (pStub->GetTblBase()->GetPrtViewInfoSize()<iOcc+1) {
            // oops, we don't any stored prtview info for this stub ... must be since we're doing area processing
            ASSERT(bAreaProcessing);
        }
        else {
            const CPrtViewInfo& pvi=pStub->GetTblBase()->GetPrtViewInfo(iOcc);
            CFmt* pFmt=pStub->GetFmt();
            CRect rcCurr(CPoint(0,0), pvi.GetCurrSize());
            CSize szExtra(pvi.GetExtraSize());
            CSize szMin(pvi.GetMinSize());
            CSize szMax(pvi.GetMaxSize());
            rcCurr.right += pFmt->GetIndent(LEFT) + pFmt->GetIndent(RIGHT) + CELL_PADDING_LEFT+CELL_PADDING_RIGHT;
            rcCurr.bottom += CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
            szMin.cx += pFmt->GetIndent(LEFT) + pFmt->GetIndent(RIGHT) + CELL_PADDING_LEFT + CELL_PADDING_RIGHT;
            szMin.cy += CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
            szMax.cx += pFmt->GetIndent(LEFT) + pFmt->GetIndent(RIGHT) + CELL_PADDING_LEFT + CELL_PADDING_RIGHT;
            szMax.cy += CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
            pStub->SetMinResize(szMin);
            pStub->SetMaxResize(szMax);
            pStub->SetExtraLP(szExtra);
            pStub->SetClientRectLP(rcCurr);
            pStub->SetCustom(pvi.IsCustom());
            pStub->SetPageBreak(pvi.IsPageBreak());
        }
    }

    rcpgobColRoot.CalcMinSizes(dc, m_bForceRemeasure);
    rcpgobRowRoot.CalcMinSizes(dc, m_bForceRemeasure);

    if (m_bForceRemeasure) {
        // just reset the stubheads
        pgobStubhead.GetTblBase()->RemoveAllPrtViewInfo();
        pgobSecondaryStubhead.GetTblBase()->RemoveAllPrtViewInfo();
    }

    // see if stubhead has persisted size information ...
    if (pgobStubhead.GetTblBase()->GetPrtViewInfoSize()>0) {
        const CPrtViewInfo& pvi=pgobStubhead.GetTblBase()->GetPrtViewInfo(0);
        CFmt* pFmt=pgobStubhead.GetFmt();
        CRect rcCurr(CPoint(0,0), pvi.GetCurrSize());
        CSize szExtra(pvi.GetExtraSize());
        CSize szMin(pvi.GetMinSize());
        CSize szMax(pvi.GetMaxSize());
        rcCurr.right += pFmt->GetIndent(LEFT) + pFmt->GetIndent(RIGHT) + CELL_PADDING_LEFT+CELL_PADDING_RIGHT;
        rcCurr.bottom += CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
        szMin.cx += pFmt->GetIndent(LEFT) + pFmt->GetIndent(RIGHT) + CELL_PADDING_LEFT + CELL_PADDING_RIGHT;
        szMin.cy += CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
        szMax.cx += pFmt->GetIndent(LEFT) + pFmt->GetIndent(RIGHT) + CELL_PADDING_LEFT + CELL_PADDING_RIGHT;
        szMax.cy += CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
        pgobStubhead.SetMinResize(szMin);
        pgobStubhead.SetMaxResize(szMax);
        pgobStubhead.SetExtraLP(szExtra);
        pgobStubhead.SetClientRectLP(rcCurr);
        pgobStubhead.SetCustom(pvi.IsCustom());
        pgobStubhead.SetPageBreak(pvi.IsPageBreak());
    }

    // see if secondary stubhead has persisted size information ...
    if (pgobSecondaryStubhead.GetTblBase()->GetPrtViewInfoSize()>0) {
        const CPrtViewInfo& pvi=pgobSecondaryStubhead.GetTblBase()->GetPrtViewInfo(0);
        CFmt* pFmt=pgobSecondaryStubhead.GetFmt();
        CRect rcCurr(CPoint(0,0), pvi.GetCurrSize());
        CSize szExtra(pvi.GetExtraSize());
        CSize szMin(pvi.GetMinSize());
        CSize szMax(pvi.GetMaxSize());
        rcCurr.right += pFmt->GetIndent(LEFT) + pFmt->GetIndent(RIGHT) + CELL_PADDING_LEFT+CELL_PADDING_RIGHT;
        rcCurr.bottom += CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
        szMin.cx += pFmt->GetIndent(LEFT) + pFmt->GetIndent(RIGHT) + CELL_PADDING_LEFT + CELL_PADDING_RIGHT;
        szMin.cy += CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
        szMax.cx += pFmt->GetIndent(LEFT) + pFmt->GetIndent(RIGHT) + CELL_PADDING_LEFT + CELL_PADDING_RIGHT;
        szMax.cy += CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
        pgobSecondaryStubhead.SetMinResize(szMin);
        pgobSecondaryStubhead.SetMaxResize(szMax);
        pgobSecondaryStubhead.SetExtraLP(szExtra);
        pgobSecondaryStubhead.SetClientRectLP(rcCurr);
        pgobSecondaryStubhead.SetCustom(pvi.IsCustom());
        pgobSecondaryStubhead.SetPageBreak(pvi.IsPageBreak());
    }

    // stubhead min width is different ... it is set to the widest of all the stubs' min
    // widths, if any of these is wider than the stubhead's default min width
    for (iStub=0 ; iStub<aStub.GetSize() ; iStub++) {
        CPgOb* pStub=aStub[iStub];
        if (pStub->GetType()==PGOB_STUB) {
            if (pStub->GetMinResize().cx>pgobStubhead.GetMinResize().cx) {
                pgobStubhead.GetMinResize().cx=pStub->GetMinResize().cx;
            }
            if (pStub->GetMinResize().cx>pgobSecondaryStubhead.GetMinResize().cx) {
                pgobSecondaryStubhead.GetMinResize().cx=pStub->GetMinResize().cx;
            }
        }
    }

    // store secondary stubhead width
    int iSecondaryStubheadWidth=0;
    if (pgobSecondaryStubhead.GetTblBase()->GetPrtViewInfoSize()>0) {
        // add persisted secondary stubhead with to row width
        ASSERT(pgobSecondaryStubhead.GetClientRectLP().Width()>0);
        iSecondaryStubheadWidth=pgobSecondaryStubhead.GetClientRectLP().Width();
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // constuct an array of cell pgobs, so we can measure them (we put the cells on pages later) ...
    CArray<CPgOb, CPgOb&> aCell;
    //Rebuild aStub and aColHead with hidden stuff for data fill
    aColHead.RemoveAll();
    aStub.RemoveAll();
    rcpgobColRoot.GetLeaves(aColHead,true);         // note: aColHead does *not* include spanners, since we have to deal with those recursively
    rcpgobRowRoot.GetLeavesAndNodes(aStub,true);    // note: aStub does include captions, since those line up without needing recursion

    BuildAndMeasureCells(iTbl, dc, aColHead, aStub, aCell);  // this might adjust stub heights or col widths

    //Rebuild aStub and aColHead without  hidden stuff for cell dimensions calcs
    aColHead.RemoveAll();
    aStub.RemoveAll();

    rcpgobColRoot.GetLeaves(aColHead);         // note: aColHead does *not* include spanners, since we have to deal with those recursively
    rcpgobRowRoot.GetLeavesAndNodes(aStub);    // note: aStub does include captions, since those line up without needing recursion
    //End rebuild

    /////////////////////////////////////////////////////////////////////////////////////
    // determine initial sizes for each column head:
    //    start with: 75% of user area (left standard, left facing, both facing) or
    //    50% of user area (both facing), equally divided by number of data columns...
    int iInitColWidth=0;
    int iInitRowWidth=0;

    // use persisted stub width, if possible...
    bool bRecalcColWidth=true;

    if (pgobStubhead.GetTblBase()->GetPrtViewInfoSize()>0) {
        // there is a stub with saved print view information ... our initial col width will be user area - this stub's width
        ASSERT(pgobStubhead.GetClientRectLP().Width()>0);
        iInitRowWidth=pgobStubhead.GetClientRectLP().Width();
        if (iSecondaryStubheadWidth==0) {
            iSecondaryStubheadWidth=iInitRowWidth;
        }
        iInitColWidth=GetHorzColArea((int)(plTemplate.GetPgWidthInches()*TWIPS_PER_INCH), fmtTblPrint, iInitRowWidth, iSecondaryStubheadWidth, 0);
        bRecalcColWidth=false;
    }

    // use previously calculated stub width, if possible ...
    if (bRecalcColWidth) {
        for (iStub=0 ;iStub<aStub.GetSize() ; iStub++) {
            CPgOb* pStub=aStub[iStub];
            if (pStub->GetType()==PGOB_STUB) {
                if (pStub->GetClientRectLP().Width()>0) {
                    // stubhead already has a width ... use it
                    iInitRowWidth=pStub->GetClientRectLP().Width();
                    iInitColWidth=rcClient.Width() - iInitRowWidth;
                    bRecalcColWidth=false;
                    break;
                }
            }
        }
        if (fmtTblPrint.GetTblLayout()==TBL_LAYOUT_BOTH_STANDARD) {
            if (iSecondaryStubheadWidth==0) {
                iSecondaryStubheadWidth=iInitRowWidth;
            }
            iInitColWidth -= iSecondaryStubheadWidth;
        }
    }

    if (bRecalcColWidth) {
        // no good persisted info on stubheads available ...

        if (aColHead.GetSize()==0) {
            // frequencies only, just give 50%
            iInitColWidth=rcClient.Width()/2;
            iInitRowWidth=iInitColWidth;
            iSecondaryStubheadWidth=0;
        }
        else {
            switch(fmtTblPrint.GetTblLayout()) {
            case TBL_LAYOUT_LEFT_STANDARD:        // stubs on left only, standard page layout (default)
            case TBL_LAYOUT_LEFT_FACING:          // stubs on left, facing-pages layout
            case TBL_LAYOUT_BOTH_FACING:          // stubs on both left and right, facing-pages layout
                iInitColWidth = (rcClient.Width()*3)/(aColHead.GetSize()*4);  // 75% total
                break;
            case TBL_LAYOUT_BOTH_STANDARD:        // stubs on both left and right, standard page layout
                iInitColWidth = rcClient.Width()/(aColHead.GetSize()*2);      // 50% total
                break;
            default:
                ASSERT(FALSE);
            }
            iInitRowWidth=iSecondaryStubheadWidth=(int)(rcClient.Width()/4);
        }
    }

    rcpgobColRoot.CalcColHeadWidths(iInitColWidth, dc, m_bForceRemeasure);

    // field spanners have width = the whole table (page width minus page margins)
    int iFieldSpannerWidth=(int)(plTemplate.GetPgWidthInches()*TWIPS_PER_INCH);
    iFieldSpannerWidth -= fmtTblPrint.GetPageMargin().left;          // subtract left margin
    iFieldSpannerWidth -= fmtTblPrint.GetPageMargin().right;         // subtract right margin

    if (!bRecalcColWidth) {
        // use the initial row width already calculated ...
        rcpgobRowRoot.CalcStubWidth(iInitRowWidth, iFieldSpannerWidth, dc, m_bForceRemeasure);
    }
    else {

        /////////////////////////////////////////////////////////////////////////////////////
        // determine min sizes for each stub row, and the stub head
        //    start with 25% of user area width; call this iInitRowWidth


        iInitRowWidth = rcClient.Width()/4;  // 25% total

        rcpgobRowRoot.CalcStubWidth(iInitRowWidth, iFieldSpannerWidth, dc, m_bForceRemeasure);

        /////////////////////////////////////////////////////////////////////////////////////
        // set all stub and sidehead widths=max stub Wcol (left standard, left facing, both facing)   or
        //     max stub Wcol * 2 (both standard)
        // re-measure each stub (data or not), based on the new width.
        // (note that field spanners are not included in this calculation)
        int iMaxStubWidth=rcpgobRowRoot.GetMaxWidthLP();

        // remove stub measurements, to force remeasurement
        for (iStub=0 ; iStub<aStub.GetSize() ; iStub++) {
            CRowColPgOb* pStub=aStub[iStub];
            pStub->SetClientRectLP(CRect(0,0,0,0));
        }

        rcpgobRowRoot.CalcStubWidth(iMaxStubWidth, iFieldSpannerWidth, dc);   // re-measure
        iInitRowWidth=iMaxStubWidth;
    }

    rcpgobRowRoot.SetStubSizes(CSize(iInitRowWidth,NONE), true, m_bForceRemeasure);

    /////////////////////////////////////////////////////////////////////////////////////
    // determine number of horizontal pages needed
    /////////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////////
    // while horz layout is not done:
    //    determine number of horizontal pages needed (sum Wstubs and Wcol as appropriate,
    //    based on layout template)
    //
    //    try to equally divide columns across horz pages, as much as possible
    //
    //    adjust column widths so that they fill up each page horizontally
    //
    //    adjust spanner widths so that they line up with columns
    //    if any spanner needs to be wider, then allocate additional space to its columns
    //
    //    when no spanners need to increase width, then horz layout is done

    bool bHorzLayoutDone = false;
    bool bPageBreak=false;     // true when we want to force a horz page break
    int iHorzColArea;          // this is the horizontal space available on each page into which we place columns
    iNumHPages=1;              // total number of horz pages we need for the table
    int iTwipsAcross=0;        // number of twips we've allocated across the current page

    for (iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
        CRowColPgOb* pColHead = aColHead[iColHead];

        pColHead->GetClientRectLP().right -= pColHead->GetExtraLP().cx;
        pColHead->GetExtraLP().cx=0;

        // calculate horizontal area, which depends on primary and/or secondary stubheads ...
        iHorzColArea = GetHorzColArea((int)(plTemplate.GetPgWidthInches()*TWIPS_PER_INCH), fmtTblPrint, iInitRowWidth, iSecondaryStubheadWidth, iNumHPages-1);

        int iColHeadSectionLayoutWidth = pColHead->CalcColeadSectionLayoutWidth();
        bool bInsideSpanner;    //=true if pColHead is not the first column head inside a spanned section (below a spanner, that is)

        // CalcColHeadSectionLayoutWidth returns the width needed for a column head's section (from the start of the
        // section up to and including the pColHead). This calc also includes spanners above the column heads.
        // If we are moving among column heads within a section, we need to remove the previous calculated width,
        // since we're still in the same section and its measure will be superceded.
        bInsideSpanner=false;
        if (iColHead>0) {
            if (pColHead->GetParent()==aColHead[iColHead-1]->GetParent()) {
                // same section as previous column head...
                iTwipsAcross -= aColHead[iColHead-1]->CalcColeadSectionLayoutWidth();
                bInsideSpanner=true;
            }
        }

        if (iTwipsAcross+iColHeadSectionLayoutWidth>iHorzColArea || bPageBreak) {
            // placing this column head would extend us past the end of this page, so let's put it on the next page ...
            iNumHPages++;
            iTwipsAcross=0;

            // if we can't fit a single colhead on the page (ie, iColHead==0), then abort the build (we'll get reset to a default layout)
            if (iColHead==0) {
                return false;
            }

            // split the spanner (and the spanner's parents), so that it also goes on the new horz page
            SplitSpanner(rcpgobColRoot, iColHead);

            // rebuild the column heads (since the tree structure has changed)
            aColHead.RemoveAll();
            rcpgobColRoot.GetLeaves(aColHead);  // rebuild our column head array, since the tree has just changed
            pColHead = aColHead[iColHead];      // pColHead is now the at the start of the newly inserted subtree
            iColHeadSectionLayoutWidth = pColHead->CalcColeadSectionLayoutWidth();
        }

        // lay in this column head
        iTwipsAcross += iColHeadSectionLayoutWidth;
        pColHead->SetHPage(iNumHPages-1);  // track horizontal page
        bPageBreak=pColHead->IsPageBreak(); // user wants to break *after* pColHead, so we insert the break *before* the next colhead
    }

    // if any horz pages don't have at least 1 column, then abort (and probably reset to default layout)
    // (we can get here when spanners aren't agreeable)
    for (iHPage=0 ; iHPage<iNumHPages ; iHPage++) {
        bool bHasColHead=false;
        for (iColHead=0 ; iColHead<aColHead.GetSize() && !bHasColHead ; iColHead++) {
            CRowColPgOb* pColHead = aColHead[iColHead];
            if (pColHead->GetHPage()==iHPage) {
                bHasColHead=true;
            }
        }
        if (!bHasColHead) {
            return false;
        }
    }

    // assign page numbers to hidden colheads (since they never received treatment above); this lets PaginateSpanners() work with hidden colheads/spanners
    rcpgobColRoot.PaginateHiddenColHeads(aColHead);

    // assign page numbers to spanners (they have the same page numbers as their children)
    rcpgobColRoot.PaginateSpanners();
    rcpgobColRoot.DebugCol();           // sanity checks ...

//    /////////////////////////////////////////////////////////////////////////////////////
//    //    try to prevent orphan colheads on the last horz page, if possible
//
//    ResolveOrphanColHeads(aColHead);    //  This is left as an exercise for the reader.


    /////////////////////////////////////////////////////////////////////////////////////
    // set spanner widths to the sum of the widths of their children
    rcpgobColRoot.CalcSpannerWidths();

    // figure out where the title or subtitles end vertically; we'll place boxheads below that
    CRect rcTitle(0,0,0,0);
    for (int iPgOb=0 ; iPgOb<plTemplate.GetNumPgObs() ; iPgOb++) {
        if (plTemplate.GetPgOb(iPgOb).GetType()==PGOB_TITLE || plTemplate.GetPgOb(iPgOb).GetType()==PGOB_SUBTITLE)  {
            CRect rcOb=plTemplate.GetPgOb(iPgOb).GetClientRectLP();
            if (rcOb.bottom>rcTitle.bottom) {
                rcTitle = plTemplate.GetPgOb(iPgOb).GetClientRectLP();
            }
        }
    }
    ASSERT(!rcTitle.IsRectEmpty());  // if not, there have been no header objects laid out

    /////////////////////////////////////////////////////////////////////////////////////
    // all columns on a horz page cannot be "custom"; make last column uncustom if needed
    for (iHPage=0 ; iHPage<iNumHPages ; iHPage++) {
        bool bAllCustom=true;
        CRowColPgOb* pLastColHeadOnPage=NULL;
        for (iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
            CRowColPgOb* pColHead=aColHead[iColHead];
            if (pColHead->GetHPage()==iHPage) {
                pLastColHeadOnPage=pColHead;
                if (!pColHead->IsCustom()) {
                    bAllCustom=false;
                }
            }
        }
        if (bAllCustom) {
            // un-customize the last colhead
            ASSERT_VALID(pLastColHeadOnPage);
            pLastColHeadOnPage->SetCustom(false);
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //    adjust column widths so that they fill up each page horizontally
    if (m_bAutoFitColumns) {
        for (iHPage=0 ; iHPage<iNumHPages ; iHPage++) {
            iHorzColArea = GetHorzColArea((int)(plTemplate.GetPgWidthInches()*TWIPS_PER_INCH), fmtTblPrint, iInitRowWidth, iSecondaryStubheadWidth, iHPage);
            EquallyDivideColHeads(iTbl, iHPage, iHorzColArea, aColHead);
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // set spanner widths to the sum of the widths of their children
    rcpgobColRoot.CalcSpannerWidths();

    /////////////////////////////////////////////////////////////////////////////////////
    // calculate min height for each boxhead, based on its calculated width
    rcpgobColRoot.CalcBoxheadHeights(dc);

    // do some sanity checks ...
    rcpgobColRoot.DebugCol();

    /////////////////////////////////////////////////////////////////////////////////////
    // make boxheads and spanners line up vertically
    int iColHeadHorzLevel=AlignColumnsVert(aColHead, rcpgobColRoot);

    /////////////////////////////////////////////////////////////////////////////////////
    // make boxheads and spanners line up horizontally
    for (iHPage=0 ; iHPage<iNumHPages ; iHPage++) {
        int iLeftColMargin=fmtTblPrint.GetPageMargin().left;

        switch(fmtTblPrint.GetTblLayout()) {
        case TBL_LAYOUT_LEFT_STANDARD:       // stubs on left only, standard page layout (default)
        case TBL_LAYOUT_BOTH_STANDARD:       // stubs on both left and right, standard page layout
            iLeftColMargin+=iInitRowWidth;
            break;
        case TBL_LAYOUT_LEFT_FACING:         // stubs on left, facing-pages layout
            if (iHPage%2==0) {
                iLeftColMargin+=iInitRowWidth;
            }
            break;
        case TBL_LAYOUT_BOTH_FACING:         // stubs on both left and right, facing-pages layout
            if (iHPage%2==0) {
                iLeftColMargin+=iInitRowWidth;
            }
            break;
        }

        rcpgobColRoot.AlignColumnsHorz(CPoint(iLeftColMargin,rcTitle.bottom), iHPage, NONE);
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // determine stubhead placement (width=Wstubs and height=sum of heights of each header level)
    CRect rcStubhead;
    rcStubhead.SetRectEmpty();
    rcStubhead.bottom=iColHeadHorzLevel;
    rcStubhead.right=iInitRowWidth;
    pgobStubhead.SetClientRectLP(rcStubhead);

    if (pgobSecondaryStubhead.GetClientRectLP().Width()==0) {
        // no persisted info for secondary stubhead ... make it the same as the primary stubhead
        pgobSecondaryStubhead.SetClientRectLP(pgobStubhead.GetClientRectLP());
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // identify page notes and end notes, if they are present (we'll reposition them and add them back later)
    // note that these indexes might change for facing page layouts, since there would be multiple
    // page/end notes in the layout template (see BuildNotes() for info).
    int iPageNoteIndex=NONE, iEndNoteIndex=NONE;
    int iPgOb=0 ;
    for (iPgOb=0 ; iPgOb<plTemplate.GetNumPgObs() ; iPgOb++) {
        if (plTemplate.GetPgOb(iPgOb).GetType()==PGOB_PAGENOTE) {
            iPageNoteIndex=iPgOb;
        }
        else if (plTemplate.GetPgOb(iPgOb).GetType()==PGOB_ENDNOTE) {
            iEndNoteIndex=iPgOb;
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // determine number of vertical pages needed    (account for end note on bottom page)
    /////////////////////////////////////////////////////////////////////////////////////


    // figure out where the top of footers and page notes are ...
    int iStubBottom;            // the highest object below stubs; could be a footer or page note
    iStubBottom = (int)(plTemplate.GetPgHgtInches()*TWIPS_PER_INCH)     // default is bottom of user area (page height
                  - fmtTblPrint.GetPageMargin().bottom;                 // minus bottom margin)

    for (iPgOb=0 ; iPgOb<plTemplate.GetNumPgObs() ; iPgOb++) {
        if (plTemplate.GetPgOb(iPgOb).GetType()==PGOB_FOOTER_LEFT || plTemplate.GetPgOb(iPgOb).GetType()==PGOB_FOOTER_CENTER || plTemplate.GetPgOb(iPgOb).GetType()==PGOB_FOOTER_RIGHT || plTemplate.GetPgOb(iPgOb).GetType()==PGOB_PAGENOTE)  {
            iStubBottom= __min(plTemplate.GetPgOb(iPgOb).GetClientRectLP().top, iStubBottom);
        }
    }

    iNumVPages=1;              // total number of vertical pages we need for the table
    int iTwipsDown=0;          // number of twips we've allocated down the current page
    int iVertStubArea;         // this is the vertical area available on each page into which we place stubs

    iVertStubArea  = iStubBottom;               // top of footer area
    iVertStubArea -= rcTitle.bottom;            // - bottom of title area

    // special processing for boxheads, based on header frequency --
    switch(fmtTblPrint.GetHeaderFrequency()) {
    case HEADER_FREQUENCY_NONE:
        // no boxheads ... never subtract iColHeadHorzLevel from iVertStubArea
        break;
    case HEADER_FREQUENCY_TOP_TABLE:
        // boxheads on first page only ... subtract iColHeadHorzLevel from iVertStubArea, then add it back in when we advance to the next page
        iVertStubArea -= iColHeadHorzLevel;         // - bottom of column head area
        break;
    case HEADER_FREQUENCY_TOP_PAGE:
        // boxheads on all pages ... subtract iColHeadHorzLevel from iVertStubArea
        iVertStubArea -= iColHeadHorzLevel;         // - bottom of column head area
        break;
    default:
        ASSERT(FALSE);
    }

    // special processing for end note, if present.  These appear only on the last page, so we
    // need to move backward through the stubs until we've equaled or exceeded the end note's height.
    // From this point on, the end note is accounted for in the vertical stub area.
    int iEndNoteThreshold=0;
    int iStubIndexForEndNotes=NONE;
    if (iEndNoteIndex!=NONE) {
        int iEndNoteHgt=plTemplate.GetPgOb(iEndNoteIndex).GetClientRectLP().Height();
        for (iStub=aStub.GetSize()-1 ; iStub>=0 ; iStub--) {
            CRowColPgOb* pStub = aStub[iStub];
            iEndNoteThreshold += pStub->GetClientRectLP().Height();
            if (iEndNoteThreshold>=iEndNoteHgt) {
                iStubIndexForEndNotes=iStub;
                break;
            }
        }
        ASSERT(iStub>0);
    }

    bPageBreak=false;
    for (iStub=0 ; iStub<aStub.GetSize() ; iStub++) {
        CRowColPgOb* pStub = aStub[iStub];

        if (iStubIndexForEndNotes==iStub) {
            iVertStubArea -= plTemplate.GetPgOb(iEndNoteIndex).GetClientRectLP().Height();
        }

        // skip excluded objects in this analysis
        if (pStub->GetType()!=PGOB_NOTINCLUDED) {
            if (iTwipsDown+pStub->GetClientRectLP().Height()>iVertStubArea || bPageBreak) {
                // placing this stub would extend us below the bottom of this page, so let's put it on the next page ...
                iNumVPages++;
                iTwipsDown=0;

                // add back the boxhead area if we're only putting boxheads on the first page
                if (fmtTblPrint.GetHeaderFrequency()==HEADER_FREQUENCY_TOP_TABLE && iNumVPages==2) {
                    iVertStubArea += iColHeadHorzLevel;
                }
            }

            // lay in this stub
            iTwipsDown+= pStub->GetClientRectLP().Height();
            pStub->SetVPage(iNumVPages-1);  // track vertical page
        }
        bPageBreak=pStub->IsPageBreak(); // user wants to break *after* pStub, so we insert the break *before* the next one
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // determine vertical placement for stubs and captions (these flow down from stubhead), spanning vertical pages
    for (iVPage=0 ; iVPage<iNumVPages ; iVPage++) {
        CPoint ptTopLeft(rcClient.left, rcTitle.bottom);  //top-left position of first stub on each page

        // put stubheads on each page only if we're adding boxheads
        bool bAddBoxheads;
        switch(fmtTblPrint.GetHeaderFrequency()) {
        case HEADER_FREQUENCY_TOP_TABLE:
            bAddBoxheads=(iVPage==0);
            break;
        case HEADER_FREQUENCY_TOP_PAGE:
            bAddBoxheads=true;
            break;
        case HEADER_FREQUENCY_NONE:
            bAddBoxheads=false;
            break;
        default:
            ASSERT(FALSE);
        }

        if (bAddBoxheads) {
            // offset stubhead down the page ...
            pgobStubhead.GetClientRectLP().OffsetRect(- pgobStubhead.GetClientRectLP().TopLeft());
            pgobStubhead.GetClientRectLP().OffsetRect(ptTopLeft);

            // advance down below the stubhead
            ptTopLeft.y += pgobStubhead.GetClientRectLP().Height();
        }

        // put stubs and captions (except for hidden captions) on their specific pages ...
        for (iStub=0 ; iStub<aStub.GetSize() ; iStub++)  {
            CRowColPgOb* pStub = aStub[iStub];

            // hidden captions come through to us here, but hidden stubs are not present in aStub[].  So, we still need to test for visibility...
            if (pStub->GetVPage()==iVPage && pStub->GetType()!=PGOB_NOTINCLUDED && pStub->GetFmt()->GetHidden()!=HIDDEN_YES && !pStub->GetHideRowForAllZeroCells())  {

                // this stub is on this vertical page ...

                // offset stub down the page ...
                pStub->GetClientRectLP().OffsetRect(ptTopLeft);

                // spread captions across the whole table if the they span cells
                if (pStub->GetType()==PGOB_CAPTION) {
                    CFmt* pFmt=pStub->GetFmt();
                    ASSERT(pFmt->GetSpanCells()==SPAN_CELLS_YES || pFmt->GetSpanCells()==SPAN_CELLS_NO);
                    if (pFmt->GetSpanCells()==SPAN_CELLS_YES) {
                        // spanning cells .. make the caption's right side flush with the right side of the last colhead on this horzpage
                        CRect rcStub=pStub->GetClientRectLP();
                        for (iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
                            CRowColPgOb* pColHead = aColHead[iColHead];

                            if (pStub->GetHPage()==pColHead->GetHPage() || pStub->GetHPage()==NONE) {
                                // page match
                                if (pColHead->GetClientRectLP().right>rcStub.right) {
                                    rcStub.right=pColHead->GetClientRectLP().right;
                                }
                            }
                        }
                        pStub->SetClientRectLP(rcStub);
                    }
                }

                // advance down
                ptTopLeft.y += pStub->GetClientRectLP().Height();
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////
    // pages are laid out horizontally, then vertically.  For example, if a table requires
    // 4 horz pages and 3 vertical pages, then the order of pages in pgMgr would be:
    //          H0   H1   H2   H3
    //         ----------------------
    //     V0  | 0    1    2    3                here, iNumHPages=4, iNumVPages=3
    //     V1  | 4    5    6    7                pg 6 is H2, V1
    //     V2  | 8    9   10   11
    //////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////
    // add pages to the page manager, as dictated by the layout
    for (int iPg=0 ; iPg<iNumHPages*iNumVPages ; iPg++) {
        CPgLayout pl(plTemplate);   // copy objects already on the first page into this new page
        pgMgr.AddPgLayout(pl);
    }

    //////////////////////////////////////////////////////////////////////////////////////
    // The page layout template will either have 2 titles (standard layouts) 4 titles (facing layouts).
    // (See full description in comments for BuildTitles()).
    //
    // for standard layouts,
    //      title 0 does not have continuation ==> put on horz page H0:V0
    //      title 1 has continuation ==> put on all other pages
    //
    // for facing layouts,
    //      title 0 does not have continuation ==> put on horz page H0:V0 (left-side)
    //      title 1 does not have continuation ==> put on horz page H1:V0 (right-side)
    //      title 2 has continuation ==> put on H0:V1, H0:V2, H0:V3, ... (left sides)
    //      title 3 has continuation ==> put on H1:V1, H1:V2, H1:V3, ... (right sides)
    //
    //////////////////////////////////////////////////////////////////////////////////////
    // Facing page layouts have 2 page/end note objects.(See full description in comments for BuildNotes()):
    //      note 0 is for left-side pages
    //      note 1 is for right-side pages
    //

    bool bFacingLayout=(fmtTblPrint.GetTblLayout()==TBL_LAYOUT_LEFT_FACING || fmtTblPrint.GetTblLayout()==TBL_LAYOUT_BOTH_FACING);
    for (iHPage=0 ; iHPage<iNumHPages ; iHPage++) {
        for (iVPage=0 ; iVPage<iNumVPages ; iVPage++) {
            CPgLayout& pl = pgMgr.GetPgLayout(iNumHPages * iVPage + iHPage);
            int iTitle=0;   // which title we're working on (0:3 or 0:1)
            int iSubTitle=0; // which subtitle we're working on (0:1)
            for (iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
                CPgOb& pgob=pl.GetPgOb(iPgOb);

                // handle titles...
                if (pgob.GetType()==PGOB_TITLE) {
                    switch(iTitle++) {
                    case 0:  //first title on the page template -- no continuation marker, non-facing page layout
                        if ((!bFacingLayout && iHPage==0 && iVPage==0) || (bFacingLayout && iVPage==0 && (iHPage%2)==0)) {
                            // non-continuation ... keep this title!
                        }
                        else {
                            // nuke this title!
                            pl.RemovePgObAt(iPgOb--);
                        }
                        break;
                    case 1:  //second title on the page template -- continuation marker, non-facing page layout
                        if ((!bFacingLayout && (iHPage>0 || iVPage>0)) || (bFacingLayout && iVPage==0 && (iHPage%2)==1)) {
                            // right-side non-continuation page or standard continuation page ... keep this title
                        }
                        else {
                            // nuke this title!
                            pl.RemovePgObAt(iPgOb--);
                        }
                        break;
                    case 2:  //third title on the page template -- no continuation marker, facing page layouts
                        ASSERT(bFacingLayout);
                        if (iVPage>0 && (iHPage%2)==0) {
                            // non-continuation left-side page ... keep
                        }
                        else {
                            // nuke this title!
                            pl.RemovePgObAt(iPgOb--);
                        }
                        break;
                    case 3:  //fourth title on the page template -- continuation marker, facing page layouts
                        ASSERT(bFacingLayout);
                        if (iVPage>0 && (iHPage%2)==1) {
                            // non-continuation right-side page ... keep
                        }
                        else {
                            // nuke this title!
                            pl.RemovePgObAt(iPgOb--);
                        }
                        break;
                    default:
                        // internal error ... more than 4 titles on the page template
                        ASSERT(FALSE);
                    }
                }
                else if (pgob.GetType()==PGOB_SUBTITLE) {
                    // handle subtitles...
                    switch(iSubTitle++) {
                    case 0:  //first subtitle on the page template
                        if (!bFacingLayout || (bFacingLayout && (iHPage%2)==0)) {
                            // non-continuation or left-side facing ... keep this subtitle!
                        }
                        else {
                            // nuke this title!
                            pl.RemovePgObAt(iPgOb--);
                        }
                        break;
                    case 1:  //second subtitle on the page template
                        if (bFacingLayout && (iHPage%2)==1) {
                            // right-side facing page ... keep this subtitle
                        }
                        else {
                            // nuke this title!
                            pl.RemovePgObAt(iPgOb--);
                        }
                        break;
                    default:
                        // internal error ... more than 2 titles on the page template
                        ASSERT(FALSE);
                    }
                }
            }
            ASSERT((bFacingLayout && iTitle==4)||(!bFacingLayout && iTitle==2));   // page template must have 2 or 4 titles

            if (bFacingLayout) {
                // handle page notes when layout is facing pages...
                int iPageNote=0;  // which page note we're working on (0:1 or 0);
                for (iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
                    CPgOb& pgob=pl.GetPgOb(iPgOb);
                    if (pgob.GetType()==PGOB_PAGENOTE) {
                        switch(iPageNote++) {
                        case 0:  //first page note on the page template ... for left-side pages
                            if ((iHPage%2)==0) {
                                // left-side page ... keep this note!
                            }
                            else {
                                // nuke this note!
                                pl.RemovePgObAt(iPgOb--);
                            }
                            break;
                        case 1:  //second page note on the page template ... for right-side pages
                            if ((iHPage%2)!=0) {
                                // right-side page ... keep this note!
                            }
                            else {
                                // nuke this note!
                                pl.RemovePgObAt(iPgOb--);
                            }
                            break;
                        default:
                            ASSERT(FALSE);
                        }
                    }
                }
                ASSERT(iPageNote==2 || iPageNote==0);

                // handle end notes when layout is facing pages...
                int iEndNote=0;   // which end note we're working on (0:1 or 0);
                for (iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
                    CPgOb& pgob=pl.GetPgOb(iPgOb);
                    if (pgob.GetType()==PGOB_ENDNOTE) {
                        switch(iEndNote++) {
                        case 0:  //first end note on the page template ... for left-side pages
                            if ((iHPage%2)==0) {
                                // left-side page ... keep this note!
                            }
                            else {
                                // nuke this note!
                                pl.RemovePgObAt(iPgOb--);
                            }
                            break;
                        case 1:  //second end note on the page template ... for right-side pages
                            if ((iHPage%2)!=0) {
                                // right-side page ... keep this note!
                            }
                            else {
                                // nuke this note!
                                pl.RemovePgObAt(iPgOb--);
                            }
                            break;
                        default:
                            ASSERT(FALSE);
                        }
                    }
                }
                ASSERT(iEndNote==2 || iEndNote==0);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////
    // lay out objects across and down pages, taking layout template into account
    //         (boxheads are repeated down through vertical pages)
    for (iHPage=0 ; iHPage<iNumHPages ; iHPage++) {

        // figure out where colheads end for this horz page (for placing secondary stubhead)
        int iRightStub=0;           // position for right hand stubs, if used
        for (iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
            CRowColPgOb* pColHead = aColHead[iColHead];
            if (pColHead->GetHPage()==iHPage) {
                if (pColHead->GetClientRectLP().right>iRightStub) {
                    iRightStub=pColHead->GetClientRectLP().right;
                }
            }
        }

        for (iVPage=0 ; iVPage<iNumVPages ; iVPage++) {
            CPgLayout& pl = pgMgr.GetPgLayout(iNumHPages * iVPage + iHPage);

            // re-locate page/end note indices; at this point there is at most 1 page/end note
            iPageNoteIndex=iEndNoteIndex=NONE;
            for (iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
                const CPgOb& pgob=pl.GetPgOb(iPgOb);

                if (pgob.GetType()==PGOB_PAGENOTE) {
                    iPageNoteIndex=iPgOb;
                }
                else if (pgob.GetType()==PGOB_ENDNOTE) {
                    iEndNoteIndex=iPgOb;
                }
            }

            bool bAddBoxheads=true;     // do we add boxheads (and stubhead) at the top of this page?
            bool bAddStubsLeft=true;    // do we add stubs on the left side of this page?
            bool bAddStubsRight=false;  // do we add stubs on the right side of this page?

            switch(fmtTblPrint.GetTblLayout()) {
            case TBL_LAYOUT_LEFT_STANDARD:       // stubs on left only, standard page layout (default)
                bAddStubsLeft=true;
                bAddStubsRight=false;
                break;
            case TBL_LAYOUT_LEFT_FACING:         // stubs on left, facing-pages layout
                bAddStubsLeft=(iHPage%2==0);
                bAddStubsRight=false;
                break;
            case TBL_LAYOUT_BOTH_STANDARD:       // stubs on both left and right, standard page layout
                bAddStubsLeft=true;
                bAddStubsRight=true;
                break;
            case TBL_LAYOUT_BOTH_FACING:         // stubs on both left and right, facing-pages layout
                bAddStubsLeft=(iHPage%2==0);
                bAddStubsRight=!bAddStubsLeft;
                break;
            }

            switch(fmtTblPrint.GetHeaderFrequency()) {
            case HEADER_FREQUENCY_TOP_TABLE:
                bAddBoxheads=(iVPage==0);
                break;
            case HEADER_FREQUENCY_TOP_PAGE:
                bAddBoxheads=true;
                break;
            case HEADER_FREQUENCY_NONE:
                bAddBoxheads=false;
                break;
            default:
                ASSERT(FALSE);
            }

            //stubheads appear on all pages where boxheads appear
            if (bAddStubsLeft && bAddBoxheads) {
                // stubhead was positioned when we laid in stubs above, so we just need to add it to the page.
                pgobStubhead.SetHPage(iHPage);
                pgobStubhead.SetVPage(iVPage);
                pl.AddPgOb(pgobStubhead);
            }
            if (bAddStubsRight && bAddBoxheads) {
                // need to put the secondary stubhead on the right, past the right-most colhead (iRightStub)
                pgobSecondaryStubhead.GetClientRectLP().OffsetRect(- pgobSecondaryStubhead.GetClientRectLP().TopLeft());
                pgobSecondaryStubhead.GetClientRectLP().OffsetRect(iRightStub,pgobStubhead.GetClientRectLP().top);
                pgobSecondaryStubhead.SetHPage(iHPage);
                pgobSecondaryStubhead.SetVPage(iVPage);
                pl.AddPgOb(pgobSecondaryStubhead);
            }

            // add stubs to the pages to which they belong
            for (iStub=0 ; iStub<aStub.GetSize() ; iStub++)  {
                CRowColPgOb* pStub = aStub[iStub];

                if (pStub->GetType()!=PGOB_NOTINCLUDED) {
                    // stubs and captions appear depending on table layout...
                    if (pStub->GetVPage()==iVPage && pStub->GetFmt()->GetHidden()!=HIDDEN_YES && !pStub->GetHideRowForAllZeroCells())  {
                        if (bAddStubsLeft) {
                            pStub->SetHPage(iHPage);
                            pStub->SetVPage(iVPage);
                            pl.AddPgOb(*pStub);
                        }
                        if (bAddStubsRight) {
                            // make a copy of this stub, mirror it on the right, and add it in ...
                            CPgOb pgobStubRight(*pStub);
                            pgobStubRight.SetType(PGOB_STUB_RIGHT);
                            pgobStubRight.GetClientRectLP().right=pgobStubRight.GetClientRectLP().left + pgobSecondaryStubhead.GetClientRectLP().Width(); // force stub to have same width as secondary stubhead
                            pgobStubRight.GetClientRectLP().OffsetRect(iRightStub-pgobStubRight.GetClientRectLP().left,0);
                            pgobStubRight.SetHPage(iHPage);
                            pgobStubRight.SetVPage(iVPage);
                            pl.AddPgOb(pgobStubRight);
                        }
                    }
                }
            }

            // put the page's page note (if present) below the bottom stub on every vertical page
            if (iPageNoteIndex!=NONE) {
                ASSERT(pl.GetPgOb(iPageNoteIndex).GetType()==PGOB_PAGENOTE);
                int iBottomOfStubArea=0;
                for (iStub=0 ; iStub<aStub.GetSize() ; iStub++) {
                    CRowColPgOb* pStub = aStub[iStub];
                    if (pStub->GetVPage()==iVPage) {
                        iBottomOfStubArea=__max(iBottomOfStubArea, pStub->GetClientRectLP().bottom);
                    }
                }
                ASSERT(iBottomOfStubArea>0);

                CPgOb& pgobPageNote=pl.GetPgOb(iPageNoteIndex);
                pgobPageNote.GetClientRectLP().bottom=iBottomOfStubArea+pgobPageNote.GetClientRectLP().Height();
                pgobPageNote.GetClientRectLP().top=iBottomOfStubArea;
            }

            // put the table's end note (if present) below the bottom stub on the last vertical page
            if (iEndNoteIndex!=NONE) {
                ASSERT(pl.GetPgOb(iEndNoteIndex).GetType()==PGOB_ENDNOTE);
                if (iVPage==iNumVPages-1) {
                    int iBottomOfStubArea=0;
                    for (iStub=0 ; iStub<aStub.GetSize() ; iStub++) {
                        CRowColPgOb* pStub = aStub[iStub];
                        if (pStub->GetVPage()==iVPage) {
                            iBottomOfStubArea=__max(iBottomOfStubArea, pStub->GetClientRectLP().bottom);
                        }
                    }
                    for (int i=0 ; i<pl.GetNumPgObs() ; i++) {
                        CPgOb& pgobBottomStub=pl.GetPgOb(i);
                        if (pgobBottomStub.GetType()==PGOB_PAGENOTE) {
                            iBottomOfStubArea=__max(iBottomOfStubArea, pgobBottomStub.GetClientRectLP().bottom);
                        }
                    }
                    ASSERT(iBottomOfStubArea>0);

                    CPgOb& pgobEndNote=pl.GetPgOb(iEndNoteIndex);
                    pgobEndNote.GetClientRectLP().bottom=iBottomOfStubArea+pgobEndNote.GetClientRectLP().Height();
                    pgobEndNote.GetClientRectLP().top=iBottomOfStubArea;
                }
                else {
                    // not on the bottom vertical page ... nuke the page's end note
                    pl.RemovePgObAt(iEndNoteIndex);
                }
            }

            // add boxheads and spanners to their pages
            pl.AddRowColPgOb(rcpgobColRoot, bAddBoxheads, iHPage, NONE);

        }
    }

    // add cells to their pages
    int iDataCellIndex=0;  // counter for accessing data cell array, below
    for (iStub=0 ; iStub<aStub.GetSize() ; iStub++) {
        CRowColPgOb* pStub = aStub[iStub];

        // skip excluded objects ...
        if (pStub->GetType()==PGOB_NOTINCLUDED) {
            continue;
        }

        // skip captions ...
        if (pStub->GetType()==PGOB_CAPTION || pStub->GetType()==PGOB_READER_BREAK) {
            //Savy changed this code when captions span cells .
            //Savy changed this code to fall through to add pgobject when captions dont span
             bool bFieldSpanner=(pStub->GetFmt()->GetSpanCells()==SPAN_CELLS_YES);
             if(bFieldSpanner)
                continue;
        }

        // skip reader break ...
        /*if (pStub->GetType()==PGOB_READER_BREAK) {
            continue;
        }*/

        for (iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
            CRowColPgOb* pColHead = aColHead[iColHead];

            // get the cell ...
            CPgOb& pgobCell = aCell[iDataCellIndex++];

            // the page for this cell is determined by the vert stub page and horz boxhead page...
            CPgLayout& pl = pgMgr.GetPgLayout(iNumHPages * pStub->GetVPage() + pColHead->GetHPage());
            pgobCell.SetHPage(iHPage);
            pgobCell.SetVPage(iVPage);

            // line up the cell ...
            CRect rcCell(pColHead->GetClientRectLP().left, pStub->GetClientRectLP().top, pColHead->GetClientRectLP().right, pStub->GetClientRectLP().bottom);
            pgobCell.SetClientRectLP(rcCell);

            pl.AddPgOb(pgobCell);
        }
    }

    // set tbl for each pgob
    for (int iPg=0 ; iPg<pgMgr.GetNumPages() ; iPg++) {
        CPgLayout& pl = pgMgr.GetPgLayout(iPg);
        for (iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
            pl.GetPgOb(iPgOb).SetTbl(iTbl);
        }

        // coordinate lines between the various page objects
        pl.CoordinateLines(fmtTbl.GetBorderLeft(), fmtTbl.GetBorderTop(), fmtTbl.GetBorderRight(), fmtTbl.GetBorderBottom());
    }

Dbg_Dump_Tree(&rcpgobRowRoot);
Dbg_Dump_Tree(&rcpgobColRoot);

    // success!
    return true;
}


////////////////////////////////////////////////////////////////////////////////////
//
//        GetHorzColArea
//
// Returns the horizontal area available (in twips) for for laying out columns.
// This depends on the table layout:
// * left standard: page width - left margin - right margin - stub width
// * left facing: for horz page 0,2,4,..., same as left standard
//                for horz page 1,3,5,..., page width - left margin - right margin
// * both standard: page width - left margin - right margin - stub width - secondary stubhead width
// * both facing: for horz page 0,2,4,..., same as left standard
//                for horz page 1,3,5,..., page width - left margin - right margin
//
///////////////////////////////////////////////////////////////////////////////////////
int CTabPrtView::GetHorzColArea(int iPageWidth, const CTblPrintFmt& fmtTblPrint, int iStubWidth, int iSecondaryStubheadWidth, int iHPage)
{

    int iHorzColArea;

    iHorzColArea = iPageWidth;                              // total page width, in twips
    iHorzColArea -= fmtTblPrint.GetPageMargin().left;          // subtract left margin
    iHorzColArea -= fmtTblPrint.GetPageMargin().right;         // subtract right margin

        switch(fmtTblPrint.GetTblLayout()) {
        case TBL_LAYOUT_LEFT_STANDARD:       // stubs on left only, standard page layout (default)
            iHorzColArea -= iStubWidth;
            break;
        case TBL_LAYOUT_LEFT_FACING:         // stubs on left, facing-pages layout
            if (iHPage%2==0) {
                iHorzColArea -= iStubWidth;
            }
            else {
                // do nothing
            }
            break;
        case TBL_LAYOUT_BOTH_STANDARD:       // stubs on both left and right, standard page layout
            iHorzColArea -= iStubWidth;
            iHorzColArea -= iSecondaryStubheadWidth;
            break;
        case TBL_LAYOUT_BOTH_FACING:         // stubs on both left and right, facing-pages layout
            if (iHPage%2==0) {
                //left side page ... use regular row width
                iHorzColArea -= iStubWidth;
            }
            else {
                // right side page ... use secondary stubhead
                iHorzColArea -= iSecondaryStubheadWidth;
            }
            break;
        default:
            ASSERT(FALSE);
        }
    return iHorzColArea;
}


////////////////////////////////////////////////////////////////////////////////////
//
//        BuildAndMeasureCells
//
// Builds array of cells for the current table and measures them.  Also
// adjusts stub heights and column widths so that they accommodate the
// widest/highest cells in the table.
//
///////////////////////////////////////////////////////////////////////////////////////
void CTabPrtView::BuildAndMeasureCells(int iTbl, CDC& dc, CArray<CRowColPgOb*, CRowColPgOb*>& aColHead, CArray<CRowColPgOb*, CRowColPgOb*>& aStub, CArray<CPgOb, CPgOb&>& aCell)
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
   // CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    CTable* pTbl = pSet->GetTable(iTbl);
    const CFmtReg& fmtReg=pSet->GetFmtReg();
    double dAbs = PRT_VIEW_MISSING;

    CTabSetFmt* pTabSetFmt=DYNAMIC_DOWNCAST(CTabSetFmt,fmtReg.GetFmt(FMT_ID_TABSET,0));
    CArray<CTabPrtViewCellInfo, CTabPrtViewCellInfo&> aCellInfo;
    CIMSAString sZeroFill =pTabSetFmt->GetZeroMask();

    // see if a run has happened (and the table has data) ...
    bool bDataAvailable = (pTbl->GetTabDataArray().GetSize()>0);  // if not, then just show blank cells

    // rows need to account for area processing ... (a "slice" is an area instance)
    bool bAreaProcessing=(pTbl->GetTabDataArray().GetSize()>1);
    int iSlice=0;    // will only advance to 1+ if area processing
    int iDataCellsPerSlice=bAreaProcessing?pTbl->GetTabDataArray()[0]->GetCellArray().GetSize():0;


    int iStub, iColHead, iDataCellIndex=0;  // counter for accessing data cell array, below
    int iColHeadSize =aColHead.GetSize();
    for (iStub=0 ; iStub<aStub.GetSize() ; iStub++) {
        CRowColPgOb* pStub = aStub[iStub];

        if (pStub->GetType()!=PGOB_STUB) {
            // don't need to add cells for captions
            //Add dummmy cells
            for (iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
                CTabPrtViewCellInfo x(NULL, 0);
                if(pStub->GetType()==PGOB_CAPTION || pStub->GetType()==PGOB_READER_BREAK){
                    bool bFieldSpanner=(pStub->GetFmt()->GetSpanCells()==SPAN_CELLS_YES);
                    if(bFieldSpanner){//No additional cells /pgobs if field spanner is yes
                        //This is done to display the spanned text correctly
                        continue;
                    }
                    CFmt* pFmt = NULL;
                    //Remove this once done
                   /* if(pStub->GetTblBase() == pTbl->GetAreaCaption()){
                        if(pTbl->GetAreaCaption()->GetDerFmt()){
                           pFmt->Assign(*pTabVar->GetDerFmt());
                           pFmt->CopyNonDefaultValues(DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_AREACAPTION,0)));
                        }
                        else{
                            pFmt = new CFmt(*DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_AREACAPTION,0)));
                        }
                    }
                    else {
                        CTabVar* pTabVar = DYNAMIC_DOWNCAST(CTabVar,pStub->GetTblBase());
                        ASSERT(pTabVar);
                        if(pTabVar->GetDerFmt()){
                           pFmt->Assign(*pTabVar->GetDerFmt());
                           pFmt->CopyNonDefaultValues(DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_CAPTION,0)));
                        }
                        else{
                            pFmt = new CFmt(*DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_CAPTION,0)));
                        }
                    }*/
                    CRowColPgOb* pColHead = aColHead[iColHead];
					if(pStub->GetType()==PGOB_READER_BREAK){
						pFmt = NULL;
					}
                    else if(pStub->GetTblBase() == pTbl->GetAreaCaption()){
                        pFmt = new CFmt(*DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_AREA_CAPTION,0)));
                    }
                    else {
                        pFmt = new CFmt(*DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(FMT_ID_CAPTION,0)));
                    }
                    int iPanel=GetRowPanel(aStub, iStub);
                    CPgOb pgobCell;
					if(pStub->GetType()==PGOB_READER_BREAK){
						pgobCell.SetType(PGOB_READER_BREAK);
						CDataCellFmt* pDataCellFmt = new CDataCellFmt(*DYNAMIC_DOWNCAST(CDataCellFmt,fmtReg.GetFmt(FMT_ID_STUB,0)));
			            GetDataCellFormat(pColHead, pStub, pDataCellFmt, iColHead, iPanel);
						pFmt = pDataCellFmt;

					}else {
						pgobCell.SetType(PGOB_CAPTION);
	                    GetCellFormat4Captions(pColHead, pStub, pFmt, iColHead, iPanel);
					}

                    pgobCell.SetTblBase(pTbl->GetAreaCaption());
                    LPToPoints(pFmt, GetLogPixelsY());
                    PointsToTwips(pFmt);
                    pgobCell.SetDrawFormatFlags(GetAlignmentFlags(pFmt)|DT_SINGLELINE);
                    if(pFmt->GetHidden() == HIDDEN_YES) {
                        pgobCell.SetHiddenFlag(true);
                    }
                    pgobCell.SetFmt(pFmt);
                    aCell.Add(pgobCell);

                    aCellInfo.Add(x);
                }
                else {
                    aCellInfo.Add(x);
                }
            }
            continue;
        }
        iColHeadSize =aColHead.GetSize();
        for (iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
            CRowColPgOb* pColHead = aColHead[iColHead];

            // each combination of stub/boxhead has a cell ...
            CPgOb pgobCell;
            pgobCell.SetType(PGOB_DATACELL);

            // get format attributes for cells
            CDataCellFmt* pDataCellFmt = new CDataCellFmt(*DYNAMIC_DOWNCAST(CDataCellFmt,fmtReg.GetFmt(FMT_ID_DATACELL,0)));
            LPToPoints(pDataCellFmt, GetLogPixelsY());
            PointsToTwips(pDataCellFmt);
            int iPanel=GetRowPanel(aStub, iStub);
            GetDataCellFormat(pColHead, pStub, pDataCellFmt, iColHead, iPanel);

            // check for custom text
            double dData = PRT_VIEW_MISSING;  // cell value
			// don't get data for cells that are not in stub rows (captions & reader breaks)
			if(bDataAvailable && pStub->GetType()==PGOB_STUB) {
				dData=pTbl->GetTabDataArray()[iSlice]->GetCellArray()[iDataCellIndex++];
                // update slice if area processing
				if (bAreaProcessing && iDataCellIndex>=iDataCellsPerSlice) {
                    iDataCellIndex=0;
                    iSlice++;
                }
            }

            if(pDataCellFmt->GetHidden() == HIDDEN_YES) {
                pgobCell.SetHiddenFlag(true);
            }
            else if (pDataCellFmt->IsTextCustom()) {
               pgobCell.SetText(pDataCellFmt->GetCustom().m_sCustomText);
            }
            else {
                // captions and reader breaks have blank cell values
                if (pStub->GetType()!=PGOB_STUB) {
                    pgobCell.SetText(_T(""));
                }
                else {
                    // we've got a data cell ... assign cell value (or leave it blank if no data is available) ...
                    if (!bDataAvailable) {
                        pgobCell.SetText(_T(""));
                    }
                }
            }

            // assign format to this cell
            pgobCell.SetFmt(pDataCellFmt);

            // put in alignment options
            pgobCell.SetDrawFormatFlags(GetAlignmentFlags(pDataCellFmt)|DT_SINGLELINE);

            aCell.Add(pgobCell);

            // Add CTabPrtViewCellInfo so that we can regroup cell data later (necessary since Serpro separates
            // counts and percents, and we usually want them integrated). Note that we don't set the m_pPgOb
            // member yet, because CArray::Add shift its data memory as it grows.
            CTabPrtViewCellInfo x(NULL, dData);
            aCellInfo.Add(x);
        }
    }

    // set m_pPgOb members in the aCellInfo array, now that aCell has been filled
    int iIndex=0;
    int iCellInfo=0;
    for (iStub=0 ; iStub<aStub.GetSize() ; iStub++) {
        CRowColPgOb* pStub = aStub[iStub];
        if (pStub->GetType()!=PGOB_STUB) {
            for (iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
                if(pStub->GetType()== PGOB_CAPTION || pStub->GetType()== PGOB_READER_BREAK ){
                    bool bFieldSpanner=(pStub->GetFmt()->GetSpanCells()==SPAN_CELLS_YES);
                    if(bFieldSpanner){//No additional cells /pgobs if field spanner is yes
                        //This is done to display the spanned text correctly
                        continue;
                    }
                    CPgOb* pPgOb=&aCell[iIndex];
                    if(pPgOb->GetHiddenFlag()){
                        iIndex++;
                        iCellInfo++;
                        continue;
                    }
                    iIndex++;
                    aCellInfo[iCellInfo++].SetPgOb(pPgOb);
                }
                else {
                    aCellInfo[iCellInfo++].SetPgOb(NULL);
                }
            }
            // skip captions
            continue;
        }
        for (iColHead=0 ; iColHead<iColHeadSize ; iColHead++) {
            CRowColPgOb* pColHead = aColHead[iColHead];
            CPgOb* pPgOb=&aCell[iIndex];
            if(pPgOb->GetHiddenFlag()){
                iIndex++;
                iCellInfo++;
                continue;
            }
            iIndex++;
            aCellInfo[iCellInfo++].SetPgOb(pPgOb);
        }
    }


    /////////////////////////////////////////////////////////////////////////////////
    // re-arrange data cell values so that they correspond to our table layout
    if (bDataAvailable) {
        ApplyTransformation(aColHead, aStub, aCellInfo);
    }

    /////////////////////////////////////////////////////////////////////////////////
    // assign cell texts (for cells with data)
    if (bDataAvailable) {
        for (int iCellInfo=0 ; iCellInfo<aCellInfo.GetSize() ; iCellInfo++) {
            CTabPrtViewCellInfo& x=aCellInfo[iCellInfo];
            CPgOb* pPgOb=x.GetPgOb();
            if(!pPgOb || pPgOb->GetType() != PGOB_DATACELL){
                continue;
            }

			pPgOb->SetIsZeroCell(false); // assume it isn't zero, set to true if it is later

            double dData=x.GetData();
            CDataCellFmt* pDataCellFmt=DYNAMIC_DOWNCAST(CDataCellFmt,pPgOb->GetFmt());
            if(!pDataCellFmt || (!pPgOb && dData ==0)){
                continue; //dummmy cell
            }

            // check for custom text
            if (pDataCellFmt->IsTextCustom()) {
               pPgOb->SetText(pDataCellFmt->GetCustom().m_sCustomText);
               iDataCellIndex++;
            }
            else {
                // check for invalid value
                dData < 0  ? dAbs = -dData : dAbs = dData;
                if(dAbs <= 1.0e50){
                    pPgOb->SetText(pTbl->FormatDataCell(dData, pTabSetFmt, pDataCellFmt));
                }
                else {
                    //pPgOb->SetText("");
                    pPgOb->SetText(sZeroFill);

                }

				if(dData ==0  || dData >= 1.0e50 ) {
					pPgOb->SetIsZeroCell(true);
                }
            }
        }
    }

    /////////////////////////////////////////////////////////////////////////////////
    // measure the minimum width/height for the cell
    for (int iCell=0 ; iCell<aCell.GetSize() ; iCell++) {
        CPgOb& pgobCell=aCell[iCell];

        CRect rcMin;
        if (pgobCell.GetText().IsEmpty()) {
            rcMin.SetRectEmpty();
        }
        else {
            CDataCellFmt* pDataCellFmt=DYNAMIC_DOWNCAST(CDataCellFmt, pgobCell.GetFmt());

            // load font
            dc.SelectObject(pDataCellFmt->GetFont());

            rcMin.SetRect(0,0,10,10);
            dc.DrawText(pgobCell.GetText(), &rcMin, DT_SINGLELINE | DT_CALCRECT);
            ASSERT(rcMin.Width()>0 && rcMin.Height()>0 && rcMin.Width()<MAXINT && rcMin.Height()<MAXINT);

            // unload font
            dc.SelectStockObject(OEM_FIXED_FONT);
        }

        // add vertical indentation (padding)
        rcMin.InflateRect(CELL_PADDING_LEFT,CELL_PADDING_TOP,CELL_PADDING_RIGHT,CELL_PADDING_BOTTOM);
        pgobCell.SetMinResize(rcMin.Size());

        CSize szMax(pgobCell.GetMaxResize());
        if (szMax.cy<rcMin.Height()) {
            szMax.cy=rcMin.Height();  // csc 11/24/04
            pgobCell.SetMaxResize(szMax);
        }
    }

    /////////////////////////////////////////////////////////////////////////////////
    // measure cells
    if (bDataAvailable) {
        // make sure that each column head is at least as wide as the widest cell in its column and
        // that each stub is at least as high as the highest cell in its row ...
        iDataCellIndex=0;
        for (iStub=0 ; iStub<aStub.GetSize() ; iStub++) {
            CRowColPgOb* pStub = aStub[iStub];

            if (pStub->GetType()!=PGOB_STUB) {
                // skip captions
				if(pStub->GetType()==PGOB_CAPTION || pStub->GetType()==PGOB_READER_BREAK){
                    bool bFieldSpanner=(pStub->GetFmt()->GetSpanCells()==SPAN_CELLS_YES);
                    if(bFieldSpanner){//No additional cells /pgobs if field spanner is yes
                        //This is done to display the spanned text correctly
                        continue;
                    }
				}

				// for all others, skip over the cells
				iDataCellIndex += aColHead.GetSize();
                continue;
            }

			bool bWholeRowZero = true;
            for (iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
                CRowColPgOb* pColHead = aColHead[iColHead];

                // get the cell for this stub/column head combination...
                CPgOb& pgobCell = aCell[iDataCellIndex++];


                // check width
                if (pgobCell.GetMinResize().cx>pColHead->GetMinResize().cx) {
                    pColHead->GetMinResize().cx = pgobCell.GetMinResize().cx;
                }

                // check height
                if (pgobCell.GetMinResize().cy>pStub->GetMinResize().cy) {
                    pStub->GetMinResize().cy = pgobCell.GetMinResize().cy;
                }

				// check zero row
				if (!pgobCell.GetHiddenFlag() && !pgobCell.GetIsZeroCell()) {
					bWholeRowZero = false;
				}
			}

			if (bWholeRowZero) {
				CDataCellFmt* pStubFmt=DYNAMIC_DOWNCAST(CDataCellFmt,pStub->GetFmt());
				if (pStubFmt->GetZeroHidden()) {
					pStub->SetHideRowForAllZeroCells(true);
				}
				else {
					pStub->SetHideRowForAllZeroCells(false);
				}
			}
			else {
				pStub->SetHideRowForAllZeroCells(false);
			}
        }

		// second pass to hide cells from all zero rows
	    iDataCellIndex=0;
        for (iStub=0 ; iStub<aStub.GetSize() ; iStub++) {
            CRowColPgOb* pStub = aStub[iStub];

            if (pStub->GetType()!=PGOB_STUB) {
                // skip captions
				if(pStub->GetType()==PGOB_CAPTION || pStub->GetType()==PGOB_READER_BREAK){
                    bool bFieldSpanner=(pStub->GetFmt()->GetSpanCells()==SPAN_CELLS_YES);
                    if(bFieldSpanner){//No additional cells /pgobs if field spanner is yes
                        //This is done to display the spanned text correctly
                        continue;
                    }
				}

				// for all others, skip over the cells
				iDataCellIndex += aColHead.GetSize();
                continue;
            }

            for (iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
                CRowColPgOb* pColHead = aColHead[iColHead];

                // get the cell for this stub/column head combination...
                CPgOb& pgobCell = aCell[iDataCellIndex++];

				if (pStub->GetHideRowForAllZeroCells()) {
					pgobCell.SetHiddenFlag(true);
				}
			}
        }
	}

    //Remove "Hidden" PgOb
    for(int iIndex = aCell.GetSize()-1 ; iIndex >=0 ; iIndex--){
        CPgOb& pgobCell = aCell[iIndex];
        if(pgobCell.GetHiddenFlag() == HIDDEN_YES){
            aCell.RemoveAt(iIndex);
        }
    }

}
void CTabPrtView::DoRowTransformation(CArray<CRowColPgOb*, CRowColPgOb*>& aColHead,CArray<CRowColPgOb*, CRowColPgOb*>& aStub, CArray<CTabPrtViewCellInfo,
                                      CTabPrtViewCellInfo&>& aCellInfo,int iStartStub)
{
    CTabValue* pTabVal = NULL;
    CTabVar* pTabVar = NULL; //

    ASSERT(aCellInfo.GetSize() == aColHead.GetSize()* aStub.GetSize());

    ASSERT(iStartStub < aStub.GetSize());
    CRowColPgOb* pStartStub = aStub[iStartStub];

    if(pStartStub->GetType() == PGOB_CAPTION) {
        pTabVar = DYNAMIC_DOWNCAST(CTabVar,pStartStub->GetTblBase());
        ASSERT(pTabVar);
    }
    else if(pStartStub->GetType() == PGOB_STUB) {
        pTabVar = DYNAMIC_DOWNCAST(CTabVar,pStartStub->GetParent()->GetTblBase());
        ASSERT(pTabVar);
    }
    else {//we get row only which are either captions or stubs
        ASSERT(FALSE);
    }
    ASSERT(pTabVar);
    CTabVar* pRoot = pTabVar;
    while(pRoot = pRoot->GetParent()){
        if(pRoot->GetParent() == NULL)
            break;
    }
    ASSERT(pRoot);
    bool bIsOneRowVar = pRoot->GetNumChildren() == 1 && pRoot->GetChild(0)->GetNumChildren() ==0;
    //Get the percent position
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
 //   CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    const CFmtReg& fmtReg=pSet->GetFmtReg();

    CTallyFmt* pTallyFmt = pTabVar->GetTallyFmt();
    CTallyFmt* pDefTallyFmt =DYNAMIC_DOWNCAST(CTallyFmt,pSet->GetFmtReg().GetFmt(FMT_ID_TALLY_ROW));
    (!pTallyFmt) ? pTallyFmt =pDefTallyFmt: pTallyFmt;

    ASSERT(pTallyFmt);

    CArray<CTallyFmt::InterleavedStatPair> aInterleavedStats;
    pTallyFmt->GetInterleavedStats(aInterleavedStats);
    for (int iPair = 0; iPair < aInterleavedStats.GetCount(); ++ iPair) {
        const CTallyFmt::InterleavedStatPair& p = aInterleavedStats.GetAt(iPair);
        int iFirstStat = p.m_first;
        int iSecondStat = p.m_second;


        int iStartRow = iStartStub;
        int iMaxRows = aStub.GetSize();

        CRowColPgOb* pStub = NULL;
        CTabVar* pParentTabVar = NULL;

        CArray<double,double> arrFirsts;
        CArray<double,double> arrSeconds;
        int iCellIndex =0;

        CArray<CTabPrtViewCellInfo*, CTabPrtViewCellInfo*> arrDataCells;
        int iNumCols = aColHead.GetSize();
        int iIndex = iStartRow;
        bool bAreaBreak =false;
        bool bVarDone = false;
        while(iIndex < aStub.GetSize()) {
            if(bAreaBreak){
                if(!bIsOneRowVar){//done with this area for this var
                    break;
                }
                else {//proceed to the next area for this var
                    iIndex++;
                    bAreaBreak = false;
                }
            }
            else if(bVarDone){
                break;
            }

            arrFirsts.RemoveAll();
            arrSeconds.RemoveAll();
            arrDataCells.RemoveAll();
            iStartRow = iIndex;
            for(iIndex =iStartRow; iIndex < aStub.GetSize(); iIndex++){
                ASSERT(pTabVar);
                pStub = aStub[iIndex];
                if(pStub->GetTblBase()->GetText().CompareNoCase(AREA_TOKEN) ==0){
                    bAreaBreak = true;
                    break;
                }
                pTabVal = DYNAMIC_DOWNCAST(CTabValue,pStub->GetTblBase());
                if(!pTabVal){
                    continue;
                }
                pParentTabVar = DYNAMIC_DOWNCAST(CTabVar,pStub->GetParent()->GetTblBase());
                if(!pParentTabVar || pParentTabVar != pTabVar){
                    bVarDone = true;
                    break;
                }

                if( pTabVal->GetStatId() == iFirstStat || pTabVal->GetStatId() == iSecondStat) {
                    //Add to Array Of Counts
                    for(int iCol = 0; iCol <iNumCols; iCol++){
                        arrDataCells.Add( &aCellInfo[iIndex*iNumCols+iCol]);
                        iCellIndex++;
                    }
                }
            }
            //Separate Cells into firsts and seconds
            int iCell = 0;
            for(iCell =0; iCell < arrDataCells.GetSize() /2 ; iCell++){
                arrFirsts.Add(arrDataCells[iCell]->GetData());
            }
            for(iCell =iCell; iCell < arrDataCells.GetSize() ; iCell++){
                arrSeconds.Add(arrDataCells[iCell]->GetData());
            }
            ASSERT(arrFirsts.GetSize() == arrSeconds.GetSize());
            bool bFromFirsts = true;

            double dTemp =0;
            int iFirstsIndex=0;
            int iSecondsIndex=0;
            iCell =0;
            while(iCell < arrDataCells.GetSize()){
                for(int iCol =0; iCol < iNumCols; iCol++){
                    if(bFromFirsts){
                        arrDataCells[iCell]->SetData(arrFirsts[iFirstsIndex]);
                        iFirstsIndex++;
                    }
                    else {
                        arrDataCells[iCell]->SetData(arrSeconds[iSecondsIndex]);
                        iSecondsIndex++;
                    }
                    iCell++;
                }
                bFromFirsts = ! bFromFirsts;
            }
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////
//
//        ApplyTransformation
//
// Re-organizes cell data values from the order that Serpro produces, into the order
// that our table wants.
//
///////////////////////////////////////////////////////////////////////////////////////
void CTabPrtView::ApplyTransformation(CArray<CRowColPgOb*, CRowColPgOb*>& aColHead, CArray<CRowColPgOb*, CRowColPgOb*>& aStub, CArray<CTabPrtViewCellInfo, CTabPrtViewCellInfo&>& aCellInfo)
{

    int iIndex =0;
    int iNumcols = aColHead.GetSize();

    // remove excluded stubs
    for (int iStub=0 ; iStub<aStub.GetSize() ; iStub++) {
        ASSERT(aStub[iStub]->GetType()!=PGOB_NOTINCLUDED); // these should all be gone by now
    }


    ///////////////////////////////////////////////////////////////////////////////////////////
    ///BEGIN ROW TRANSFORMATION
    //////////////////////////////////////////////////////////////////////////////////////////

    //if you have only one element in the row then start the process from the stub
    int iTbl = 0; //for now ../later  on pass it as a parameter
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}

    CTable* pTbl = pSet->GetTable(iTbl);
    if(pTbl->GetRowRoot()->GetNumChildren() == 1 &&pTbl->GetRowRoot()->GetChild(0)->GetNumChildren() ==0) {
        CTabVar* pTabVar = pTbl->GetRowRoot()->GetChild(0);
        //bHasCountsNPercents = pSet->HasCountsNPercent(pTabVar);
        int iStartStub =-1;
        for(int iIndex =0; iIndex < aStub.GetSize(); iIndex++){
            CTabVar* pTabVar = DYNAMIC_DOWNCAST(CTabVar,aStub[iIndex]->GetTblBase());
            CTabValue* pTabValue = DYNAMIC_DOWNCAST(CTabValue,aStub[iIndex]->GetTblBase());
            if(pTabVar || pTabValue){
                iStartStub = iIndex;
                break;
            }
        }
        if(iStartStub >= 0) {
            DoRowTransformation(aColHead,aStub,aCellInfo,iStartStub);
        }
    }
    else {//Do the transformation on rows for more the one var in rows
        int iRow =0;
        int iStartRow =0;
        for(iRow = iStartRow; iRow < aStub.GetSize(); iRow++) {
            //if it is row group continue;
            CRowColPgOb* pStub = aStub[iRow];
            if ( pStub->GetType()==PGOB_CAPTION ) { //process only Row Col group
                CTabVar* pTabVar = DYNAMIC_DOWNCAST(CTabVar,pStub->GetTblBase());
               // ASSERT(pTabVar);
                if(!pTabVar || pTabVar->GetNumChildren()>0){
                    continue ;
                }
                else {
                    DoRowTransformation(aColHead,aStub,aCellInfo,iRow);
                }

            }
            else {
                continue;
            }

        }
    }
    //////////////////////////////////////////////////////////////////////////////////////////
    ///END ROW TRANSFORMATION
    //////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////
    //BEGIN COLUMN TRANSFORMATION
    //////////////////////////////////////////////////////////////////////////////////////////

    //Do the transformation on columns
    for(iIndex = 0; iIndex < iNumcols;iIndex++){
        CRowColPgOb* pColHead = aColHead[iIndex];
        ASSERT(pColHead);

        CTabVar* pTabVar = DYNAMIC_DOWNCAST(CTabVar,pColHead->GetParent()->GetTblBase());
        CTallyFmt* pTallyFmt = pTabVar->GetTallyFmt();
        if(!pTallyFmt){
            pTallyFmt = DYNAMIC_DOWNCAST(CTallyFmt,pSet->GetFmtReg().GetFmt(FMT_ID_TALLY_COL));
        }

        CArray<CTallyFmt::InterleavedStatPair> aInterleavedStats;
        pTallyFmt->GetInterleavedStats(aInterleavedStats);
        for (int iPair = 0; iPair < aInterleavedStats.GetCount(); ++ iPair) {

            const CTallyFmt::InterleavedStatPair& p = aInterleavedStats.GetAt(iPair);
            int iFirstStat = p.m_first;
            int iSecondStat = p.m_second;
            int iChild = 0;
            for (iChild = 0; iChild < pColHead->GetParent()->GetNumChildren(); iChild++){
                CTabValue* pTabVal = DYNAMIC_DOWNCAST(CTabValue,pColHead->GetParent()->GetChild(iChild)->GetTblBase());
                ASSERT(pTabVal);
                if (pTabVal && pTabVal->GetStatId() == iFirstStat) {
                    break;
                }
            }
            ASSERT(iChild < pColHead->GetParent()->GetNumChildren());
            int iStartChild = iChild;
            for (iChild = pColHead->GetParent()->GetNumChildren()-1; iChild >=0  ; iChild--){
                CTabValue* pTabVal = DYNAMIC_DOWNCAST(CTabValue,pColHead->GetParent()->GetChild(iChild)->GetTblBase());
                ASSERT(pTabVal);
                if (pTabVal && pTabVal->GetStatId() == iSecondStat) {
                    break;
                }
            }
            ASSERT(iChild >= 0);
            int iEndChild = iChild;

            //Get first half  //Store the  firsts in firsts array

            CArray<double,double&> arrFirsts;
            double dValue =0;

            int iTotalCat = (iEndChild-iStartChild+1)/2;
            int iFirstsCol = -1;
            for(iFirstsCol = iIndex+iStartChild; iFirstsCol < iIndex+iStartChild+iTotalCat ;iFirstsCol++){
                for (int iRow = 0; iRow < aStub.GetSize() ; iRow++) {
                    ASSERT(iRow*iNumcols+iFirstsCol<aCellInfo.GetSize());
                    CTabPrtViewCellInfo printViewCell =  aCellInfo[iRow*iNumcols+iFirstsCol];
                    double dValue = printViewCell.GetData();
                    arrFirsts.Add(dValue);
                }
            }
            //Get next half  //store the  seconds in seconds array
            CArray<double,double&> arrSeconds;
            for(int iSecondsCol = iFirstsCol; iSecondsCol < iFirstsCol+iTotalCat ;iSecondsCol++){
               for (int iRow = 0; iRow < aStub.GetSize() ; iRow++) {
                    ASSERT(iRow*iNumcols+iSecondsCol<aCellInfo.GetSize());
                    CTabPrtViewCellInfo printViewCell =  aCellInfo[iRow*iNumcols+iSecondsCol];
                    double dValue = printViewCell.GetData();
                    arrSeconds.Add(dValue);
                }

            }
            //Now arrange using alternate mode;
            int iFirstsIndex=0;
            int iSecondsIndex=0;
            int iLocalIndex=0;
            for(int iCountsCol = iIndex+iStartChild; iCountsCol < iIndex+iStartChild+iTotalCat*2 ;iCountsCol++){
                for (int iRow = 0; iRow < aStub.GetSize() ; iRow++) {
                    CUGCell cellGrid;
                    CIMSAString sVal;
                    if((iLocalIndex)% 2 == 0){
                       CTabPrtViewCellInfo& printViewCell =  aCellInfo[iRow*iNumcols+iCountsCol];
                       printViewCell.SetData(arrFirsts[iFirstsIndex]);
                       iFirstsIndex++;
                    }
                    else{
                       CTabPrtViewCellInfo& printViewCell =  aCellInfo[iRow*iNumcols+iCountsCol];
                       printViewCell.SetData(arrSeconds[iSecondsIndex]);
                       iSecondsIndex++;
                    }
                }
                iLocalIndex++;
            }

        }

        iIndex += pColHead->GetParent()->GetNumChildren()-1;
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    //END COLUMN TRANSFORMATION
    //////////////////////////////////////////////////////////////////////////////////////////
}


/////////////////////////////////////////////////////////////////////////////////
//
//        GetDataCellFormat
//
//  see also CTblGrid::GetFmt4DataCell (same function, but for the grid)
//
/////////////////////////////////////////////////////////////////////////////////
/*void CTabPrtView::GetDataCellFormat(CRowColPgOb* pColHead,CRowColPgOb* pStub, CDataCellFmt* pDataCellFmt, int iColHead, int iPanel) const
{
    ASSERT_VALID(pColHead);
    ASSERT_VALID(pStub);
    ASSERT_VALID(pDataCellFmt);

    // init
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    const CFmtReg& fmtReg = pDoc->GetTableSpec()->GetFmtReg();

    // captions only have to worry about lines, and we handle those separately (got to, since captions have CFmt formats, stubs have CDataCellFmt formats)
    if (pStub->GetFmt()->GetID()==FMT_ID_CAPTION) {
        // handle lines for captions ...

        // formats without defaults
        CFmt* pCaptionFmt=DYNAMIC_DOWNCAST(CFmt,pStub->GetFmt());
        CFmt* pColFmt=DYNAMIC_DOWNCAST(CFmt,pColHead->GetFmt());
        ASSERT(!pColFmt->ContainsDefaultValues());
        ASSERT(!pCaptionFmt->ContainsDefaultValues());

        // formats with defaults
        CFmt* pSourceCaptionFmt=DYNAMIC_DOWNCAST(CFmt,pStub->GetTblBase()->GetFmt());
        CFmt* pSourceColFmt=DYNAMIC_DOWNCAST(CFmt,pColHead->GetTblBase()->GetFmt());
        if (pSourceCaptionFmt==NULL) {
            // no source stub fmt specified, use the default
            pSourceCaptionFmt=pCaptionFmt;
        }
        if (pSourceColFmt==NULL) {
            // no source colhead fmt specified, use the default
            pSourceColFmt=pColFmt;
        }
        ASSERT_VALID(pSourceCaptionFmt);
        ASSERT_VALID(pSourceColFmt);

        bool bCaptionExtends = pSourceCaptionFmt->GetLinesExtend();
        bool bColExtends = pSourceColFmt->GetLinesExtend();
        bColExtends = true;
        if (bCaptionExtends || bColExtends) {
            //if it does not extend you don't need to do anything

            //Lines can't be applied on an individual cell  .So special cells line attribute
            //need not be considered
            if (bCaptionExtends)  {
                bool bTop = true;
                bool bBottom = true;
                if (!pCaptionFmt->GetLinesExtend()) {
                    bTop = false;
                    bBottom = false;
                    int iNumSiblings= pStub->GetParent()->GetNumChildren();
                    if (pStub==pStub->GetParent()->GetChild(0)) {
                        // we are the leftmost child
                        if (pStub->GetNumLeaves()==0 && pStub->GetLevel()>2) {
                            //this case covers row items more than one A+B and first stub shld not
                            //have the top line  because The spanner covers it //Other wise you
                            //will get top lines for both spanner and stub
                            bTop =false;
                        }
                        else {
                            bTop = true;
                        }
                    }
                    if (pStub==pStub->GetParent()->GetChild(iNumSiblings-1)) {
                        bBottom = true;
                    }
                }
                if (bTop) {
                    pDataCellFmt->SetLineTop(pCaptionFmt->GetLineTop());
                }
                if (bBottom) {
                    pDataCellFmt->SetLineBottom(pCaptionFmt->GetLineBottom());
                }
            }
            if (bColExtends) {
                // use col val if col extends and not row
                bool bLeft = true;
                bool bRight = true;
                if (!pColFmt->GetLinesExtend()) {
                    bLeft = false;
                    bRight = false;
                    int iNumSiblings=pColHead->GetParent()->GetNumChildren();
                    if (pColHead==pColHead->GetParent()->GetChild(0)) {
                        bLeft = true;
                    }
                    if (pColHead==pColHead->GetParent()->GetChild(iNumSiblings-1)) {
                        bRight = true;
                    }
                }
                if (bLeft) {
                    pDataCellFmt->SetLineLeft(pColFmt->GetLineLeft());
                }
                if (bRight) {
                    pDataCellFmt->SetLineRight(pColFmt->GetLineRight());
                }
            }
        }

        // that's all we need to do with captions ...
        return;
    }

    // figure out if this data cell has a special cell (only attempt for data rows)
    CSpecialCell* pSpecialCell=NULL;
    CDataCellFmt* pSpecialCellFmt=NULL;
    if (pStub->GetNumChildren()==0) {
//        int iPanel=GetRowPanel(pStub);
        CTabValue* pTabVal=DYNAMIC_DOWNCAST(CTabValue, pStub->GetTblBase());
        pSpecialCell=pTabVal->FindSpecialCell(iPanel, iColHead+1);
    }
    if (pSpecialCell) {
        ASSERT_VALID(pSpecialCell);
        pSpecialCellFmt=pSpecialCell->GetDerFmt();
    }

    // formats without defaults
    CDataCellFmt* pStubFmt=DYNAMIC_DOWNCAST(CDataCellFmt,pStub->GetFmt());
    CDataCellFmt* pColFmt=DYNAMIC_DOWNCAST(CDataCellFmt,pColHead->GetFmt());
    ASSERT(!pColFmt->ContainsDefaultValues());
    ASSERT(!pStubFmt->ContainsDefaultValues());

    // formats with defaults
    CDataCellFmt* pSourceStubFmt=DYNAMIC_DOWNCAST(CDataCellFmt,pStub->GetTblBase()->GetFmt());
    CDataCellFmt* pSourceColFmt=DYNAMIC_DOWNCAST(CDataCellFmt,pColHead->GetTblBase()->GetFmt());
    if (pSourceStubFmt==NULL) {
        // no source stub fmt specified, use the default
        pSourceStubFmt=pStubFmt;
    }
    if (pSourceColFmt==NULL) {
        // no source colhead fmt specified, use the default
        pSourceColFmt=pColFmt;
    }
    ASSERT_VALID(pSourceStubFmt);
    ASSERT_VALID(pSourceColFmt);

    ////////////////////////////////////////////////////////////////////////////////
    // analyze all of the fmt attributes, depending on whether or not (and how) they extend from
    // rows/columns into cells.  Here's how:
    //
    //   ATTRIB     COLHEAD  (EXTENDS)    DATACELL(AVAILABLE)       SPANNER/CAPTION(EXTENDS)
    //-----------------------------------------------------------------------------------------
    //   ALIGN      NEVER                   YES                      NO
    //   LINES      YES/NO                  NO                       YES/NO
    //   INDENT     YES/NO                  YES                      NO
    //   HIDDEN     ALWAYS                  NO                       NO
    //   FONT       YES/NO                  YES                      NO
    //   COLOR      YES/NO                  YES                      NO
    //   NUMDECIMALS  ALWAYS                YES                      NO
    //   CUSTOM-TEXT NEVER                  YES                      NO
    //
    bool bRowCust;     // true if the row is customized (ie, non-default)
    bool bColCust;     // true if the row is customized (ie, non-default)
    bool bRowExtends;  // true if a row attribute extends into cells
    bool bColExtends;  // true if a column attribute extends into cells

    ////////////////////////////////////////////////////////////////////////////////
    // alignment
    if (pSpecialCellFmt && pSpecialCellFmt->GetHorzAlign()!=HALIGN_DEFAULT)  {
        // use special cell's horizontal alignment
        pDataCellFmt->SetHorzAlign(pSpecialCellFmt->GetHorzAlign());
    }
    // *** otherwise, do nothing; alignment NEVER extends into cells

    if (pSpecialCellFmt && pSpecialCellFmt->GetVertAlign()!=VALIGN_DEFAULT)  {
        // use special cell's vertical alignment
        pDataCellFmt->SetVertAlign(pSpecialCellFmt->GetVertAlign());
    }
    // *** otherwise, do nothing; alignment NEVER extends into cells

    ////////////////////////////////////////////////////////////////////////////////
    // custom text
    if (pSpecialCellFmt && pSpecialCellFmt->IsTextCustom())  {
        // use special cell's horizontal alignment
        pDataCellFmt->SetCustom(pSpecialCellFmt->GetCustom());
    }
    // *** do nothing; custom text NEVER extends into cells

    ////////////////////////////////////////////////////////////////////////////////
    // text color
    bRowCust = (pSourceStubFmt->IsTextColorCustom() && pSourceStubFmt->GetIndex()!=0);
    bColCust = (pSourceColFmt->IsTextColorCustom() && pSourceColFmt->GetIndex()!=0);
    bRowExtends = pStubFmt->GetTextColorExtends();
    bColExtends = pColFmt->GetTextColorExtends();

    if (pSpecialCellFmt && pSpecialCellFmt->IsTextColorCustom())  {
        // use special cell's text color
        pDataCellFmt->SetTextColor(pSpecialCellFmt->GetTextColor());
    }
    else if (bRowExtends || bColExtends) {
        // if it does not extend you don't need to do anything
        if (bRowCust && bColCust && pStubFmt->GetTextColor().m_rgb != pColFmt->GetTextColor().m_rgb) {
            ASSERT(pSpecialCellFmt);
        }
        if (bRowExtends && !bColExtends) {
            //use rows val if row extends and not col
            pDataCellFmt->SetTextColor(pStubFmt->GetTextColor());
        }
        else if (!bRowExtends && bColExtends) {
            // use col val if col extends and not row
            pDataCellFmt->SetTextColor(pColFmt->GetTextColor());
        }
        else if (bRowCust) {
            // use rows val if  row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetTextColor(pStubFmt->GetTextColor());
        }
        else if (bColCust) {
            // use Columns val if col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            pDataCellFmt->SetTextColor(pColFmt->GetTextColor());
        }
        else {
            // none of them are custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetTextColor(pStubFmt->GetTextColor());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // fill color
    bRowCust = (pSourceStubFmt->IsFillColorCustom() && pSourceStubFmt->GetIndex()!=0);
    bColCust = (pSourceColFmt->IsFillColorCustom() && pSourceColFmt->GetIndex()!=0);
    bRowExtends = pStubFmt->GetFillColorExtends();
    bColExtends = pColFmt->GetFillColorExtends();

    if (pSpecialCellFmt && pSpecialCellFmt->IsFillColorCustom())  {
        // use special cell's text color
        pDataCellFmt->SetFillColor(pSpecialCellFmt->GetFillColor());
    }
    else if (bRowExtends || bColExtends) {
        // if it does not extend you don't need to do anything
        if (bRowCust && bColCust && pStubFmt->GetFillColor().m_rgb != pColFmt->GetFillColor().m_rgb) {
            ASSERT(pSpecialCellFmt);
        }
        else if (bRowExtends && !bColExtends) {
            //use rows val if row extends and not col
            pDataCellFmt->SetFillColor(pStubFmt->GetFillColor());
        }
        else if (!bRowExtends && bColExtends) {
            // use col val if col extends and not row
            pDataCellFmt->SetFillColor(pColFmt->GetFillColor());
        }
        else if (bRowCust) {
            // use rows val if  row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetFillColor(pStubFmt->GetFillColor());
        }
        else if (bColCust) {
            // use Columns val if col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            pDataCellFmt->SetFillColor(pColFmt->GetFillColor());
        }
        else {
            // none of them are custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetFillColor(pStubFmt->GetFillColor());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // font
    bRowCust = (pSourceStubFmt->IsFontCustom() && pSourceStubFmt->GetIndex()!=0);
    bColCust = (pSourceColFmt->IsFontCustom() && pSourceColFmt->GetIndex()!=0);
    bRowExtends = pStubFmt->GetFontExtends();
    bColExtends = pColFmt->GetFontExtends();

    if (pSpecialCellFmt && pSpecialCellFmt->IsFontCustom()) {
        // use special cell's font
        pDataCellFmt->SetFont(pSpecialCellFmt->GetFont());
        //SAVY&& fixed the "diminishing bug"
        //LPToPoints(pSpecialCellFmt, GetLogPixelsY());
        LPToPoints(pDataCellFmt, GetLogPixelsY());
        PointsToTwips(pDataCellFmt);
    }
    else if (bRowExtends || bColExtends) {
        // if it does not extend you dont need to do anything
        LOGFONT lfRow ,lfCol;
        pStubFmt->GetFont()->GetLogFont(&lfRow);
        pColFmt->GetFont()->GetLogFont(&lfCol);
        int iRet = memcmp(&lfRow,&lfCol,sizeof(LOGFONT));
        if (bRowCust && bColCust && iRet==0) {
            ASSERT (pSpecialCellFmt);
        }
        if (bRowExtends && !bColExtends) {
            // use rows val if row extends and not col
            pDataCellFmt->SetFont(pStubFmt->GetFont());
        }
        else if (!bRowExtends && bColExtends) {
            // use col val if col extends and not row
            pDataCellFmt->SetFont(pColFmt->GetFont());
        }
        else if (bRowCust)  {
            // use rows val if row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetFont(pStubFmt->GetFont());
        }
        else if (bColCust)  {
            //use Columns val if col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            pDataCellFmt->SetFont(pColFmt->GetFont());
        }
        else {
            // none of them are custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetFont(pStubFmt->GetFont());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // number of decimals
    bRowCust = (pSourceStubFmt->GetNumDecimals()!=NUM_DECIMALS_DEFAULT && pSourceStubFmt->GetIndex()!=0);
    bColCust = (pSourceColFmt->GetNumDecimals()!=NUM_DECIMALS_DEFAULT && pSourceColFmt->GetIndex()!=0);

    // decimals always extend
    bRowExtends = true;
    bColExtends = true;

    if (pSpecialCellFmt && pSpecialCellFmt->GetNumDecimals()!=NUM_DECIMALS_DEFAULT)  {
        // use special cell's number of decimals
        pDataCellFmt->SetNumDecimals(pSpecialCellFmt->GetNumDecimals());
    }
    else if (bRowExtends || bColExtends) {
        // if it does not extend you dont need to do anything
        if (bRowCust && bColCust &&  pStubFmt->GetNumDecimals()!=pColFmt->GetNumDecimals()) {
            ASSERT(pSpecialCellFmt);
        }
        if (bRowExtends && !bColExtends) {
            //use rows val if row extends and not col
            pDataCellFmt->SetNumDecimals(pStubFmt->GetNumDecimals());
        }
        else if (!bRowExtends && bColExtends) {
            // use col val if col extends and not row
            pDataCellFmt->SetNumDecimals(pColFmt->GetNumDecimals());
        }
        else if (bRowCust)  {
            // use rows val if row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetNumDecimals(pStubFmt->GetNumDecimals());
        }
        else if (bColCust)  {
            // use Columns val if col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            pDataCellFmt->SetNumDecimals(pColFmt->GetNumDecimals());
        }
        else {
            // none of them are custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetNumDecimals(pStubFmt->GetNumDecimals());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // hidden always extends
    bRowExtends = true;
    bColExtends = true;
    bRowCust = (pSourceStubFmt->GetHidden()!=HIDDEN_DEFAULT && pSourceStubFmt->GetIndex()!=0);
    bColCust = (pSourceColFmt->GetHidden()!=HIDDEN_DEFAULT && pSourceColFmt->GetIndex()!=0);

    if (bRowExtends || bColExtends) {
        // if it does not extend you dont need to do anything
        if (bRowCust && bColCust &&  pStubFmt->GetHidden()!=pColFmt->GetHidden()) {
            ASSERT(pSpecialCellFmt);
        }
        // hidden isn't used in special cells
//        if (pSpecialCellFmt && pSpecialCellFmt->GetHidden()!=HIDDEN_DEFAULT)  {
//            // use special cell's hidden
//            ASSERT(pSpecialCellFmt->GetHidden()!=HIDDEN_NOT_APPL);
//            pDataCellFmt->SetHidden(pSpecialCellFmt->GetHidden());
//        }
        if(pSpecialCellFmt){ //hidden is not appl for data cells . the attributes is infered from stub/colhead
            if(pStubFmt->GetHidden() == HIDDEN_YES || pColFmt->GetHidden() == HIDDEN_YES){
                pDataCellFmt->SetHidden(HIDDEN_YES);
            }
            else {
                pDataCellFmt->SetHidden(HIDDEN_NO);
            }
        }
        else if (bRowExtends && !bColExtends) {
            // use rows val if row extends and not col
            pDataCellFmt->SetHidden(pStubFmt->GetHidden());
        }
        else if (!bRowExtends && bColExtends) {
            // use col val if col extends and not row
            pDataCellFmt->SetHidden(pColFmt->GetHidden());
        }
        else if (bRowCust)  {
            // use rows val if row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetHidden(pStubFmt->GetHidden());
        }
        else if (bColCust)  {
            // use Columns val if col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            pDataCellFmt->SetHidden(pColFmt->GetHidden());
        }
        else {
            // none of them are custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetHidden(pStubFmt->GetHidden());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // left indent
    bRowCust = (pSourceStubFmt->IsIndentCustom(LEFT) && pSourceStubFmt->GetIndex()!=0);
    bColCust = pSourceColFmt->IsIndentCustom(LEFT) && pSourceColFmt->GetIndex()!=0;

    bRowExtends = pStubFmt->GetIndentationExtends();
    bColExtends = pColFmt->GetIndentationExtends();

    if (pSpecialCellFmt && pSpecialCellFmt->IsIndentCustom(LEFT)) {
        // use special cell's indent
        pDataCellFmt->SetIndent(LEFT, pSpecialCellFmt->GetIndent(LEFT));
    }
    else if (bRowExtends || bColExtends) {
        // if it does not extend you dont need to do anything
        if (bRowCust && bColCust && pStubFmt->GetIndent(LEFT) != pColFmt->GetIndent(LEFT)) {
            ASSERT(pSpecialCellFmt);
        }
        if (bRowExtends && !bColExtends) {
            // use rows val  if row extends and not col
            pDataCellFmt->SetIndent(LEFT,pStubFmt->GetIndent(LEFT));
        }
        else if (!bRowExtends && bColExtends) {
            // use col val if col extends and not row
            pDataCellFmt->SetIndent(LEFT,pColFmt->GetIndent(LEFT));
        }
        else if (bRowCust)  {
            // use rows val if  row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetIndent(LEFT,pStubFmt->GetIndent(LEFT));
        }
        else if (bColCust)  {
            // use Columns val if col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            pDataCellFmt->SetIndent(LEFT,pColFmt->GetIndent(LEFT));
        }
        else {
            // none of them are custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetIndent(LEFT,pStubFmt->GetIndent(LEFT));
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // right indent
    bRowCust = (pSourceStubFmt->IsIndentCustom(RIGHT) && pSourceStubFmt->GetIndex()!=0);
    bColCust = pSourceColFmt->IsIndentCustom(RIGHT) && pSourceColFmt->GetIndex()!=0;

    bRowExtends = pStubFmt->GetIndentationExtends();
    bColExtends = pColFmt->GetIndentationExtends();

    if (pSpecialCellFmt && pSpecialCellFmt->IsIndentCustom(RIGHT)) {
        // use special cell's indent
        pDataCellFmt->SetIndent(RIGHT, pSpecialCellFmt->GetIndent(RIGHT));
    }
    else if (bRowExtends || bColExtends) {
        // if it does not extend you dont need to do anything
        if (bRowCust && bColCust && pStubFmt->GetIndent(RIGHT) != pColFmt->GetIndent(RIGHT)) {
            ASSERT(pSpecialCellFmt);
        }
        if (bRowExtends && !bColExtends) {
            // use rows val  if row extends and not col
            pDataCellFmt->SetIndent(RIGHT,pStubFmt->GetIndent(RIGHT));
        }
        else if (!bRowExtends && bColExtends) {
            // use col val if col extends and not row
            pDataCellFmt->SetIndent(RIGHT,pColFmt->GetIndent(RIGHT));
        }
        else if (bRowCust) {
            // use rows val if row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetIndent(RIGHT,pStubFmt->GetIndent(RIGHT));
        }
        else if (bColCust) {
            // use Columns val  if  col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            pDataCellFmt->SetIndent(RIGHT,pColFmt->GetIndent(RIGHT));
        }
        else {
            // none of them are custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetIndent(RIGHT,pStubFmt->GetIndent(RIGHT));
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // lines
    bRowCust = false; //Lines are independent of col (top/bottom)
    bColCust = false; //lines are independent of row (left/right)
    bRowExtends = pSourceStubFmt->GetLinesExtend();
    bColExtends = pSourceColFmt->GetLinesExtend();
    bColExtends = true;
    if (bRowExtends || bColExtends) {
        //if it does not extend you don't need to do anything

        //Lines can't be applied on an individual cell  .So special cells line attribute
        //need not be considered
        if (bRowExtends)  {
            bool bTop = true;
            bool bBottom = true;
            if (!pStubFmt->GetLinesExtend()) {
                bTop = false;
                bBottom = false;
                int iNumSiblings= pStub->GetParent()->GetNumChildren();
                if (pStub==pStub->GetParent()->GetChild(0)) {
                    // we are the leftmost child
                    if (pStub->GetNumLeaves()==0 && pStub->GetLevel()>2) {
                        //this case covers row items more than one A+B and first stub shld not
                        //have the top line  because The spanner covers it //Other wise you
                        //will get top lines for both spanner and stub
                        bTop =false;
                    }
                    else {
                        bTop = true;
                    }
                }
                if (pStub==pStub->GetParent()->GetChild(iNumSiblings-1)) {
                    bBottom = true;
                }
            }
            if (bTop) {
                pDataCellFmt->SetLineTop(pStubFmt->GetLineTop());
            }
            if (bBottom) {
                pDataCellFmt->SetLineBottom(pStubFmt->GetLineBottom());
            }
        }
        if (bColExtends) {
            // use col val if col extends and not row
            bool bLeft = true;
            bool bRight = true;
            if (!pColFmt->GetLinesExtend()) {
                bLeft = false;
                bRight = false;
                int iNumSiblings=pColHead->GetParent()->GetNumChildren();
                if (pColHead==pColHead->GetParent()->GetChild(0)) {
                    bLeft = true;
                }
                if (pColHead==pColHead->GetParent()->GetChild(iNumSiblings-1)) {
                    bRight = true;
                }
            }
            if (bLeft) {
               pDataCellFmt->SetLineLeft(pColFmt->GetLineLeft());
            }
            if (bRight) {
                pDataCellFmt->SetLineRight(pColFmt->GetLineRight());
            }
        }
    }
}*/


/////////////////////////////////////////////////////////////////////////////////
//
//      GetRowPanel
//
// See also CTblGrid::GetRowPanel
//
// Call this only for data cells . Panel number is 1 based
//
/////////////////////////////////////////////////////////////////////////////////
int CTabPrtView::GetRowPanel(CArray<CRowColPgOb*, CRowColPgOb*>& aStub, int iStub) const
{
    int iPanel=0;
    CRowColPgOb* pStub1=aStub[iStub];
    CTblBase* pTblBase1=pStub1->GetTblBase();
    ASSERT(pTblBase1);

    for (int iStub2=0 ; iStub2<aStub.GetSize() ; iStub2++) {
        CRowColPgOb* pStub2=aStub[iStub2];
        CTblBase* pTblBase2=pStub2->GetTblBase();
        ASSERT(pTblBase2);

        if (pTblBase1==pTblBase2) {
            iPanel++;
        }
        if (pStub1==pStub2) {
            return iPanel;
        }
    }

    // didn't find ourselves ... something is wrong!
    ASSERT(FALSE);
    return 0;
}


/*
int CTabPrtView::GetRowPanel(CRowColPgOb* pStub) const
{
    int iPanel=0;

    ASSERT(pStub->GetNumChildren()==0); // only call this for stubs
    CRowColPgOb* pParent=pStub->GetParent();
    ASSERT(pParent->GetType()!=PGOB_ROOT);

    for (int iChild=0 ; iChild<pParent->GetNumChildren() ; iChild++) {
        CRowColPgOb* pChild=pParent->GetChild(iChild);
        if (pChild->GetTblBase()==pStub->GetTblBase()) {
            iPanel++;
        }
        if (pChild==pStub) {
            return iPanel;
        }
    }

    // something is wrong!
    ASSERT(FALSE);
    return 0;
}
*/

////////////////////////////////////////////////////////////////////////////////////
//
//        SplitSpanner
//
// Splits spanners so that different branches go on different horz pages.
// Nodes in the tree (ie, spanners) along the "split path" are duplicated so
// that they go onto both horz pages.
//
// This is done by splitting the column tree along the split path, into 2 branches.
//
// Process:
// - determine the split path, which is the path from column head to tree root
//   that touches the nodes that need to be split into a new branch and put on
//   both horz pages.
// - identify the split point
// - create a new subtree, duplicate of the one identified by the split point
// - prune the new subtree, so that it only contains a path to our section & nodes we
//   haven't visited yet (to the right) (in effect, splitting the tree into left-side and right-side)
// - remove the extra copy of pColHead from the left-side tree
// - prune unbalanced branches (if we weren't inside a spanner, then there might be a
//   branch that doesn't end in column heads)
// - graft the 2 subtrees together, forming a tree that is split into horz pages
///////////////////////////////////////////////////////////////////////////////////////
void CTabPrtView::SplitSpanner(CRowColPgOb& rcpgobColRoot, int iColHead)
{
    // build an array of column heads (leaves in the tree)
    CArray<CRowColPgOb*, CRowColPgOb*> aColHead;
    rcpgobColRoot.GetLeaves(aColHead);  // rebuild our column head array, since the tree has just changed
    CRowColPgOb* pColHead = aColHead[iColHead];

    CRowColPgOb* pHiddenParent = pColHead->GetParent();
    int iHiddenIndex = pHiddenParent->GetChildIndex(pColHead)-1;
    if(iHiddenIndex >= 0 ){//SAVY added this code .In case of A*B split when first colhead
        //is hidden .The split does not work correctly . So reassign the split point
        //to this hidden colhead. Make sure if subsequent problem happen to search
        //behind until the first non hidden colhead under this spanner is chosen
        //as the new split point.
        if(pHiddenParent->GetChild(iHiddenIndex)->GetFmt()->GetHidden() == HIDDEN_YES){
            pColHead = pHiddenParent->GetChild(iHiddenIndex);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // - determine the split path, which is the path from column head to tree root
    //   that touches the nodes that need to be split into a new branch and put on
    //   both horz pages.

    // see if we are inside a spanned set of column heads, as opposed to between spanned sections
    bool bInsideSpanner = (pColHead->GetParent()==aColHead[iColHead-1]->GetParent());

    // move up through spanners until we find the root ...
    CArray<int,int> aPath; // nodes to follow in order to traverse the path from root to our leaf
    CRowColPgOb* pSubTree=pColHead;
    if (bInsideSpanner) {
        aPath.Add(pColHead->GetParent()->GetChildIndex(pColHead));
        pSubTree = pSubTree->GetParent();
    }
    while (pSubTree->GetType()!=PGOB_ROOT) {
        aPath.Add(pSubTree->GetParent()->GetChildIndex(pSubTree));
        pSubTree = pSubTree->GetParent();
    }

    ////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////
    // identify the split point -- this is the tree rooted at pSubTree !!

    ////////////////////////////////////////////////////////////////////////////////////
    // create a new sub tree, duplicate of the one identified by the split pint
    CRowColPgOb* pNewSubTree = new CRowColPgOb(*pSubTree);

    // prune the new subtree, so that it only contains a path to our section & nodes we haven't visited yet (to the right)
    // (in effect, we're splitting the tree)
    CRowColPgOb* pPruneNewSubTree=pNewSubTree;
    CRowColPgOb* pPruneSubTree=pSubTree;
    for (int i=aPath.GetSize()-1 ; i>=0 ; i--) {
        // aPath[i] shows the trail to get to our leaf
        // for the new subtree (pPruneNewSubTree), nuke branches to the left of the path
        // for the old subtree (pPruneSubTree), nuke branches to the right of the path (or on the path)

        // prune new sub tree
        ASSERT(aPath[i]!=NONE);
        ASSERT(aPath[i]<pPruneNewSubTree->GetNumChildren());
        for (int j=0 ; j<aPath[i] ; j++) {
            pPruneNewSubTree->RemoveChildAt(0);
        }

        // prune old sub tree
        while (pPruneSubTree->GetNumChildren() > aPath[i]+1) {
            pPruneSubTree->RemoveChildAt(aPath[i]+1);
        }

        ASSERT(pPruneNewSubTree->GetNumChildren()>0);
        ASSERT(pPruneSubTree->GetNumChildren()>0);
        pPruneNewSubTree = pPruneNewSubTree->GetChild(0);  // follow the path
        pPruneSubTree = pPruneSubTree->GetChild(pPruneSubTree->GetNumChildren()-1);  // follow the path
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // remove the extra copy of pColHead from the left-side tree

    // pColHead is now on both pSubTree and pNewSubTree; remove it from pSubTree
    CRowColPgOb* pParent = pColHead->GetParent();
    for (int iChild=pParent->GetChildIndex(pColHead) ; iChild<pParent->GetNumChildren() ; ) {
        pParent->RemoveChildAt(iChild);
    }
#ifdef _DEBUG
    //Print the oldTree
    CRowColPgOb* pSaveSubTree = pNewSubTree;
    pNewSubTree = pNewSubTree->GetChild(0);
    TRACE(_T("printing new tree %s \n") , (LPCTSTR)pNewSubTree->GetText());

    for(int iIndex = 0; iIndex<pNewSubTree->GetNumChildren() ;iIndex++){
        CRowColPgOb* pChildVal = pNewSubTree->GetChild(iIndex);
        TRACE(_T("\t child --> %s\n") , (LPCTSTR)pNewSubTree->GetChild(iIndex)->GetText());
        if(pChildVal->GetNumChildren() > 0 ){
            for(int iChildVal = 0; iChildVal<pChildVal->GetNumChildren() ;iChildVal++){
                CRowColPgOb* pSubChildVal = pChildVal->GetChild(iChildVal);
                TRACE(_T("\t\t child --> %s\n ") , (LPCTSTR)pSubChildVal->GetText());
                if(pSubChildVal->GetNumChildren() > 0 ){
                    for(int iChildVal = 0; iChildVal<pSubChildVal->GetNumChildren() ;iChildVal++){
                        TRACE(_T("\t\t\t child --> %s\n ") , (LPCTSTR)pSubChildVal->GetChild(iChildVal)->GetText());
                    }
                }
            }
        }
    }

    pNewSubTree = pSaveSubTree;
    //Print before merge
    pNewSubTree = pSubTree->GetChild(pSubTree->GetNumChildren()-1);
    TRACE(_T("printing old tree before merge %s \n") , (LPCTSTR)pNewSubTree->GetText());

    for(int iIndex = 0; iIndex<pNewSubTree->GetNumChildren() ;iIndex++){
        CRowColPgOb* pChildVal = pNewSubTree->GetChild(iIndex);
        TRACE(_T("\t child --> %s\n") , (LPCTSTR)pNewSubTree->GetChild(iIndex)->GetText());
        if(pChildVal->GetNumChildren() > 0 ){
            for(int iChildVal = 0; iChildVal<pChildVal->GetNumChildren() ;iChildVal++){
                CRowColPgOb* pSubChildVal = pChildVal->GetChild(iChildVal);
                TRACE(_T("\t\t child --> %s\n ") , (LPCTSTR)pSubChildVal->GetText());
                if(pSubChildVal->GetNumChildren() > 0 ){
                    for(int iChildVal = 0; iChildVal<pSubChildVal->GetNumChildren() ;iChildVal++){
                        TRACE(_T("\t\t\t child --> %s\n ") , (LPCTSTR)pSubChildVal->GetChild(iChildVal)->GetText());
                    }
                }
            }
        }
    }
    pNewSubTree = pSaveSubTree;
    pSaveSubTree = pSubTree->GetChild(pSubTree->GetNumChildren()-1);
#endif

    ////////////////////////////////////////////////////////////////////////////////////
    // prune unbalanced branches (if we weren't inside a spanner, then there might be a branch that doesn't end in column heads)
    while (pParent->GetNumChildren()==0) {
        ASSERT(!bInsideSpanner);
        int iChild=pParent->GetParent()->GetChildIndex(pParent);
        pParent=pParent->GetParent();
        pParent->RemoveChildAt(iChild);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // graft the 2 subtrees together, forming a tree that is split into horz pages
    for (int iChild=0 ; iChild<pNewSubTree->GetNumChildren() ; iChild++) {
        CRowColPgOb* pChild = new CRowColPgOb(*pNewSubTree->GetChild(iChild));
        rcpgobColRoot.AddChild(pChild);
    }
#ifdef _DEBUG
    TRACE(_T("printing old tree %s \n") , (LPCTSTR)pSaveSubTree->GetText());

    for(int iIndex = 0; iIndex<pSaveSubTree->GetNumChildren() ;iIndex++){
        CRowColPgOb* pChildVal = pSaveSubTree->GetChild(iIndex);
        TRACE(_T("\t child --> %s\n") , (LPCTSTR)pSaveSubTree->GetChild(iIndex)->GetText());
        if(pChildVal->GetNumChildren() > 0 ){
            for(int iChildVal = 0; iChildVal<pChildVal->GetNumChildren() ;iChildVal++){
                CRowColPgOb* pSubChildVal = pChildVal->GetChild(iChildVal);
                TRACE(_T("\t\t child --> %s\n ") , (LPCTSTR)pSubChildVal->GetText());
                if(pSubChildVal->GetNumChildren() > 0 ){
                    for(int iChildVal = 0; iChildVal<pSubChildVal->GetNumChildren() ;iChildVal++){
                        TRACE(_T("\t\t\t child --> %s\n ") , (LPCTSTR)pSubChildVal->GetChild(iChildVal)->GetText());
                    }
                }
            }
        }
    }
    //Print the NewTree
#endif
    // free memory
    delete pNewSubTree;
}


/////////////////////////////////////////////////////////////////////////////
//
//        ColHeadOrphanProtect
//
// Identify situations where we have an orphan (a single column
// head is located on the last horz page).  For example:
//
//     horz page 1    horz page 2    horz page 3    horz page 4
//     -----------    -----------    -----------    -----------
//     cols 1,2,3     cols 4,5,6,7   col 8,9,10     col 11
//
// In these cases, ColHeadOrphanProtect() returns true if it is possible
// to shift one column from the second-to-last page to the last page.
// In the example, this would mean shifting column 10 from horz page 3
// to horz page 4.
//
// We only try to resolve orphans if the following conditions are true:
//    n>1
//    c>3
//    aColHeadsPerPage(n-2)>2    (2nd-to-last-page has > 2 colheads in it)
//    aColHeadsPerPage(n-1)=1    (last page has 1 colhead in it)
//    iColHead=c-2
//
// where
//    n is the number of horz pages
//    c is the total number of col heads
//    aColHeadsPerPage is the number of col heads on horz page i
//    iColHead is the column head being layed out
//
// If these conditions are satisfied and the last two colheads can fit on
// a single horz page (in the example, if cols 10 & 11 can fit alone on horz
// page 4), then we will able to avoid the orphan.  In that case, the function
// returns true.  Otherwise it returns false.
//
/////////////////////////////////////////////////////////////////////////////
// note that rcpgobColRoot is a local copy, so we can manipulate it (split up spanners, etc.) as needed
/*  TODO
bool CTabPrtView::ResolveOrphanColHeads(int iColHead, int iHorzColArea, int iTwipsAcross, int iColHeadSectionLayoutWidth, CRowColPgOb rcpgobColRoot) const
{
    CArray<CRowColPgOb*, CRowColPgOb*> aColHead;
    rcpgobColRoot.GetLeaves(aColHead);         // note: aColHead does *not* include spanners, since we have to deal with those recursively

    int c=aColHead.GetSize();
    ASSERT(c>0);
    int n=aColHead[c-1]->GetHPage();
    CArray<int,int> aColHeadsPerHorzPage;

    for (int i=0 ; i<c; i++) {
        aColHeadsPerHorzPage.Add(0);
    }
    for (int i=0 ; i<c ; i++) {
        aColHeadsPerHorzPage[aColHead[i]->GetHPage()]++;
    }
for (int i=0 ; i<c; i++)
TRACE("horz page %d has %d col heads in it\n", i, aColHeadsPerHorzPage[i]);

    // see if conditions are satisfied; if not, get out...
// if page break then return false
    bool bRet=(n>1 && c>3 && aColHeadsPerHorzPage[n-2]>2 && aColHeadsPerHorzPage[n-1]==1 && iColHead==c-2);
    if (!bRet) {
        return false;
    }

    // 5 fits here
    iTwipsAcross+iColHeadSectionLayoutWidth<=iHorzColArea

    // 6 does not
    iTwipsAcross += iColHeadSectionLayoutWidth;
    if (pColHead[6]->GetParent()==aColHead[5]->GetParent()) {
        // same section as previous column head...
        iTwipsAcross -= aColHead[5]->CalcColeadSectionLayoutWidth();
        bInsideSpanner=true;
    }
    iTwipsAcross+iColHeadSectionLayoutWidth>iHorzColArea


    // 5 & 6 fit OK on one page


}
*/

/////////////////////////////////////////////////////////////////////////////
//
//        EquallyDivideColHeads
//
// Allocates "slack" equitably to column heads on a given page.  Calculations are done based
// on column widths excluding previously allocated "extra" space.
//
// Process:
//    determine horizontal "slack" available (empty space that needs to be allocated to columns) (=HorzColArea)
//    determine "ideal" column head width (horizontal column area / number of column heads on a page) (=IdealWidth)
//    count column heads where width<ideal (=NumColHeadsLessThanIdeal)
//    sum column head widths where width<idea (=SumColHeadWidthsLessThanIdeal)
//    calc candidate min column head width  (=PossibleMinColHeadWidth=(SumColHeadWidthsLessThanIdeal + HorzSlackArea)/NumColHeadsLessThanIdeal)
//    count eligible column heads, where width<PossibleMinColHeadWidth (=NumAdjustableColHeads)
//    sum eligible column head widths, where width<PossibleMinColHeadWidth (=SumAdjustableColHeadWidths)
//    sum [width - PossibleMinColHeadWidth] where width>PossibleMinColHeadWidth and width<IdealWidth (=ExtraSlack)
//    calc min column head width (=MinColHeadWidth = (SumColHeadWidthsLessThanIdeal + HorzSlackArea - ExtraSlack)/NumAdjustableColHeads)
//    set column "extra" width=prev column width - MinColHeadWidth
//    set column width=MinColHeadWidth where column width<MinColHeadWidth
//    apply rounding error (unallocated slack) to last column on page
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::EquallyDivideColHeads(int iTbl, int iHPage, int iHorzColArea, CArray<CRowColPgOb*, CRowColPgOb*>& aColHead)
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    CTable* pTbl = pSet->GetTable(iTbl);

    int iWidth;  // column head width, including border
    int iHorzSlackArea=iHorzColArea;   // slack area, which we can divide among column heads to make them equal width
    int iNumColHeadsOnPage=0;
    //    determine horizontal "slack" available (empty space that needs to be allocated to columns) (=HorzColArea)
    for (int iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
        CRowColPgOb* pColHead = aColHead[iColHead];
        if (pColHead->GetHPage()==iHPage) {
            iWidth = pColHead->GetClientRectLP().Width() - pColHead->GetExtraLP().cx;
            iHorzSlackArea -= iWidth;
            iNumColHeadsOnPage++;
            ASSERT(iHorzSlackArea>=0);
        }
    }

    //    determine "ideal" column head width (horizontal column area / number of column heads on a page) (=IdealWidth)
    ASSERT(iNumColHeadsOnPage>0);
    int iIdealWidth = iHorzColArea/iNumColHeadsOnPage;

    //    count column heads where width<ideal (=NumColHeadsLessThanIdeal)
    //    sum column head widths where width<idea (=SumColHeadWidthsLessThanIdeal)
    int iNumColHeadsLessThanIdeal=0;
    int iSumColHeadWidthsLessThanIdeal=0;
    for (int iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
        CRowColPgOb* pColHead = aColHead[iColHead];
        if (pColHead->GetHPage()==iHPage && !pColHead->IsCustom()) {
           iWidth = pColHead->GetClientRectLP().Width() - pColHead->GetExtraLP().cx;
            if (iWidth<iIdealWidth) {
                iNumColHeadsLessThanIdeal++;
                iSumColHeadWidthsLessThanIdeal += iWidth;
            }
        }
    }

    int iPossibleMinColHeadWidth;
    if (iNumColHeadsLessThanIdeal==0) {
        // this can happen if all the thin colheads are custom
        iPossibleMinColHeadWidth=iHorzColArea;
    }
    else {
        //    calc candidate min column head width  (=PossibleMinColHeadWidth=(SumColHeadWidthsLessThanIdeal + HorzSlackArea)/NumColHeadsLessThanIdeal)
        iPossibleMinColHeadWidth=(iSumColHeadWidthsLessThanIdeal + iHorzSlackArea)/iNumColHeadsLessThanIdeal;
    }

    //    count eligible column heads, where width<PossibleMinColHeadWidth (=NumAdjustableColHeads)
    //    sum eligible column head widths, where width<PossibleMinColHeadWidth (=SumAdjustableColHeadWidths)
    //    sum [width - PossibleMinColHeadWidth] where width>PossibleMinColHeadWidth and width<IdealWidth (=ExtraSlack)
    int iNumAdjustableColHeads=0;
    int SumAdjustableColHeadWidths=0;
    int iExtraSlack=0;
    for (int iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
        CRowColPgOb* pColHead = aColHead[iColHead];
        if (pColHead->GetHPage()==iHPage && !pColHead->IsCustom()) {
            iWidth = pColHead->GetClientRectLP().Width() - pColHead->GetExtraLP().cx;
            if (iWidth<iPossibleMinColHeadWidth) {
                iNumAdjustableColHeads++;
                SumAdjustableColHeadWidths += iWidth;
            }
            if (iWidth>iPossibleMinColHeadWidth && iWidth<iIdealWidth) {
                iExtraSlack += iWidth - iPossibleMinColHeadWidth;
            }
        }
    }

    if (iNumAdjustableColHeads==0) {
        // no work to do!
        return;
    }

    //    calc min column head width (=MinColHeadWidth = (SumAdjustableColHeadWidths + HorzSlackArea - ExtraSlack)/NumAdjustableColHeads)
    int iMinColHeadWidth = (SumAdjustableColHeadWidths + iHorzSlackArea - iExtraSlack)/iNumAdjustableColHeads;

    for (int iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
        CRowColPgOb* pColHead = aColHead[iColHead];
        if (pColHead->GetHPage()==iHPage && !pColHead->IsCustom()) {
            iWidth = pColHead->GetClientRectLP().Width() - pColHead->GetExtraLP().cx;
            if (iWidth<iMinColHeadWidth) {
                //    set column "extra" width=prev col width - MinColHeadWidth
                pColHead->SetExtraLP(iMinColHeadWidth-iWidth,0);

                //    set column width=MinColHeadWidth where column width<MinColHeadWidth
                pColHead->GetClientRectLP().right = pColHead->GetClientRectLP().left + iMinColHeadWidth;
            }
            else
                int iii=5;
        }
    }

    //    apply rounding error (unallocated slack) to last non-custom column on page
    int iUnusedSlack=iHorzColArea;
    CRowColPgOb* pLastColHeadOnPage=NULL;
    for (int iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
        CRowColPgOb* pColHead = aColHead[iColHead];
        if (pColHead->GetHPage()==iHPage) {
            iUnusedSlack -= pColHead->GetClientRectLP().Width();
            if (!pColHead->IsCustom() || pLastColHeadOnPage==NULL) {
                pLastColHeadOnPage = pColHead;
            }
        }
    }
    ASSERT(iUnusedSlack>=0);
    if (iUnusedSlack!=0) {
        pLastColHeadOnPage->GetExtraLP().cx += iUnusedSlack;
        pLastColHeadOnPage->GetClientRectLP().right += iUnusedSlack;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        AlignColumnsVert
//
// Vertically aligns columns, on a level-by-level basis.
//
// If levels are missing from any parts of the column tree (which can
// happen if columns are AxB+C or the table has been customized), then
// column heads are descended as needed so that all column heads line up
// horizontally.  Hidden spanners are accounted for.
//
// Returns bottom level of column heads.
//
/////////////////////////////////////////////////////////////////////////////
int CTabPrtView::AlignColumnsVert(CArray<CRowColPgOb*, CRowColPgOb*>& aColHead, CRowColPgOb& rcpgobColRoot)
{
    // line up all levels except column heads horizontally (ignores gaps in levels)

    int iColDepth=rcpgobColRoot.GetDepth();
    for (int iLevel=1 ; iLevel<iColDepth ; iLevel++) {
        int iLevelHgt = rcpgobColRoot.GetMaxHeightLP(iLevel);
        rcpgobColRoot.SetColHeight(iLevel, iLevelHgt);
    }

    // determine the column heads' max horz position
    int iColHead;
    int iColHeadHorzLevel=0;
    for (iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {

        // pColHead is the column head that we are working on
        CRowColPgOb* pColHead = aColHead[iColHead];

        // climb the column head's parents, and calc the height of this branch
        int iBranchHgt=0;                   // a particular branch's height, from root to node above the column head
        CRowColPgOb* pParent = pColHead;
        while (pParent->GetType()!=PGOB_ROOT) {
            if (pParent->GetFmt()->GetHidden()!=HIDDEN_YES) {
                iBranchHgt+=pParent->GetClientRectLP().Height();
            }
            pParent=pParent->GetParent();
        }
        iColHeadHorzLevel=__max(iColHeadHorzLevel, iBranchHgt);
    }

    // line up the column heads
    for (iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
        CRowColPgOb* pColHead = aColHead[iColHead];

        // climb the column head's parents, and calc the height up to the column head
        int iBranchHgt=0;                   // a particular branch's height, from root to node above the column head
        CRowColPgOb* pParent = pColHead->GetParent();
        while (pParent->GetType()!=PGOB_ROOT) {
            iBranchHgt+=pParent->GetClientRectLP().Height();
            pParent=pParent->GetParent();
        }

        CRect rcColHead(pColHead->GetClientRectLP());
        ASSERT(rcColHead.top==0);
        ASSERT(rcColHead.Height()<=iColHeadHorzLevel-iBranchHgt);
        rcColHead.bottom=iColHeadHorzLevel-iBranchHgt;
        pColHead->SetClientRectLP(rcColHead);
    }

    return iColHeadHorzLevel;
}


/////////////////////////////////////////////////////////////////////////////
//
//        Build
//
// Builds all pages for all tables in an application.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::Build(bool bPreserveCurrentPgViewInfo /*=false*/)
{
    CWaitCursor wait;
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
	 ASSERT_VALID(pDoc);
     pSet = pDoc->GetTableSpec();
	}
    const CFmtReg& fmtReg = pSet->GetFmtReg();

    // init
    int iDefaultPageNum=1;   // default starting printed page number
    int iPg=0;

    // see if selected printer has changed, force remeasuring if so
    if (pSet->HasSelectedPrinterChanged()) {
        ForceRemeasure();
        pSet->SetSelectedPrinterChanged(false);
    }

    // back up page viewing info
    int iSaveViewPg;   // the starting page (top/left position) in the view
    ZOOM_STATE eSaveZoomState=ZOOM_STATE_100_PERCENT;
    int iSaveHBarPos=0, iSaveVBarPos=0;
    if (bPreserveCurrentPgViewInfo) {
        iSaveViewPg=m_aiViewPg[0];
        eSaveZoomState=GetZoomState();
        iSaveHBarPos=m_pHSBar->GetScrollPos();
        iSaveVBarPos=m_pVSBar->GetScrollPos();
    }

    m_pgMgr.RemoveAllPages();
    for (int iTbl=0 ; iTbl<pSet->GetNumTables() ; iTbl++) {
        CPgLayout plTemplate;   // contains the common objects (header, footer, title) that occur on all pages of this table
        CTable* pTbl = pSet->GetTable(iTbl);  // the table we're building

        // table print format
        CTblPrintFmt fmtTblPrint;
        if (pTbl->GetTblPrintFmt()) {
            // table has its own print fmt, copy it over so we can work with it (if tblprintfmt is null, then it's just working with the default)
            fmtTblPrint=*pTbl->GetTblPrintFmt();
            fmtTblPrint.CopyNonDefaultValues(DYNAMIC_DOWNCAST(CTblPrintFmt,fmtReg.GetFmt(FMT_ID_TBLPRINT,0)));
        }
        else {
            fmtTblPrint=*DYNAMIC_DOWNCAST(CTblPrintFmt,fmtReg.GetFmt(FMT_ID_TBLPRINT,0));
        }

        /////////////////////////////////////////////////////////////////////////////////////
        // determine this table's page size and orientation
        float fPgWidthInches=0.0f, fPgHgtInches=0.0f;

        switch(fmtTblPrint.GetPaperType()) {
        case PAPER_TYPE_A3:       // A3 297 x 420 mm
            fPgWidthInches = 297.0f / 10.0f / CM_PER_INCH;
            fPgHgtInches = 420.0f / 10.0f / CM_PER_INCH;
            break;
        case PAPER_TYPE_A4:       // A4 210 x 297 mm
            fPgWidthInches = 210.0f / 10.0f / CM_PER_INCH;
            fPgHgtInches = 297.0f / 10.0f / CM_PER_INCH;
            break;
        case PAPER_TYPE_LETTER:   // Letter 8 1/2 x 11 in
            fPgWidthInches = 8.5f;
            fPgHgtInches = 11.0f;
            break;
        case PAPER_TYPE_LEGAL:    // Legal 8 1/2 x 14 in
            fPgWidthInches = 8.5f;
            fPgHgtInches = 14.0f;
            break;
        case PAPER_TYPE_TABLOID:   //  Tabloid 11 x 17 in
            fPgWidthInches = 11.0f;
            fPgHgtInches = 17.0f;
            break;
        default:
            ASSERT(FALSE);
        }

        ASSERT(fmtTblPrint.GetPageOrientation()==PAGE_ORIENTATION_LANDSCAPE || fmtTblPrint.GetPageOrientation()==PAGE_ORIENTATION_PORTRAIT);
        if (fmtTblPrint.GetPageOrientation()==DMORIENT_LANDSCAPE)  {
            // swap width and height
            float fTmp = fPgWidthInches;
            fPgWidthInches = fPgHgtInches;
            fPgHgtInches = fTmp;
        }

        plTemplate.SetPgWidthInches(fPgWidthInches);
        plTemplate.SetPgHgtInches(fPgHgtInches);

        // add the page layout, so that it can be used by PrepareMapMode
        int iPage = m_pgMgr.AddPgLayout(plTemplate);

        ASSERT_VALID(m_pPrtDC);
        m_pPrtDC->m_bPrinting=TRUE;
        PrepareMapMode(m_pPrtDC, iPage);

        // now remove the page layout (we'll add it back after we finish building)
        m_pgMgr.RemovePageAt(iPage);

        // get client rect, based on prepared screen origin and extent
        CRect rcClient;
        rcClient.TopLeft() = m_pPrtDC->GetWindowOrg();
        rcClient.BottomRight() = m_pPrtDC->GetWindowExt();

        plTemplate.SetUserAreaLP(rcClient);

        /////////////////////////////////////////////////////////////////////////////////////
        // lay out the page objects that are fixed:
        // - margins
        // - header
        // - footer
        BuildHeaders(plTemplate, iTbl, *m_pPrtDC);
        BuildFooters(plTemplate, iTbl, *m_pPrtDC);
        BuildTitles(plTemplate, iTbl, *m_pPrtDC);
        BuildNotes(plTemplate, iTbl, *m_pPrtDC);

        /////////////////////////////////////////////////////////////////////////////////////
        // build rows and columns; these might span multiple pages, so we build them into a
        // temporary pgmgr structure, and pass the current CPgLayout to use as a
        // template (it's const and won't be changed)...
        CPgMgr pgMgrRC;
        if (!BuildRowsAndCols(plTemplate, pgMgrRC, iTbl, *m_pPrtDC)) {
            // build failed!  inform the user that we have to revert to a default layout
            // frequently this happens because stubs are too wide to fit columns, so force ourselves to left standard layout
            AfxMessageBox(_T("Unable to lay out this table, about to try again using default layout"), MB_ICONEXCLAMATION);
            m_bForceRemeasure=true;
            fmtTblPrint.SetTblLayout(TBL_LAYOUT_LEFT_STANDARD);
            pTbl->GetTblPrintFmt()->SetTblLayout(TBL_LAYOUT_LEFT_STANDARD);
            pDoc->SetModifiedFlag(TRUE);
            if (!BuildRowsAndCols(plTemplate, pgMgrRC, iTbl, *m_pPrtDC)) {
                AfxMessageBox(_T("Unable to lay out this table, even using default layout settings!"), MB_ICONSTOP);
                return;
            }
        }

        int iPrintedPageNum=fmtTblPrint.GetStartPage();
        if (iPrintedPageNum==START_PAGE_DEFAULT) {
            iPrintedPageNum=iDefaultPageNum;
        }
        for (iPg=0 ; iPg<pgMgrRC.GetNumPages() ; iPg++) {
            CPgLayout& pl=pgMgrRC.GetPgLayout(iPg);

            // assign page numbers for this page ...
            pl.SetPrintedPageNum(iPrintedPageNum++);

            // now add the rows and columns into the main CPgMgr...
            m_pgMgr.AddPgLayout(pl);
        }
        iDefaultPageNum=iPrintedPageNum;
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // nuke all tbl ob dimensions (note that we store them away when the view gets
    // taken down, so that we can serialize them later)

    // reset all obs in the table ...
    for (iPg=0 ; iPg<m_pgMgr.GetNumPages() ; iPg++) {
        CPgLayout& pl=m_pgMgr.GetPgLayout(iPg);
        for (int iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
            CPgOb& pgob=pl.GetPgOb(iPgOb);
            if (NULL==pgob.GetTblBase()) {
                ASSERT(pgob.GetType()==PGOB_DATACELL);  // datacells are the only things on the page that don't have tblobs
            }
            else {
                if (pgob.GetTblBase()->GetPrtViewInfoSize()>0) {
                    pgob.SetCustom(pgob.GetTblBase()->GetPrtViewInfo(0).IsCustom());
                    pgob.SetPageBreak(pgob.GetTblBase()->GetPrtViewInfo(0).IsPageBreak());
                    pgob.GetTblBase()->RemoveAllPrtViewInfo();
                }
            }
        }
    }

    // store size info ...
    for (iPg=0 ; iPg<m_pgMgr.GetNumPages() ; iPg++) {
        const CPgLayout& pl=m_pgMgr.GetPgLayout(iPg);
        for (int iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
            const CPgOb& pgob=pl.GetPgOb(iPgOb);
            if (NULL!=pgob.GetTblBase()) {
                CSize szCurr(pgob.GetClientRectLP().Size());
                CSize szMin(pgob.GetMinResize());
                CSize szMax(pgob.GetMaxResize());
                szCurr.cx -= pgob.GetFmt()->GetIndent(LEFT) + pgob.GetFmt()->GetIndent(RIGHT) + CELL_PADDING_LEFT + CELL_PADDING_RIGHT;
                szCurr.cy -= CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
                szMin.cx -= pgob.GetFmt()->GetIndent(LEFT) + pgob.GetFmt()->GetIndent(RIGHT) + CELL_PADDING_LEFT + CELL_PADDING_RIGHT;
                szMin.cy -= CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
                szMax.cx -= pgob.GetFmt()->GetIndent(LEFT) + pgob.GetFmt()->GetIndent(RIGHT) +CELL_PADDING_LEFT + CELL_PADDING_RIGHT;
                szMax.cy -= CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
                CPrtViewInfo pvi(szCurr, szMin, szMax, pgob.GetExtraLP(), pgob.IsCustom(), pgob.IsPageBreak());
                pgob.GetTblBase()->AddPrtViewInfo(pvi);
            }
        }
    }

    UpdateScrollBarRanges();
    ResetPageViewLayout();
    m_bForceRemeasure=false;

    if (bPreserveCurrentPgViewInfo) {

        // react if the total number of pages has changed and this affects placeholders (-1) entries in the pageview list ...

        // add pages to the save view, following sequence from newly built view
        int iNextPg;
        if (iSaveViewPg>=m_pgMgr.GetNumPages()) {
            iNextPg=m_pgMgr.GetNumPages() - GetNumVertPgs()*GetNumHorzPgs();
            if (iNextPg<0) {
                iNextPg=0;
            }
        }
        else {
            iNextPg=iSaveViewPg;
        }
        m_aiViewPg.RemoveAll();
        for (int iPg=0 ; iPg<GetNumVertPgs()*GetNumHorzPgs() ; iPg++) {
            if (iNextPg<m_pgMgr.GetNumPages()) {
                m_aiViewPg.Add(iNextPg++);
            }
        }

        // fill out the saved view with placeholders
        while (m_aiViewPg.GetSize()<GetNumVertPgs()*GetNumHorzPgs()) {
            m_aiViewPg.Add(NONE);
        }

        SetZoomState(eSaveZoomState);
        m_pHSBar->SetScrollPos(iSaveHBarPos);
        m_pVSBar->SetScrollPos(iSaveVBarPos);
    }
    Invalidate();
}

void CTabPrtView::SetBookLayout(bool bBookLayout)
{
    m_bBookLayout=bBookLayout;
    if (m_bBookLayout) {
        m_szPagesToView.cx = 2;
        m_szPagesToView.cy = 1;

        // always show even page on left
        if (m_aiViewPg[0] > 0 && m_aiViewPg[0] % 2 == 0) {
            m_aiViewPg[0]--;
        }

        SetZoomState(ZOOM_STATE_2H_1V);
    }
    else {
        m_szPagesToView.cx = 1;
        m_szPagesToView.cy = 1;
    }
}

void CTabPrtView::ResetPageViewLayout()
{
    // indicate which pages to view
    if ((int)GetZoomState()>ZOOM_STATE_100_PERCENT) {
        SetZoomState(ZOOM_STATE_100_PERCENT);
    }

    // can't have this on unless in 2x1
    if (m_szPagesToView!=CSize(2,1)) {
        m_bBookLayout = false;
    }

    const int iStartPage=GetCurrFirstViewPg(); // start with curr page
    int iPage = iStartPage;
    m_aiViewPg.RemoveAll();
    for (int iPgDown=0 ; iPgDown<GetNumVertPgs() ; iPgDown++) {
        for (int iPgAcross=0 ; iPgAcross<GetNumHorzPgs() ; iPgAcross++) {
            if (IsBookLayout() && iPgAcross == 0 && iPage==0) {
                // insert blank placeholder in front of page 1; this puts even pages on the left and odd pages on the right
                m_aiViewPg.Add(NONE);
                ASSERT(iPgDown == 0);
            }
            else if (iPage<m_pgMgr.GetNumPages()) {
                m_aiViewPg.Add(iPage++);
            }
            else {
                // add placeholder at end
                m_aiViewPg.Add(NONE);
            }
        }
    }
    m_pVSBar->SetScrollPos(GetCurrFirstViewPg());
    m_pHSBar->SetScrollPos(0);
    Invalidate();
}

void CTabPrtView::OnSelChangeZoomComboBox()
{
    // get the combo box from the toolbar
    CComboBox* pCombo = GetZoomComboBox();
    ASSERT_VALID(pCombo);

    // zoom state is stored in item data of combo
    ASSERT(pCombo->GetCurSel() >= 0);
    UINT iData = pCombo->GetItemData(pCombo->GetCurSel());
    SendMessage(UWM::Table::Zoom, (ZOOM_STATE) iData, GetCurrFirstViewPg());
    SetFocus();  // set focus back to main view so you don't get strange results with arrow keys afterwards.
}

CComboBox* CTabPrtView::GetZoomComboBox()
{
	// get the combo box from the toolbar
	CWnd* pTabToolbar = AfxGetMainWnd()->GetDescendantWindow(TABTOOLBAR);
	CWnd* pSTabToolbar = AfxGetMainWnd()->GetDescendantWindow(SAMPTOOLBAR);//Savy (R) sampling app 20081231
	if (pTabToolbar == NULL && pSTabToolbar == NULL) {
		return NULL;
	}
	//Savy (R) sampling app 20081231
	if(!m_pTabSet){
		ASSERT_VALID(pTabToolbar);
		CComboBox* pCombo = (CComboBox*) pTabToolbar->GetDlgItem(ID_TAB_ZOOM_COMBO);
		ASSERT_VALID(pCombo);
		return pCombo;
	}
	else{
		ASSERT_VALID(pSTabToolbar);
		CComboBox* pCombo = (CComboBox*) pSTabToolbar->GetDlgItem(ID_TAB_ZOOM_COMBO);
		ASSERT_VALID(pCombo);
		return pCombo;
	}
}

void CTabPrtView::InitZoomCombo()
{
    CComboBox* pCombo = GetZoomComboBox();
    if (pCombo == NULL) {
        return;
    }

    pCombo->ResetContent();
    pCombo->AddString(_T("25%"));
    pCombo->SetItemData(0, ZOOM_STATE_4H_4V);
    pCombo->AddString(_T("50%"));
    pCombo->SetItemData(1, ZOOM_STATE_3H_3V);
    pCombo->AddString(_T("75%"));
    pCombo->SetItemData(2, ZOOM_STATE_2H_2V);
    pCombo->AddString(_T("100%"));
    pCombo->SetItemData(3, ZOOM_STATE_100_PERCENT);
    pCombo->AddString(_T("125%"));
    pCombo->SetItemData(4, ZOOM_STATE_125_PERCENT);
    pCombo->AddString(_T("150%"));
    pCombo->SetItemData(5, ZOOM_STATE_150_PERCENT);
    pCombo->AddString(_T("175%"));
    pCombo->SetItemData(6, ZOOM_STATE_175_PERCENT);
    pCombo->AddString(_T("200%"));
    pCombo->SetItemData(7, ZOOM_STATE_200_PERCENT);
    pCombo->AddString(_T("225%"));
    pCombo->SetItemData(8, ZOOM_STATE_225_PERCENT);
    pCombo->AddString(_T("250%"));
    pCombo->SetItemData(9, ZOOM_STATE_250_PERCENT);
    pCombo->AddString(_T("275%"));
    pCombo->SetItemData(10, ZOOM_STATE_275_PERCENT);
    pCombo->AddString(_T("300%"));
    pCombo->SetItemData(11, ZOOM_STATE_300_PERCENT);
}

void CTabPrtView::UpdateZoomCombo()
{
    CComboBox* pCombo = GetZoomComboBox();
    if (pCombo == NULL) {
        return;
    }

    ZOOM_STATE eCompatState = m_eZoomState;
    switch (m_eZoomState) {
        case ZOOM_STATE_4H_4V:
        case ZOOM_STATE_4H_3V:
            eCompatState = ZOOM_STATE_4H_4V;
            break;
        case ZOOM_STATE_3H_3V:
        case ZOOM_STATE_3H_2V:
            eCompatState = ZOOM_STATE_3H_3V;
            break;
        case ZOOM_STATE_2H_2V:
        case ZOOM_STATE_2H_1V:
            eCompatState = ZOOM_STATE_2H_2V;
            break;
    }
    int i = 0;
    for (i = 0; i < pCombo->GetCount(); ++i) {
        if (((ZOOM_STATE) pCombo->GetItemData(i)) == eCompatState) {
            pCombo->SetCurSel(i);
            break;
        }
    }
    ASSERT(i < pCombo->GetCount()); // should have found match
}

/////////////////////////////////////////////////////////////////////////////
//
//                             OnZoom
//
// Zooms pages in the view, based on the parameters passed in wParam and lParam:
//
//    wParam gives the new zoom state to use.
//
//    lParam gives the page number to show (applicable when switching from multi to single-page view)
//         (-1 will use the top-left page)
//
/////////////////////////////////////////////////////////////////////////////
LONG CTabPrtView::OnZoom(WPARAM wParam, LPARAM lParam /*=-1*/)
{
    ASSERT(m_aiViewPg.GetSize()>0);
    ASSERT(wParam>=(WPARAM)ZOOM_STATE_4H_4V && wParam<=(WPARAM)ZOOM_STATE_300_PERCENT);

    ZOOM_STATE eNewZoomState=(ZOOM_STATE)wParam;
    int iStartPg=(int)lParam;

    if (iStartPg==-1) {
        iStartPg=GetCurrFirstViewPg();
    }
    ASSERT(iStartPg>=0 && iStartPg<m_pgMgr.GetNumPages());

    // don't let us zoom out to a level that has more than the minimum amount of dead space
    switch(eNewZoomState) {
    case ZOOM_STATE_4H_4V:
        if (m_pgMgr.GetNumPages()<=4*3) {
            eNewZoomState=ZOOM_STATE_4H_3V;
        }
        break;
    case ZOOM_STATE_4H_3V:
        if (m_pgMgr.GetNumPages()<=3*3) {
            eNewZoomState=ZOOM_STATE_3H_3V;
        }
        break;
    case ZOOM_STATE_3H_3V:
        if (m_pgMgr.GetNumPages()<=3*2) {
            eNewZoomState=ZOOM_STATE_3H_2V;
        }
        break;
    case ZOOM_STATE_3H_2V:
        if (m_pgMgr.GetNumPages()<=2*2) {
            eNewZoomState=ZOOM_STATE_2H_2V;
        }
        break;
    case ZOOM_STATE_2H_2V:
        if (m_pgMgr.GetNumPages()<=2*1) {
            eNewZoomState=ZOOM_STATE_2H_1V;
        }
        break;
    case ZOOM_STATE_2H_1V:
        if (m_pgMgr.GetNumPages()==1) {
            eNewZoomState=ZOOM_STATE_100_PERCENT;
        }
        break;
    // this doesn't affect other cases
    }

    if (GetZoomState()==eNewZoomState) {
        // no change being made; avoid flicker and bail out
        return 0L;
    }

    if (GetZoomState()<ZOOM_STATE_100_PERCENT && eNewZoomState>=ZOOM_STATE_100_PERCENT) {
        // moving from a multi-page view to a single-page view;

        // back up multiple page view info ...
        m_aiSaveViewPg.RemoveAll();
        m_szSavePagesToView=m_szPagesToView;
        for (int iViewPg=0 ; iViewPg<m_aiViewPg.GetSize() ; iViewPg++) {
            m_aiSaveViewPg.Add(m_aiViewPg[iViewPg]);
        }
    }

    // set the new zoom state
    SetZoomState(eNewZoomState);

    // update info on how many and which horz/vert pages we will be displaying concurrently
    switch(eNewZoomState) {
    case ZOOM_STATE_4H_4V:
        m_szPagesToView=CSize(4,4);
        break;
    case ZOOM_STATE_4H_3V:
        m_szPagesToView=CSize(4,3);
        break;
    case ZOOM_STATE_3H_3V:
        m_szPagesToView=CSize(3,3);
        break;
    case ZOOM_STATE_3H_2V:
        m_szPagesToView=CSize(3,2);
        break;
    case ZOOM_STATE_2H_2V:
        m_szPagesToView=CSize(2,2);
        break;
    case ZOOM_STATE_2H_1V:
        m_szPagesToView=CSize(2,1);
        break;
    case ZOOM_STATE_100_PERCENT:
    case ZOOM_STATE_125_PERCENT:
    case ZOOM_STATE_150_PERCENT:
    case ZOOM_STATE_175_PERCENT:
    case ZOOM_STATE_200_PERCENT:
    case ZOOM_STATE_225_PERCENT:
    case ZOOM_STATE_250_PERCENT:
    case ZOOM_STATE_275_PERCENT:
    case ZOOM_STATE_300_PERCENT:
        m_szPagesToView=CSize(1,1);
        break;
    default:
        ASSERT(FALSE);
    }

    // can't have this on unless in 2x1
    if (m_szPagesToView!=CSize(2,1)) {
        m_bBookLayout = false;
    }

    // always show all as many pages as possible
 /*   iStartPg=min(iStartPg, m_pgMgr.GetNumPages()-m_szPagesToView.cx*m_szPagesToView.cy);
    if (iStartPg<0) {
        // this happens if we have more space than pages to show
        iStartPg=0;
    } */

    m_aiViewPg.RemoveAll();
    for (int iVert=0 ; iVert<m_szPagesToView.cy ; iVert++) {
        for (int iHorz=0 ; iHorz<m_szPagesToView.cx ; iHorz++) {
            int iNewPg=iStartPg+iVert*m_szPagesToView.cx+iHorz;
            if (iNewPg>=m_pgMgr.GetNumPages()) {
                // add a placeholder page, since we're past the end
                iNewPg=NONE;
            }
            m_aiViewPg.Add(iNewPg);
        }
    }
    ASSERT(m_szPagesToView.cx * m_szPagesToView.cy == m_aiViewPg.GetSize());

    // sanity checks ... make sure that the page in wParam is currently being viewed
    bool bOK=false;
    for (int iViewPg=0 ; iViewPg<m_aiViewPg.GetSize() ; iViewPg++) {
        if (m_aiViewPg[iViewPg]==iStartPg) {
            bOK=true;
            break;
        }
    }
    ASSERT(bOK);

    m_pHSBar->SetScrollPos(0);
    m_pVSBar->SetScrollPos(iStartPg);
    RedrawWindow();
    UpdateScrollBarRanges();

    // show current page number (should be page 1) on the navigation bar
    GetNavBar().SetPageInfo(GetCurrFirstViewPg()+1, m_pgMgr.GetNumPages());

    return 0L;
}


/////////////////////////////////////////////////////////////////////////////
//
//                             OnLButtonDown
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnLButtonDown(UINT nFlags, CPoint point)
{
    CTblPrtViewHitOb obHit;
    PRTVIEW_HITTEST ht;
    bool bCtrl = (GetKeyState(VK_CONTROL)<0);
    bool bShift = (GetKeyState(VK_SHIFT)<0);

    ASSERT(!IsResizing());

    ht = HitTest(point, obHit);

    switch(ht) {
    case PRTVIEW_HIT_DEAD_SPACE:  {
        // act like PRTVIEW_HIT_VBORDER_COLHEAD if we're close to the right side of a col header ...
        // act like PRTVIEW_HIT_HBORDER_STUB if we're close to the bottom of a stub or caption ...
        // otherwise, just magnify
        // note: this code block is similar to what is used in OnSetCursor!
        bool bCloseToColHead=false;
        bool bCloseToStub=false;
        const CPgLayout& pl=m_pgMgr.GetPgLayout(obHit.GetPg());
        for (int iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
            const CPgOb& pgob=pl.GetPgOb(iPgOb);
            if (pgob.GetType()==PGOB_COLHEAD) {
                const CRect& rcClient=pgob.GetClientRectDP();
                int iDistanceRightSide=point.x - rcClient.right;
                if (rcClient.top<=point.y && rcClient.bottom>=point.y && iDistanceRightSide>=0 && iDistanceRightSide<GetHitTestBorder()) {
                    obHit.SetPgOb(iPgOb);
                    bCloseToColHead=true;
                    break;
                }
            }
            if (pgob.GetType()==PGOB_STUB || pgob.GetType()==PGOB_STUB_RIGHT || pgob.GetType()==PGOB_CAPTION || pgob.GetType()==PGOB_READER_BREAK) {
                const CRect& rcClient=pgob.GetClientRectDP();
                int iDistanceBottom=point.y - rcClient.bottom;
                if (rcClient.left<=point.x && rcClient.right>=point.x
                && iDistanceBottom>=0 && iDistanceBottom<GetHitTestBorder()) {
                    obHit.SetPgOb(iPgOb);
                    bCloseToStub=true;
                    break;
                }
            }
        }
        if (bCloseToColHead) {
            StartResizeCol(obHit);
        }
        else if (bCloseToStub) {
            StartResizeRow(obHit);
        }
        break;  }

    case PRTVIEW_HIT_HEADER:
    case PRTVIEW_HIT_FOOTER:
    case PRTVIEW_HIT_TITLE:
    case PRTVIEW_HIT_SUBTITLE:
    case PRTVIEW_HIT_CELL:
    case PRTVIEW_HIT_PAGENOTE:
    case PRTVIEW_HIT_ENDNOTE:
    case PRTVIEW_HIT_STUB:
    case PRTVIEW_HIT_CAPTION:
    case PRTVIEW_HIT_COLHEAD:
    case PRTVIEW_HIT_STUBHEAD:
    case PRTVIEW_HIT_SPANNER:
    case PRTVIEW_HIT_HBORDER_HEADER:
    case PRTVIEW_HIT_HBORDER_FOOTER:
    case PRTVIEW_HIT_HBORDER_TITLE:
    case PRTVIEW_HIT_HBORDER_SUBTITLE:
    case PRTVIEW_HIT_HBORDER_CELL:
    case PRTVIEW_HIT_HBORDER_PAGENOTE:
    case PRTVIEW_HIT_HBORDER_ENDNOTE:
    case PRTVIEW_HIT_HBORDER_COLHEAD:
    case PRTVIEW_HIT_HBORDER_SPANNER:
    case PRTVIEW_HIT_HBORDER_STUBHEAD:
    case PRTVIEW_HIT_VBORDER_HEADER:
    case PRTVIEW_HIT_VBORDER_FOOTER:
    case PRTVIEW_HIT_VBORDER_TITLE:
    case PRTVIEW_HIT_VBORDER_SUBTITLE:
    case PRTVIEW_HIT_VBORDER_CELL:
    case PRTVIEW_HIT_VBORDER_PAGENOTE:
    case PRTVIEW_HIT_VBORDER_ENDNOTE:
    case PRTVIEW_HIT_VBORDER_STUB:
    case PRTVIEW_HIT_VBORDER_CAPTION:
    case PRTVIEW_HIT_VBORDER_SPANNER:
        break;

    case PRTVIEW_HIT_VBORDER_STUBHEAD:  {
        const CPgLayout& pl=m_pgMgr.GetPgLayout(obHit.GetPg());
        const CPgOb& pgobStubhead=pl.GetPgOb(obHit.GetPgOb());  // might be primary, might be seconardary stubhead ...

        // figure out if we're working on the primary or secondary stubhead (if one is present)    // we're resizing pgob (the hit ob) ... exception: rightmost colhead really resizes secondary stubhead, if one is present
        bool bResizingPrimaryStubhead=false;
        for (int iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
            const CPgOb& pgob=pl.GetPgOb(iPgOb);
            if (pgob.GetType()==PGOB_STUBHEAD) {
                // first stubhead is always the primary
                bResizingPrimaryStubhead=(iPgOb==obHit.GetPgOb());
                break;
            }
        }

        int iDistanceLeftSide=abs(point.x - pgobStubhead.GetClientRectDP().left);
        int iDistanceRightSide=abs(point.x - pgobStubhead.GetClientRectDP().right);
        // left<right means that we're on the left border of the stubhead, otherwise on the right border

        // do nothing if we are on the left vertical border of the primary stubhead (not a resizable spot!)
        if (bResizingPrimaryStubhead && iDistanceLeftSide<iDistanceRightSide) {
            // do nothing ...
            break;
        }

        // perhaps the user is trying to resize the last colhead (which is adjacent to the secondary stubhead, if present)
        if (iDistanceLeftSide<iDistanceRightSide) {
            // we've hit just inside the left side of this colhead, which means we're really resizing the object to our left
            for (int iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
                const CPgOb& pgob=pl.GetPgOb(iPgOb);
                if (pgob.GetType()==PGOB_COLHEAD && pgob.GetClientRectLP().right==pgobStubhead.GetClientRectLP().left) {
                    // user is trying to resize the column next to the secondary stubhead
                    obHit.SetPgOb(iPgOb);
                    break;
                }
            }
        }
        StartResizeCol(obHit);
        break;  }

    case PRTVIEW_HIT_HBORDER_STUB:
    case PRTVIEW_HIT_HBORDER_CAPTION: {
        // figure out which ob is being resized (depends on which side of the HBORDER we hit; could be adjacent to obHit)
        const CPgLayout& pl=m_pgMgr.GetPgLayout(obHit.GetPg());
        const CPgOb& pgob=pl.GetPgOb(obHit.GetPgOb());
        int iDistanceTopSide=abs(point.y - pgob.GetClientRectDP().top);
        int iDistanceBottomSide=abs(point.y - pgob.GetClientRectDP().bottom);
        if (iDistanceTopSide<iDistanceBottomSide) {
            // we've hit just above this stub/caption, which means we're really resizing the object above us
            bool bFound=false;
            for (int iPgOb=obHit.GetPgOb()-1 ; iPgOb>=0 ; iPgOb--) {
                obHit.SetPgOb(obHit.GetPgOb()-1);
                const CPgOb& pgobAbove=pl.GetPgOb(obHit.GetPgOb());
                if ((pgobAbove.GetType()==PGOB_STUB || pgobAbove.GetType()==PGOB_STUB_RIGHT || pgobAbove.GetType()==PGOB_CAPTION || pgobAbove.GetType()==PGOB_READER_BREAK) && pgobAbove.GetClientRectDP().bottom==pgob.GetClientRectDP().top) {
                    // found it!
                    bFound=true;
                    break;
                }
            }
            if (!bFound) {
                // we're actually resizing the stubhead, but that's not allowed currently; just magnify
                break;
            }
        }
        StartResizeRow(obHit);
        break;   }

    case PRTVIEW_HIT_VBORDER_COLHEAD: {
        // figure out which ob is being resized (depends on which side of the VBORDER we hit; could be adjacent to obHit)
        const CPgLayout& pl=m_pgMgr.GetPgLayout(obHit.GetPg());
        const CPgOb& pgob=pl.GetPgOb(obHit.GetPgOb());
        int iDistanceLeftSide=abs(point.x - pgob.GetClientRectDP().left);
        int iDistanceRightSide=abs(point.x - pgob.GetClientRectDP().right);
        if (iDistanceLeftSide<iDistanceRightSide) {
            // we've hit just inside the left side of this colhead, which means we're really resizing the object to our left
            bool bFound=false;
            for (int iPgOb=obHit.GetPgOb()-1 ; iPgOb>=0 ; iPgOb--) {
                obHit.SetPgOb(obHit.GetPgOb()-1);
                const CPgOb& pgobLeft=pl.GetPgOb(obHit.GetPgOb());
                if (pgobLeft.GetType()==PGOB_COLHEAD && pgobLeft.GetClientRectDP().right==pgob.GetClientRectDP().left) {
                    // found it!
                    bFound=true;
                    break;
                }
            }
            if (!bFound) {
                // didn't find the left-side colhead; we're actually resizing the primary stubhead, so find it ...
                for (int iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
                    const CPgOb& pgobStubhead=pl.GetPgOb(iPgOb);
                    if (pgobStubhead.GetType()==PGOB_STUBHEAD && pgobStubhead.GetClientRectLP().right==pgob.GetClientRectLP().left) {
                        obHit.SetPgOb(iPgOb);
                        bFound=true;
                        break;
                    }
                }
                ASSERT(bFound);  // no stubhead if we assert here
            }
        }
        StartResizeCol(obHit);
        break;   }

    case PRTVIEW_NO_HIT:
        // no action
        break;

    default: {
        CString s;
        s.Format(_T("internal error 962: HT returned %d"), (int)ht);
        AfxMessageBox(s);
        ASSERT(FALSE);
        }
    }

    CView::OnLButtonDown(nFlags, point);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             Magnify
//
// Magnifies the view in or out, by sending a UWM::Table::Zoom message.
//
// See additional comments in CTabPrtView::OnZoom.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::Magnify(const CTblPrtViewHitOb& obHit)
{
    bool bCtrl = (GetKeyState(VK_CONTROL)<0);
    int iZoomInOut=(bCtrl?-1:+1);   // =-1 to zoom out, +1 to zoom in
//    int iNumViewPages=GetNumHorzPgs()*GetNumVertPgs();

    ZOOM_STATE eNewZoomState;
    if (bCtrl) {
        // zooming out
        if (GetZoomState()==ZOOM_STATE_4H_4V) {
            // can't zoom out any more
            eNewZoomState=ZOOM_STATE_4H_4V;
        }
        else {
            eNewZoomState=(ZOOM_STATE)((int)GetZoomState() - 1);
        }
    }
    else {
        // zooming in
        if (GetZoomState()==ZOOM_STATE_300_PERCENT) {
            // can't zoom in any more
            eNewZoomState=ZOOM_STATE_300_PERCENT;
        }
        else {
            eNewZoomState=(ZOOM_STATE)((int)GetZoomState() + 1);
        }
    }

    if (eNewZoomState!=GetZoomState()) {
        SendMessage(UWM::Table::Zoom, (WPARAM)eNewZoomState, (LPARAM)obHit.GetPg());
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                             OnLButtonUp
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (IsResizing()) {
        const CTblPrtViewHitOb& resizeOb = GetResizeHitOb();
        CPgOb& pgob = m_pgMgr.GetPgLayout(resizeOb.GetPg()).GetPgOb(resizeOb.GetPgOb());

        // calc client area (subtract scroll bar area)
        CRect rcClientDP;
        GetClientRect(&rcClientDP);
        rcClientDP.BottomRight() -= m_szSBar;

        // start with this page's area in device units
        CRect rcResizeDP(m_pgMgr.GetPgLayout(resizeOb.GetPg()).GetUserAreaDP());

        // exclude areas that fall vertically outside of the client
        rcResizeDP.top = __max(rcResizeDP.top, rcClientDP.top);
        rcResizeDP.bottom = __min(rcResizeDP.bottom, rcClientDP.bottom);

        ASSERT(GetResizeOrientation()==RESIZE_ROW || GetResizeOrientation()==RESIZE_COL);
        if (GetResizeOrientation()==RESIZE_COL) {
            /////////////////////////////////////////////////////
            // resizing a column header
            /////////////////////////////////////////////////////

            // get the resize bar width
            float fRatio = (float)pgob.GetClientRectDP().Width()/(float)pgob.GetClientRectLP().Width();
            int iBarWidthDP= (int)((float)RESIZE_BAR_WIDTH_LP * fRatio + 0.5f) + 2;

            // invalidate old rect
            rcResizeDP.left=m_iCurResizePosDP - iBarWidthDP/2;
            rcResizeDP.right=m_iCurResizePosDP + iBarWidthDP/2+1;
            InvalidateRect(rcResizeDP);

            // don't let the resize bar go past the min resize point
            if (point.x<m_iMinResizePosDP) {
                point.x=m_iMinResizePosDP;
            }
            m_iCurResizePosDP=point.x;

            // update the object's size
            int iResizeXLP;  // amount to increase width (X direction) (in logical units)
            iResizeXLP=(int)((float)(m_iCurResizePosDP - pgob.GetClientRectDP().right)/fRatio);

            // sometimes rounding error will cause us to advance slightly past the min size; prevent this
            if (pgob.GetClientRectLP().left+pgob.GetMinResize().cx>pgob.GetClientRectLP().right+iResizeXLP) {
                iResizeXLP=pgob.GetClientRectLP().left+pgob.GetMinResize().cx-pgob.GetClientRectLP().right;
            }

            DoResize(CSize(iResizeXLP,0));
        }
        else {
            /////////////////////////////////////////////////////
            // resizing a stub or caption
            /////////////////////////////////////////////////////

            // get the resize bar width
            float fRatio = (float)pgob.GetClientRectDP().Height()/(float)pgob.GetClientRectLP().Height();
            int iBarWidthDP= (int)((float)RESIZE_BAR_WIDTH_LP * fRatio + 0.5f) + 2;

            // invalidate old rect
            rcResizeDP.top=m_iCurResizePosDP - iBarWidthDP/2;
            rcResizeDP.bottom=m_iCurResizePosDP + iBarWidthDP/2+1;
            InvalidateRect(rcResizeDP);

            // don't let the resize bar go past the min resize point
            if (point.y<m_iMinResizePosDP) {
                point.y=m_iMinResizePosDP;
            }
            m_iCurResizePosDP=point.y;

            // update the object's size
            int iResizeYLP;  // amount to increase width (Y direction) (in logical units)
            iResizeYLP=(int)((float)(m_iCurResizePosDP - pgob.GetClientRectDP().bottom)/fRatio);

            // sometimes rounding error will cause us to advance slightly past the min size; prevent this
            if (pgob.GetClientRectLP().top+pgob.GetMaxResize().cy>pgob.GetClientRectLP().bottom+iResizeYLP) {
                iResizeYLP=pgob.GetClientRectLP().top+pgob.GetMaxResize().cy-pgob.GetClientRectLP().bottom;
            }

            DoResize(CSize(0,iResizeYLP));
        }
        EndResize();

        Build(true);   // will restore current page view and zoom settings

        GetDocument()->SetModifiedFlag(TRUE);
    }
    CView::OnLButtonUp(nFlags, point);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             OnMouseMove
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnMouseMove(UINT nFlags, CPoint point)
{
    CView::OnMouseMove(nFlags, point);
    if (IsResizing()) {
        const CTblPrtViewHitOb& resizeOb = GetResizeHitOb();
        const CPgOb& pgob = m_pgMgr.GetPgLayout(resizeOb.GetPg()).GetPgOb(resizeOb.GetPgOb());

        //////////////////////////////////////////////////////////////////////////////////////
        // invalidate the spot where the resize bar was and is moving to ...

        // calc client area (subtract scroll bar area)
        CRect rcClientDP;
        GetClientRect(&rcClientDP);
        rcClientDP.BottomRight() -= m_szSBar;

        // start with this page's area in device units
        CRect rcResizeDP(m_pgMgr.GetPgLayout(resizeOb.GetPg()).GetUserAreaDP());

        ASSERT(GetResizeOrientation()==RESIZE_ROW || GetResizeOrientation()==RESIZE_COL);
        if (GetResizeOrientation()==RESIZE_COL) {
            //////////////////////////////////////////
            // resizing a column
            //////////////////////////////////////////

            // get the resize bar width
            float fRatio = (float)pgob.GetClientRectDP().Width()/(float)pgob.GetClientRectLP().Width();
            int iBarWidthDP= (int)((float)RESIZE_BAR_WIDTH_LP * fRatio + 0.5f) + 2;

            // exclude areas that fall vertically outside of the client
            rcResizeDP.top = __max(rcResizeDP.top, rcClientDP.top);
            rcResizeDP.bottom = __min(rcResizeDP.bottom, rcClientDP.bottom);

            // don't let the resize bar beyond the min resize point
            if (point.x<m_iMinResizePosDP) {
                point.x=m_iMinResizePosDP;
            }

            // redraw the bar if it has moved
            if (point.x!=m_iCurResizePosDP) {

                // invalidate old rect
                rcResizeDP.left=m_iCurResizePosDP - iBarWidthDP/2;
                rcResizeDP.right=m_iCurResizePosDP + iBarWidthDP/2+1;
                InvalidateRect(rcResizeDP);

                // move bar to new location
                m_iCurResizePosDP=point.x;
                rcResizeDP.left=m_iCurResizePosDP - iBarWidthDP/2;
                rcResizeDP.right=m_iCurResizePosDP + iBarWidthDP/2+1;
                InvalidateRect(rcResizeDP);
                UpdateWindow();
            }
        }
        else {
            //////////////////////////////////////////
            // resizing a stub or caption
            //////////////////////////////////////////

            // get the resize bar width
            float fRatio = (float)pgob.GetClientRectDP().Height()/(float)pgob.GetClientRectLP().Height();
            int iBarWidthDP= (int)((float)RESIZE_BAR_WIDTH_LP * fRatio + 0.5f) + 2;

            // exclude areas that fall vertically outside of the client
            rcResizeDP.left = __max(rcResizeDP.left, rcClientDP.left);
            rcResizeDP.right = __min(rcResizeDP.right, rcClientDP.right);

            // don't let the resize bar beyond the min resize point
            if (point.y<m_iMinResizePosDP) {
                point.y=m_iMinResizePosDP;
            }

            // redraw the bar if it has moved
            if (point.y!=m_iCurResizePosDP) {

                // invalidate old rect
                rcResizeDP.top=m_iCurResizePosDP - iBarWidthDP/2;
                rcResizeDP.bottom=m_iCurResizePosDP + iBarWidthDP/2+1;
                InvalidateRect(rcResizeDP);

                // move bar to new location
                m_iCurResizePosDP=point.y;
                rcResizeDP.top=m_iCurResizePosDP - iBarWidthDP/2;
                rcResizeDP.bottom=m_iCurResizePosDP + iBarWidthDP/2+1;
                InvalidateRect(rcResizeDP);
                UpdateWindow();
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                             OnContextMenu
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
    // cancel resizing, if it is ongoing
    if (IsResizing()) {
        EndResize();
    }

    CTblPrtViewHitOb obHit;
    PRTVIEW_HITTEST ht;

    // convert cursor position to logical units, which are compatible with the CPageOb layout rects
    CPoint ptDPTest(point);
    ScreenToClient(&ptDPTest);

    m_aSelected.RemoveAll();
    ht = HitTest(ptDPTest, obHit);

    // show and track dundas menu
    BCMenu popMenu;
    popMenu.CreatePopupMenu();

    popMenu.AppendMenu(m_bApplyAcrossPanels?MF_CHECKED:MF_UNCHECKED, ID_EDIT_APPLY_ACROSS_PANELS, _T("&Apply changes across panels"));
    popMenu.AppendMenu(m_bAutoFitColumns?MF_CHECKED:MF_UNCHECKED, ID_EDIT_AUTOFIT_COLUMNS, _T("automatically &Fit columns across each page"));

    switch(ht) {
    case PRTVIEW_HIT_COLHEAD:
    case PRTVIEW_HIT_HBORDER_COLHEAD:
    case PRTVIEW_HIT_VBORDER_COLHEAD:
        popMenu.AppendMenu(MF_SEPARATOR);
        popMenu.AppendMenu ((m_pgMgr.GetPgLayout(obHit.GetPg()).GetPgOb(obHit.GetPgOb()).IsPageBreak()?MF_CHECKED:MF_UNCHECKED), ID_EDIT_COL_BREAK, _T("Page &break after"));
        break;

    case PRTVIEW_HIT_STUB:
    case PRTVIEW_HIT_HBORDER_STUB:
    case PRTVIEW_HIT_VBORDER_STUB:
    case PRTVIEW_HIT_CAPTION:
    case PRTVIEW_HIT_HBORDER_CAPTION:
    case PRTVIEW_HIT_VBORDER_CAPTION:
        popMenu.AppendMenu(MF_SEPARATOR);
        popMenu.AppendMenu ((m_pgMgr.GetPgLayout(obHit.GetPg()).GetPgOb(obHit.GetPgOb()).IsPageBreak()?MF_CHECKED:MF_UNCHECKED), ID_EDIT_STUB_BREAK, _T("Page &break after"));
        break;
    case PRTVIEW_HIT_DEAD_SPACE:
        // table won't be set in hit object, just use the tbl for pgob 0
        ASSERT(obHit.GetPgOb()==NONE);
        ASSERT(m_pgMgr.GetPgLayout(obHit.GetPg()).GetNumPgObs()>0);
        obHit.SetPgOb(0);
        break;
    case PRTVIEW_NO_HIT:
        // table won't be set in hit object, just use the tbl for pgob 0
        ASSERT(obHit.GetPgOb()==NONE && obHit.GetPg() == NONE);
        obHit.SetPgOb(0);
        obHit.SetPg(GetCurrFirstViewPg()); // use currently displayed page
        break;
    default:
        break;
    }

    popMenu.AppendMenu(MF_SEPARATOR);
    popMenu.AppendMenu (MF_STRING, ID_EDIT_TBL_PRINTFMT, _T("Format Print (Table)"));

    popMenu.AppendMenu(MF_SEPARATOR);
    popMenu.AppendMenu(ht==PRTVIEW_HIT_DEAD_SPACE?MF_GRAYED:MF_STRING, ID_EDIT_RESTORE_PRTVIEW_DEFAULTS, _T("Restore &default table layout"));

    popMenu.AppendMenu(MF_SEPARATOR);
    popMenu.AppendMenu(m_undoStack.CanUndo()?MF_STRING:MF_GRAYED, ID_EDIT_UNDO, _T("&Undo"));
    popMenu.AppendMenu(m_undoStack.CanRedo()?MF_STRING:MF_GRAYED, ID_EDIT_REDO, _T("&Redo"));

    popMenu.AppendMenu(MF_SEPARATOR);
    popMenu.AppendMenu(MF_STRING, ID_EDIT_GOTO, _T("Goto..."));

    popMenu.LoadToolbar(IDR_TABLE_FRAME);
    m_aSelected.Add(obHit);
    popMenu.TrackPopupMenu(TPM_RIGHTBUTTON, point.x, point.y, this);

}


/////////////////////////////////////////////////////////////////////////////
//
//                             OnViewFacing
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnViewFacing()
{
    CFacingPagesDlg dlg;
    dlg.m_sHorz.Str(GetNumHorzPgs());
    dlg.m_sVert.Str(GetNumVertPgs());

    if (dlg.DoModal()==IDOK) {
        m_szPagesToView.cx = (int)dlg.m_sHorz.Val();
        m_szPagesToView.cy = (int)dlg.m_sVert.Val();
        m_bBookLayout = false;
        SetZoomState(m_szPagesToView);
        UpdateScrollBarRanges();
        ResetPageViewLayout();
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                             OnViewBookLayout
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnViewBookLayout()
{
    SetBookLayout(!m_bBookLayout);
    UpdateScrollBarRanges();
    ResetPageViewLayout();
}


/////////////////////////////////////////////////////////////////////////////
//
//                             OnGoto
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnGoto()
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    ASSERT(pSet->GetNumTables()>0);

    CPrtViewGotoAreaDlg dlgArea(pSet, pDoc->GetAreaLabelLookup());
    CPrtViewGotoPageDlg dlgPg;
    dlgPg.m_iPage=GetCurrFirstViewPg()+1;  // dlg is 1-based, our systems are 0-based
    CPrtViewGotoTblDlg dlgTbl(pSet);

    // figure out which table is currently being displayed, which is the tbl for the first pgob on the current page
    // dlg is 1-based, our systems are 0-based
    dlgTbl.m_iTbl=m_pgMgr.GetPgLayout(GetCurrFirstViewPg()).GetPgOb(0).GetTbl();
    dlgArea.m_iTbl=dlgTbl.m_iTbl;
    dlgArea.SetArea(GetFirstAreaOnPage(GetCurrFirstViewPg()));

    CTreePropertiesDlg dlgAll(_T("Go to"));
    dlgPg.Create(CPrtViewGotoPageDlg::IDD, &dlgAll);

    dlgAll.AddPage(&dlgPg, _T("Page"));
    if (pSet->GetNumTables() > 1) {
        dlgTbl.Create(CPrtViewGotoTblDlg::IDD, &dlgAll);
        dlgAll.AddPage(&dlgTbl, _T("Table"));
    }
    int iNumAreaLevels = pDoc->GetTableSpec()->GetConsolidate()->GetNumAreas();
    if (iNumAreaLevels > 0 && pSet->GetNumTables() > 0) {
        CTable* pTable = pSet->GetTable(0);
        if (pTable->GetTabDataArray().GetSize() > 0) { // only show area if there has been a run
            dlgArea.Create(CPrtViewGotoAreaDlg::IDD, &dlgAll);
            dlgAll.AddPage(&dlgArea, _T("Area"));
        }
    }

    dlgAll.SetPage(&dlgPg);

    if (dlgAll.DoModal()==IDOK) {

        if (dlgAll.GetPage() == &dlgPg) {
            dlgPg.m_iPage--;
            if (dlgPg.m_iPage>=m_pgMgr.GetNumPages()) {
                dlgPg.m_iPage=m_pgMgr.GetNumPages()-1;
            }
            if (dlgPg.m_iPage<0) {
                dlgPg.m_iPage=0;
            }
            GotoPage(dlgPg.m_iPage);
        }
        else if (dlgAll.GetPage() == &dlgTbl) {
            if (dlgTbl.m_iTbl<0) {
                dlgTbl.m_iTbl=0;
            }
            if (dlgTbl.m_iTbl>pSet->GetNumTables()-1) {
                dlgTbl.m_iTbl=pSet->GetNumTables()-1;
            }
            GotoTbl(dlgTbl.m_iTbl);
        }
        else if (dlgAll.GetPage() == &dlgArea) {
            if (dlgArea.m_iTbl<0) {
                dlgArea.m_iTbl=0;
            }
            if (dlgArea.m_iTbl>pSet->GetNumTables()-1) {
                dlgArea.m_iTbl=pSet->GetNumTables()-1;
            }
            GotoArea(dlgArea.m_iTbl, dlgArea.GetArea());
        }
        else {
            ASSERT(!_T("invalid page for goto dlg"));
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                             OnUpdateViewZoom
//                             OnUpdateViewFacing
//                             OnUpdateViewBooklayout
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnUpdateViewZoom(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(TRUE);
}

void CTabPrtView::OnUpdateViewFacing(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(TRUE);
}

void CTabPrtView::OnUpdateViewBooklayout(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_bBookLayout);
    pCmdUI->Enable(TRUE);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             StartResizeCol
//
// Starts resizing a column (column header only).
//
// Note that the class members m_iCurResizePosDP, m_iMinResizePosDP, and
// m_iMaxResizePosDP are all given in device units.  This makes them easier
// to manipulate, since points in OnLButtonXXX and OnMouseMove are also in
// device units.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::StartResizeCol(const CTblPrtViewHitOb& obHit)
{
    ASSERT(!IsResizing());
    SetCapture();
    SetResizeOrientation(RESIZE_COL);

    CPgOb pgob = m_pgMgr.GetPgLayout(obHit.GetPg()).GetPgOb(obHit.GetPgOb());
    float fRatio = (float)pgob.GetClientRectDP().Width()/(float)pgob.GetClientRectLP().Width();

    if (pgob.GetType()==PGOB_STUBHEAD) {
        // 12/03/04
        pgob.GetMinResize().cx += CELL_PADDING_LEFT + CELL_PADDING_RIGHT;
    }

    m_iMinResizePosDP=pgob.GetClientRectDP().left + (int)((float)pgob.GetMinResize().cx*fRatio);
    m_iCurResizePosDP=pgob.GetClientRectDP().right;
    SetResizeOb(obHit);

    //////////////////////////////////////////////////////////////////////////////////////
    // invalidate the spot where the resize bar goes
    //////////////////////////////////////////////////////////////////////////////////////

    // calc client area (subtract horz scroll bar)
    CRect rcClientDP;
    GetClientRect(&rcClientDP);
    rcClientDP.bottom -= m_szSBar.cy;

    // start with this page's area in device units
    CRect rcResizeDP(m_pgMgr.GetPgLayout(obHit.GetPg()).GetUserAreaDP());

    // exclude areas that fall vertically outside of the client
    rcResizeDP.top = __max(rcResizeDP.top, rcClientDP.top);
    rcResizeDP.bottom = __min(rcResizeDP.bottom, rcClientDP.bottom);

    // left/right are just the width of the resize bar
    int iBarWidthDP= (int)((float)RESIZE_BAR_WIDTH_LP * fRatio + 0.5f) + 2;
    rcResizeDP.left=pgob.GetClientRectDP().right-iBarWidthDP/2;
    rcResizeDP.right=pgob.GetClientRectDP().right+iBarWidthDP/2;

    InvalidateRect(rcResizeDP);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             StartResizeRow
//
// Starts resizing a row (stub or caption).
//
// Works like StartResizeCol, but for rows.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::StartResizeRow(const CTblPrtViewHitOb& obHit)
{
    ASSERT(!IsResizing());
    SetCapture();
    SetResizeOrientation(RESIZE_ROW);

    const CPgOb& pgob = m_pgMgr.GetPgLayout(obHit.GetPg()).GetPgOb(obHit.GetPgOb());
    float fRatio = (float)pgob.GetClientRectDP().Height()/(float)pgob.GetClientRectLP().Height();

    m_iMinResizePosDP=pgob.GetClientRectDP().top + (int)((float)pgob.GetMaxResize().cy*fRatio);
    m_iCurResizePosDP=pgob.GetClientRectDP().bottom;
    SetResizeOb(obHit);


    //////////////////////////////////////////////////////////////////////////////////////
    // invalidate the spot where the resize bar goes
    //////////////////////////////////////////////////////////////////////////////////////

    // calc client area (subtract vert scroll bar)
    CRect rcClientDP;
    GetClientRect(&rcClientDP);
    rcClientDP.right -= m_szSBar.cx;

    // start with this page's area in device units
    CRect rcResizeDP(m_pgMgr.GetPgLayout(obHit.GetPg()).GetUserAreaDP());

    // exclude areas that fall horizontally outside of the client
    rcResizeDP.left = __max(rcResizeDP.left, rcClientDP.left);
    rcResizeDP.right = __min(rcResizeDP.right, rcClientDP.right);

    // top/bottom are just the height of the resize bar
    int iBarHeightDP= (int)((float)RESIZE_BAR_WIDTH_LP * fRatio + 0.5f) + 2;
    rcResizeDP.top=pgob.GetClientRectDP().bottom-iBarHeightDP/2;
    rcResizeDP.bottom=pgob.GetClientRectDP().bottom+iBarHeightDP/2;

    InvalidateRect(rcResizeDP);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             EndResize
//
// Stops a resize operation, for either a row or a column.  This happens
// after the user cancels (presses escape) or completes (drag + lbuttonup)
// a resize operation
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::EndResize()
{
    ASSERT(IsResizing());

    const CTblPrtViewHitOb& resizeOb = GetResizeHitOb();
    const CPgOb& pgob = m_pgMgr.GetPgLayout(resizeOb.GetPg()).GetPgOb(resizeOb.GetPgOb());

    // calc client area (subtract scroll bar area)
    CRect rcClientDP;
    GetClientRect(&rcClientDP);
    rcClientDP.BottomRight() -= m_szSBar;

    // start with this page's area in device units
    CRect rcResizeDP(m_pgMgr.GetPgLayout(resizeOb.GetPg()).GetUserAreaDP());

    // exclude areas that fall vertically outside of the client
    rcResizeDP.top = __max(rcResizeDP.top, rcClientDP.top);
    rcResizeDP.bottom = __min(rcResizeDP.bottom, rcClientDP.bottom);

    ASSERT(GetResizeOrientation()==RESIZE_COL || GetResizeOrientation()==RESIZE_ROW);
    if (GetResizeOrientation()==RESIZE_COL) {
        ////////////////////////////////////
        // resizing a column header
        ////////////////////////////////////

        // get the resize bar width
        float fRatio = (float)pgob.GetClientRectDP().Width()/(float)pgob.GetClientRectLP().Width();
        int iBarWidthDP= (int)((float)RESIZE_BAR_WIDTH_LP * fRatio + 0.5f) + 2;

        // invalidate old rect
        rcResizeDP.left=m_iCurResizePosDP - iBarWidthDP/2;
        rcResizeDP.right=m_iCurResizePosDP + iBarWidthDP/2+1;
    }
    else {
        ////////////////////////////////////
        // resizing a stub or caption
        ////////////////////////////////////

        // get the resize bar width
        float fRatio = (float)pgob.GetClientRectDP().Height()/(float)pgob.GetClientRectLP().Height();
        int iBarWidthDP= (int)((float)RESIZE_BAR_WIDTH_LP * fRatio + 0.5f) + 2;

        // invalidate old rect
        rcResizeDP.top=m_iCurResizePosDP - iBarWidthDP/2;
        rcResizeDP.bottom=m_iCurResizePosDP + iBarWidthDP/2+1;
    }

    InvalidateRect(rcResizeDP);

    m_iCurResizePosDP=NONE;
    m_iMinResizePosDP=NONE;
    m_iMaxResizePosDP=NONE;
    SetResizeOrientation(RESIZE_INACTIVE);
    ReleaseCapture();
    UpdateWindow();
}


/////////////////////////////////////////////////////////////////////////////
//
//                             DoResize
//
// Resizes pgobs based on the size parameter and the object being resized.
//
// Size is given in logical units, + increases width/height, - decreases.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::DoResize(const CSize& szAmount)
{
    CPgOb pgobResize=m_pgMgr.GetPgLayout(GetResizeHitOb().GetPg()).GetPgOb(GetResizeHitOb().GetPgOb());
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    CTable* pTbl = pSet->GetTable(pgobResize.GetTbl());
    int iPrtViewInfoOcc=NONE; // occurrence number within the CTblOb's prtviewinfo array (0-->1st panel, 1-->2nd panel, etc.)

    // resizing stubhead is handled differently ...
    if (pgobResize.GetType()==PGOB_STUBHEAD) {
        // prep for undo/redo
        PushRestoreDefaultsForUndo(pgobResize.GetTbl());

        // enlarge (but don't offset) object
        pgobResize.GetClientRectLP().right += szAmount.cx;
        pgobResize.GetClientRectLP().bottom += szAmount.cy;
        pgobResize.SetExtraLP(0,0);
        pgobResize.SetCustom();
        ASSERT(pgobResize.GetClientRectLP().top+pgobResize.GetMaxResize().cy<=pgobResize.GetClientRectLP().bottom);
        ASSERT(pgobResize.GetClientRectLP().left+pgobResize.GetMinResize().cx<=pgobResize.GetClientRectLP().right);

        // update persistence info...
        CSize szCurr(pgobResize.GetClientRectLP().Size());
        CSize szMin(pgobResize.GetMinResize());
        CSize szMax(pgobResize.GetMaxResize());
        szCurr.cx -= pgobResize.GetFmt()->GetIndent(LEFT) + pgobResize.GetFmt()->GetIndent(RIGHT) + CELL_PADDING_LEFT + CELL_PADDING_RIGHT;
        szCurr.cy -= CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
        szMin.cx -= pgobResize.GetFmt()->GetIndent(LEFT) + pgobResize.GetFmt()->GetIndent(RIGHT) + CELL_PADDING_LEFT + CELL_PADDING_RIGHT;
        szMin.cy -= CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
        szMax.cx -= pgobResize.GetFmt()->GetIndent(LEFT) + pgobResize.GetFmt()->GetIndent(RIGHT) + CELL_PADDING_LEFT + CELL_PADDING_RIGHT;
        szMax.cy -= CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
        CPrtViewInfo pvi(szCurr, szMin, szMax, pgobResize.GetExtraLP(), true, pgobResize.IsPageBreak());
        pgobResize.GetTblBase()->SetPrtViewInfo(0, pvi);
        return;
    }

    if (GetResizeOrientation()==RESIZE_ROW) {

        // prep for undo/redo
        PushStubResizeForUndo(pgobResize.GetTbl());

        // for resizing stubs, we need to isolate the leftmost page that contains the stub being resized (that is, the resize stub that occurs on HPage 0)
        pgobResize.SetHPage(0);
    }
    else {

        // prep for undo/redo
        PushBoxheadResizeForUndo(pgobResize.GetTbl());

        // for resizing column headers, we need to isolate the topmost page that contains the stub being resized (that is, the resize stub that occurs on VPage 0)
        pgobResize.SetVPage(NONE);
    }

    for (int iPg=0 ; iPg<m_pgMgr.GetNumPages() ; iPg++) {
        CPgLayout& pl=m_pgMgr.GetPgLayout(iPg);
        for (int iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
            CPgOb& pgob=pl.GetPgOb(iPgOb);
            if (pgob.GetHPage()!=0 && GetResizeOrientation()==RESIZE_ROW) {
                continue;
            }
            if (pgob.GetTblBase()==pgobResize.GetTblBase())  {
                // found an instance of this pgob, which means that we're ...
                iPrtViewInfoOcc++;

                if (pgob.GetClientRectLP()==pgobResize.GetClientRectLP() && pgob.GetTblBase()==pgobResize.GetTblBase() && pgob.GetVPage()==pgobResize.GetVPage() && pgob.GetHPage()==pgobResize.GetHPage()) {
                    // found the particular instance that we are resizing ...

                    // enlarge (but don't offset) object
                    pgob.GetClientRectLP().right += szAmount.cx;
                    pgob.GetClientRectLP().bottom += szAmount.cy;
                    pgob.SetExtraLP(0,0);
                    pgob.SetCustom();
                    ASSERT(pgob.GetClientRectLP().top+pgob.GetMaxResize().cy<=pgob.GetClientRectLP().bottom);
                    ASSERT(pgob.GetClientRectLP().left+pgob.GetMinResize().cx<=pgob.GetClientRectLP().right);

                    // update persistence info...
                    CSize szCurr(pgob.GetClientRectLP().Size());
                    CSize szMin(pgob.GetMinResize());
                    CSize szMax(pgob.GetMaxResize());
                    szCurr.cx -= pgob.GetFmt()->GetIndent(LEFT) + pgob.GetFmt()->GetIndent(RIGHT) + CELL_PADDING_LEFT + CELL_PADDING_RIGHT;
                    szCurr.cy -= CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
                    szMin.cx -= pgob.GetFmt()->GetIndent(LEFT) + pgob.GetFmt()->GetIndent(RIGHT) + CELL_PADDING_LEFT + CELL_PADDING_RIGHT;
                    szMin.cy -= CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
                    szMax.cx -= pgob.GetFmt()->GetIndent(LEFT) + pgob.GetFmt()->GetIndent(RIGHT) + CELL_PADDING_LEFT + CELL_PADDING_RIGHT;
                    szMax.cy -= CELL_PADDING_TOP + CELL_PADDING_BOTTOM;
                    CPrtViewInfo pvi(szCurr, szMin, szMax, pgob.GetExtraLP(), true, pgob.IsPageBreak());

                    if (m_bApplyAcrossPanels) {
                        // apply this resize setting across all panels ...
                        for (int iPrtViewInfo=0 ; iPrtViewInfo<pgob.GetTblBase()->GetPrtViewInfoSize() ; iPrtViewInfo++) {
                            CPrtViewInfo& pviAcross=pgob.GetTblBase()->GetPrtViewInfo(iPrtViewInfo);
                            pviAcross.SetCurrSize(pvi.GetCurrSize());
                            pviAcross.SetExtraSize(pvi.GetExtraSize());
                            pviAcross.SetCustom(pvi.IsCustom());
                            pviAcross.SetPageBreak(pvi.IsPageBreak());
                        }
                    }
                    else {
                        // update the specific occurrence within the tbl ob's printviewinfo array ...
                        pgob.GetTblBase()->SetPrtViewInfo(iPrtViewInfoOcc, pvi);
                    }

                    // all done!
                    return;
                }
            }
        }
    }
}



/////////////////////////////////////////////////////////////////////////////
//
//                             OnEditTablePrintFmt
//
// Brings up the tbl print format dialog box
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnEditTablePrintFmt()
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    int iTbl = NONE;
    if (m_aSelected.GetSize()==0) {
        // if you pick from menu, this will not be set, instead use current page
        CPgOb& pgob=m_pgMgr.GetPgLayout(GetCurrFirstViewPg()).GetPgOb(0);
        iTbl=pgob.GetTbl();
    }
    else {
        ASSERT(m_aSelected.GetSize()>0);
        CTblPrtViewHitOb obHit=m_aSelected[0];
        CPgOb& pgob=m_pgMgr.GetPgLayout(obHit.GetPg()).GetPgOb(obHit.GetPgOb());
        iTbl=pgob.GetTbl();
    }
    CTable* pTbl = pSet->GetTable(iTbl);

    // CTblPrintFmt fmtTblPrint(*pTbl->GetTblPrintFmt());  //working copy
    CTblPrintFmt fmtTblPrint;   // working copy
    if (pTbl->GetTblPrintFmt()) {
        fmtTblPrint=*pTbl->GetTblPrintFmt();
    }
    CFmtReg* pFmtReg = pSet->GetFmtRegPtr();

    // prep working copy of the tblprintfmt
    int iIndex=pFmtReg->GetNextCustomFmtIndex(fmtTblPrint);
    if (fmtTblPrint.GetIndex()==0) {
        // was using the stock default ... make the working copy use just defaults (except for printer driver/output/device and units)
        fmtTblPrint.SetTblLayout(TBL_LAYOUT_DEFAULT);
        fmtTblPrint.SetPageBreak(PAGE_BREAK_DEFAULT);
        fmtTblPrint.SetCtrHorz(CENTER_TBL_DEFAULT);
        fmtTblPrint.SetCtrVert(CENTER_TBL_DEFAULT);
        fmtTblPrint.SetPaperType(PAPER_TYPE_DEFAULT);
        fmtTblPrint.SetPageOrientation(PAGE_ORIENTATION_DEFAULT);
        fmtTblPrint.SetStartPage(START_PAGE_DEFAULT);
        fmtTblPrint.SetPageMargin(CRect(PAGE_MARGIN_DEFAULT,PAGE_MARGIN_DEFAULT,PAGE_MARGIN_DEFAULT,PAGE_MARGIN_DEFAULT));
        fmtTblPrint.SetHeaderFrequency(HEADER_FREQUENCY_DEFAULT);
        fmtTblPrint.SetUnits(pSet->GetTabSetFmt()->GetUnits());
    }
    fmtTblPrint.SetIndex(iIndex);

    CTblPrintFmtDlg dlg;

    // initialize dialog variables ...
    dlg.m_pFmtTblPrint=&fmtTblPrint;
    dlg.m_pDefaultFmtTblPrint=DYNAMIC_DOWNCAST(CTblPrintFmt, pFmtReg->GetFmt(FMT_ID_TBLPRINT,0));

    // note that we specify that our header/footer font is "default" if --
    // - the CFmtFont is null (that is, IsFontCustom()==false), or
    // - the format's fmt registry ID is zero
    bool bDefaultHeaderFont;
    if (pTbl->GetHeader(0)->GetDerFmt()) {
        iIndex=pTbl->GetHeader(0)->GetDerFmt()->GetIndex();
        bDefaultHeaderFont=dlg.m_bDefaultHeaderFont=(!pTbl->GetHeader(0)->GetDerFmt()->IsFontCustom() || iIndex==0);
    }
    else {
        iIndex=0;
        bDefaultHeaderFont=dlg.m_bDefaultHeaderFont=true;
    }
    LOGFONT lfHeaderCurrent; //currently active header font
    if (dlg.m_bDefaultHeaderFont) {
        ::ZeroMemory(&dlg.m_lfHeader, sizeof(LOGFONT));
    }
    else {
        pTbl->GetHeader(0)->GetDerFmt()->GetFont()->GetLogFont(&dlg.m_lfHeader);
    }
    (DYNAMIC_DOWNCAST(CFmt, pFmtReg->GetFmt(FMT_ID_HEADER_LEFT,0)))->GetFont()->GetLogFont(&dlg.m_lfDefaultHeader);
    lfHeaderCurrent=bDefaultHeaderFont?dlg.m_lfDefaultHeader:dlg.m_lfHeader;

    bool bDefaultFooterFont;
    if (pTbl->GetFooter(0)->GetDerFmt()) {
        iIndex=pTbl->GetFooter(0)->GetDerFmt()->GetIndex();
        bDefaultFooterFont=dlg.m_bDefaultFooterFont=(!pTbl->GetFooter(0)->GetDerFmt()->IsFontCustom() || iIndex==0);
    }
    else {
        iIndex=0;
        bDefaultFooterFont=dlg.m_bDefaultFooterFont=true;
    }
    LOGFONT lfFooterCurrent; //currently active footer font
    if (dlg.m_bDefaultFooterFont) {
        ::ZeroMemory(&dlg.m_lfFooter, sizeof(LOGFONT));
    }
    else {
        pTbl->GetFooter(0)->GetDerFmt()->GetFont()->GetLogFont(&dlg.m_lfFooter);
    }
    (DYNAMIC_DOWNCAST(CFmt, pFmtReg->GetFmt(FMT_ID_FOOTER_LEFT,0)))->GetFont()->GetLogFont(&dlg.m_lfDefaultFooter);
    lfFooterCurrent=bDefaultFooterFont?dlg.m_lfDefaultFooter:dlg.m_lfFooter;

    SetupFolioText(pTbl->GetHeader(0), FMT_ID_HEADER_LEFT, dlg.m_sHeaderLeft, pFmtReg);
    SetupFolioText(pTbl->GetHeader(1), FMT_ID_HEADER_CENTER, dlg.m_sHeaderCenter, pFmtReg);
    SetupFolioText(pTbl->GetHeader(2), FMT_ID_HEADER_RIGHT, dlg.m_sHeaderRight, pFmtReg);
    SetupFolioText(pTbl->GetFooter(0), FMT_ID_FOOTER_LEFT, dlg.m_sFooterLeft, pFmtReg);
    SetupFolioText(pTbl->GetFooter(1), FMT_ID_FOOTER_CENTER, dlg.m_sFooterCenter, pFmtReg);
    SetupFolioText(pTbl->GetFooter(2), FMT_ID_FOOTER_RIGHT, dlg.m_sFooterRight, pFmtReg);

    bool bBuild=false;  //=true if something changed
    if (dlg.DoModal()==IDOK)  {
        bool bPushedUndo=false;
        bool bTblPrintChanged=(fmtTblPrint.GetHeaderFrequency()!=dlg.m_eHeaderFrequency
            || fmtTblPrint.GetTblLayout()!=dlg.m_eTblLayout
            || fmtTblPrint.GetStartPage()!=dlg.m_iStartPage
            || fmtTblPrint.GetPageMargin()!=dlg.m_rcPageMargin);

        if (bTblPrintChanged) {
            PushFormatPrintCommand(iTbl);
            bPushedUndo=true;

            // need to add a custom tblprintfmt to the registry ...
            fmtTblPrint.SetHeaderFrequency(dlg.m_eHeaderFrequency);
            fmtTblPrint.SetTblLayout(dlg.m_eTblLayout);
            fmtTblPrint.SetStartPage(dlg.m_iStartPage);
            fmtTblPrint.SetPageMargin(dlg.m_rcPageMargin);

            CTblPrintFmt* pFmtTblPrintNew=new CTblPrintFmt(fmtTblPrint);

            iIndex=pFmtReg->GetNextCustomFmtIndex(fmtTblPrint);
            pFmtTblPrintNew->SetIndex(iIndex);
            if (!pFmtReg->AddFmt(pFmtTblPrintNew)) {
                ASSERT(FALSE);
            }
            pTbl->SetTblPrintFmt(pFmtTblPrintNew);
            bBuild=true;
        }

        if (FolioTextNeedsUpdate(pTbl->GetHeader(0), FMT_ID_HEADER_LEFT, dlg.m_sHeaderLeft, pFmtReg)) {
            if (!bPushedUndo) {
                PushFormatPrintCommand(iTbl);
                bPushedUndo=true;
            }
            UpdateFolioText(pTbl->GetHeader(0), FMT_ID_HEADER_LEFT, dlg.m_sHeaderLeft, pFmtReg);
            bBuild = true;
        }
        if (FolioTextNeedsUpdate(pTbl->GetHeader(1), FMT_ID_HEADER_CENTER, dlg.m_sHeaderCenter, pFmtReg)) {
            if (!bPushedUndo) {
                PushFormatPrintCommand(iTbl);
                bPushedUndo=true;
            }
            UpdateFolioText(pTbl->GetHeader(1), FMT_ID_HEADER_CENTER, dlg.m_sHeaderCenter, pFmtReg);
            bBuild = true;
        }
        if (FolioTextNeedsUpdate(pTbl->GetHeader(2), FMT_ID_HEADER_RIGHT, dlg.m_sHeaderRight, pFmtReg)) {
            if (!bPushedUndo) {
                PushFormatPrintCommand(iTbl);
                bPushedUndo=true;
            }
            UpdateFolioText(pTbl->GetHeader(2), FMT_ID_HEADER_RIGHT, dlg.m_sHeaderRight, pFmtReg);
            bBuild = true;
        }
        if (FolioTextNeedsUpdate(pTbl->GetFooter(0), FMT_ID_FOOTER_LEFT, dlg.m_sFooterLeft, pFmtReg)) {
            if (!bPushedUndo) {
                PushFormatPrintCommand(iTbl);
                bPushedUndo=true;
            }
            UpdateFolioText(pTbl->GetFooter(0), FMT_ID_FOOTER_LEFT, dlg.m_sFooterLeft, pFmtReg);
            bBuild = true;
        }
        if (FolioTextNeedsUpdate(pTbl->GetFooter(1), FMT_ID_FOOTER_CENTER, dlg.m_sFooterCenter, pFmtReg)) {
            if (!bPushedUndo) {
                PushFormatPrintCommand(iTbl);
                bPushedUndo=true;
            }
            UpdateFolioText(pTbl->GetFooter(1), FMT_ID_FOOTER_CENTER, dlg.m_sFooterCenter, pFmtReg);
            bBuild = true;
        }
        if (FolioTextNeedsUpdate(pTbl->GetFooter(2), FMT_ID_FOOTER_RIGHT, dlg.m_sFooterRight, pFmtReg)) {
            if (!bPushedUndo) {
                PushFormatPrintCommand(iTbl);
                bPushedUndo=true;
            }
            UpdateFolioText(pTbl->GetFooter(2), FMT_ID_FOOTER_RIGHT, dlg.m_sFooterRight, pFmtReg);
            bBuild = true;
        }

        LOGFONT lfHeader;
        if (dlg.m_bDefaultHeaderFont) {
            lfHeader=dlg.m_lfDefaultHeader;
        }
        else {
            lfHeader=dlg.m_lfHeader;
        }
        bool bHeaderFmtChanged=(memcmp(&lfHeader, &lfHeaderCurrent, sizeof(LOGFONT))!=0 || (bDefaultHeaderFont!=dlg.m_bDefaultHeaderFont));
        if (bHeaderFmtChanged) {
            if (!bPushedUndo) {
                PushFormatPrintCommand(iTbl);
                bPushedUndo=true;
            }

            // we'll create 3 new header formats, one each for left/center/right, and apply them

            // ... left header
            CFmt* pHeaderLeftFmt=new CFmt;
            if (pTbl->GetHeader(0)->GetDerFmt()) {
                *pHeaderLeftFmt=*pTbl->GetHeader(0)->GetDerFmt();
            }
            else {
                pHeaderLeftFmt->SetID(FMT_ID_HEADER_LEFT);
            }
            iIndex=pFmtReg->GetNextCustomFmtIndex(FMT_ID_HEADER_LEFT);
            pHeaderLeftFmt->SetIndex(iIndex);
            if (dlg.m_bDefaultHeaderFont) {
                pHeaderLeftFmt->SetFont((CFont*)NULL);
            }
            else {
                pHeaderLeftFmt->SetFont(&dlg.m_lfHeader);
            }

            if (!pFmtReg->AddFmt(pHeaderLeftFmt)) {
                ASSERT(FALSE);
            }
            pTbl->GetHeader(0)->SetFmt(pHeaderLeftFmt);

            // ... center header
            CFmt* pHeaderCenterFmt=new CFmt;
            if (pTbl->GetHeader(1)->GetDerFmt()) {
                *pHeaderCenterFmt=*pTbl->GetHeader(1)->GetDerFmt();
            }
            else {
                pHeaderCenterFmt->SetID(FMT_ID_HEADER_CENTER);
            }
            iIndex=pFmtReg->GetNextCustomFmtIndex(FMT_ID_HEADER_CENTER);
            pHeaderCenterFmt->SetIndex(iIndex);
            if (dlg.m_bDefaultHeaderFont) {
                pHeaderCenterFmt->SetFont((CFont*)NULL);
            }
            else {
                pHeaderCenterFmt->SetFont(&dlg.m_lfHeader);
            }

            if (!pFmtReg->AddFmt(pHeaderCenterFmt)) {
                ASSERT(FALSE);
            }
            pTbl->GetHeader(1)->SetFmt(pHeaderCenterFmt);

            // ... right header
            CFmt* pHeaderRightFmt=new CFmt;
            if (pTbl->GetHeader(2)->GetDerFmt()) {
                *pHeaderRightFmt=*pTbl->GetHeader(2)->GetDerFmt();
            }
            else {
                pHeaderRightFmt->SetID(FMT_ID_HEADER_RIGHT);
            }
            iIndex=pFmtReg->GetNextCustomFmtIndex(FMT_ID_HEADER_RIGHT);
            pHeaderRightFmt->SetIndex(iIndex);
            if (dlg.m_bDefaultHeaderFont) {
                pHeaderRightFmt->SetFont((CFont*)NULL);
            }
            else {
                pHeaderRightFmt->SetFont(&dlg.m_lfHeader);
            }

            if (!pFmtReg->AddFmt(pHeaderRightFmt)) {
                ASSERT(FALSE);
            }
            pTbl->GetHeader(2)->SetFmt(pHeaderRightFmt);

            bBuild=true;
        }

        LOGFONT lfFooter;
        if (dlg.m_bDefaultFooterFont) {
            lfFooter=dlg.m_lfDefaultFooter;
        }
        else {
            lfFooter=dlg.m_lfFooter;
        }
        bool bFooterFmtChanged=(memcmp(&lfFooter, &lfFooterCurrent, sizeof(LOGFONT))!=0 || (bDefaultFooterFont!=dlg.m_bDefaultFooterFont));
        if (bFooterFmtChanged) {
            if (!bPushedUndo) {
                PushFormatPrintCommand(iTbl);
                bPushedUndo=true;
            }

            // we'll create 3 new footer formats, one each for left/center/right, and apply them

            // ... left footer
            CFmt* pFooterLeftFmt=new CFmt;
            if (pTbl->GetFooter(0)->GetDerFmt()) {
                *pFooterLeftFmt=*pTbl->GetFooter(0)->GetDerFmt();
            }
            else {
                pFooterLeftFmt->SetID(FMT_ID_FOOTER_LEFT);
            }
            iIndex=pFmtReg->GetNextCustomFmtIndex(FMT_ID_FOOTER_LEFT);
            pFooterLeftFmt->SetIndex(iIndex);
            if (dlg.m_bDefaultFooterFont) {
                pFooterLeftFmt->SetFont((CFont*)NULL);
            }
            else {
                pFooterLeftFmt->SetFont(&dlg.m_lfFooter);
            }

            if (!pFmtReg->AddFmt(pFooterLeftFmt)) {
                ASSERT(FALSE);
            }
            pTbl->GetFooter(0)->SetFmt(pFooterLeftFmt);

            // ... center footer
            CFmt* pFooterCenterFmt=new CFmt;
            if (pTbl->GetFooter(1)->GetDerFmt()) {
                *pFooterCenterFmt=*pTbl->GetFooter(1)->GetDerFmt();
            }
            else {
                pFooterCenterFmt->SetID(FMT_ID_FOOTER_CENTER);
            }
            iIndex=pFmtReg->GetNextCustomFmtIndex(FMT_ID_FOOTER_CENTER);
            pFooterCenterFmt->SetIndex(iIndex);
            if (dlg.m_bDefaultFooterFont) {
                pFooterCenterFmt->SetFont((CFont*)NULL);
            }
            else {
                pFooterCenterFmt->SetFont(&dlg.m_lfFooter);
            }

            if (!pFmtReg->AddFmt(pFooterCenterFmt)) {
                ASSERT(FALSE);
            }
            pTbl->GetFooter(1)->SetFmt(pFooterCenterFmt);

            // ... right footer
            CFmt* pFooterRightFmt=new CFmt;
            if (pTbl->GetFooter(2)->GetDerFmt()) {
                *pFooterRightFmt=*pTbl->GetFooter(2)->GetDerFmt();
            }
            else {
                pFooterRightFmt->SetID(FMT_ID_FOOTER_RIGHT);
            }
            iIndex=pFmtReg->GetNextCustomFmtIndex(FMT_ID_FOOTER_RIGHT);
            pFooterRightFmt->SetIndex(iIndex);
            if (dlg.m_bDefaultFooterFont) {
                pFooterRightFmt->SetFont((CFont*)NULL);
            }
            else {
                pFooterRightFmt->SetFont(&dlg.m_lfFooter);
            }

            if (!pFmtReg->AddFmt(pFooterRightFmt)) {
                ASSERT(FALSE);
            }
            pTbl->GetFooter(2)->SetFmt(pFooterRightFmt);

            bBuild=true;
        }

        if (bBuild) {
            Build(true);
            pDoc->SetModifiedFlag(TRUE);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                             OnEditColBreak
//
// Toggles the page break attribute for a column head.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnEditColBreak()
{
    ASSERT(m_aSelected.GetSize()>0);
    CTblPrtViewHitOb& obHit=m_aSelected[0];
    ASSERT(obHit.GetPg()>NONE && obHit.GetPgOb()>NONE);
    CPgOb& pgobColBreak=m_pgMgr.GetPgLayout(obHit.GetPg()).GetPgOb(obHit.GetPgOb());

    // store resize info for undo/redo
    PushBoxheadResizeForUndo(pgobColBreak.GetTbl());

    // toggle previous page break setting
    bool bPageBreak=!pgobColBreak.IsPageBreak();
    pgobColBreak.SetPageBreak(bPageBreak);

    // find this pgob's occurrence in its tblob's prtviewinfo array...
    int iOcc=-1;
    for (int iPg=0 ; iPg<m_pgMgr.GetNumPages() ; iPg++) {
        CPgLayout& pl=m_pgMgr.GetPgLayout(iPg);
        for (int iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
            CPgOb& pgob=pl.GetPgOb(iPgOb);
            if (pgob.GetTblBase()==pgobColBreak.GetTblBase()) {
                iOcc++;
            }
            if (obHit.GetPg()==iPg && obHit.GetPgOb()==iPgOb) {
                // done
                iPg=m_pgMgr.GetNumPages();  // breaks out of outer loop
                break;
            }
        }
    }

    if (m_bApplyAcrossPanels) {
        // apply this resize setting across all panels ...
        for (int iPrtViewInfo=0 ; iPrtViewInfo<pgobColBreak.GetTblBase()->GetPrtViewInfoSize() ; iPrtViewInfo++) {
            CPrtViewInfo& pviAcross=pgobColBreak.GetTblBase()->GetPrtViewInfo(iPrtViewInfo);
            pviAcross.SetPageBreak(bPageBreak);
        }
    }
    else {
        // update the specific occurrence within the tbl ob's printviewinfo array ...
        pgobColBreak.GetTblBase()->GetPrtViewInfo(iOcc).SetPageBreak(bPageBreak);
    }

    Build(true);   // will restore current page view and zoom settings
    GetDocument()->SetModifiedFlag(TRUE);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             OnEditStubBreak
//
// Toggles the page break attribute for a stub or caption.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnEditStubBreak()
{
    ASSERT(m_aSelected.GetSize()>0);
    CTblPrtViewHitOb& obHit=m_aSelected[0];
    ASSERT(obHit.GetPg()>NONE && obHit.GetPgOb()>NONE);
    CPgOb& pgobStubBreak=m_pgMgr.GetPgLayout(obHit.GetPg()).GetPgOb(obHit.GetPgOb());

    // store resize info for undo/redo
    PushStubResizeForUndo(pgobStubBreak.GetTbl());

    // toggle previous page break setting
    bool bPageBreak=!pgobStubBreak.IsPageBreak();
    pgobStubBreak.SetPageBreak(bPageBreak);

    // find this pgob's occurrence in its tblob's prtviewinfo array...
    int iOcc=-1;
    for (int iPg=0 ; iPg<m_pgMgr.GetNumPages() ; iPg++) {
        CPgLayout& pl=m_pgMgr.GetPgLayout(iPg);
        for (int iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
            CPgOb& pgob=pl.GetPgOb(iPgOb);
            if (pgob.GetTblBase()==pgobStubBreak.GetTblBase()) {
                iOcc++;
            }
            if (obHit.GetPg()==iPg && obHit.GetPgOb()==iPgOb) {
                // done
                iPg=m_pgMgr.GetNumPages();  // breaks out of outer loop
                break;
            }
        }
    }

    if (m_bApplyAcrossPanels) {
        // apply this resize setting across all panels ...
        for (int iPrtViewInfo=0 ; iPrtViewInfo<pgobStubBreak.GetTblBase()->GetPrtViewInfoSize() ; iPrtViewInfo++) {
            CPrtViewInfo& pviAcross=pgobStubBreak.GetTblBase()->GetPrtViewInfo(iPrtViewInfo);
            pviAcross.SetPageBreak(bPageBreak);
        }
    }
    else {
        // update the specific occurrence within the tbl ob's printviewinfo array ...
        pgobStubBreak.GetTblBase()->GetPrtViewInfo(iOcc).SetPageBreak(bPageBreak);
    }
    Build(true);   // will restore current page view and zoom settings
    GetDocument()->SetModifiedFlag(TRUE);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             OnEditApplyAcrossPanels
//
// Toggles whether or not to apply resize and other operations across
// panels of stubs/columns.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnEditApplyAcrossPanels()
{
    m_bApplyAcrossPanels=!m_bApplyAcrossPanels;
}


/////////////////////////////////////////////////////////////////////////////
//
//                             OnEditAutoFitColumns
//
// Toggles whether or not to automatically fit columns across each horz page.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnEditAutoFitColumns()
{
    m_bAutoFitColumns=!m_bAutoFitColumns;

    // if user has turned ON autofit, then rebuild the layout to show the effect
    //if (m_bAutoFitColumns) {
	if(true){//rebuild all the time
        CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
        //CTabSet* pSet = pDoc->GetTableSpec();
		//Savy (R) sampling app 20081224
		CTabSet* pSet = NULL;

		if(!pDoc){
			ASSERT(m_pTabSet);
			pSet = m_pTabSet;
		}
		else{
			pSet = pDoc->GetTableSpec();
		}
        for (int iTbl=0 ; iTbl<pSet->GetNumTables() ; iTbl++) {
            PushBoxheadResizeForUndo(iTbl);
        }
        Build(true);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                             OnRestorePrtViewDefaults
//
// Restores default table layout, thus nuking any customized size settings
// and page breaks.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnRestorePrtViewDefaults()
{
    ASSERT(m_aSelected.GetSize()>0);

    if (AfxMessageBox(_T("Restore default layout for this table?"), MB_YESNOCANCEL|MB_ICONQUESTION)!=IDYES) {
        return;
    }

    bool bDirty=false;
    CTblPrtViewHitOb obHit=m_aSelected[0];
    int iTbl=m_pgMgr.GetPgLayout(obHit.GetPg()).GetPgOb(obHit.GetPgOb()).GetTbl();

    // prep for undo/redo
    PushRestoreDefaultsForUndo(iTbl);

    // loop through all pages in the layout ...
    for (int iPg=0 ; iPg<m_pgMgr.GetNumPages() ; iPg++) {
        CPgLayout& pl=m_pgMgr.GetPgLayout(iPg);

        // loop through all the page's objects
        for (int iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
            CPgOb& pgob=pl.GetPgOb(iPgOb);

            // only handle pgobs for the table being restored
            if (pgob.GetTbl()!=iTbl) {
                continue;
            }

            if (pgob.GetTblBase()!=NULL) {

                // loop through all the object's prtview saved info ...
                for (int iPrtViewInfo=0 ; iPrtViewInfo<pgob.GetTblBase()->GetPrtViewInfoSize() ; iPrtViewInfo++) {
                    CPrtViewInfo& pvi=pgob.GetTblBase()->GetPrtViewInfo(iPrtViewInfo);

                    // turn off any customization that each prtviewinfo might have ...
                    if (pvi.IsCustom() || pvi.IsPageBreak()) {
                        bDirty=true;
                    }
                    pgob.GetTblBase()->RemoveAllPrtViewInfo();
                }
            }
            else {
                ASSERT(pgob.GetType()==PGOB_DATACELL);
            }
        }
    }

    if (bDirty) {
        GetDocument()->SetModifiedFlag(TRUE);
    }
    Build();
}


/////////////////////////////////////////////////////////////////////////////
//
//                             GotoTbl
//
// Scrolls through the document so that table iTbl is visible, and preferably
// at the upper-left corner (if multi-page viewing is active).
//
// iTbl is 0-based
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::GotoTbl(int iTbl, bool bRedraw /* = true */)
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}

    // sanity checks
    ASSERT(iTbl>=0 && iTbl<pSet->GetNumTables());

    // figure out which page has the first object that is from this table ...
    for (int iPg=0 ; iPg<m_pgMgr.GetNumPages() ; iPg++) {
        const CPgLayout& pl=m_pgMgr.GetPgLayout(iPg);
        for (int iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
            const CPgOb& pgob=pl.GetPgOb(iPgOb);
            if (pgob.GetTbl()==iTbl) {
                // found something from this table ... show the page!
                GotoPage(iPg, bRedraw);
                return;
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                             GetPageRangeForTable
//
// Finds first and last page for a given table
//
// iTbl is 0-based, so are page nums
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::GetPageRangeForTable(int iTbl, int& iPgStart, int& iPgEnd)
{
    iPgStart = m_pgMgr.GetNumPages();
    iPgEnd = 0;
    for (int iPg=0 ; iPg<m_pgMgr.GetNumPages() ; iPg++) {
        const CPgLayout& pl=m_pgMgr.GetPgLayout(iPg);
        for (int iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {
            const CPgOb& pgob=pl.GetPgOb(iPgOb);
            if (pgob.GetTbl()==iTbl) {
                if (iPg < iPgStart) {
                    iPgStart = iPg;
                }
                if (iPg > iPgEnd) {
                    iPgEnd = iPg;
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                             GetPagesPerScreen
//
// Get num pages viewed on screen at same time
//
/////////////////////////////////////////////////////////////////////////////
int CTabPrtView::GetPagesPerScreen() const
{
    return GetNumHorzPgs() * GetNumVertPgs();
}


/////////////////////////////////////////////////////////////////////////////
//
//                             GetCurrFirstViewPg
//
// Get the first page of all pages being viewed.
//
/////////////////////////////////////////////////////////////////////////////
int CTabPrtView::GetCurrFirstViewPg() const
{
    int iCurrPage = m_aiViewPg[0];
    if (iCurrPage < 0) {
        iCurrPage = 0;
    }

    return iCurrPage;
}

/////////////////////////////////////////////////////////////////////////////
//
//                             GetCurrLastViewPg
//
// Get the first page of all pages being viewed.
//
/////////////////////////////////////////////////////////////////////////////
int CTabPrtView::GetCurrLastViewPg() const
{
    for (int i = GetNumViewPgs()-1; i >= 0; --i) {
        if (m_aiViewPg[i] != NONE) {
            return m_aiViewPg[i];
        }
    }
    return NONE;
}

/////////////////////////////////////////////////////////////////////////////
//
//                             GotoPage
//
// Scrolls through the document so that page iPage is visible, and preferably
// at the upper-left corner (if multi-page viewing is active).
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::GotoPage(int iPage, bool bRedraw /* = true */)
{
    int iNumViewPages=m_aiViewPg.GetSize();
    bool bScrolled=false;

    ASSERT(iPage>=0 && iPage<m_pgMgr.GetNumPages());
    ASSERT(GetNumVertPgs()*GetNumHorzPgs()==iNumViewPages);

    SetRedraw(FALSE);

    if (GetZoomState()<=ZOOM_STATE_100_PERCENT) {

        if (iPage != GetCurrFirstViewPg()) {

            bScrolled = true;

            // update array of pages that are being concurrently viewed ...
            m_aiViewPg.RemoveAll();

            // convert page to scroll pos and update scroll bar
            int iScrollPos = iPage;
            if (IsBookLayout() && iPage != 0) {
                iScrollPos++; // first blank page
            }
            m_pVSBar->SetScrollPos(iScrollPos);

            int iStartPage = iPage; // used to iterate through pages that will be viewed

            if (IsBookLayout()) {
                iStartPage = iPage % 2 + iPage; // don't allow start on odd page
                iStartPage--; // make space for blank first page
            }

            for (int iPgDown=0 ; iPgDown<GetNumVertPgs() ; iPgDown++) {
                for (int iPgAcross=0 ; iPgAcross<GetNumHorzPgs() ; iPgAcross++) {
                    if (iStartPage<0) {
                        // insert blank placeholder in front of page 1; this puts even pages on the left and odd pages on the right
                        m_aiViewPg.Add(NONE);
                        iStartPage++;
                    }
                    else if (iStartPage<m_pgMgr.GetNumPages()) {
                        // add the next page to the list
                        m_aiViewPg.Add(iStartPage++);
                    }
                    else {
                        // no pages left, add placeholders
                        m_aiViewPg.Add(NONE);
                    }
                }
            }
        }
    }
    else {
        // zooming ... just swap to the desired page
        bScrolled=(iPage!=m_aiViewPg[0]);
        m_aiViewPg[0]=iPage;
    }

    // update the tree
    if (bScrolled) {
        CTabulateDoc* pDoc = (CTabulateDoc*)GetDocument();
		if(pDoc){//Savy&&& take care of this
        CTabTreeCtrl* pTreeCtrl = pDoc->GetTabTreeCtrl();
	    const int iTbl = m_pgMgr.GetPgLayout(GetCurrFirstViewPg()).GetPgOb(0).GetTbl();
	    CTabSet* pSet = pDoc->GetTableSpec();
        CTable* pTbl = pSet->GetTable(iTbl);
        CWnd* pOldFocus = GetFocus();
        pTreeCtrl->SelectTable(pTbl, false); // update tree, but tell it not to update page
                                             // so we don't get infinite recursion
        if (GetFocus() != pOldFocus) {
            pOldFocus->SetFocus();  // SelectTable moves focus over to tree ctrl,
                                    // want to keep it on CTabPrtView so key shortcuts still work
			}
        }
    }

    SetRedraw(TRUE);
    if (bScrolled && bRedraw) {
        Invalidate();
    }
    GetNavBar().SetPageInfo(GetCurrFirstViewPg()+1, m_pgMgr.GetNumPages());
}

/////////////////////////////////////////////////////////////////////////////
//
//                             GotoArea
//
// Scrolls through the document so that the page of  table iTbl with area
// sArea is visible, and preferably at the upper-left corner (if multi-page viewing is active).
//
// iTbl is 0-based
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::GotoArea(int iTbl, const CString& sArea, bool bRedraw /* = true */)
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}

    // sanity checks
    ASSERT(iTbl>=0 && iTbl<pSet->GetNumTables());

    // figure out which page has the first object that is from this table ...
    for (int iPg=0 ; iPg<m_pgMgr.GetNumPages() ; iPg++) {
        const CPgLayout& pl=m_pgMgr.GetPgLayout(iPg);
        for (int iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {

            const CPgOb& pgob=pl.GetPgOb(iPgOb);
            if (pgob.GetTbl()!=iTbl) {
                continue;
            }

            // look for a caption or stub
            // with text or custom text that matches %AreaName%
            if (pgob.GetType() == PGOB_CAPTION ||
                pgob.GetType() == PGOB_STUB) {
                CTblBase* pTblBase = pgob.GetTblBase();
                if (pTblBase) {
                    CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt, pTblBase->GetFmt());
                    if (pTblBase->GetText().CompareNoCase(AREA_TOKEN) == 0 ||
                        (pFmt && pFmt->IsTextCustom() && (pFmt->GetCustomText().CompareNoCase(AREA_TOKEN) == 0))) {
                            if (sArea == pgob.GetText()) {
                                GotoPage(iPg, bRedraw);
                                return;
                            }
                    }
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                             GetFirstAreaOnPage
//
// Finds the first area on the given page that is displayed.
//
/////////////////////////////////////////////////////////////////////////////
CString CTabPrtView::GetFirstAreaOnPage(int iPg)
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}

    // sanity checks
    ASSERT(iPg>=0 && iPg<m_pgMgr.GetNumPages());

    const CPgLayout& pl=m_pgMgr.GetPgLayout(iPg);
    for (int iPgOb=0 ; iPgOb<pl.GetNumPgObs() ; iPgOb++) {

        const CPgOb& pgob=pl.GetPgOb(iPgOb);

        // look for a caption or stub
        // with text or custom text that matches %AreaName%
        if (pgob.GetType() == PGOB_CAPTION ||
            pgob.GetType() == PGOB_STUB) {
            CTblBase* pTblBase = pgob.GetTblBase();
            if (pTblBase) {
                CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt, pTblBase->GetFmt());
                if (pTblBase->GetText().CompareNoCase(AREA_TOKEN) == 0 ||
                    (pFmt && pFmt->IsTextCustom() && (pFmt->GetCustomText().CompareNoCase(AREA_TOKEN) == 0))) {
                        return pgob.GetText();
                }
            }
        }
    }
    return CString();
}

/////////////////////////////////////////////////////////////////////////////
//
//                             OnViewFirstPage
//
// Move prt view to first page.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnViewFirstPage()
{
    GotoPage(0);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             OnViewLastPage
//
// Move prt view to last page.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnViewLastPage()
{
    GotoPage(m_pgMgr.GetNumPages()-1);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             OnViewPrevPage
//
// Move prt view to previous page.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnViewPrevPage()
{
    int iPage=GetCurrFirstViewPg();
    if (iPage > 0) {
        if (IsBookLayout()) {
            if (iPage == 1) { // count by 2's for book layout
                GotoPage(0);
            }
            else {
                GotoPage(iPage-2);
            }
        }
        else {
            GotoPage(iPage-1);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                             OnViewNextPage
//
// Move prt view to next page.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnViewNextPage()
{
    int iPage=GetCurrFirstViewPg();
    if (IsBookLayout()) {
        if (iPage < m_pgMgr.GetNumPages() - 2) { // count by 2's for book layout
            if (iPage == 0) {
                GotoPage(1);
            }
            else {
                GotoPage(iPage+2);
            }
        }
    }
    else {
        if (iPage < m_pgMgr.GetNumPages()-1) {
            GotoPage(iPage+1);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                             OnUpdateViewFirstPage
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnUpdateViewFirstPage(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(GetCurrFirstViewPg()>0);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             OnUpdateViewLastPage
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnUpdateViewLastPage(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(GetCurrFirstViewPg() < m_pgMgr.GetNumPages()-1);
}

/////////////////////////////////////////////////////////////////////////////
//
//                             OnUpdateViewPrevPage
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnUpdateViewPrevPage(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(GetCurrFirstViewPg()>0);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             OnUpdateViewNextPage
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnUpdateViewNextPage(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(GetCurrFirstViewPg() < m_pgMgr.GetNumPages()-1);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             UndoBoxheadResize
//
// Undo/redo action for boxhead resizing.  Called in response to an ID_EDIT_UNDO
// or ID_EDIT_REDO command.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::UndoBoxheadResize(CArray<CResizeCmdInfo, CResizeCmdInfo&>& aResizeCmdInfo, int iTbl)
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    CTable* pTbl=pSet->GetTable(iTbl);
    const CFmtReg& fmtReg = pSet->GetFmtReg();
    int iColHead;

    CRowColPgOb root;
    BuildRowColTree(pTbl->GetColRoot(), &root, fmtReg, true);
    root.SetType(PGOB_ROOT);

    CArray<CRowColPgOb*, CRowColPgOb*> aColHead;
    root.GetLeaves(aColHead);         // note: aColHead does *not* include spanners, since we have to deal with those recursively

    for (iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
        CRowColPgOb* pColHead = aColHead[iColHead];
        CTblBase* pTblBaseOb=pColHead->GetTblBase();

        // sanity checks ...
        ASSERT(iColHead==aResizeCmdInfo[iColHead].m_iIndex);
//        ASSERT(pTblBaseOb->GetPrtViewInfoSize()==aResizeCmdInfo[iColHead].m_aPrtViewInfo.GetSize());

        // remove all prtview info from the colheads ...
        pTblBaseOb->RemoveAllPrtViewInfo();

        // move the saved prtviewinfo into the col head
        CArray<CPrtViewInfo, CPrtViewInfo&>& pvi=aResizeCmdInfo[iColHead].m_aPrtViewInfo;
        for (int iPVI=0 ; iPVI<pvi.GetSize() ; iPVI++)  {
            pTblBaseOb->AddPrtViewInfo(pvi[iPVI]);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                             UndoStubResize
//
// Undo/redo action for stub resizing.  Called in response to an ID_EDIT_UNDO
// or ID_EDIT_REDO command.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::UndoStubResize(CArray<CResizeCmdInfo, CResizeCmdInfo&>& aResizeCmdInfo, int iTbl)
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    CTable* pTbl=pSet->GetTable(iTbl);
    const CFmtReg& fmtReg = pSet->GetFmtReg();
    int iStub;

    CRowColPgOb root;
    root.SetTbl(iTbl);
    BuildRowColTree(pTbl->GetRowRoot(), &root, fmtReg, false);
    root.SetType(PGOB_ROOT);

    CArray<CRowColPgOb*, CRowColPgOb*> aStub;
    root.GetLeavesAndNodes(aStub);    // note: aStub does include captions, since those line up without needing recursion

    for (iStub=0 ; iStub<aStub.GetSize() ; iStub++) {
        CRowColPgOb* pStub = aStub[iStub];
        CTblBase* pTblBaseOb=pStub->GetTblBase();

        // sanity checks ...
        ASSERT(iStub==aResizeCmdInfo[iStub].m_iIndex);
//        ASSERT(pTblBaseOb->GetPrtViewInfoSize()==aResizeCmdInfo[iStub].m_aPrtViewInfo.GetSize());

        // remove all prtview info from the stubs and captions ...
        pTblBaseOb->RemoveAllPrtViewInfo();

        // move the saved prtviewinfo into the col head
        CArray<CPrtViewInfo, CPrtViewInfo&>& pvi=aResizeCmdInfo[iStub].m_aPrtViewInfo;
        for (int iPVI=0 ; iPVI<pvi.GetSize() ; iPVI++)  {
            pTblBaseOb->AddPrtViewInfo(pvi[iPVI]);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                             UndoStubHeadResize
//
// Undo/redo action for stubhead resizing.  Called in response to an ID_EDIT_UNDO
// or ID_EDIT_REDO command.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::UndoStubHeadResize(CArray<CResizeCmdInfo, CResizeCmdInfo&>& aResizeCmdInfo, int iTbl)
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    CTable* pTbl=pSet->GetTable(iTbl);

    for (int iStubHead=0 ; iStubHead<2 ; iStubHead++) {
        CTblOb* pStubHead=pTbl->GetStubhead(iStubHead);

        // sanity checks ...
        ASSERT(iStubHead==aResizeCmdInfo[iStubHead].m_iIndex);
        ASSERT(pStubHead->GetPrtViewInfoSize()==aResizeCmdInfo[iStubHead].m_aPrtViewInfo.GetSize());

        // remove all prtview info from the stubs and captions ...
        pStubHead->RemoveAllPrtViewInfo();

        // move the saved prtviewinfo into the col head
        CArray<CPrtViewInfo, CPrtViewInfo&>& pvi=aResizeCmdInfo[iStubHead].m_aPrtViewInfo;
        for (int iPVI=0 ; iPVI<pvi.GetSize() ; iPVI++)  {
            pStubHead->AddPrtViewInfo(pvi[iPVI]);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                             UndoFormatPrint
//
// Undo/redo action for format/print dialog options.  Called in response to an ID_EDIT_UNDO
// or ID_EDIT_REDO command.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::UndoFormatPrint(CPrintFormatCmdInfo& formatInfo, int iTbl)
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    CTable* pTbl=pSet->GetTable(iTbl);

    CTblPrintFmt* pTblPrintFmt=pTbl->GetTblPrintFmt();
    pTblPrintFmt->SetHeaderFrequency(formatInfo.GetHeaderFrequency());
    pTblPrintFmt->SetPageMargin(formatInfo.GetPageMargin());
    pTblPrintFmt->SetStartPage(formatInfo.GetStartPage());
    pTblPrintFmt->SetTblLayout(formatInfo.GetTblLayout());

    for (int i=0 ; i<3 ; i++) {
        *pTbl->GetHeader(i)=*formatInfo.GetHeader(i);
        *pTbl->GetFooter(i)=*formatInfo.GetFooter(i);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                             PushBoxheadResizeForUndo
//
// Stores boxhead sizing information, thus allowing for undo/redo operations.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::PushBoxheadResizeForUndo(int iTbl, bool bUndo /*=true*/)
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    CTable* pTbl=pSet->GetTable(iTbl);
    const CFmtReg& fmtReg = pSet->GetFmtReg();

    CRowColPgOb root;
    BuildRowColTree(pTbl->GetColRoot(), &root, fmtReg, true);
    root.SetType(PGOB_ROOT);

    CArray<CRowColPgOb*, CRowColPgOb*> aColHead;
    root.GetLeaves(aColHead);         // note: aColHead does *not* include spanners, since we have to deal with those recursively

    auto pCmd = std::make_shared<CBoxheadResizeCommand>(this, iTbl);
    bool bAutoFitColumns=false;
    for (int iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
        CRowColPgOb* pColHead=aColHead[iColHead];
        CTblBase* pTblBaseOb=pColHead->GetTblBase();
        CResizeCmdInfo c;
        c.m_iIndex=iColHead;

        for (int iPVI=0 ; iPVI<pTblBaseOb->GetPrtViewInfoSize() ; iPVI++) {
            c.m_aPrtViewInfo.Add(pTblBaseOb->GetPrtViewInfo(iPVI));

            if (pTblBaseOb->GetPrtViewInfo(iPVI).GetExtraSize()!=CSize(0,0)) {
                bAutoFitColumns=true;
            }
        }
        pCmd->Add(c);
    }

    // We have to store the autofit columns setting because the build operation following an
    // undo/redo has to have it set the same as when the undo/redo push was done.  But the
    // setting might now be different from what was used to get the current layout, so we
    // deduce it: if any colhead has extra space, then auto-fit is live, otherwise it isn't.
    pCmd->SetAutoFitColumns(bAutoFitColumns);

    if (bUndo) {
        m_undoStack.PushUndo(pCmd);
    }
    else {
        m_undoStack.PushRedo(pCmd);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                             PushStubResizeForUndo
//
// Stores stub sizing information, thus allowing for undo/redo operations.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::PushStubResizeForUndo(int iTbl, bool bUndo /*=true*/)
{
	//Today SAvy changes 20081224
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    CTable* pTbl=pSet->GetTable(iTbl);
    const CFmtReg& fmtReg = pSet->GetFmtReg();

    CRowColPgOb root;
    root.SetTbl(iTbl);
    BuildRowColTree(pTbl->GetRowRoot(), &root, fmtReg, false);
    root.SetType(PGOB_ROOT);

    CArray<CRowColPgOb*, CRowColPgOb*> aStub;
    root.GetLeavesAndNodes(aStub);    // note: aStub does include captions, since those line up without needing recursion

    auto pCmd = std::make_shared<CStubResizeCommand>(this, iTbl);
    for (int iStub=0 ; iStub<aStub.GetSize() ; iStub++) {
        CRowColPgOb* pStub=aStub[iStub];
        CTblBase* pTblBaseOb=pStub->GetTblBase();
        CResizeCmdInfo c;
        c.m_iIndex=iStub;

        for (int iPVI=0 ; iPVI<pTblBaseOb->GetPrtViewInfoSize() ; iPVI++) {
            c.m_aPrtViewInfo.Add(pTblBaseOb->GetPrtViewInfo(iPVI));
        }
        pCmd->Add(c);
    }

    if (bUndo) {
        m_undoStack.PushUndo(pCmd);
    }
    else {
        m_undoStack.PushRedo(pCmd);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                             PushRestoreDefaultsForUndo
//
// Stores information for undoing/redoing a "restore defaults" command.  This
// involves resizing information for:
//    - boxheads
//    - stubs/captions
//    - stubheads
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::PushRestoreDefaultsForUndo(int iTbl, bool bUndo /*=true*/)
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    CTable* pTbl=pSet->GetTable(iTbl);
    const CFmtReg& fmtReg = pSet->GetFmtReg();

    auto pCmd = std::make_shared<CRestoreDefaultsCommand>(this, iTbl);

    //////////////////////////////////////////////////
    // store boxhead resizing information ...
    //////////////////////////////////////////////////
    CRowColPgOb rootCol;
    BuildRowColTree(pTbl->GetColRoot(), &rootCol, fmtReg, true);
    rootCol.SetType(PGOB_ROOT);

    CArray<CRowColPgOb*, CRowColPgOb*> aColHead;
    rootCol.GetLeaves(aColHead);         // note: aColHead does *not* include spanners, since we have to deal with those recursively

    for (int iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
        CRowColPgOb* pColHead=aColHead[iColHead];
        CTblBase* pTblBaseOb=pColHead->GetTblBase();
        CResizeCmdInfo c;
        c.m_iIndex=iColHead;

        for (int iPVI=0 ; iPVI<pTblBaseOb->GetPrtViewInfoSize() ; iPVI++) {
            c.m_aPrtViewInfo.Add(pTblBaseOb->GetPrtViewInfo(iPVI));
        }
        pCmd->AddBoxhead(c);
    }

    //////////////////////////////////////////////////
    // store stub/caption resizing information ...
    //////////////////////////////////////////////////
    CRowColPgOb rootRow;
    rootRow.SetTbl(iTbl);
    BuildRowColTree(pTbl->GetRowRoot(), &rootRow, fmtReg, false);
    rootRow.SetType(PGOB_ROOT);

    CArray<CRowColPgOb*, CRowColPgOb*> aStub;
    rootRow.GetLeavesAndNodes(aStub);    // note: aStub does include captions, since those line up without needing recursion

    for (int iStub=0 ; iStub<aStub.GetSize() ; iStub++) {
        CRowColPgOb* pStub=aStub[iStub];
        CTblBase* pTblBaseOb=pStub->GetTblBase();
        CResizeCmdInfo c;
        c.m_iIndex=iStub;

        for (int iPVI=0 ; iPVI<pTblBaseOb->GetPrtViewInfoSize() ; iPVI++) {
            c.m_aPrtViewInfo.Add(pTblBaseOb->GetPrtViewInfo(iPVI));
        }
        pCmd->AddStub(c);
    }

    //////////////////////////////////////////////////
    // store stubhead resizing information ...
    //////////////////////////////////////////////////
    for (int iStubHead=0 ; iStubHead<2 ; iStubHead++) {
        CTblOb* pStubHead=pTbl->GetStubhead(iStubHead);
        CResizeCmdInfo c;
        c.m_iIndex=iStubHead;

        for (int iPVI=0 ; iPVI<pStubHead->GetPrtViewInfoSize() ; iPVI++) {
            c.m_aPrtViewInfo.Add(pStubHead->GetPrtViewInfo(iPVI));
        }
        pCmd->AddStubHead(c);
    }

    if (bUndo) {
        m_undoStack.PushUndo(pCmd);
    }
    else {
        m_undoStack.PushRedo(pCmd);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                             PushFormatPrintCommand
//
// Stores settings from the format print dialog box, for undoing/redoing
// these.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::PushFormatPrintCommand(int iTbl, bool bUndo /*=true*/)
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    CTable* pTbl=pSet->GetTable(iTbl);

    CTblPrintFmt* pTblPrintFmt=pTbl->GetTblPrintFmt();
    if (pTblPrintFmt == NULL) {
        const CFmtReg& fmtReg = pSet->GetFmtReg();
        pTblPrintFmt = DYNAMIC_DOWNCAST(CTblPrintFmt, fmtReg.GetFmt(FMT_ID_TBLPRINT));
    }

    auto pCmd = std::make_shared<CFormatPrintCommand>(this, iTbl);
    CPrintFormatCmdInfo& formatInfo=pCmd->GetPrintFormatCmdInfo();

    formatInfo.SetHeaderFrequency(pTblPrintFmt->GetHeaderFrequency());
    formatInfo.SetPageMargin(pTblPrintFmt->GetPageMargin());
    formatInfo.SetStartPage(pTblPrintFmt->GetStartPage());
    formatInfo.SetTblLayout(pTblPrintFmt->GetTblLayout());

    for (int i=0 ; i<3 ; i++) {
        *formatInfo.GetHeader(i)=*pTbl->GetHeader(i);
        *formatInfo.GetFooter(i)=*pTbl->GetFooter(i);
    }

    if (bUndo) {
        m_undoStack.PushUndo(pCmd);
    }
    else {
        m_undoStack.PushRedo(pCmd);
    }
}



/////////////////////////////////////////////////////////////////////////////
//
//                             OnEditUndo
//
// Performs undo operation, and preps for redo.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnEditUndo()
{
    ASSERT(m_undoStack.CanUndo());
    std::shared_ptr<CPrtViewCommand> pCmd = m_undoStack.Undo();

    // push onto the redo stack
    if (pCmd->IsKindOf(RUNTIME_CLASS(CBoxheadResizeCommand))) {
        PushBoxheadResizeForUndo((DYNAMIC_DOWNCAST(CResizeCommand, pCmd.get()))->GetTbl(), false);
    }
    else if (pCmd->IsKindOf(RUNTIME_CLASS(CStubResizeCommand))) {
        PushStubResizeForUndo((DYNAMIC_DOWNCAST(CResizeCommand, pCmd.get()))->GetTbl(), false);
    }
    else if (pCmd->IsKindOf(RUNTIME_CLASS(CStubHeadResizeCommand))) {
        ASSERT(FALSE); // not used solo
    }
    else if (pCmd->IsKindOf(RUNTIME_CLASS(CRestoreDefaultsCommand))) {
        PushRestoreDefaultsForUndo((DYNAMIC_DOWNCAST(CRestoreDefaultsCommand, pCmd.get()))->GetTbl(), false);
    }
    else if (pCmd->IsKindOf(RUNTIME_CLASS(CFormatPrintCommand))) {
        PushFormatPrintCommand((DYNAMIC_DOWNCAST(CFormatPrintCommand, pCmd.get()))->GetTbl(), false);
    }
    else {
        ASSERT(FALSE);
    }

    // do the undo operation
    pCmd->Execute();
}


/////////////////////////////////////////////////////////////////////////////
//
//                             OnEditRedo
//
// Performs redo operation, and preps for undo.
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnEditRedo()
{
    ASSERT(m_undoStack.CanRedo());
    std::shared_ptr<CPrtViewCommand> pCmd = m_undoStack.Redo();

    // push onto the undo stack
    if (pCmd->IsKindOf(RUNTIME_CLASS(CBoxheadResizeCommand))) {
        PushBoxheadResizeForUndo((DYNAMIC_DOWNCAST(CResizeCommand, pCmd.get()))->GetTbl());
    }
    else if (pCmd->IsKindOf(RUNTIME_CLASS(CStubResizeCommand))) {
        PushStubResizeForUndo((DYNAMIC_DOWNCAST(CResizeCommand, pCmd.get()))->GetTbl());
    }
    else if (pCmd->IsKindOf(RUNTIME_CLASS(CStubHeadResizeCommand))) {
        ASSERT(FALSE); // not used solo
    }
    else if (pCmd->IsKindOf(RUNTIME_CLASS(CRestoreDefaultsCommand))) {
        PushRestoreDefaultsForUndo((DYNAMIC_DOWNCAST(CRestoreDefaultsCommand, pCmd.get()))->GetTbl());
    }
    else if (pCmd->IsKindOf(RUNTIME_CLASS(CFormatPrintCommand))) {
        PushFormatPrintCommand((DYNAMIC_DOWNCAST(CFormatPrintCommand, pCmd.get()))->GetTbl());
    }
    else {
        ASSERT(FALSE);
    }

    // do the redo operation
    pCmd->Execute();
}


/////////////////////////////////////////////////////////////////////////////
//
//                             OnUpdateEditUndo
//                             OnUpdateEditRedo
//
/////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnUpdateEditUndo(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(m_undoStack.CanUndo());
}

void CTabPrtView::OnUpdateEditRedo(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(m_undoStack.CanRedo());
}

////////////////////////////////////////////////////////////////////////////////////
//
//                              CTabPrtView::OnViewZoomIn
//
//
////////////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnViewZoomIn()
{
    ZOOM_STATE eNewZoomState;
    // zooming in
    switch (GetZoomState()) {
        case ZOOM_STATE_300_PERCENT:
            eNewZoomState=ZOOM_STATE_300_PERCENT;
            break;
        case ZOOM_STATE_4H_4V:
        case ZOOM_STATE_4H_3V:
            eNewZoomState = ZOOM_STATE_3H_3V;
            break;
        case ZOOM_STATE_3H_3V:
        case ZOOM_STATE_3H_2V:
            eNewZoomState = ZOOM_STATE_2H_2V;
            break;
        case ZOOM_STATE_2H_2V:
        case ZOOM_STATE_2H_1V:
            eNewZoomState = ZOOM_STATE_100_PERCENT;
            break;
        default:
            eNewZoomState=(ZOOM_STATE)((int)GetZoomState() + 1);
    }

    if (eNewZoomState!=GetZoomState()) {
        SendMessage(UWM::Table::Zoom, (WPARAM)eNewZoomState, GetCurrFirstViewPg());
    }
}

////////////////////////////////////////////////////////////////////////////////////
//
//                              CTabPrtView::OnViewZoomOut
//
//
////////////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnViewZoomOut()
{
    ZOOM_STATE eNewZoomState;
    switch (GetZoomState()) {
        case ZOOM_STATE_4H_4V:
        case ZOOM_STATE_4H_3V:
            eNewZoomState=ZOOM_STATE_4H_4V;
            break;
        case ZOOM_STATE_3H_3V:
        case ZOOM_STATE_3H_2V:
            eNewZoomState = ZOOM_STATE_4H_4V;
            break;
        case ZOOM_STATE_2H_2V:
        case ZOOM_STATE_2H_1V:
            eNewZoomState = ZOOM_STATE_3H_3V;
            break;
        case ZOOM_STATE_100_PERCENT:
            eNewZoomState = ZOOM_STATE_2H_2V;
            break;
        default:
            eNewZoomState=(ZOOM_STATE)((int)GetZoomState() - 1);
    }

    if (eNewZoomState!=GetZoomState()) {
        SendMessage(UWM::Table::Zoom, (WPARAM)eNewZoomState, GetCurrFirstViewPg());
    }
}

////////////////////////////////////////////////////////////////////////////////////
//
//                              CTabPrtView::OnPrintViewClose
//
//
////////////////////////////////////////////////////////////////////////////////////
void CTabPrtView::OnPrintViewClose()
{
    AfxGetMainWnd()->SendMessage(WM_COMMAND, ID_VIEW_PAGEPRINTVIEW);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
//                             CPgOb
//
/////////////////////////////////////////////////////////////////////////////
CPgOb::CPgOb() : m_rcClientLP(0,0,0,0),m_rcClientDP(0,0,0,0),m_szMinResize(0,0), m_szMaxResize(0,0), m_szExtraLP(0,0)
{
    m_pTblBase=NULL;
    m_pPgObFmt=NULL;
    m_iTbl=NONE;
    m_uFormat=DT_LEFT|DT_TOP|DT_WORDBREAK|DT_NOPREFIX;
    m_ePgObType=PGOB_UNDETERMINED;
    m_sText.Empty();
    m_bCustom=false;
    m_bPageBreak=false;
    m_iHPage=-1;
    m_iVPage=-1;
    m_bHiddenFlag = false;
//    SetFont((CFont*)NULL);
}

CPgOb::CPgOb(const CPgOb& p)
{
    m_pPgObFmt=NULL;
    *this = p;
}


/*V*/ CPgOb::~CPgOb()
{
    SAFE_DELETE(m_pPgObFmt);
}

bool CPgOb::operator==(const CPgOb& p) const
{
    return (GetClientRectLP()==p.GetClientRectLP() &&
        GetClientRectDP()==p.GetClientRectDP() &&
        GetTblBase()==p.GetTblBase() &&
        GetFmt()==p.GetFmt() &&
        GetTbl()==p.GetTbl() &&
        GetHPage()==p.GetHPage() &&
        GetVPage()==p.GetVPage() &&
        GetMinResize()==p.GetMinResize() &&
        GetMaxResize()==p.GetMaxResize() &&
        GetExtraLP()==p.GetExtraLP() &&
        GetDrawFormatFlags()==p.GetDrawFormatFlags() &&
        GetText()==p.GetText() &&
        IsCustom()==p.IsCustom() &&
        IsPageBreak()==p.IsPageBreak() &&
        GetType()==p.GetType());
}

bool CPgOb::operator!=(const CPgOb& p) const
{
    return !operator==(p);
}

CPgOb& CPgOb::operator=(const CPgOb& p)
{
    SAFE_DELETE(m_pPgObFmt);
//    m_pPgObFmt=new CFmt;
//    if (p.GetFmt()) {
//        *m_pPgObFmt=*p.GetFmt();
//    }

    if (p.GetFmt()) {
        if (p.GetFmt()->IsKindOf(RUNTIME_CLASS(CDataCellFmt))) {
            m_pPgObFmt=new CDataCellFmt;
        }
        else if (p.GetFmt()->IsKindOf(RUNTIME_CLASS(CFmt))) {
            m_pPgObFmt=new CFmt;
        }
        else {
            ASSERT(FALSE);
        }
        m_pPgObFmt->Assign(*p.GetFmt());
    }
    else {
        // no previous fmt
        m_pPgObFmt=new CFmt;
    }

    SetClientRectLP(p.GetClientRectLP());
    SetClientRectDP(p.GetClientRectDP());
    SetExtraLP(p.GetExtraLP());
    SetCustom(p.IsCustom());
    SetPageBreak(p.IsPageBreak());
    SetTblBase(p.GetTblBase());
    SetTbl(p.GetTbl());
    SetHPage(p.GetHPage());
    SetVPage(p.GetVPage());
    SetMinResize(p.GetMinResize());
    SetMaxResize(p.GetMaxResize());
    SetDrawFormatFlags(p.GetDrawFormatFlags());
    SetType(p.GetType());
    SetText(p.GetText());
    SetHiddenFlag(p.GetHiddenFlag());
    return *this;
}

/////////////////////////////////////////////////////////////////////////////
//
//                             CRowColPgOb
//
/////////////////////////////////////////////////////////////////////////////
CRowColPgOb::CRowColPgOb() : CPgOb()
{
    m_aChildren.RemoveAll();
    m_pParent=NULL;
    m_iLevel=0;
	m_bHideRowIfAllZero = false;
}

CRowColPgOb::CRowColPgOb(const CRowColPgOb& rcp)
{
    *this = rcp;
}


/*V*/ CRowColPgOb::~CRowColPgOb()
{
    RemoveAllChildren();

}

bool CRowColPgOb::operator==(const CRowColPgOb& rcp) const
{
    if (CPgOb::operator!=(rcp) || GetNumChildren()!=rcp.GetNumChildren() || GetLevel()!=rcp.GetLevel() || m_iHPage!=rcp.m_iHPage || m_iVPage!=rcp.m_iVPage || m_pParent!=rcp.m_pParent)  {
        return false;
    }

    for (int i=0 ; i<GetNumChildren() ; i++)  {
        if (*GetChild(i)!=*rcp.GetChild(i))  {
            return false;
        }
    }
    return true;
}


bool CRowColPgOb::operator!=(const CRowColPgOb& rcp) const
{
    return !operator==(rcp);
}

CRowColPgOb& CRowColPgOb::operator=(const CRowColPgOb& rcp)
{
    CPgOb::operator=(rcp);

    SetLevel(rcp.GetLevel());
    m_aChildren.RemoveAll();
    m_pParent = rcp.m_pParent;
	m_bHideRowIfAllZero = rcp.m_bHideRowIfAllZero;

    for (int i=0 ; i<rcp.GetNumChildren() ; i++)  {
        CRowColPgOb* pChild = new CRowColPgOb(*rcp.GetChild(i));
        AddChild(pChild);
        //m_aChildren.Add(rcp.GetChild(i));
    }
    return *this;
}


/////////////////////////////////////////////////////////////////////////////
//
//        AddChild
//
// Adds a child RowColPgOb.  User needs to allocate memory before calling
// this rountine.  For example:
//      CRowColPgOb* pChild = new CRolColPgOb;
//      AddChild(pChild);
//
/////////////////////////////////////////////////////////////////////////////
void CRowColPgOb::AddChild(CRowColPgOb* pChild)
{
    m_aChildren.Add(pChild);
    pChild->SetParent(this);
}


/////////////////////////////////////////////////////////////////////////////
//
//        InsertChildAt
//
// Inserts a child RowColPgOb.  User needs to allocate memory before calling
// this rountine.  For example:
//      CRowColPgOb* pChild = new CRolColPgOb;
//      InsertChildAt(pChild,2);
//
/////////////////////////////////////////////////////////////////////////////
void CRowColPgOb::InsertChildAt(int iIndex, CRowColPgOb* pChild, int iCount /*=1*/)
{
    m_aChildren.InsertAt(iIndex, pChild, iCount);
}

/////////////////////////////////////////////////////////////////////////////
//
//        RemoveChildAt
//
// Removes a child RowColPgOb, and frees the memory allocated for it.
//
/////////////////////////////////////////////////////////////////////////////
void CRowColPgOb::RemoveChildAt(int i)
{
    ASSERT(i>=0 && i<GetNumChildren());
    CRowColPgOb* pRCOb = GetChild(i);
    delete pRCOb;
    m_aChildren.RemoveAt(i);
}

/////////////////////////////////////////////////////////////////////////////
//
//        RemoveChild
//
// Removes the child indicated by the passed RowColPgOb, and frees the memory allocated for it.
// Asserts if the child is not found.
//
/////////////////////////////////////////////////////////////////////////////
void CRowColPgOb::RemoveChild(CRowColPgOb* p)
{
    for (int iChild=0 ; iChild<GetNumChildren() ; iChild++) {
        if (GetChild(iChild)==p) {
            RemoveChildAt(iChild);
            return;
        }
    }
    ASSERT(FALSE); // child not found!
}


/////////////////////////////////////////////////////////////////////////////
//
//        RemoveAllChildren
//
// Removes all children, and frees the memory allocated for them.
//
/////////////////////////////////////////////////////////////////////////////
void CRowColPgOb::RemoveAllChildren()
{
    while (GetNumChildren()>0) {
        RemoveChildAt(0);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//        GetChildIndex
//
// Returns the index for a given child, or -1 if the child was not found
//
/////////////////////////////////////////////////////////////////////////////
int CRowColPgOb::GetChildIndex(CRowColPgOb* pChild) const
{
    for (int iChild=0 ; iChild<GetNumChildren() ; iChild++) {
        if (GetChild(iChild)==pChild) {
            return iChild;
        }
    }
    return NONE;  //not found
}




/////////////////////////////////////////////////////////////////////////////
//
//        CalcMinSizes
//
// Determines minimum height and width for a tree (of boxheads and stubs)
//
/////////////////////////////////////////////////////////////////////////////
void CRowColPgOb::CalcMinSizes(CDC& dc, bool bForceRemeasure /*=false*/)
{
    ASSERT(GetType()!=PGOB_UNDETERMINED);

    // don't calculate for the (unused) root
    if (GetType()!=PGOB_ROOT) {

        // get the format ...
        CFmt* pFmt = GetFmt(); //GetTblOb()->GetFmt();
        ASSERT_VALID(pFmt);

        // use persisted dimensions, if possible
        bool bRecalcLayout=(GetTblBase()->GetPrtViewInfoSize()==0);

        if (!bRecalcLayout) {
            if (!IsCustom()) {
                // remeasure, since data might have changed
                bRecalcLayout=true;
            }
        }

        // use previously calculated dimensions, if possible
        if (bRecalcLayout) {
            if (GetMinResize()!=CSize(0,0)) {
                bRecalcLayout=false;
            }
        }

        // see if we are forcing remeasurement (following printer change)
        if (bForceRemeasure) {
            bRecalcLayout=true;
        }

        // otherwise, redo the calcs
        if (bRecalcLayout) {
            CString sText=GetText();
            CRect rcMin, rcMax;

            if (sText.IsEmpty()) {
                // placeholder
                sText=_T("X");
            }

            // load font
            ASSERT_VALID(pFmt->GetFont());
            CFont* pFont=pFmt->GetFont();

            dc.SelectObject(pFont);

            // measure minimum width ...
            rcMin.SetRect(0,0,1,MAXINT);  // thin and tall
            dc.DrawText(sText, &rcMin, GetDrawFormatFlags() | DT_CALCRECT);
            ASSERT(rcMin.Width()>0 && rcMin.Height()>0 && rcMin.Width()<MAXINT && rcMin.Height()<MAXINT);

            rcMin.bottom += CELL_PADDING_TOP + CELL_PADDING_BOTTOM;   // add vertical indentation (padding)
            rcMin.right += pFmt->GetIndent(LEFT) + pFmt->GetIndent(RIGHT) + CELL_PADDING_LEFT + CELL_PADDING_RIGHT;    // add horizontal indentation (padding)
            SetMinResize(rcMin.Size());

            // measure maximum width ...
            rcMax.SetRect(0,0,MAXINT,1);  // short and fat
            dc.DrawText(sText, &rcMax, GetDrawFormatFlags() | DT_CALCRECT);
            ASSERT(rcMax.Width()>0 && rcMax.Height()>0 && rcMax.Width()<MAXINT && rcMax.Height()<MAXINT);

            rcMax.bottom += CELL_PADDING_TOP + CELL_PADDING_BOTTOM;   // add vertical indentation (padding)
            rcMax.right += pFmt->GetIndent(LEFT) + pFmt->GetIndent(RIGHT) + CELL_PADDING_LEFT + CELL_PADDING_RIGHT;    // add horizontal indentation (padding)
            SetMaxResize(rcMax.Size());

            dc.SelectStockObject(OEM_FIXED_FONT);
        }
    }
    else {
        SetMinResize(CSize(0,0));
        SetMaxResize(CSize(0,0));
    }

    // recurse through this object's children, and calculate min sizes for each...
    for (int iChild=0 ; iChild<GetNumChildren() ; iChild++) {
        GetChild(iChild)->CalcMinSizes(dc, bForceRemeasure);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//        GetDepth
//
// Returns the depth of a row or column tree.  The depth is the number
// of nested levels in the tree.
//
/////////////////////////////////////////////////////////////////////////////
int CRowColPgOb::GetDepth() const
{
    ASSERT(GetType()!=PGOB_UNDETERMINED);

    int iBranchDepth=GetLevel();
    for (int iChild=0 ; iChild<GetNumChildren() ; iChild++) {
        iBranchDepth= std::max(iBranchDepth, GetChild(iChild)->GetDepth());
    }
    return iBranchDepth;
}




/////////////////////////////////////////////////////////////////////////////
//
//        GetNumLeaves
//
// Returns the number of leaves in a CRowColPgOb tree.  (Leaves are entries
// in the tree that do not have any children.)
//
/////////////////////////////////////////////////////////////////////////////
int CRowColPgOb::GetNumLeaves() const
{
    ASSERT(GetType()!=PGOB_UNDETERMINED);

    if (GetNumChildren()==0) {
        return 1;
    }
    else {
        int iNumLeaves=0;
        for (int iChild=0 ; iChild<GetNumChildren() ; iChild++) {
            iNumLeaves += GetChild(iChild)->GetNumLeaves();
        }
        return iNumLeaves;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        GetLeaf
//
// Returns the a specific leaf from within a CRowColPgOb tree.  The index
// indicates the leaf found if iterating depth-first, left-to-right (as if
// counting column heads across a table).
//
/////////////////////////////////////////////////////////////////////////////
CRowColPgOb* CRowColPgOb::GetLeaf(int iIndex)
{
    CArray<CRowColPgOb*, CRowColPgOb*> aLeaves;
    GetLeaves(aLeaves);
    ASSERT(iIndex>=0 && iIndex<aLeaves.GetSize());
    return aLeaves[iIndex];
}


/////////////////////////////////////////////////////////////////////////////
//
//        GetLeaves
//
// Returns an array of all the leaves in a CRowColPgOb.
//
/////////////////////////////////////////////////////////////////////////////
void CRowColPgOb::GetLeaves(CArray<CRowColPgOb*, CRowColPgOb*>& aLeaves,bool bIncludeHidden /*=false*/)
{
    if (GetNumChildren()==0) {
        if(!bIncludeHidden){
            if (GetType()!=PGOB_ROOT && GetType()!=PGOB_NOTINCLUDED && GetFmt()->GetHidden()!=HIDDEN_YES) {
                aLeaves.Add(this);
            }
        }
        else {
            if (GetType()!=PGOB_ROOT && GetType()!=PGOB_NOTINCLUDED) {
                aLeaves.Add(this);
            }
        }
    }
    else {
        for (int iChild=0 ; iChild<GetNumChildren() ; iChild++) {
            GetChild(iChild)->GetLeaves(aLeaves,bIncludeHidden);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//        GetLeavesAndNodes
//
// Returns an array of all the leaves and nodes in a CRowColPgOb.
// Uses depth first traversal.
//
/////////////////////////////////////////////////////////////////////////////
void CRowColPgOb::GetLeavesAndNodes(CArray<CRowColPgOb*, CRowColPgOb*>& aLeaves ,bool bIncludeHidden /*=false*/)
{
    if(!bIncludeHidden){
        if (GetType()!=PGOB_ROOT && GetType()!=PGOB_NOTINCLUDED &&
			GetFmt()->GetHidden()!=HIDDEN_YES &&
			!GetHideRowForAllZeroCells()) {
            aLeaves.Add(this);
        }
    }
    else {
        if (GetType()!=PGOB_ROOT && GetType()!=PGOB_NOTINCLUDED) {
            aLeaves.Add(this);
        }
    }
    for (int iChild=0 ; iChild<GetNumChildren() ; iChild++) {
        GetChild(iChild)->GetLeavesAndNodes(aLeaves,bIncludeHidden);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        GetMaxWidthLP
//        GetMaxHeightLP
//
// Returns the width/height of the widest/highest object (logical units) in a row or column tree
//
// If iLevel!=NONE, then only objects with m_iLevel==iLevel are considered.
//
/////////////////////////////////////////////////////////////////////////////
int CRowColPgOb::GetMaxWidthLP(int iLevel /*=NONE*/) const
{
    if (GetNumChildren()==0) {
        if (GetFmt()->GetSpanCells()==SPAN_CELLS_YES) {
            // field spanner ... width isn't applicable for these calculations
            return 0;
        }
        else if ((iLevel==NONE || iLevel==GetLevel()) && GetFmt()->GetHidden()==HIDDEN_NO) {  // csc 1/29/05
            return GetClientRectLP().Width();
        }
        else {
            return 0; // hidden ... no width
        }
    }

    int iMaxWidthLP=0;
    for (int iChild=0 ; iChild<GetNumChildren() ; iChild++) {
        iMaxWidthLP= std::max(iMaxWidthLP, GetChild(iChild)->GetMaxWidthLP());
    }
    return iMaxWidthLP;
}

int CRowColPgOb::GetMaxHeightLP(int iLevel) const
{
        if (iLevel==GetLevel()) {
            bool bHidden=false;
            if (GetFmt()) {
                if (GetFmt()->GetHidden()==HIDDEN_YES) {
                    bHidden=true;
                }
            }

            if (!bHidden) {
                return GetClientRectLP().Height();
            }
            else {
                return 0;  // hidden ... no height
            }
        }
        else {
            int iMaxHeightLP=0;
            for (int iChild=0 ; iChild<GetNumChildren() ; iChild++) {
                iMaxHeightLP= std::max(iMaxHeightLP, GetChild(iChild)->GetMaxHeightLP(iLevel));
            }
            return iMaxHeightLP;
        }
}


/////////////////////////////////////////////////////////////////////////////
//
//        GetSumWidthLP
//        GetSumHeightLP
//
// Returns the sum of the widths/heights (logical units) of the leaves in a row or column tree
//
/////////////////////////////////////////////////////////////////////////////
int CRowColPgOb::GetSumWidthLP() const
{
    if (GetNumChildren()==0) {
        return GetClientRectLP().Width();
    }
    else {
        int iChildWidth=0;
        for (int iChild=0 ; iChild<GetNumChildren() ; iChild++) {
            iChildWidth += GetChild(iChild)->GetSumWidthLP();
        }
        return iChildWidth;
    }
}

int CRowColPgOb::GetSumHeightLP() const
{
    if (GetNumChildren()==0) {
        return GetClientRectLP().Height();
    }
    else {
        int iChildHeight=0;
        for (int iChild=0 ; iChild<GetNumChildren() ; iChild++) {
            // only include highest of the children ...
            CRowColPgOb* pChild = GetChild(iChild);
            iChildHeight = pChild->GetSumHeightLP();
        }
        return iChildHeight;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        CalcColHeadWidths
//
// Determines initial widths for data columns.
//
// Heuristic:
//    start with: 75% of user area (left standard, left facing, both facing) or 50% of
//          user area (both facing), equally divided by number of data columns;
//          call this width lInitColWidth
//    if any column does not fit in this width, then try 1" width (if lInitColWidth<1")
//    if any column still does not fit, then set lInitColWidth=2"
//    get min height (Hinit) for each column head's lInitColWidth
//
/////////////////////////////////////////////////////////////////////////////
void CRowColPgOb::CalcColHeadWidths(long lInitColWidth, CDC& dc, bool bForceRemeasure /*=false*/)
{
    ASSERT(GetType()!=PGOB_UNDETERMINED);
    CFmt* pFmt=GetFmt();

    // we only work on data columns
    if (GetNumChildren()==0 && GetType()==PGOB_COLHEAD && GetFmt()->GetHidden()!=HIDDEN_YES) {

        // use persisted info, if possible
        bool bRecalcLayout=(GetTblBase()->GetPrtViewInfoSize()==0);
        if (!bRecalcLayout) {
            ASSERT(!GetClientRectLP().IsRectEmpty());

            // recalc if we're not persisted as customized
            if (!IsCustom()) {
                bRecalcLayout=true;
            }
        }

        // use previous calcs, if possible
        if (bRecalcLayout) {
            if (!GetClientRectLP().IsRectNull()) {
                bRecalcLayout=false;
            }
        }

        // recalc if our width < min resize (happens when cell values start small, then become large after a run)
        if (!bRecalcLayout) {
            if (GetMinResize().cx>GetClientRectLP().Width()) {
                bRecalcLayout=true;
            }
        }
        if (bForceRemeasure) {
            bRecalcLayout=true;
        }

        if (bRecalcLayout) {
            CRect rcDraw(0,0,lInitColWidth,1);
            CString sText=GetText();

            // load font
            ASSERT_VALID(pFmt->GetFont());
            CFont* pFont=pFmt->GetFont();
            dc.SelectObject(pFont);

            dc.DrawText(sText, &rcDraw, GetDrawFormatFlags()|DT_CALCRECT);

            // if any column does not fit in this width, then try 1" width (if lInitColWidth<1")
            if (rcDraw.Width()>lInitColWidth && lInitColWidth<TWIPS_PER_INCH) {
                rcDraw.SetRect(0,0,TWIPS_PER_INCH,1);
                dc.DrawText(sText, &rcDraw, GetDrawFormatFlags()|DT_CALCRECT);
            }

            // if any column still does not fit, then set lInitColWidth=2"
            if (rcDraw.Width()>TWIPS_PER_INCH) {
                rcDraw.SetRect(0,0,2*TWIPS_PER_INCH,1);
                dc.DrawText(sText, &rcDraw, GetDrawFormatFlags()|DT_CALCRECT);
            }

            // make sure that draw width is at least = the column head's min width (necessary since
            // min width takes cell contents into consideration)
            if (rcDraw.Width()<GetMinResize().cx) {
                rcDraw.right = rcDraw.left + GetMinResize().cx;
                GetExtraLP().cx=0;
            }

            // unload font
            dc.SelectStockObject(OEM_FIXED_FONT);
            SetClientRectLP(rcDraw);
        }
    }

    for (int iChild=0 ; iChild<GetNumChildren() ; iChild++) {
        GetChild(iChild)->CalcColHeadWidths(lInitColWidth, dc);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//        CalcColeadSectionLayoutWidth
//
// Determines the width necessary to layout a column head, taking into consideration
// its section predessors and spanners.
//
// For example, if a section of column heads has 3 columns "total","male","female", then calling
// CalcBoxheadWidth with pColHead="male" would return the width necessary to lay
// out "total" and "male" together, along with
// all of their spanners.  Calling the function with column head="total" would give the width
// needed to lay out total and its spanners (assuming nothing else in the section).  Calling
// with "female" would give the width required to layout the whole section (t,m,f), along with spanners.
//
// "Extra space" (which has been added by the layout algorithm for spacing) is not
// included as part of these calcs.
//
/////////////////////////////////////////////////////////////////////////////
int CRowColPgOb::CalcColeadSectionLayoutWidth()
{
    CRowColPgOb* pParent = GetParent();

    ASSERT(GetNumChildren()==0);   // this function is for column heads only!
    ASSERT(pParent!=NULL);

    // first, calculate the min space to layout the column heads in this section, up to and including pColHead
    int iLayoutWidth=0;
    bool bFound=false;
    for (int iChild=0 ; iChild<GetParent()->GetNumChildren() ; iChild++) {
        CRowColPgOb* pChild = pParent->GetChild(iChild);
        if (pChild->GetFmt()->GetHidden()==HIDDEN_YES) {
            // don't consider this child in the analysis
            continue;
        }

        iLayoutWidth += pChild->GetClientRectLP().Width() - pChild->GetExtraLP().cx;
        if (pChild==this) {
            bFound=true;
            break;
        }
    }
    ASSERT(bFound);    // sanity check

    // now, iterate up through parent spanners, checking to see if any require more width ...
    while (pParent->GetType()!=PGOB_ROOT) {
        if (pParent->GetFmt()->GetHidden()==HIDDEN_YES) {
            // don't consider this parent in the analysis
        }
        else {
            int iParentWidth=pParent->GetClientRectLP().Width() - pParent->GetExtraLP().cx;
            if (iParentWidth>iLayoutWidth) {
                iLayoutWidth = iParentWidth;
            }
            if (pParent->GetMinResize().cx>iLayoutWidth) {
                iLayoutWidth = pParent->GetMinResize().cx;
            }
        }
        pParent = pParent->GetParent();
    }

    // all done
    return iLayoutWidth;
}



/////////////////////////////////////////////////////////////////////////////
//
//        CalcSpannerWidths
//
// Sets spanner widths equal to the sum of their children's widths.  If
// a spanner requires more space, then enlarge its children as needed.
//
// Returns total column area width
//
/////////////////////////////////////////////////////////////////////////////
int CRowColPgOb::CalcSpannerWidths(int iExtraSpace /*=0*/)
{
    ASSERT(GetType()!=PGOB_UNDETERMINED);

    // we only work on spanners
    if (GetNumChildren()>0) {

        // a spanner's width is equal to the sum of its children's widths ...
        int iSpannedWidth=0;
        int iChild;
        for (iChild=0 ; iChild<GetNumChildren() ; iChild++) {
            iSpannedWidth += GetChild(iChild)->CalcSpannerWidths(iExtraSpace);
        }

        // if spanner needs more space, then subdivide its children ...
        if (GetMinResize().cx>iSpannedWidth && GetFmt()->GetHidden()==HIDDEN_NO) {
            int iNeededExtraSpace = GetMinResize().cx - iSpannedWidth;
            int iSpacePerChild = iNeededExtraSpace/GetNumChildren();
            for (iChild=0 ; iChild<GetNumChildren() ; iChild++) {
                CRowColPgOb* pChild = GetChild(iChild);
                pChild->CalcSpannerWidths(iSpacePerChild);
            }
            iSpannedWidth = GetMinResize().cx;
        }

        // adjust our width ...
        GetExtraLP().cx = iSpannedWidth - GetMinResize().cx;
        GetClientRectLP().right = iSpannedWidth;
        return iSpannedWidth;
    }
    else {
        // we're a column head ... just return our width
        int iWidth=0;
        if (GetFmt()->GetHidden()==HIDDEN_NO) {
            // return 0 for hidden colheads
            GetClientRectLP().right += iExtraSpace;
            iWidth=GetClientRectLP().Width();
        }
        return iWidth;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        PaginateHiddenColHeads
//
// Assigns horz page numbers to hidden column heads.  These receive the
// same horz page number as their siblings.
//
// We pass aColHead through for quick searching for sibling colheads.
//
/////////////////////////////////////////////////////////////////////////////
void CRowColPgOb::PaginateHiddenColHeads(CArray<CRowColPgOb*, CRowColPgOb*>& aColHead)
{
    ASSERT(aColHead.GetSize()>0);

    if (GetNumChildren()>0) {
        // loop through children
        for (int iChild=0 ; iChild<GetNumChildren() ; iChild++) {
            CRowColPgOb* pChild=GetChild(iChild);
            pChild->PaginateHiddenColHeads(aColHead);
        }
    }
    else {
        // no children ... see if we are hidden
        if (GetFmt()->GetHidden()==HIDDEN_YES) {
            // find a sibling with the same parent; we'll be on the same horz page as this sibling
            bool bFound=false;
            const CRowColPgOb* pParent=GetParent();
            ASSERT(pParent!=NULL);
            for (int iColHead=0 ; iColHead<aColHead.GetSize() ; iColHead++) {
                CRowColPgOb* pColHead=aColHead[iColHead];
                if (pParent==pColHead->GetParent() && pColHead->GetFmt()->GetHidden()==HIDDEN_NO) {
                    bFound=true;
                    m_iHPage=pColHead->GetHPage();
                    break;
                }
            }
            ASSERT(bFound);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        PaginateSpanners
//
// Assigns horz page numbers to spanners, based on their children's
// page numbers.
//
/////////////////////////////////////////////////////////////////////////////
int CRowColPgOb::PaginateSpanners()
{
    for (int iChild=0 ; iChild<GetNumChildren() ; iChild++) {
        CRowColPgOb* pChild=GetChild(iChild);
        m_iHPage = pChild->PaginateSpanners();
    }
    return m_iHPage;
}


/////////////////////////////////////////////////////////////////////////////
//
//        AlignColumnsHorz
//
// Aligns boxheads and spanners horizontally, so that they flow across a page.
// Can be limited to only work on specific horz and vert pages. (Passing
// NONE as iHPage or iVPage includes all.)
//
/////////////////////////////////////////////////////////////////////////////
void CRowColPgOb::AlignColumnsHorz(CPoint ptTopLeft, int iHPage /*=NONE*/, int iVPage /*=NONE*/)
{
    ASSERT(GetType()!=PGOB_UNDETERMINED);
    int iChild;

    if (GetType()==PGOB_ROOT) {
        // root owns children w/ different pages, so we iterate them to start with
        for (iChild=0 ; iChild<GetNumChildren() ; iChild++) {
            CRowColPgOb* pChild = GetChild(iChild);
            pChild->AlignColumnsHorz(ptTopLeft, iHPage, iVPage);
            if ((pChild->m_iHPage==iHPage || iHPage==NONE) && (pChild->m_iVPage==iVPage || iVPage==NONE))  {
                ptTopLeft.x += pChild->GetClientRectLP().Width();
            }
        }
    }
    else {
        if (((m_iHPage==iHPage || iHPage==NONE) && (m_iVPage==iVPage || iVPage==NONE)) || GetType()==PGOB_ROOT) {
            // offset ourselves ...
            GetClientRectLP().OffsetRect(ptTopLeft);

            // offset our children ...
            ptTopLeft.y += GetClientRectLP().Height();
            for (iChild=0 ; iChild<GetNumChildren() ; iChild++) {
                CRowColPgOb* pChild = GetChild(iChild);
                pChild->AlignColumnsHorz(ptTopLeft, iHPage, iVPage);
                if ((m_iHPage==iHPage || iHPage==NONE) && (m_iVPage==iVPage || iVPage==NONE)) {
                    ptTopLeft.x += pChild->GetClientRectLP().Width();
                }
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        SetColHeight
//
// Sets columns for a given level to the same height.
//
/////////////////////////////////////////////////////////////////////////////
void CRowColPgOb::SetColHeight(int iLevel, int iHeight)
{
    if (m_iLevel==iLevel) {
        GetClientRectLP().bottom = GetClientRectLP().top + iHeight;
    }
    else if (m_iLevel<iLevel) {
        for (int iChild=0 ; iChild<GetNumChildren() ; iChild++) {
            GetChild(iChild)->SetColHeight(iLevel, iHeight);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        CalcBoxheadHeights
//
// Determines initial heights for columns (data and spanners)
//
// Heuristic:
//    given the column's width, measure the height required to render the column cell's text
//
/////////////////////////////////////////////////////////////////////////////
void CRowColPgOb::CalcBoxheadHeights(CDC& dc)
{
    ASSERT(GetType()!=PGOB_UNDETERMINED);
    CFont font;
    CRect rcDraw(0,0,0,0);

    if (GetType()!=PGOB_ROOT) {
        CFmt* pFmt = GetFmt();
        if (!GetText().IsEmpty()) {
            rcDraw.SetRect(0,0,GetClientRectLP().Width(),1);
            CString sText=GetText();
            if (sText.IsEmpty()) {
                rcDraw.bottom=0;
            }
            else {
                if (GetNumChildren()>0) {
                    rcDraw.right = GetSumWidthLP();
                }

                // account for padding on left and right
                rcDraw.left += pFmt->GetIndent(LEFT)+CELL_PADDING_LEFT;
                rcDraw.right -= (pFmt->GetIndent(RIGHT)+CELL_PADDING_RIGHT);

                ASSERT_VALID(pFmt->GetFont());
                CFont* pFont=pFmt->GetFont();
                dc.SelectObject(pFont);

                // use DrawText to get the necessary height only; ignore width, since it's already been calculated
                int iHgt = dc.DrawText(sText, &rcDraw, GetDrawFormatFlags()|DT_CALCRECT);
                rcDraw.SetRect(0,0,GetClientRectLP().Width(),iHgt);

                // pad height
                rcDraw.InflateRect(0,0,0,CELL_PADDING_TOP + CELL_PADDING_BOTTOM);

                // unload font
                dc.SelectStockObject(OEM_FIXED_FONT);
            }
        }
    }

    SetClientRectLP(rcDraw);
    for (int iChild=0 ; iChild<GetNumChildren() ; iChild++) {
        GetChild(iChild)->CalcBoxheadHeights(dc);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        CalcStubWidth
//
// Determines initial widths for data columns.
//
// Heuristic:
//    determine min sizes for each stub or sidehead, and the stub head
//    start with 25% of user area width; call this iInitRowWidth
//    if any stub does not fix in this width, then try iInitRowWidth=35% of user area width
//    if any stub does not fix in this width, then set iInitRowWidth=50% of user area width
//    pad the left of each data stub by 0.25" for each level of indentation
//
// Field spanners always have a set width, iFieldSpannerWidth, that spans
// the whole table.
//
/////////////////////////////////////////////////////////////////////////////
void CRowColPgOb::CalcStubWidth(int& iInitRowWidth, int iFieldSpannerWidth, CDC& dc, bool bForceRemeasure /*=false*/)
{
    ASSERT(GetType()!=PGOB_UNDETERMINED);
    CFmt* pFmt=GetFmt();

    // we work on both data rows and non-data rows (captions, reader breaks)
    if (GetType()==PGOB_STUB || GetType()==PGOB_CAPTION || GetType()==PGOB_READER_BREAK) {

        bool bFieldSpanner=(pFmt->GetSpanCells()==SPAN_CELLS_YES);

        // sanity check
        if (bFieldSpanner) {
            ASSERT(GetType()==PGOB_CAPTION);
        }

        // use persisted info, if possible
        bool bRecalcLayout=(GetTblBase()->GetPrtViewInfoSize()==0);

        if (bForceRemeasure) {
            bRecalcLayout=true;
        }

        // use previous calcs, if possible
        if (!bRecalcLayout) {
            CPrtViewInfo& pvi=GetTblBase()->GetPrtViewInfo(0);
            if (pvi.GetCurrSize().cx!=iInitRowWidth && !pvi.IsCustom() && !bFieldSpanner) {
                // user has resized stub width, need to remeasure
                bRecalcLayout=true;
            }
        }
        else {
            if (GetClientRectLP().Height()>0) {
                bRecalcLayout=false;
            }
        }

        if (bRecalcLayout) {

            CRect rcDraw;
            CString sText=GetText();

            // load font
            ASSERT_VALID(pFmt->GetFont());
            CFont* pFont=pFmt->GetFont();
            dc.SelectObject(pFont);

            if (bFieldSpanner) {
                // field spanners have a fixed width = the table's width; since we don't know the
                // table's width yet, we just set the client rect empty; field spanner measurements
                // get set after we finish measuring all the stubs
                rcDraw.SetRect(0,0,iFieldSpannerWidth,1);

                dc.DrawText(sText, &rcDraw, GetDrawFormatFlags()|DT_CALCRECT);
                ASSERT(rcDraw.Width()<=iFieldSpannerWidth);
                rcDraw.right=rcDraw.left+iFieldSpannerWidth;
                rcDraw.bottom += CELL_PADDING_TOP + CELL_PADDING_BOTTOM;   // add vertical indentation (padding)
            }
            else {
                rcDraw.SetRect(0,0,iInitRowWidth,1);

                dc.DrawText(sText, &rcDraw, GetDrawFormatFlags()|DT_CALCRECT);

                // if any stub does not fix in this width, then try iInitRowWidth=35% of user area width
                if (rcDraw.Width()>iInitRowWidth) {
                    iInitRowWidth = (long) ((float)iInitRowWidth * 4.0f * 0.35f);  // was 25%; convert to 100% then to 35%
                    rcDraw.SetRect(0,0,iInitRowWidth,1);
                    dc.DrawText(sText, &rcDraw, GetDrawFormatFlags()|DT_CALCRECT);
                }

                // if any stub does not fix in this width, then set iInitRowWidth=50% of user area width
                if (rcDraw.Width()>iInitRowWidth) {
                    iInitRowWidth = (long)((float)iInitRowWidth * 0.5f / 0.35f);  // was 35%; convert to 100% then to 50%
                    rcDraw.SetRect(0,0,iInitRowWidth,1);
                    dc.DrawText(sText, &rcDraw, GetDrawFormatFlags()|DT_CALCRECT);
                }

                // pad the left of each data stub by 0.25" for each level of indentation
                rcDraw.right += (GetLevel()-1) * STUB_INDENT;

				// JH 5/16/06 - not getting padding here after change to stub font
				rcDraw.bottom += CELL_PADDING_TOP + CELL_PADDING_BOTTOM;

            }

            // unload font
            dc.SelectStockObject(OEM_FIXED_FONT);

            SetClientRectLP(rcDraw);
        }
    }

    for (int iChild=0 ; iChild<GetNumChildren() ; iChild++) {
        GetChild(iChild)->CalcStubWidth(iInitRowWidth, iFieldSpannerWidth, dc, bForceRemeasure);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//        SetStubSizes
//
// Sets stubs to a specified size.  Adds padding as needed.
//
// szLP     -- size to assign to the object
//                szLP.cx==-1 will cause width to remain unchanged
//                szLP.cy==-1 will cause height to remain unchanged
// bRecurse -- true if child CRowColPgObs should be set to this size as well
//
/////////////////////////////////////////////////////////////////////////////
void CRowColPgOb::SetStubSizes(const CSize& szLP, bool bRecurse /*=false*/, bool bForceRemeasure /*=false*/)
{
    if (GetType()!=PGOB_ROOT)  {
        ASSERT(GetType()==PGOB_STUB || GetType()==PGOB_CAPTION || GetType()==PGOB_NOTINCLUDED
            || GetType()==PGOB_READER_BREAK);

        CFmt* pFmt = GetFmt();
        bool bRecalcLayout=(GetTblBase()->GetPrtViewInfoSize()==0);
        bool bFieldSpanner=(pFmt->GetSpanCells()==SPAN_CELLS_YES);

        // sanity check...
        if (bFieldSpanner) {
            ASSERT(GetType()==PGOB_CAPTION);
        }

        if (bForceRemeasure) {
            bRecalcLayout=true;
        }

        // use previous calcs, if possible
        if (!bRecalcLayout && (GetType()==PGOB_STUB || GetType()==PGOB_CAPTION || GetType()==PGOB_READER_BREAK)) {
            CPrtViewInfo& pvi=GetTblBase()->GetPrtViewInfo(0);
            if (pvi.GetCurrSize().cx!=szLP.cx && !pvi.IsCustom()) {
                // user has resized stub width, need to remeasure
                bRecalcLayout=true;
            }
        }

        CRect rcClientLP = GetClientRectLP();
        if (szLP.cx!=NONE) {
            if (!bFieldSpanner) {
                rcClientLP.right = rcClientLP.left + szLP.cx;
            }
        }
        else {
            // pad (if recalcing)
            if (bRecalcLayout) {
                if (!bFieldSpanner) {
                    rcClientLP.right += pFmt->GetIndent(LEFT) + pFmt->GetIndent(RIGHT) + CELL_PADDING_LEFT + CELL_PADDING_RIGHT;
                }
            }
        }

        if (szLP.cy!=NONE && !bForceRemeasure) {
            rcClientLP.bottom = rcClientLP.top + szLP.cy;
        }
        else {
            // pad (if recalcing)
            if (rcClientLP.Width()<GetMinResize().cx) {
                rcClientLP.bottom=GetMinResize().cy;
            }
            else if (rcClientLP.Height()<GetMaxResize().cy) {
                rcClientLP.bottom=GetMaxResize().cy;
            }
        }
        SetClientRectLP(rcClientLP);
    }

    if (bRecurse) {
        for (int iChild=0 ; iChild<GetNumChildren() ; iChild++) {
            GetChild(iChild)->SetStubSizes(szLP, bRecurse, bForceRemeasure);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//        DebugCol
//
// Sanity checks for column trees (spanners and boxheads)
//
// Checks that:
// - a non-root node's horz page must be same as the horz page of all its children
// - an object's parent has the object as one of its children
//
/////////////////////////////////////////////////////////////////////////////
void CRowColPgOb::DebugCol()
{
    ASSERT(GetType()==PGOB_COLHEAD || GetType()==PGOB_SPANNER || GetType()==PGOB_ROOT);

    for (int iChild=0 ; iChild<GetNumChildren() ; iChild++) {
        CRowColPgOb* pChild = GetChild(iChild);
        if (GetType()!=PGOB_ROOT) {
            ASSERT(m_iHPage==pChild->m_iHPage || pChild->GetFmt()->GetHidden()==HIDDEN_YES || GetFmt()->GetHidden()==HIDDEN_YES);
        }
        pChild->DebugCol();
    }

    if (GetType()==PGOB_ROOT) {
        ASSERT(GetParent()==NULL);
    }
    else {
        ASSERT(GetParent()!=NULL);
        ASSERT(GetParent()->GetChildIndex(this)!=NONE);
    }
}

void CRowColPgOb::SetHideRowForAllZeroCells(bool b)
{
	m_bHideRowIfAllZero = b;
}

bool CRowColPgOb::GetHideRowForAllZeroCells() const
{
	return m_bHideRowIfAllZero;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
//                             CPgLayout
//
/////////////////////////////////////////////////////////////////////////////
CPgLayout::CPgLayout() : m_rcUserLP(0,0,0,0), m_rcUserDP(0,0,0,0)
{
    m_iPrintedPageNum=0;
    m_aPgOb.RemoveAll();
}


CPgLayout::CPgLayout(const CPgLayout& p)
{
    *this = p;
}


/////////////////////////////////////////////////////////////////////////////
//
//        AddToPage
//
// Adds a rowcolpgob and all its children to a CPageLayout.
//
// If iHPage (horizontal page) or iVPage (vertical page) are specified (!=NONE),
// then only nodes on those pages are added.
//
// bAddBoxheads is used to turn off adding boxheads, based on header frequency.
//
/////////////////////////////////////////////////////////////////////////////
void CPgLayout::AddRowColPgOb(CRowColPgOb& rcpgob, bool bAddBoxheads, int iHPage /*=NONE*/, int iVPage /*=NONE*/)
{
    if (rcpgob.GetType()!=PGOB_ROOT) {
        bool bPageMatch;
        bool bAddPgOb;
        bPageMatch=((iHPage==rcpgob.GetHPage() || iHPage==NONE) && (iVPage==rcpgob.GetVPage() || iVPage==NONE));

        if (rcpgob.GetType()==PGOB_SPANNER||rcpgob.GetType()==PGOB_COLHEAD||rcpgob.GetType()==PGOB_STUBHEAD) {
            // object is a spanner, colhead, or stubhead ... add if we're on the right page and we're adding boxheads to this page ...
            bAddPgOb=(bPageMatch && bAddBoxheads);
        }
        else {
            // not a spanner/colhead/stubhead object ... add if we're on the right page ...
            bAddPgOb=bPageMatch;
        }

        // don't add hidden objects...
        bAddPgOb &= (rcpgob.GetFmt()->GetHidden()==HIDDEN_YES?FALSE:TRUE);    // csc 1/29/05

        if (bAddPgOb) {
            AddPgOb(rcpgob);
        }
    }
    for (int iChild=0 ; iChild<rcpgob.GetNumChildren() ; iChild++) {
        AddRowColPgOb(*rcpgob.GetChild(iChild), bAddBoxheads, iHPage, iVPage);
    }
}


CPgLayout& CPgLayout::operator=(const CPgLayout& p)
{
    m_aPgOb.RemoveAll();
    for (int i=0 ; i<p.GetNumPgObs() ; i++) {
        CPgOb pgob=p.GetPgOb(i);
        m_aPgOb.Add(pgob);
    }
    SetUserAreaLP(p.GetUserAreaLP());
    SetUserAreaDP(p.GetUserAreaDP());
    SetPgHgtInches(p.GetPgHgtInches());
    SetPgWidthInches(p.GetPgWidthInches());
    SetPrintedPageNum(p.GetPrintedPageNum());
    return *this;
}


////////////////////////////////////////////////////////////////////////////////////
//
//        CoordinateLines
//        GetLinePrecedence
//
// For all the intersections on a page, lines need to be coordinated.  Use
// the following procedure, where A and B are the objects' intersecting lines:
//    if A=B ==> do nothing
//    otherwise (A<>B):
//      if A=LINE_NONE       then    A=B (anything trumps no line)
//      if B=LINE_NONE       then    B=A (anything trumps no line)
//      if A=LINE_NOT_APPL   then    A=B (anything trumps not applicable)
//      if B=LINE_NOT_APPL   then    B=A (anything trumps not applicable)
//      if A=LINE_THICK      then    B=A (thick trumps thin)
//      if B=LINE_THICK      then    A=B (thick trumps thin)
//      otherwise                    A=B=LINE_THIN
//
// Table borders are passed as parameters.
//
////////////////////////////////////////////////////////////////////////////////////
LINE CPgLayout::GetLinePrecedence(LINE lineA, LINE lineB) const
{
    if (lineA==lineB) {
        return lineA;
    }
    if (lineA==LINE_NONE || lineA==LINE_NOT_APPL) {
        return lineB;
    }
    if (lineB==LINE_NONE || lineB==LINE_NOT_APPL) {
        return lineA;
    }
    if (lineA==LINE_THICK || lineB==LINE_THICK) {
        return LINE_THICK;
    }
    return LINE_THIN;
}

void CPgLayout::CoordinateLines(LINE eBorderLeft, LINE eBorderTop, LINE eBorderRight, LINE eBorderBottom)
{
    int iPgObA, iPgObB;
    LINE line;

    ////////////////////////////////////////////////////////////////////////////////////////
    // put table borders onto the sides of pgobs that face the outside of the table ...
    // (note: it's best to do this here, because eventually we'd like to support
    // multiple tables appearing on a single page.)

    // identify border areas...
    CRect rcBorder(m_rcUserLP.right, m_rcUserLP.bottom, -1,-1);    // (min horz, min vert, max horz, max vert), ie the border areas
    for (iPgObA=0 ; iPgObA<GetNumPgObs() ; iPgObA++) {
        CPgOb& pgobA=GetPgOb(iPgObA);
        const CRect& rcA=pgobA.GetClientRectLP();

        if (pgobA.GetType()==PGOB_COLHEAD ||
            pgobA.GetType()==PGOB_SPANNER ||
            pgobA.GetType()==PGOB_STUB ||
            pgobA.GetType()==PGOB_CAPTION ||
            pgobA.GetType()==PGOB_STUBHEAD ||
            pgobA.GetType()==PGOB_DATACELL ||
            pgobA.GetType()==PGOB_STUB_RIGHT ||
            pgobA.GetType()==PGOB_READER_BREAK) {
            // borders only apply to column, stub, and data areas ...
            if (rcA.left<rcBorder.left) {
                rcBorder.left=rcA.left;
            }
            if (rcA.top<rcBorder.top) {
                rcBorder.top=rcA.top;
            }
            if (rcA.right>rcBorder.right) {
                rcBorder.right=rcA.right;
            }
            if (rcA.bottom>rcBorder.bottom) {
                rcBorder.bottom=rcA.bottom;
            }
        }
    }

    // apply borders to objects that run along them...
    for (iPgObA=0 ; iPgObA<GetNumPgObs() ; iPgObA++) {
        CPgOb& pgobA=GetPgOb(iPgObA);
        const CRect& rcA=pgobA.GetClientRectLP();
        CFmt* pFmtA=pgobA.GetFmt();

        if (pgobA.GetType()==PGOB_COLHEAD ||
            pgobA.GetType()==PGOB_SPANNER ||
            pgobA.GetType()==PGOB_STUB ||
            pgobA.GetType()==PGOB_CAPTION ||
            pgobA.GetType()==PGOB_STUBHEAD ||
            pgobA.GetType()==PGOB_DATACELL ||
            pgobA.GetType()==PGOB_STUB_RIGHT ||
            pgobA.GetType()==PGOB_READER_BREAK) {
            // borders only apply to column, stub, and data areas ...
            if (rcA.left==rcBorder.left) {
                line=GetLinePrecedence(pFmtA->GetLineLeft(), eBorderLeft);
                pFmtA->SetLineLeft(line);
            }
            if (rcA.top==rcBorder.top) {
                line=GetLinePrecedence(pFmtA->GetLineTop(), eBorderTop);
                pFmtA->SetLineTop(line);
            }
            if (rcA.right==rcBorder.right) {
                line=GetLinePrecedence(pFmtA->GetLineRight(), eBorderRight);
                pFmtA->SetLineRight(line);
            }
            if (rcA.bottom==rcBorder.bottom) {
                line=GetLinePrecedence(pFmtA->GetLineBottom(), eBorderBottom);
                pFmtA->SetLineBottom(line);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////
    // analyze intersecting pgobs...
    for (iPgObA=0 ; iPgObA<GetNumPgObs() ; iPgObA++) {
        CPgOb& pgobA=GetPgOb(iPgObA);
        const CRect& rcA=pgobA.GetClientRectLP();
        CFmt* pFmtA=pgobA.GetFmt();

        for (iPgObB=iPgObA+1 ; iPgObB<GetNumPgObs() ; iPgObB++) {
            CPgOb& pgobB=GetPgOb(iPgObB);
            const CRect& rcB=pgobB.GetClientRectLP();
            CFmt* pFmtB=pgobB.GetFmt();

            // special case for titles and non-autofitting spanners
            if (pgobA.GetType()==PGOB_TITLE && pFmtA->GetLineBottom()==LINE_NONE) {
                continue;
            }
            if (pgobB.GetType()==PGOB_TITLE && pFmtB->GetLineBottom()==LINE_NONE) {
                continue;
            }

            // check for vertical intersection (partial or complete)
            if ((rcA.left<=rcB.left && rcA.right>rcB.left) || (rcA.left>=rcB.left && rcA.right<rcB.right)) {

                // see if A's top hits B's bottom
                if (rcA.top==rcB.bottom) {
                    line=GetLinePrecedence(pFmtA->GetLineTop(), pFmtB->GetLineBottom());
                    pFmtA->SetLineTop(line);
                    pFmtB->SetLineBottom(line);
                }

                // see if A's bottom hits B's top
                if (rcA.bottom==rcB.top) {
                    line=GetLinePrecedence(pFmtA->GetLineBottom(), pFmtB->GetLineTop());
                    pFmtA->SetLineBottom(line);
                    pFmtB->SetLineTop(line);
                }
            }

            // check for horizontal intersection (partial or complete)
            if ((rcA.top<=rcB.top && rcA.bottom>rcB.top) || (rcA.top>=rcB.top && rcA.bottom<rcB.bottom)) {

                // see if A's left hits B's right
                if (rcA.left==rcB.right) {
                    line=GetLinePrecedence(pFmtA->GetLineLeft(), pFmtB->GetLineRight());
                    pFmtA->SetLineLeft(line);
                    pFmtB->SetLineRight(line);
                }

                // see if A's right hits B's left
                if (rcA.right==rcB.left) {
                    line=GetLinePrecedence(pFmtA->GetLineRight(), pFmtB->GetLineLeft());
                    pFmtA->SetLineRight(line);
                    pFmtB->SetLineLeft(line);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
//                             CPgMgr
//
/////////////////////////////////////////////////////////////////////////////
CPgMgr::CPgMgr()
{
    m_aPgLayout.RemoveAll();
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTblGrid::ForceHideAreaCaptionInOneRowTable()
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabPrtView::ForceHideAreaCaptionInOneRowTable(CTable* pTbl)
{
    bool bRet = false;
    bool bOneRow = pTbl->GetRowRoot() && pTbl->GetRowRoot()->GetNumChildren() == 1 && pTbl->GetRowRoot()->GetChild(0)->GetNumChildren() ==0  &&  pTbl->GetColRoot()->GetNumChildren() != 0;

    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    //CTabSet* pSet = pDoc->GetTableSpec();
	//Savy (R) sampling app 20081224
	CTabSet* pSet = NULL;

	if(!pDoc){
		ASSERT(m_pTabSet);
		pSet = m_pTabSet;
	}
	else{
     pSet = pDoc->GetTableSpec();
	}
    bool bHasAreaCaption = false;
    bHasAreaCaption = pSet->GetConsolidate()->GetNumAreas() > 0?bHasAreaCaption = true:bHasAreaCaption=false;

    if(bOneRow){
        if(bHasAreaCaption){
            CTblOb* pAreaCaption = pTbl->GetAreaCaption();
            CFmt* pAreaCaptionFmt = pAreaCaption->GetDerFmt();
            CFmtReg* pFmtReg = pTbl->GetFmtRegPtr();
            CFmt* pDefAreaCaptionFmt = DYNAMIC_DOWNCAST(CFmt,pFmtReg->GetFmt(FMT_ID_AREA_CAPTION));
            if(pAreaCaptionFmt && pAreaCaptionFmt->GetIndex() !=0){ //do nothing
            }
            else{
                CTabVar* pTabVar = pTbl->GetRowRoot()->GetChild(0);
                ASSERT(pTabVar);
                if(pTabVar && pTabVar->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) == 0){
                    bRet = true;
                }
            }
        }
    }
    return bRet;
}

/////////////////////////////////////////////////////////////////////////////////
//
//        GetDataCellFormat
//
//  see also CTblGrid::GetFmt4DataCell (same function, but for the grid)
//void CTabPrtView::GetCellFormat4Captions(CRowColPgOb* pColHead,CRowColPgOb* pStub, CFmt* pCellFmt, int iColHead, int iPanel) const
//
/////////////////////////////////////////////////////////////////////////////////
void CTabPrtView::GetCellFormat4Captions(CRowColPgOb* pColHead,CRowColPgOb* pStub, CFmt* pCellFmt, int iColHead, int iPanel) const
{
    ASSERT_VALID(pColHead);
    ASSERT_VALID(pStub);
    ASSERT_VALID(pCellFmt);
    CFmt* pFmt = NULL;
    // init
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
	//Savy (R) sampling app 20081229
	 CTabSet* pTabSet = NULL;
	if(pDoc){
		pTabSet = pDoc->GetTableSpec();
	}
	else{
		ASSERT(m_pTabSet); //this is set in the constructor from zSampF
		pTabSet = m_pTabSet;
	}
	//

    const CFmtReg& fmtReg = pTabSet->GetFmtReg();

    // captions only have to worry about lines, and we handle those separately (got to, since captions have CFmt formats, stubs have CDataCellFmt formats)
    if (pStub->GetFmt()->GetID()==FMT_ID_CAPTION || pStub->GetFmt()->GetID()==FMT_ID_AREA_CAPTION) {
        // handle lines for captions ...

        // formats without defaults
        CFmt* pCaptionFmt=DYNAMIC_DOWNCAST(CFmt,pStub->GetFmt());
        CFmt* pColFmt=DYNAMIC_DOWNCAST(CFmt,pColHead->GetFmt());
        ASSERT(!pColFmt->ContainsDefaultValues());
        ASSERT(!pCaptionFmt->ContainsDefaultValues());

        // formats with defaults
        CFmt* pSourceCaptionFmt=DYNAMIC_DOWNCAST(CFmt,pStub->GetTblBase()->GetFmt());
        CFmt* pSourceColFmt=DYNAMIC_DOWNCAST(CFmt,pColHead->GetTblBase()->GetFmt());
        if (pSourceCaptionFmt==NULL) {
            // no source stub fmt specified, use the default
            pSourceCaptionFmt=pCaptionFmt;
        }
        if (pSourceColFmt==NULL) {
            // no source colhead fmt specified, use the default
            pSourceColFmt=pColFmt;
        }
        ASSERT_VALID(pSourceCaptionFmt);
        ASSERT_VALID(pSourceColFmt);

        bool bCaptionExtends = pSourceCaptionFmt->GetLinesExtend();
        bool bColExtends = pSourceColFmt->GetLinesExtend();

        CFmt    eValdLineFmt4Row;
        CFmt    eValdLineFmt4Col;
        eValdLineFmt4Row.Assign(*pStub->GetFmt());
        eValdLineFmt4Col.Assign(*pColHead->GetFmt());
        (const_cast<CTabPrtView*>(this))->GetLineFmt4NonDataCell(pColHead ,fmtReg, true,FMT_ID_COLHEAD, eValdLineFmt4Col);
        (const_cast<CTabPrtView*>(this))->GetLineFmt4NonDataCell(pStub ,fmtReg, false,FMT_ID_CAPTION, eValdLineFmt4Row);

        bCaptionExtends = eValdLineFmt4Row.GetLinesExtend();
        bColExtends = eValdLineFmt4Col.GetLinesExtend();


        if (bCaptionExtends || bColExtends) {
            //if it does not extend you don't need to do anything

            //Lines can't be applied on an individual cell  .So special cells line attribute
            //need not be considered
            if (bCaptionExtends)  {
                bool bTop = true;
                bool bBottom = true;
                if (!pCaptionFmt->GetLinesExtend()) {
                    bTop = false;
                    bBottom = false;
                    int iNumSiblings= pStub->GetParent()->GetNumChildren();
                    if (pStub==pStub->GetParent()->GetChild(0)) {
                        // we are the leftmost child
                        if (pStub->GetNumLeaves()==0 && pStub->GetLevel()>2) {
                            //this case covers row items more than one A+B and first stub shld not
                            //have the top line  because The spanner covers it //Other wise you
                            //will get top lines for both spanner and stub
                            bTop =false;
                        }
                        else {
                            bTop = true;
                        }
                    }
                    if (pStub==pStub->GetParent()->GetChild(iNumSiblings-1)) {
                        bBottom = true;
                    }
                }
                if (bTop) {
                    pCellFmt->SetLineTop(pCaptionFmt->GetLineTop());
                }
                if (bBottom) {
                    pCellFmt->SetLineBottom(pCaptionFmt->GetLineBottom());
                }
            }
            if (bColExtends) {
                // use col val if col extends and not row
                bool bLeft = true;
                bool bRight = true;
                if (!pColFmt->GetLinesExtend()) {
                    bLeft = false;
                    bRight = false;
                    int iNumSiblings=pColHead->GetParent()->GetNumChildren();
                    if (pColHead==pColHead->GetParent()->GetChild(0)) {
                        bLeft = true;
                    }
                    if (pColHead==pColHead->GetParent()->GetChild(iNumSiblings-1)) {
                        bRight = true;
                    }
                }
                if (bLeft) {
                    pCellFmt->SetLineLeft(pColFmt->GetLineLeft());
                }
                if (bRight) {
                    pCellFmt->SetLineRight(pColFmt->GetLineRight());
                }
            }
        }

        // that's all we need to do with captions ...
      //  return;
    }

    // formats without defaults
    CFmt* pStubFmt=DYNAMIC_DOWNCAST(CFmt,pStub->GetFmt());
    CFmt* pColFmt=DYNAMIC_DOWNCAST(CFmt,pColHead->GetFmt());
    ASSERT(!pColFmt->ContainsDefaultValues());
    ASSERT(!pStubFmt->ContainsDefaultValues());

    // formats with defaults
    CFmt* pSourceStubFmt=DYNAMIC_DOWNCAST(CFmt,pStub->GetTblBase()->GetFmt());
    CFmt* pSourceColFmt=DYNAMIC_DOWNCAST(CFmt,pColHead->GetTblBase()->GetFmt());
    if (pSourceStubFmt==NULL) {
        // no source stub fmt specified, use the default
        pSourceStubFmt=pStubFmt;
    }
    if (pSourceColFmt==NULL) {
        // no source colhead fmt specified, use the default
        pSourceColFmt=pColFmt;
    }
    ASSERT_VALID(pSourceStubFmt);
    ASSERT_VALID(pSourceColFmt);

    ////////////////////////////////////////////////////////////////////////////////
    // analyze all of the fmt attributes, depending on whether or not (and how) they extend from
    // rows/columns into cells.  Here's how:
    //
    //   ATTRIB     COLHEAD  (EXTENDS)    DATACELL(AVAILABLE)       SPANNER/CAPTION(EXTENDS)
    //-----------------------------------------------------------------------------------------
    //   ALIGN      NEVER                   YES                      NO
    //   LINES      YES/NO                  NO                       YES/NO
    //   INDENT     YES/NO                  YES                      NO
    //   HIDDEN     ALWAYS                  NO                       NO
    //   FONT       YES/NO                  YES                      NO
    //   COLOR      YES/NO                  YES                      NO
    //   NUMDECIMALS  ALWAYS                YES                      NO
    //   CUSTOM-TEXT NEVER                  YES                      NO
    //
    bool bRowCust;     // true if the row is customized (ie, non-default)
    bool bColCust;     // true if the row is customized (ie, non-default)
    bool bRowExtends;  // true if a row attribute extends into cells
    bool bColExtends;  // true if a column attribute extends into cells

    ////////////////////////////////////////////////////////////////////////////////
    // text color
    bRowCust = (pSourceStubFmt->IsTextColorCustom() && pSourceStubFmt->GetIndex()!=0);
    bColCust = (pSourceColFmt->IsTextColorCustom() && pSourceColFmt->GetIndex()!=0);
    bRowExtends = pStubFmt->GetTextColorExtends();
    bColExtends = pColFmt->GetTextColorExtends();

    if (bRowExtends || bColExtends) {
        // if it does not extend you don't need to do anything
        if (bRowExtends && !bColExtends) {
            //use rows val if row extends and not col
            pCellFmt->SetTextColor(pStubFmt->GetTextColor());
        }
        else if (!bRowExtends && bColExtends) {
            // use col val if col extends and not row
            pCellFmt->SetTextColor(pColFmt->GetTextColor());
        }
        else if (bRowCust) {
            // use rows val if  row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pCellFmt->SetTextColor(pStubFmt->GetTextColor());
        }
        else if (bColCust) {
            // use Columns val if col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            pCellFmt->SetTextColor(pColFmt->GetTextColor());
        }
        else {
            // none of them are custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pCellFmt->SetTextColor(pStubFmt->GetTextColor());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // fill color
    bRowCust = (pSourceStubFmt->IsFillColorCustom() && pSourceStubFmt->GetIndex()!=0);
    bColCust = (pSourceColFmt->IsFillColorCustom() && pSourceColFmt->GetIndex()!=0);
    bRowExtends = pStubFmt->GetFillColorExtends();
    bColExtends = pColFmt->GetFillColorExtends();

    if (bRowExtends || bColExtends) {
        // if it does not extend you don't need to do anything
        if (bRowExtends && !bColExtends) {
            //use rows val if row extends and not col
            pCellFmt->SetFillColor(pStubFmt->GetFillColor());
        }
        else if (!bRowExtends && bColExtends) {
            // use col val if col extends and not row
            pCellFmt->SetFillColor(pColFmt->GetFillColor());
        }
        else if (bRowCust) {
            // use rows val if  row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pCellFmt->SetFillColor(pStubFmt->GetFillColor());
        }
        else if (bColCust) {
            // use Columns val if col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            pCellFmt->SetFillColor(pColFmt->GetFillColor());
        }
        else {
            // none of them are custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pCellFmt->SetFillColor(pStubFmt->GetFillColor());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // font
    bRowCust = (pSourceStubFmt->IsFontCustom() && pSourceStubFmt->GetIndex()!=0);
    bColCust = (pSourceColFmt->IsFontCustom() && pSourceColFmt->GetIndex()!=0);
    bRowExtends = pStubFmt->GetFontExtends();
    bColExtends = pColFmt->GetFontExtends();

    if (bRowExtends || bColExtends) {
        // if it does not extend you dont need to do anything
        LOGFONT lfRow ,lfCol;
        pStubFmt->GetFont()->GetLogFont(&lfRow);
        pColFmt->GetFont()->GetLogFont(&lfCol);
        int iRet = memcmp(&lfRow,&lfCol,sizeof(LOGFONT));
        if (bRowExtends && !bColExtends) {
            // use rows val if row extends and not col
            pCellFmt->SetFont(pStubFmt->GetFont());
        }
        else if (!bRowExtends && bColExtends) {
            // use col val if col extends and not row
            pCellFmt->SetFont(pColFmt->GetFont());
        }
        else if (bRowCust)  {
            // use rows val if row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pCellFmt->SetFont(pStubFmt->GetFont());
        }
        else if (bColCust)  {
            //use Columns val if col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            pCellFmt->SetFont(pColFmt->GetFont());
        }
        else {
            // none of them are custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pCellFmt->SetFont(pStubFmt->GetFont());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // hidden always extends
    bRowExtends = true;
    bColExtends = true;
    bRowCust = (pSourceStubFmt->GetHidden()!=HIDDEN_DEFAULT && pSourceStubFmt->GetIndex()!=0);
    bColCust = (pSourceColFmt->GetHidden()!=HIDDEN_DEFAULT && pSourceColFmt->GetIndex()!=0);

    if(pStubFmt->GetHidden() == HIDDEN_YES || pColFmt->GetHidden() == HIDDEN_YES){
        pCellFmt->SetHidden(HIDDEN_YES);
    }
    else {
        pCellFmt->SetHidden(HIDDEN_NO);
    }
    /*if (bRowExtends || bColExtends) {
       if (bRowExtends && !bColExtends) {
            // use rows val if row extends and not col
            pCellFmt->SetHidden(pStubFmt->GetHidden());
        }
        else if (!bRowExtends && bColExtends) {
            // use col val if col extends and not row
            pCellFmt->SetHidden(pColFmt->GetHidden());
        }
        else if (bRowCust)  {
            // use rows val if row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pCellFmt->SetHidden(pStubFmt->GetHidden());
        }
        else if (bColCust)  {
            // use Columns val if col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            pCellFmt->SetHidden(pColFmt->GetHidden());
        }
        else {
            // none of them are custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pCellFmt->SetHidden(pStubFmt->GetHidden());
        }
    }*/

    ////////////////////////////////////////////////////////////////////////////////
    // left indent
    bRowCust = (pSourceStubFmt->IsIndentCustom(LEFT) && pSourceStubFmt->GetIndex()!=0);
    bColCust = pSourceColFmt->IsIndentCustom(LEFT) && pSourceColFmt->GetIndex()!=0;

    bRowExtends = pStubFmt->GetIndentationExtends();
    bColExtends = pColFmt->GetIndentationExtends();

    if (bRowExtends || bColExtends) {
        if (bRowExtends && !bColExtends) {
            // use rows val  if row extends and not col
            pCellFmt->SetIndent(LEFT,pStubFmt->GetIndent(LEFT));
        }
        else if (!bRowExtends && bColExtends) {
            // use col val if col extends and not row
            pCellFmt->SetIndent(LEFT,pColFmt->GetIndent(LEFT));
        }
        else if (bRowCust)  {
            // use rows val if  row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pCellFmt->SetIndent(LEFT,pStubFmt->GetIndent(LEFT));
        }
        else if (bColCust)  {
            // use Columns val if col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            pCellFmt->SetIndent(LEFT,pColFmt->GetIndent(LEFT));
        }
        else {
            // none of them are custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pCellFmt->SetIndent(LEFT,pStubFmt->GetIndent(LEFT));
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // right indent
    bRowCust = (pSourceStubFmt->IsIndentCustom(RIGHT) && pSourceStubFmt->GetIndex()!=0);
    bColCust = pSourceColFmt->IsIndentCustom(RIGHT) && pSourceColFmt->GetIndex()!=0;

    bRowExtends = pStubFmt->GetIndentationExtends();
    bColExtends = pColFmt->GetIndentationExtends();

    if (bRowExtends || bColExtends) {
        if (bRowExtends && !bColExtends) {
            // use rows val  if row extends and not col
            pCellFmt->SetIndent(RIGHT,pStubFmt->GetIndent(RIGHT));
        }
        else if (!bRowExtends && bColExtends) {
            // use col val if col extends and not row
            pCellFmt->SetIndent(RIGHT,pColFmt->GetIndent(RIGHT));
        }
        else if (bRowCust) {
            // use rows val if row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pCellFmt->SetIndent(RIGHT,pStubFmt->GetIndent(RIGHT));
        }
        else if (bColCust) {
            // use Columns val  if  col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            pCellFmt->SetIndent(RIGHT,pColFmt->GetIndent(RIGHT));
        }
        else {
            // none of them are custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pCellFmt->SetIndent(RIGHT,pStubFmt->GetIndent(RIGHT));
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // lines
    bRowCust = false; //Lines are independent of col (top/bottom)
    bColCust = false; //lines are independent of row (left/right)
    bRowExtends = pSourceStubFmt->GetLinesExtend();
    bColExtends = pSourceColFmt->GetLinesExtend();

    if (bRowExtends || bColExtends) {
        //if it does not extend you don't need to do anything

        //Lines can't be applied on an individual cell  .So special cells line attribute
        //need not be considered
        if (bRowExtends)  {
            bool bTop = true;
            bool bBottom = true;
            if (!pStubFmt->GetLinesExtend()) {
                bTop = false;
                bBottom = false;
                int iNumSiblings= pStub->GetParent()->GetNumChildren();
                if (pStub==pStub->GetParent()->GetChild(0)) {
                    // we are the leftmost child
                    if (pStub->GetNumLeaves()==0 && pStub->GetLevel()>2) {
                        //this case covers row items more than one A+B and first stub shld not
                        //have the top line  because The spanner covers it //Other wise you
                        //will get top lines for both spanner and stub
                        bTop =false;
                    }
                    else {
                        bTop = true;
                    }
                }
                if (pStub==pStub->GetParent()->GetChild(iNumSiblings-1)) {
                    bBottom = true;
                }
            }
            if (bTop) {
                pCellFmt->SetLineTop(pStubFmt->GetLineTop());
            }
            if (bBottom) {
                pCellFmt->SetLineBottom(pStubFmt->GetLineBottom());
            }
        }
        if (bColExtends) {
            // use col val if col extends and not row
            bool bLeft = true;
            bool bRight = true;
            if (!pColFmt->GetLinesExtend()) {
                bLeft = false;
                bRight = false;
                int iNumSiblings=pColHead->GetParent()->GetNumChildren();
                if (pColHead==pColHead->GetParent()->GetChild(0)) {
                    bLeft = true;
                }
                if (pColHead==pColHead->GetParent()->GetChild(iNumSiblings-1)) {
                    bRight = true;
                }
            }
            if (bLeft) {
               pCellFmt->SetLineLeft(pColFmt->GetLineLeft());
            }
            if (bRight) {
                pCellFmt->SetLineRight(pColFmt->GetLineRight());
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//        GetDataCellFormat
//
//  see also CTblGrid::GetFmt4DataCell (same function, but for the grid)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabPrtView::GetDataCellFormat(CRowColPgOb* pColHead,CRowColPgOb* pStub, CDataCellFmt* pDataCellFmt, int iColHead, int iPanel) const
{
    ASSERT_VALID(pColHead);
    ASSERT_VALID(pStub);
    ASSERT_VALID(pDataCellFmt);

    // init
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
	//Savy (R) sampling app 20081229
	CTabSet* pTabSet = NULL;
	if(pDoc){
		pTabSet = pDoc->GetTableSpec();
	}
	else{
		ASSERT(m_pTabSet); //this is set in the constructor from zSampF
		pTabSet = m_pTabSet;
	}
    const CFmtReg& fmtReg = pTabSet->GetFmtReg();

    // captions only have to worry about lines, and we handle those separately (got to, since captions have CFmt formats, stubs have CDataCellFmt formats)
    if (pStub->GetFmt()->GetID()==FMT_ID_CAPTION ||pStub->GetFmt()->GetID()==FMT_ID_AREA_CAPTION) {
        GetCellFormat4Captions(pColHead,pStub, pDataCellFmt, iColHead, iPanel);
        return;
    }

    // figure out if this data cell has a special cell (only attempt for data rows)
    CSpecialCell* pSpecialCell=NULL;
    CDataCellFmt* pSpecialCellFmt=NULL;
    if (pStub->GetNumChildren()==0) {
//        int iPanel=GetRowPanel(pStub);
        CTabValue* pTabVal=DYNAMIC_DOWNCAST(CTabValue, pStub->GetTblBase());
        pSpecialCell=pTabVal->FindSpecialCell(iPanel, iColHead+1);
    }
    if (pSpecialCell) {
        ASSERT_VALID(pSpecialCell);
        pSpecialCellFmt=pSpecialCell->GetDerFmt();
    }

    // formats without defaults
    CDataCellFmt* pStubFmt=DYNAMIC_DOWNCAST(CDataCellFmt,pStub->GetFmt());
    CDataCellFmt* pColFmt=DYNAMIC_DOWNCAST(CDataCellFmt,pColHead->GetFmt());
    ASSERT(!pColFmt->ContainsDefaultValues());
    ASSERT(!pStubFmt->ContainsDefaultValues());

    // formats with defaults
    CDataCellFmt* pSourceStubFmt=DYNAMIC_DOWNCAST(CDataCellFmt,pStub->GetTblBase()->GetFmt());
    CDataCellFmt* pSourceColFmt=DYNAMIC_DOWNCAST(CDataCellFmt,pColHead->GetTblBase()->GetFmt());
    if (pSourceStubFmt==NULL) {
        // no source stub fmt specified, use the default
        pSourceStubFmt=pStubFmt;
    }
    if (pSourceColFmt==NULL) {
        // no source colhead fmt specified, use the default
        pSourceColFmt=pColFmt;
    }
    ASSERT_VALID(pSourceStubFmt);
    ASSERT_VALID(pSourceColFmt);

    CFmt    eValdLineFmt4Row;
    CFmt    eValdLineFmt4Col;
    eValdLineFmt4Row.Assign(*pStub->GetFmt());
    eValdLineFmt4Col.Assign(*pColHead->GetFmt());
    (const_cast<CTabPrtView*>(this))->GetLineFmt4NonDataCell(pColHead ,fmtReg, true,FMT_ID_COLHEAD, eValdLineFmt4Col);
    (const_cast<CTabPrtView*>(this))->GetLineFmt4NonDataCell(pStub ,fmtReg, false,FMT_ID_STUB, eValdLineFmt4Row);
    ////////////////////////////////////////////////////////////////////////////////
    // analyze all of the fmt attributes, depending on whether or not (and how) they extend from
    // rows/columns into cells.  Here's how:
    //
    //   ATTRIB     COLHEAD  (EXTENDS)    DATACELL(AVAILABLE)       SPANNER/CAPTION(EXTENDS)
    //-----------------------------------------------------------------------------------------
    //   ALIGN      NEVER                   YES                      NO
    //   LINES      YES/NO                  NO                       YES/NO
    //   INDENT     YES/NO                  YES                      NO
    //   HIDDEN     ALWAYS                  NO                       NO
    //   FONT       YES/NO                  YES                      NO
    //   COLOR      YES/NO                  YES                      NO
    //   NUMDECIMALS  ALWAYS                YES                      NO
    //   CUSTOM-TEXT NEVER                  YES                      NO
    //
    bool bRowCust;     // true if the row is customized (ie, non-default)
    bool bColCust;     // true if the row is customized (ie, non-default)
    bool bRowExtends;  // true if a row attribute extends into cells
    bool bColExtends;  // true if a column attribute extends into cells

    ////////////////////////////////////////////////////////////////////////////////
    // alignment
    if (pSpecialCellFmt && pSpecialCellFmt->GetHorzAlign()!=HALIGN_DEFAULT)  {
        // use special cell's horizontal alignment
        pDataCellFmt->SetHorzAlign(pSpecialCellFmt->GetHorzAlign());
    }
    // *** otherwise, do nothing; alignment NEVER extends into cells

    if (pSpecialCellFmt && pSpecialCellFmt->GetVertAlign()!=VALIGN_DEFAULT)  {
        // use special cell's vertical alignment
        pDataCellFmt->SetVertAlign(pSpecialCellFmt->GetVertAlign());
    }
    // *** otherwise, do nothing; alignment NEVER extends into cells

    ////////////////////////////////////////////////////////////////////////////////
    // custom text
    if (pSpecialCellFmt && pSpecialCellFmt->IsTextCustom())  {
        // use special cell's horizontal alignment
        pDataCellFmt->SetCustom(pSpecialCellFmt->GetCustom());
    }
    // *** do nothing; custom text NEVER extends into cells

    ////////////////////////////////////////////////////////////////////////////////
    // text color
    bRowCust = (pSourceStubFmt->IsTextColorCustom() && pSourceStubFmt->GetIndex()!=0);
    bColCust = (pSourceColFmt->IsTextColorCustom() && pSourceColFmt->GetIndex()!=0);
    bRowExtends = pStubFmt->GetTextColorExtends();
    bColExtends = pColFmt->GetTextColorExtends();

    if (pSpecialCellFmt && pSpecialCellFmt->IsTextColorCustom())  {
        // use special cell's text color
        pDataCellFmt->SetTextColor(pSpecialCellFmt->GetTextColor());
    }
    else if (bRowExtends || bColExtends) {
        // if it does not extend you don't need to do anything
        if (bRowCust && bColCust && pStubFmt->GetTextColor().m_rgb != pColFmt->GetTextColor().m_rgb) {
            ASSERT(pSpecialCellFmt);
        }
        if (bRowExtends && !bColExtends) {
            //use rows val if row extends and not col
            pDataCellFmt->SetTextColor(pStubFmt->GetTextColor());
        }
        else if (!bRowExtends && bColExtends) {
            // use col val if col extends and not row
            pDataCellFmt->SetTextColor(pColFmt->GetTextColor());
        }
        else if (bRowCust) {
            // use rows val if  row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetTextColor(pStubFmt->GetTextColor());
        }
        else if (bColCust) {
            // use Columns val if col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            pDataCellFmt->SetTextColor(pColFmt->GetTextColor());
        }
        else {
            // none of them are custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetTextColor(pStubFmt->GetTextColor());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // fill color
    bRowCust = (pSourceStubFmt->IsFillColorCustom() && pSourceStubFmt->GetIndex()!=0);
    bColCust = (pSourceColFmt->IsFillColorCustom() && pSourceColFmt->GetIndex()!=0);
    bRowExtends = pStubFmt->GetFillColorExtends();
    bColExtends = pColFmt->GetFillColorExtends();

    if (pSpecialCellFmt && pSpecialCellFmt->IsFillColorCustom())  {
        // use special cell's text color
        pDataCellFmt->SetFillColor(pSpecialCellFmt->GetFillColor());
    }
    else if (bRowExtends || bColExtends) {
        // if it does not extend you don't need to do anything
        if (bRowCust && bColCust && pStubFmt->GetFillColor().m_rgb != pColFmt->GetFillColor().m_rgb) {
            ASSERT(pSpecialCellFmt);
        }
        else if (bRowExtends && !bColExtends) {
            //use rows val if row extends and not col
            pDataCellFmt->SetFillColor(pStubFmt->GetFillColor());
        }
        else if (!bRowExtends && bColExtends) {
            // use col val if col extends and not row
            pDataCellFmt->SetFillColor(pColFmt->GetFillColor());
        }
        else if (bRowCust) {
            // use rows val if  row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetFillColor(pStubFmt->GetFillColor());
        }
        else if (bColCust) {
            // use Columns val if col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            pDataCellFmt->SetFillColor(pColFmt->GetFillColor());
        }
        else {
            // none of them are custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetFillColor(pStubFmt->GetFillColor());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // font
    bRowCust = (pSourceStubFmt->IsFontCustom() && pSourceStubFmt->GetIndex()!=0);
    bColCust = (pSourceColFmt->IsFontCustom() && pSourceColFmt->GetIndex()!=0);
    bRowExtends = pStubFmt->GetFontExtends();
    bColExtends = pColFmt->GetFontExtends();

    if (pSpecialCellFmt && pSpecialCellFmt->IsFontCustom()) {
        // use special cell's font
        pDataCellFmt->SetFont(pSpecialCellFmt->GetFont());
        //SAVY&& fixed the "diminishing bug"
        //LPToPoints(pSpecialCellFmt, GetLogPixelsY());
        LPToPoints(pDataCellFmt, GetLogPixelsY());
        PointsToTwips(pDataCellFmt);
    }
    else if (bRowExtends || bColExtends) {
        // if it does not extend you dont need to do anything
        LOGFONT lfRow ,lfCol;
        pStubFmt->GetFont()->GetLogFont(&lfRow);
        pColFmt->GetFont()->GetLogFont(&lfCol);
        int iRet = memcmp(&lfRow,&lfCol,sizeof(LOGFONT));
      //  if (bRowCust && bColCust && iRet==0) {
      //      ASSERT (pSpecialCellFmt);
      //  }
        if (bRowExtends && !bColExtends) {
            // use rows val if row extends and not col
            pDataCellFmt->SetFont(pStubFmt->GetFont());
        }
        else if (!bRowExtends && bColExtends) {
            // use col val if col extends and not row
            pDataCellFmt->SetFont(pColFmt->GetFont());
        }
        else if (bRowCust)  {
            // use rows val if row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetFont(pStubFmt->GetFont());
        }
        else if (bColCust)  {
            //use Columns val if col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            pDataCellFmt->SetFont(pColFmt->GetFont());
        }
        else {
            // none of them are custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetFont(pStubFmt->GetFont());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // number of decimals
    bRowCust = (pSourceStubFmt->GetNumDecimals()!=NUM_DECIMALS_DEFAULT && pSourceStubFmt->GetIndex()!=0);
    bColCust = (pSourceColFmt->GetNumDecimals()!=NUM_DECIMALS_DEFAULT && pSourceColFmt->GetIndex()!=0);

    // decimals always extend
    bRowExtends = true;
    bColExtends = true;

    if (pSpecialCellFmt && pSpecialCellFmt->GetNumDecimals()!=NUM_DECIMALS_DEFAULT)  {
        // use special cell's number of decimals
        pDataCellFmt->SetNumDecimals(pSpecialCellFmt->GetNumDecimals());
    }
    else if (bRowExtends || bColExtends) {
        // if it does not extend you dont need to do anything
        if (bRowCust && bColCust &&  pStubFmt->GetNumDecimals()!=pColFmt->GetNumDecimals()) {
            ASSERT(pSpecialCellFmt);
        }
        if (bRowExtends && !bColExtends) {
            //use rows val if row extends and not col
            pDataCellFmt->SetNumDecimals(pStubFmt->GetNumDecimals());
        }
        else if (!bRowExtends && bColExtends) {
            // use col val if col extends and not row
            pDataCellFmt->SetNumDecimals(pColFmt->GetNumDecimals());
        }
        else if (bRowCust)  {
            // use rows val if row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetNumDecimals(pStubFmt->GetNumDecimals());
        }
        else if (bColCust)  {
            // use Columns val if col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            pDataCellFmt->SetNumDecimals(pColFmt->GetNumDecimals());
        }
        else {
            // none of them are custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetNumDecimals(pStubFmt->GetNumDecimals());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // hidden always extends
    bRowExtends = true;
    bColExtends = true;
    bRowCust = (pSourceStubFmt->GetHidden()!=HIDDEN_DEFAULT && pSourceStubFmt->GetIndex()!=0);
    bColCust = (pSourceColFmt->GetHidden()!=HIDDEN_DEFAULT && pSourceColFmt->GetIndex()!=0);

    if (bRowExtends || bColExtends) {
        // if it does not extend you dont need to do anything
        if (bRowCust && bColCust &&  pStubFmt->GetHidden()!=pColFmt->GetHidden()) {
             //  ASSERT(pSpecialCell); //captions and spanners do not have special cells
        }
        // hidden isn't used in special cells
//        if (pSpecialCellFmt && pSpecialCellFmt->GetHidden()!=HIDDEN_DEFAULT)  {
//            // use special cell's hidden
//            ASSERT(pSpecialCellFmt->GetHidden()!=HIDDEN_NOT_APPL);
//            pDataCellFmt->SetHidden(pSpecialCellFmt->GetHidden());
//        }
        if(pSpecialCellFmt){ //hidden is not appl for data cells . the attributes is infered from stub/colhead
            if(pStubFmt->GetHidden() == HIDDEN_YES || pColFmt->GetHidden() == HIDDEN_YES){
                pDataCellFmt->SetHidden(HIDDEN_YES);
            }
            else {
                pDataCellFmt->SetHidden(HIDDEN_NO);
            }
        }
        else if (bRowExtends && !bColExtends) {
            // use rows val if row extends and not col
            pDataCellFmt->SetHidden(pStubFmt->GetHidden());
        }
        else if (!bRowExtends && bColExtends) {
            // use col val if col extends and not row
            pDataCellFmt->SetHidden(pColFmt->GetHidden());
        }
        else if (bRowCust)  {
            // use rows val if row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetHidden(pStubFmt->GetHidden());
        }
        else if (bColCust)  {
            // use Columns val if col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            pDataCellFmt->SetHidden(pColFmt->GetHidden());
        }
        else {
            // none of them are custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetHidden(pStubFmt->GetHidden());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // left indent
    bRowCust = (pSourceStubFmt->IsIndentCustom(LEFT) && pSourceStubFmt->GetIndex()!=0);
    bColCust = pSourceColFmt->IsIndentCustom(LEFT) && pSourceColFmt->GetIndex()!=0;

    bRowExtends = pStubFmt->GetIndentationExtends();
    bColExtends = pColFmt->GetIndentationExtends();

    if (pSpecialCellFmt && pSpecialCellFmt->IsIndentCustom(LEFT)) {
        // use special cell's indent
        pDataCellFmt->SetIndent(LEFT, pSpecialCellFmt->GetIndent(LEFT));
    }
    else if (bRowExtends || bColExtends) {
        // if it does not extend you dont need to do anything
        if (bRowCust && bColCust && pStubFmt->GetIndent(LEFT) != pColFmt->GetIndent(LEFT)) {
            ASSERT(pSpecialCellFmt);
        }
        if (bRowExtends && !bColExtends) {
            // use rows val  if row extends and not col
            pDataCellFmt->SetIndent(LEFT,pStubFmt->GetIndent(LEFT));
        }
        else if (!bRowExtends && bColExtends) {
            // use col val if col extends and not row
            pDataCellFmt->SetIndent(LEFT,pColFmt->GetIndent(LEFT));
        }
        else if (bRowCust)  {
            // use rows val if  row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetIndent(LEFT,pStubFmt->GetIndent(LEFT));
        }
        else if (bColCust)  {
            // use Columns val if col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            pDataCellFmt->SetIndent(LEFT,pColFmt->GetIndent(LEFT));
        }
        else {
            // none of them are custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetIndent(LEFT,pStubFmt->GetIndent(LEFT));
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // right indent
    bRowCust = (pSourceStubFmt->IsIndentCustom(RIGHT) && pSourceStubFmt->GetIndex()!=0);
    bColCust = pSourceColFmt->IsIndentCustom(RIGHT) && pSourceColFmt->GetIndex()!=0;

    bRowExtends = pStubFmt->GetIndentationExtends();
    bColExtends = pColFmt->GetIndentationExtends();

    if (pSpecialCellFmt && pSpecialCellFmt->IsIndentCustom(RIGHT)) {
        // use special cell's indent
        pDataCellFmt->SetIndent(RIGHT, pSpecialCellFmt->GetIndent(RIGHT));
    }
    else if (bRowExtends || bColExtends) {
        // if it does not extend you dont need to do anything
        if (bRowCust && bColCust && pStubFmt->GetIndent(RIGHT) != pColFmt->GetIndent(RIGHT)) {
            ASSERT(pSpecialCellFmt);
        }
        if (bRowExtends && !bColExtends) {
            // use rows val  if row extends and not col
            pDataCellFmt->SetIndent(RIGHT,pStubFmt->GetIndent(RIGHT));
        }
        else if (!bRowExtends && bColExtends) {
            // use col val if col extends and not row
            pDataCellFmt->SetIndent(RIGHT,pColFmt->GetIndent(RIGHT));
        }
        else if (bRowCust) {
            // use rows val if row is custom and if both extend
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetIndent(RIGHT,pStubFmt->GetIndent(RIGHT));
        }
        else if (bColCust) {
            // use Columns val  if  col is custom and if both extend
            ASSERT(bColExtends);
            ASSERT(bRowExtends);
            pDataCellFmt->SetIndent(RIGHT,pColFmt->GetIndent(RIGHT));
        }
        else {
            // none of them are custom .both are default. Let's use row's default
            ASSERT(bRowExtends);
            ASSERT(bColExtends);
            pDataCellFmt->SetIndent(RIGHT,pStubFmt->GetIndent(RIGHT));
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // lines
    bRowCust = false; //Lines are independent of col (top/bottom)
    bColCust = false; //lines are independent of row (left/right)
    bRowExtends = eValdLineFmt4Row.GetLinesExtend();
    bColExtends = eValdLineFmt4Col.GetLinesExtend();
    //bColExtends = true;
    if (bRowExtends || bColExtends) {
        //if it does not extend you don't need to do anything

        //Lines can't be applied on an individual cell  .So special cells line attribute
        //need not be considered
        if (bRowExtends)  {
            bool bTop = true;
            bool bBottom = true;
            if (!pStubFmt->GetLinesExtend()) {
                bTop = false;
                bBottom = false;
                int iNumSiblings= pStub->GetParent()->GetNumChildren();
                if (pStub==pStub->GetParent()->GetChild(0)) {
                    // we are the leftmost child
                    if (pStub->GetNumLeaves()==0 && pStub->GetLevel()>2) {
                        //this case covers row items more than one A+B and first stub shld not
                        //have the top line  because The spanner covers it //Other wise you
                        //will get top lines for both spanner and stub
                        bTop =false;
                    }
                    else {
                        bTop = true;
                    }
                }
                if (pStub==pStub->GetParent()->GetChild(iNumSiblings-1)) {
                    bBottom = true;
                }
            }
            if (bTop) {
                pDataCellFmt->SetLineTop(pStubFmt->GetLineTop());
            }
            if (bBottom) {
                pDataCellFmt->SetLineBottom(pStubFmt->GetLineBottom());
            }
        }
        if (bColExtends) {
            // use col val if col extends and not row
            bool bLeft = true;
            bool bRight = true;
            if (!pColFmt->GetLinesExtend()) {
                bLeft = false;
                bRight = false;
                int iNumSiblings=pColHead->GetParent()->GetNumChildren();
                if (pColHead==pColHead->GetParent()->GetChild(0)) {
                    bLeft = true;
                }
                if (pColHead==pColHead->GetParent()->GetChild(iNumSiblings-1)) {
                    bRight = true;
                }
            }
            if (bLeft) {
               pDataCellFmt->SetLineLeft(pColFmt->GetLineLeft());
            }
            if (bRight) {
                pDataCellFmt->SetLineRight(pColFmt->GetLineRight());
            }
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
// //see
//  void CTblGrid::GetLineFmt4NonDataCell(CGRowColOb* pRCPgOb ,FMT_ID eGridComp, CFmt& retFmt)
//void CTabPrtView::GetLineFmt4NonDataCell(CRowColPgOb* pRCPgOb , const CFmtReg& fmtReg, bool bColumn,FMT_ID eGridComp, CFmt& retFmt)
/////////////////////////////////////////////////////////////////////////////////
void CTabPrtView::GetLineFmt4NonDataCell(CRowColPgOb* pRCPgOb , const CFmtReg& fmtReg, bool bColumn,FMT_ID eGridComp, CFmt& retFmt)
{
    //ASSERT if it is not spanner / caption / stub / colhead
    bool bComponent = (eGridComp ==  FMT_ID_CAPTION || eGridComp == FMT_ID_SPANNER || eGridComp == FMT_ID_STUB
        || eGridComp ==  FMT_ID_COLHEAD );
    bool bLeft =false; //using for left/right (or top/bottom lines)
    bool bRight  = false;
    ASSERT(bComponent);
    ASSERT(pRCPgOb);

    CFmt* pFmtSource = NULL;
    CFmt fmtSource,fmtStubHeadCopy;

    CTblBase* pTblBase =pRCPgOb->GetTblBase();
    CTabValue* pTabVal = DYNAMIC_DOWNCAST(CTabValue,pTblBase);
    CTabVar*   pTabVar = DYNAMIC_DOWNCAST(CTabVar,pTblBase);
    if(!pTabVal && !pTabVar){//wheb pgob are of area captio types
        return;
    }
    if(eGridComp==FMT_ID_CAPTION || eGridComp==FMT_ID_STUB){
        //second test in the following is for A*B where you have tabval of A as a caption
        if(!pTabVal) {
            ASSERT(pTabVar);
            pFmtSource = pTabVar->GetDerFmt();
        }
        else {
            ASSERT(pTabVal);
            pFmtSource = pTabVal->GetDerFmt();
        }
        CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
		//CTabSet* pSet = pDoc->GetTableSpec();
		//Savy (R) sampling app 20081224
		CTabSet* pSet = NULL;

		if(!pDoc){
			ASSERT(m_pTabSet);
			pSet = m_pTabSet;
		}
		else{
			pSet = pDoc->GetTableSpec();
		}
        CTable* pTbl = pSet->GetTable(pRCPgOb->GetTbl());
        if (NULL!=pTbl->GetStubhead(0)->GetDerFmt()) {
            const CFmt* pFmtStubHead = NULL;
            pFmtStubHead = pTbl->GetStubhead(0)->GetDerFmt();
            fmtStubHeadCopy.Assign(*pFmtStubHead);
            CFmt* pefFmtStubHead=DYNAMIC_DOWNCAST(CFmt,pTbl->GetFmtRegPtr()->GetFmt(FMT_ID_STUBHEAD,0));
            fmtStubHeadCopy.CopyNonDefaultValues(pefFmtStubHead);
        }
        else {
            const CFmt* pFmtStubHead = NULL;
            pFmtStubHead=DYNAMIC_DOWNCAST(CFmt,pTbl->GetFmtRegPtr()->GetFmt(FMT_ID_STUBHEAD,0));
            fmtStubHeadCopy.Assign(*pFmtStubHead);

        }
        if(fmtStubHeadCopy.GetLinesExtend()){
            retFmt.SetLinesExtend(true);
            bLeft = true;
            bRight =true;
        }
    }
    else if(eGridComp==FMT_ID_SPANNER || eGridComp==FMT_ID_COLHEAD){
        //second test in the following is for A*B where you have tabval of A as a Spanner
        if((eGridComp == FMT_ID_SPANNER) && !pTabVar){
            ASSERT(pTabVal);
            if(pRCPgOb->GetNumChildren() > 0){
                CRowColPgOb* pRCParentPgOb = pRCPgOb->GetParent();
                pTabVar = DYNAMIC_DOWNCAST(CTabVar,pRCParentPgOb->GetTblBase());
                ASSERT(pTabVar);
                pFmtSource = pTabVar->GetDerFmt();
            }
        }
        else if(pTabVar){
            pFmtSource = pTabVar->GetDerFmt();
        }
        else {
            ASSERT(pTabVal);
            pFmtSource = pTabVal->GetDerFmt();
        }
    }
    CFmt* pDefFmt  = DYNAMIC_DOWNCAST(CFmt,fmtReg.GetFmt(eGridComp));
    ASSERT(pDefFmt);
    if(pFmtSource){
        fmtSource = *pFmtSource;
    }
    else {
        fmtSource=*pDefFmt;
    }
    //CopyNonDefVals(fmtSource,*pDefFmt);
    fmtSource.CopyNonDefaultValues(pDefFmt);
    //Copy just the line related stuff to the retFmt;
    retFmt.SetLineTop(fmtSource.GetLineTop()) ;
    retFmt.SetLineBottom(fmtSource.GetLineBottom()) ;
    retFmt.SetLineLeft(fmtSource.GetLineLeft()) ;
    retFmt.SetLineRight(fmtSource.GetLineRight()) ;

    // CGRowColOb* pRoot = pGTblRow ? (CGRowColOb*)GetRowRoot()  : (CGRowColOb*)GetColRoot();
    //If the object has lines and it extends . //Set the lines to thin/ thick  using  precendence

    if(fmtSource.GetLinesExtend()){
        retFmt.SetLinesExtend(true);
        bLeft = true;
        bRight = true;
    }
    else {
        //see if "this object" is first / last in the "group"
        if(pRCPgOb->GetParent() && pRCPgOb->GetType() !=PGOB_ROOT){
            CRowColPgOb* pParent = DYNAMIC_DOWNCAST(CRowColPgOb,pRCPgOb->GetParent());
            ASSERT(pParent);
            int iNumChildren = pParent->GetNumChildren();
            ASSERT(iNumChildren !=0);
            if(pParent->GetChild(0) == pRCPgOb){
                bLeft = true;
            }
            else if(pParent->GetChild(iNumChildren-1) == pRCPgOb){
                bRight = true;
            }
        }
    }
    if(!bLeft && !bRight){//nothing to do
        return;
    }

    //Get the precedence rule thin/thick  line types if parent extends
    CRowColPgOb* pParentOb = DYNAMIC_DOWNCAST(CRowColPgOb,pRCPgOb->GetParent());
    bool bLoop  = (pParentOb && pParentOb->GetParent()&& pParentOb->GetType() !=PGOB_ROOT);
    while(bLoop){
        FMT_ID eParentFmtID  = FMT_ID_INVALID;
        CFmt retParentFmt;
        if(!bColumn) {
            eParentFmtID =  pParentOb->GetNumChildren() > 0 ? FMT_ID_CAPTION : FMT_ID_STUB;
        }
        else{
            eParentFmtID =  pParentOb->GetNumChildren() > 0? FMT_ID_SPANNER : FMT_ID_COLHEAD;
        }
        GetLineFmt4NonDataCell(pParentOb,fmtReg, bColumn,eParentFmtID, retParentFmt);
        if(retParentFmt.GetLinesExtend()){
            retFmt.SetLinesExtend(true); //by the virtue of the parent
            switch(eGridComp){
                case FMT_ID_STUB:
                case FMT_ID_CAPTION:
                     //top and bottom lines for caption need not be processed .They come from themselves
                    //left and right are derived from the stubhead
                    if (bLeft && fmtStubHeadCopy.GetLineLeft()==LINE_THICK) {
                        retFmt.SetLineLeft(LINE_THICK);
                    }
                    if (bLeft &&  fmtStubHeadCopy.GetLineLeft()==LINE_THIN && fmtSource.GetLineLeft()!=LINE_THICK) {
                        retFmt.SetLineLeft(LINE_THIN);
                    }
                    if (bRight&&  fmtStubHeadCopy.GetLineRight()==LINE_THICK ) {
                        retFmt.SetLineRight(LINE_THICK);
                    }
                    if (bRight &&  fmtStubHeadCopy.GetLineRight()==LINE_THIN && fmtSource.GetLineRight()!=LINE_THICK) {
                        retFmt.SetLineRight(LINE_THIN);
                    }
                    break;
                case FMT_ID_COLHEAD:
                case FMT_ID_SPANNER:
                    if (bLeft && retParentFmt.GetLineLeft()==LINE_THICK ) {
                        retFmt.SetLineLeft(LINE_THICK);
                    }
                    if (bLeft && retParentFmt.GetLineLeft()==LINE_THIN && fmtSource.GetLineLeft()!=LINE_THICK) {
                        retFmt.SetLineLeft(LINE_THIN);
                    }
                    if (bRight && retParentFmt.GetLineRight()==LINE_THICK) {
                        retFmt.SetLineRight(LINE_THICK);
                    }
                    if (bRight && retParentFmt.GetLineRight()==LINE_THIN && fmtSource.GetLineRight()!=LINE_THICK) {
                        retFmt.SetLineRight(LINE_THIN);
                    }
                    break;
                default:
                    ASSERT(FALSE);
            }
        }
        pParentOb = DYNAMIC_DOWNCAST(CRowColPgOb,pParentOb->GetParent());
        bLoop  = (pParentOb && pParentOb->GetParent()&& pParentOb->GetType() !=PGOB_ROOT);
    }

    //along the chain and return
}
void CTabPrtView::FixLineFmt(CRowColPgOb* pRCPgOb , const CFmtReg& fmtReg, bool bColumn)
{
    FMT_ID eGridComp = FMT_ID_INVALID;
    if(pRCPgOb->GetType() != PGOB_ROOT){
        switch(pRCPgOb->GetType()){
            case PGOB_CAPTION:
                eGridComp = FMT_ID_CAPTION;
                break;
            case PGOB_STUB:
			case PGOB_READER_BREAK:
                eGridComp = FMT_ID_STUB;
                break;
            case PGOB_COLHEAD:
                eGridComp = FMT_ID_COLHEAD;
                break;
            case PGOB_SPANNER:
                eGridComp = FMT_ID_SPANNER;
                break;
            default:
                ASSERT(FALSE);

        }
        CFmt retFmt;
        //Do not pass the original because the extends gets changed and we will lose the
        //original information .This information has to be preserved for later for datacell
        //line evaluation.
        retFmt.Assign(*pRCPgOb->GetFmt());
        GetLineFmt4NonDataCell(pRCPgOb , fmtReg, bColumn,eGridComp,retFmt);
        pRCPgOb->GetFmt()->SetLineBottom(retFmt.GetLineBottom());
        pRCPgOb->GetFmt()->SetLineTop(retFmt.GetLineTop());
        pRCPgOb->GetFmt()->SetLineLeft(retFmt.GetLineLeft());
        pRCPgOb->GetFmt()->SetLineRight(retFmt.GetLineRight());
    }
    for(int iChild =0 ;iChild < pRCPgOb->GetNumChildren(); iChild++){
        FixLineFmt(pRCPgOb->GetChild(iChild),fmtReg,bColumn);
    }
    return;
}

// SelectDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "SelectDlg.h"
#include <zUtilF/Rectext.h>
#include <zUtilO/CustomFont.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

const int SELECT_DIALOG_MIN_ROWS_FOR_SEARCH = 15;


/////////////////////
// CSelectDlg dialog
void CSelectDlg::Init() {
    m_pOptions = NULL;
    if( m_pToolTipsCtrl != NULL )
        delete m_pToolTipsCtrl;
    m_pToolTipsCtrl = NULL;

    m_bMouseMove = false;

    m_cLastRect.SetRectEmpty();

    m_cLastPoint.x = 0;
    m_cLastPoint.y = 0;

    m_bCoordCal = false;
    m_bSearch = false;
    m_bInvert = false;

    m_ListCtrlRect.SetRectEmpty();
    m_SearchEdtRect.SetRectEmpty();
    m_FindBtnRect.SetRectEmpty();
    m_OkBtnRect.SetRectEmpty();
    m_CancelBtnRect.SetRectEmpty();
    m_InverBtnRect.SetRectEmpty();
    m_OrgWndRect.SetRectEmpty();

    m_iExtraHeightAvailable = 0;
}

CSelectDlg::CSelectDlg(CWnd* pParent /*=NULL*/)
    :   CDialog(IDD_SELECT_DLG, pParent)
{
    //{{AFX_DATA_INIT(CSelectDlg)
    //}}AFX_DATA_INIT

    m_pToolTipsCtrl = NULL;
    Init();
}

CSelectDlg::~CSelectDlg() {
    if( m_pToolTipsCtrl != NULL )
        delete m_pToolTipsCtrl;
    m_pToolTipsCtrl = NULL;
}

void  CSelectDlg::GetLastRect( CRect& cLastRect ) {
    cLastRect = m_cLastRect;
}

void CSelectDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSelectDlg)
    DDX_Control(pDX, IDC_CASELIST, m_ListCtrl);
    DDX_Control(pDX, IDOK, m_OkBtn);
    DDX_Control(pDX, IDCANCEL, m_CancelBtn);
    DDX_Control(pDX, IDC_INVERTMARKS_BTN, m_InverBtn);
    DDX_Control(pDX, IDC_FIND_BTN, m_FindBtn);
    DDX_Control(pDX, IDC_SEARCHBOX_EDIT, m_SearchEdt);
    //}}AFX_DATA_MAP
}



BEGIN_MESSAGE_MAP(CSelectDlg, CDialog)
//{{AFX_MSG_MAP(CSelectDlg)
    ON_WM_SIZE()
    ON_BN_CLICKED(IDC_FIND_BTN, OnFind)
    ON_BN_CLICKED(IDC_INVERTMARKS_BTN, OnInvertMarks)
    ON_EN_SETFOCUS(IDC_SEARCHBOX_EDIT, OnSetfocusSearchBox)
    ON_WM_HELPINFO()
    ON_EN_KILLFOCUS(IDC_SEARCHBOX_EDIT, OnKillfocusSearchboxEdit)
    ON_MESSAGE(UWM::Capi::FinishSelectDialog, OnFinishedDialog)
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_MOVE()
    ON_WM_CHAR()
    ON_WM_ACTIVATE()
    ON_WM_CLOSE()
    ON_WM_KEYDOWN()
    ON_WM_SETFOCUS()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectDlg message handlers


namespace
{
    int GetTitleWidth(CWnd* pWnd, const CString& csTitle)
    {
        NONCLIENTMETRICS lMetrics;
        CDC*    pDC=pWnd->GetDC();
        CFont*  pOldFont;
        CFont   pCaptionFont;
        int     iTitleWidth;

#ifdef _DEBUG
        LOGFONT     lFont;
        CFont*   pCurFont;

        pCurFont = pDC->GetCurrentFont();
        pCurFont->GetLogFont( &lFont );
#endif

        lMetrics.cbSize = sizeof(NONCLIENTMETRICS);
        SystemParametersInfo( SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &lMetrics, 0 );

        pCaptionFont.CreateFontIndirect( &lMetrics.lfCaptionFont );
        pOldFont = pDC->SelectObject( &pCaptionFont );

        iTitleWidth = pDC->GetTextExtent( csTitle ).cx;

        pDC->SelectObject( pOldFont );

        pWnd->ReleaseDC( pDC );

        return iTitleWidth;
    }

    int GetBorderWidth()
    {
        return ::GetSystemMetrics(SM_CXEDGE) + ::GetSystemMetrics(SM_CXSIZEFRAME);
    }
}


void CSelectDlg::Start() {
    int     iMinWidth,iMaxWidth;

    m_ListCtrl.Init();


    CFont*  pFont=NULL;
    CDC*    pListDC = m_ListCtrl.GetDC();

    if( pFont == NULL ) {
        pFont = GetFont();
    }

    CFont*  pOldFont=NULL;
    if( pFont != NULL ) {

        // GHM 20111026 so that the sizing of the font in the accept statement is correct
        UserDefinedFonts* pUserFonts = nullptr;
        AfxGetApp()->GetMainWnd()->SendMessage(WM_IMSA_GET_USER_FONTS, (WPARAM)&pUserFonts);

        if( pUserFonts != nullptr && pUserFonts->IsFontDefined(UserDefinedFonts::FontType::ValueSets) ) // user has defined a particular font
            pFont = pUserFonts->GetFont(UserDefinedFonts::FontType::ValueSets);


        SetFont( pFont );
        m_ListCtrl.SetFont( pFont );

        m_ListCtrl.m_pFont = pFont;
        pOldFont = pListDC->SelectObject( pFont );
    }

    // Re-Calculate min width
    if( m_pOptions->m_bUseTitle ) {
       //__unaligned
        iMinWidth = GetTitleWidth(this, m_pOptions->m_csTitle);
        iMinWidth += 3 * GetBorderWidth();
    }
    else {
        iMinWidth = 0;
    }

    // Recalculate MaxWidth
    iMaxWidth = ::GetSystemMetrics(SM_CXSCREEN) - 200;

    // Use Title?
    if( m_pOptions->m_bUseTitle ) {
        ModifyStyle( 0, WS_CAPTION | WS_SYSMENU );
        SetWindowText( m_pOptions->m_csTitle );
    }
    else {
        ModifyStyle( WS_CAPTION | WS_SYSMENU, 0 );
    }

    // Can Cancel
    GetDlgItem(IDCANCEL)->EnableWindow( SW_SHOWNORMAL );
    ModifyStyle( WS_SYSMENU, 0 );

    // Use ColTitle?
    if( m_pOptions->m_bUseColTitle )
        m_ListCtrl.ModifyStyle( LVS_NOCOLUMNHEADER, 0 );
    else // if( !m_pOptions->m_bUseColTitle )
        m_ListCtrl.ModifyStyle( 0, LVS_NOCOLUMNHEADER );

    //Options for SelListCtrl
    m_ListCtrl.SetOptions(m_pOptions);

    //Set relative coordinates of controls in window
    SetRelCoords();

    //Set On Init Dialog default values
    SetDefaults();

    //Load Items on List Control
    m_ListCtrl.LoadItems();

    m_ListCtrl.DoAutoWidth( pListDC, 0, iMaxWidth, m_iExtraHeightAvailable );


    // See if the title is not cutted
    CRect   selRect;

    GetWindowRect(&selRect);
    if( selRect.Width() < iMinWidth ) {
        SetWindowPos( &wndTop, 0, 0, iMinWidth, selRect.Height(), SWP_NOMOVE );
        m_ListCtrl.AdjustColWidth(); // RHF Feb 14, 2003
    }
    ///

    m_ListCtrl.SetListStatus(m_pOptions->m_pbaSelections);

    // Move to the position
    BestPos();

    if( pFont != NULL )
        pListDC->SelectObject( pOldFont );
    m_ListCtrl.ReleaseDC( pListDC );
}

BOOL CSelectDlg::OnInitDialog()
{
    // RHF INIC Dec 11, 2002
    CFont* pFont= GetFont();

    //SetFont( pFont );
    m_ListCtrl.m_pFont = pFont;
    // RHF END Dec 11, 2002


    CDialog::OnInitDialog();

    Start();
    m_ListCtrl.SetFocus();

    return FALSE;
    //return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

// Join Stick Windows with Selcase Windows
// Move Stick and Selcase Windows to the best position relative to Reference Window
void CSelectDlg::BestPos() {
    CRectExt    extRect; // include sticky window if exist
    CRect       selcaseRect, bestRect, stickyRect, referenceRect, parentRect;

    // RHF INIC Dec 09, 2002
    extRect.SetRectEmpty();
    selcaseRect.SetRectEmpty();
    bestRect.SetRectEmpty();
    stickyRect.SetRectEmpty();
    referenceRect.SetRectEmpty();
    parentRect.SetRectEmpty();
    // RHF END Dec 09, 2002

    // Rectangle of the selcase

    GetWindowRect(&selcaseRect);

    // Rectangle of the parent
    if( m_pParentWnd != NULL ) // Full-Screen
        m_pParentWnd->GetWindowRect(&parentRect);
    else
        parentRect.SetRect( 0, 0, ::GetSystemMetrics( SM_CXSCREEN ),::GetSystemMetrics(SM_CYSCREEN) );

    extRect.CopyRect( selcaseRect );

    MoveWindow( selcaseRect ); // could have been recalculated in Collapse
}

void CSelectDlg::SetDefaults()
{
    //Full row selection
    m_ListCtrl.SetFullRowSel(TRUE);

    //Set corresponding icons for buttons
    HICON iconOk, iconCancel, iconInvert, iconFind;
    iconOk = AfxGetApp()->LoadIcon(IDI_OK);
    iconCancel = AfxGetApp()->LoadIcon(IDI_CANCEL);
    iconInvert = AfxGetApp()->LoadIcon(IDI_INVERT);
    iconFind = AfxGetApp()->LoadIcon(IDI_FIND);

    m_OkBtn.SetIcon(iconOk);
    m_CancelBtn.SetIcon(iconCancel);
    m_InverBtn.SetIcon(iconInvert);
    m_FindBtn.SetIcon(iconFind);

    //Tool Tips
    if( m_pToolTipsCtrl != NULL )
        delete m_pToolTipsCtrl;

    m_pToolTipsCtrl = new CToolTipCtrl;
    ASSERT( m_pToolTipsCtrl );

    m_pToolTipsCtrl->Create(this, TTS_ALWAYSTIP | WS_VISIBLE); // GHM 20120125 removed the WS_CHILD property as that seemed not to work with either VS2010 or unicode

    m_pToolTipsCtrl->AddTool(&m_OkBtn, IDS_TOOLTIP_OKBTN);
    m_pToolTipsCtrl->AddTool(&m_CancelBtn, IDS_TOOLTIP_CANCELBTN);
    m_pToolTipsCtrl->AddTool(&m_InverBtn, IDS_TOOLTIP_INVERTBTN);
    m_pToolTipsCtrl->AddTool(&m_FindBtn, IDS_TOOLTIP_FINDBTN);

    m_bInvert = ( m_pOptions->m_iMaxMark == -1 || m_pOptions->m_iMaxMark > 1 );
    m_bSearch = ( m_pOptions->m_paData->size() >= SELECT_DIALOG_MIN_ROWS_FOR_SEARCH );

    ApplyInvertAndSearchSettings();
}

void CSelectDlg::SetRelCoords()
{
    RECT WndRect;
    RECT ListCtrlRect;
    RECT OkBtnRect;
    RECT CancelBtnRect;
    RECT InverBtnRect;
    RECT SearchEdtRect;
    RECT FindBtnRect;

    m_bCoordCal = false;

    //Get relative coordinates
    m_ListCtrl.GetWindowRect(&ListCtrlRect);
    m_OkBtn.GetWindowRect(&OkBtnRect);
    m_CancelBtn.GetWindowRect(&CancelBtnRect);
    m_InverBtn.GetWindowRect(&InverBtnRect);
    m_SearchEdt.GetWindowRect(&SearchEdtRect);
    m_FindBtn.GetWindowRect(&FindBtnRect);

    GetWindowRect(&WndRect);
    GetClientRect(&m_OrgWndRect);

    this->ScreenToClient(&OkBtnRect); // pin to the top right
    m_OkBtnRect.top = OkBtnRect.top;
    m_OkBtnRect.bottom = OkBtnRect.bottom;
    m_OkBtnRect.left = OkBtnRect.left - m_OrgWndRect.right;
    m_OkBtnRect.right = OkBtnRect.right - m_OrgWndRect.right;

    this->ScreenToClient(&CancelBtnRect); // pin to the top right
    m_CancelBtnRect.top = CancelBtnRect.top;
    m_CancelBtnRect.bottom = CancelBtnRect.bottom;
    m_CancelBtnRect.left = CancelBtnRect.left - m_OrgWndRect.right;
    m_CancelBtnRect.right = CancelBtnRect.right - m_OrgWndRect.right;

    this->ScreenToClient(&InverBtnRect); // pin to the top right
    m_InverBtnRect.top = InverBtnRect.top;
    m_InverBtnRect.bottom = InverBtnRect.bottom;
    m_InverBtnRect.left = InverBtnRect.left - m_OrgWndRect.right;
    m_InverBtnRect.right = InverBtnRect.right - m_OrgWndRect.right;

    this->ScreenToClient(&FindBtnRect); // pin to the bottom right
    m_FindBtnRect.top = FindBtnRect.top - m_OrgWndRect.bottom;
    m_FindBtnRect.bottom = FindBtnRect.bottom - m_OrgWndRect.bottom;
    m_FindBtnRect.left = FindBtnRect.left - m_OrgWndRect.right;
    m_FindBtnRect.right = FindBtnRect.right - m_OrgWndRect.right;

    this->ScreenToClient(&SearchEdtRect); // pin to the bottom left, spanning from left to right
    m_SearchEdtRect.top = SearchEdtRect.top - m_OrgWndRect.bottom;
    m_SearchEdtRect.bottom = SearchEdtRect.bottom - m_OrgWndRect.bottom;
    m_SearchEdtRect.left = SearchEdtRect.left;
    m_SearchEdtRect.right = SearchEdtRect.right - m_OrgWndRect.right;

    this->ScreenToClient(&ListCtrlRect); // pin to the top left, spanning from left to right and top to bottom
    m_ListCtrlRect.top = ListCtrlRect.top;
    m_ListCtrlRect.bottom = ListCtrlRect.bottom - m_OrgWndRect.bottom;
    m_ListCtrlRect.left = ListCtrlRect.left;
    m_ListCtrlRect.right = ListCtrlRect.right - m_OrgWndRect.right;

    m_bCoordCal = true;
}


int CSelectDlg::DoModal( CSelectListCtrlOptions* pOptions )
{
    ASSERT( pOptions->m_paData && pOptions->m_paData->size()>0 );

    Init();

    m_pOptions = pOptions;

    if( !m_pOptions->Check() ) {
        AfxMessageBox( _T("Selcase with inconsistent parameters") );
        return IDCANCEL;
    }

    return CDialog::DoModal();
}

void CSelectDlg::RedoSize( int cx, int cy ) {
    HWND LstWndHandle, SrchWndHandle;

    LstWndHandle = m_ListCtrl.GetSafeHwnd( );
    SrchWndHandle = m_SearchEdt.GetSafeHwnd();

    if( LstWndHandle == NULL || SrchWndHandle == NULL )
        return;

    m_OkBtn.MoveWindow( // pinned to the top right
        m_OkBtnRect.left + cx,
        m_OkBtnRect.top,
        m_OkBtnRect.Width(),
        m_OkBtnRect.Height());

    m_CancelBtn.MoveWindow( // pinned to the top right
        m_CancelBtnRect.left + cx,
        m_CancelBtnRect.top,
        m_CancelBtnRect.Width(),
        m_CancelBtnRect.Height());

    m_InverBtn.MoveWindow( // pinned to the top right
        m_InverBtnRect.left + cx,
        m_InverBtnRect.top,
        m_InverBtnRect.Width(),
        m_InverBtnRect.Height());

    m_FindBtn.MoveWindow( // pinned to the bottom right
        m_FindBtnRect.left + cx,
        m_FindBtnRect.top + cy,
        m_FindBtnRect.Width(),
        m_FindBtnRect.Height());

    m_SearchEdt.MoveWindow( // pinned to the bottom, spanning from left to right
        m_SearchEdtRect.left,
        m_SearchEdtRect.top + cy,
        ( m_SearchEdtRect.right + cx ) - m_SearchEdtRect.left,
        m_SearchEdtRect.Height());

    m_ListCtrl.MoveWindow( // pinned to the top left, spanning from left to right and top to bottom
        m_ListCtrlRect.left,
        m_ListCtrlRect.top,
        ( m_ListCtrlRect.right + cx ) - m_ListCtrlRect.left,
        ( m_ListCtrlRect.bottom + cy ) - m_ListCtrlRect.top + m_iExtraHeightAvailable);

    UpdateWindow();
}


void CSelectDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);

    if( nType == SIZE_RESTORED && m_bCoordCal) {
        RedoSize( cx, cy );
    }
}


void CSelectDlg::OnCancel()
{
    CDialog::OnCancel();
}


void CSelectDlg::OnOK()
{
    CWnd*   pwndCtrl = GetFocus();
    //CWnd*   pwndCtrlNext = pwndCtrl;
    int     ctrl_ID=pwndCtrl->GetDlgCtrlID();

    switch (ctrl_ID) {
    case IDC_SEARCHBOX_EDIT:
        //pwndCtrlNext = GetDlgItem(IDC_FIND_BTN);
        OnFind();
        break;
    case IDC_CASELIST:
    case IDOK:
        // GetListStatus still not called
        if( m_pOptions->m_pbaSelections != NULL ) {
            m_ListCtrl.GetListStatus(m_pOptions->m_pbaSelections);
        }

        CDialog::OnOK(); // Call to ::EndDialog(IDOK)
        break;

    default:
        break;
    }
}

void CSelectDlg::OnFind()
{
    CString csTextToFind;
    m_SearchEdt.GetWindowText(csTextToFind);
    m_ListCtrl.FindText(csTextToFind);
    m_SearchEdt.SetFocus();
}

void CSelectDlg::InvertMarks()
{
    m_ListCtrl.SetMarks( CSELLISTCTRL_TOGGLE );
}

void CSelectDlg::OnInvertMarks()
{
    InvertMarks();
    m_ListCtrl.SetFocus();
}

void CSelectDlg::OnSetfocusSearchBox()
{
    m_FindBtn.SetButtonStyle(BS_DEFPUSHBUTTON, TRUE);

    m_OkBtn.ModifyStyle( BS_DEFPUSHBUTTON, BS_PUSHBUTTON);
    m_OkBtn.SetButtonStyle(BS_PUSHBUTTON);
    m_InverBtn.SetButtonStyle(BS_PUSHBUTTON);
}

void CSelectDlg::OnKillfocusSearchboxEdit()
{
    m_FindBtn.SetButtonStyle(BS_DEFPUSHBUTTON, TRUE);

    m_OkBtn.ModifyStyle( BS_DEFPUSHBUTTON, BS_PUSHBUTTON);
    m_OkBtn.SetButtonStyle(BS_PUSHBUTTON);
    m_InverBtn.SetButtonStyle(BS_PUSHBUTTON);
}

BOOL CSelectDlg::PreTranslateMessage(MSG* pMsg)
{
    if( m_pToolTipsCtrl != NULL )
        m_pToolTipsCtrl->RelayEvent(pMsg);

    return CDialog::PreTranslateMessage(pMsg);
}


void CSelectDlg::ApplyInvertAndSearchSettings()
{
    m_ListCtrl.SetMultiSelect(m_bInvert);
    m_InverBtn.ShowWindow(m_bInvert ? SW_SHOWNORMAL : SW_HIDE);

    m_SearchEdt.ShowWindow(m_bSearch ? SW_SHOWNORMAL : SW_HIDE);
    m_FindBtn.ShowWindow(m_bSearch ? SW_SHOWNORMAL : SW_HIDE);

    if( !m_bSearch )
    {
        // we will eventually resize the window if no search bar or if only the invert toggle button is shown
        CWnd* pDesiredBottomMostWnd = m_bInvert ? (CWnd*)&m_InverBtn : (CWnd*)&m_CancelBtn;

        RECT desiredBottomMostWndRect;
        pDesiredBottomMostWnd->GetWindowRect(&desiredBottomMostWndRect);

        RECT actualBottomMostWndRect;
        m_FindBtn.GetWindowRect(&actualBottomMostWndRect);

        m_iExtraHeightAvailable = actualBottomMostWndRect.bottom - desiredBottomMostWndRect.bottom;

        m_iExtraHeightAvailable += m_FindBtnRect.right; // add back the (negative value) spacing at the bottom of the dialog
    }
}


BOOL CSelectDlg::OnHelpInfo(HELPINFO* /*pHelpInfo*/) {
    return TRUE;
}

BOOL CSelectDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    // TODO: Add your specialized code here and/or call the base class
    NMHDR*  pMsg=(NMHDR*) lParam;

    if( pMsg->idFrom == IDC_CASELIST && pMsg->code == LVN_ITEMCHANGED ) {
        // If has a limit for the marked elements.
        if( m_pOptions->m_iMaxMark >= 0 ) {
            int     iElem=m_ListCtrl.GetItemCount();
            int     iMarkedElem=m_ListCtrl.GetSelectedCount();
            bool    bEnableInvert;

            bEnableInvert = ( iElem - iMarkedElem <= m_pOptions->m_iMaxMark );
            GetDlgItem(IDC_INVERTMARKS_BTN)->EnableWindow( bEnableInvert );
        }
    }

    return CDialog::OnNotify(wParam, lParam, pResult);
}

LONG CSelectDlg::OnFinishedDialog(UINT wParam, LONG /*lParam*/)
{
    //  GetListStatus still not called
    if( m_pOptions->m_pbaSelections != NULL )
        m_ListCtrl.GetListStatus(m_pOptions->m_pbaSelections);

    EndDialog( (int) wParam );

    return 0;
}


void CSelectDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
    CDialog::OnLButtonUp(nFlags, point);

    m_bMouseMove = false;
    m_cLastPoint.x = 0;
    m_cLastPoint.y = 0;

    ReleaseCapture();
}


void CSelectDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
    CDialog::OnLButtonDown(nFlags, point);

    m_bMouseMove = true;
    m_cLastPoint = point;

    SetCapture();
}

void CSelectDlg::OnMouseMove(UINT nFlags, CPoint point)
{
    CDialog::OnMouseMove(nFlags, point);

    if( m_bMouseMove ) {
        int     iDiffX=point.x-m_cLastPoint.x;
        int     iDiffY=point.y-m_cLastPoint.y;

        CRect   rect;
        GetWindowRect( rect );

        rect.left   += iDiffX;
        rect.top    += iDiffY;
        rect.bottom += iDiffY;
        rect.right  += iDiffX;

        SetWindowPos(NULL, rect.left, rect.top,
            rect.Width(), rect.Height(), SWP_NOACTIVATE);
        ShowWindow(SW_SHOWNORMAL );

        m_cLastRect = rect;
    }
}


void CSelectDlg::OnMove(int x, int y)
{
    CDialog::OnMove(x, y);

    GetWindowRect( m_cLastRect );
}

void CSelectDlg::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    // TODO: Add your message handler code here and/or call default
    if( ::IsWindow(m_ListCtrl.m_hWnd) ) {
        m_ListCtrl.SetFocus();
        m_ListCtrl.SendMessage( WM_CHAR, nChar, nRepCnt );
    }

    CDialog::OnChar(nChar, nRepCnt, nFlags);
}

void CSelectDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    // TODO: Add your message handler code here and/or call default

    if( ::IsWindow(m_ListCtrl.m_hWnd) ) {
        m_ListCtrl.SetFocus();
        m_ListCtrl.SendMessage( WM_KEYDOWN, nChar, nRepCnt );
    }

    CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CSelectDlg::OnSetFocus(CWnd* pOldWnd)
{
    CDialog::OnSetFocus(pOldWnd);

    if( ::IsWindow(m_ListCtrl.m_hWnd) )
        m_ListCtrl.SetFocus();
}

void CSelectDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) {
    if( nState == WA_CLICKACTIVE || nState == WA_ACTIVE ) {
        SetMenuBar( true );
    }
    else if( nState == WA_INACTIVE ) {
        SetMenuBar( false );
    }

    CDialog::OnActivate(nState, pWndOther, bMinimized);

    if( nState == WA_CLICKACTIVE || nState == WA_ACTIVE ) {
        if( ::IsWindow(m_ListCtrl.m_hWnd) )
            m_ListCtrl.SetFocus();
    }
}

void CSelectDlg::SetMenuBar(BOOL OnOff) {
    if( ::IsWindow(m_ListCtrl.m_hWnd) ) {
        m_ListCtrl.SetListStatus(m_pOptions->m_pbaSelections);
        m_ListCtrl.SetMenuBar( OnOff );
    }
}

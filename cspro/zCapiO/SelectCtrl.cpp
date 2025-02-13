#include "StdAfx.h"
#include "SelectCtrl.h"
#include "SelectDlg.h"
#include <zUtilO/CustomFont.h>


const int MAX_KEYLEN = 1024;

////////////////////////
// CSelectListCtrlOptions Class

CSelectListCtrlOptions::CSelectListCtrlOptions()
{
    m_paData = NULL;
    m_paColumnTitles = NULL;
    m_pbaSelections = NULL;
    m_rowTextColors = nullptr;

    m_iMinMark = 0;
    m_iMaxMark = -1;  // Allow mark all
    m_bEndOnLimits = false; // Finish the dialog with OK when the iMaxMark are reached
    m_bUseTitle = true;
    m_csTitle = _T("");
    m_bUseColTitle = true;
    m_iKeyBuffMaxLen = 0; // Native behaviour
    m_bHasStickyCtrl = false; // Sticky control key. When the user press up/down/space the ctrl-key is ON

    m_pFont = NULL; // Font

    m_bHighLightFirst = false; // RHF Dec 18, 2002 Use true (menu & selcase) when the first option start highlighted

    m_bUseParentWindowLimitsForSizing = true;
}


bool CSelectListCtrlOptions::Check() {
    // Options must exist
    if( m_paData == NULL )
        return false;

    if( m_iMinMark > m_iMaxMark && m_iMaxMark != -1 )
        return false;

    if( m_iMinMark < 0 )
        return false;

    if( m_iMaxMark < -1 )
        return false;

    // if m_iMaxMark==0 then m_bEndOnLimits must be FALSE
    if( m_bEndOnLimits && m_iMaxMark == 0 )
        return false;

    if( m_iKeyBuffMaxLen < 0 )
        return false;

    if( m_pbaSelections != NULL ) {
        int     iSize, iNumSelected;
        bool    bSelected;

        iSize = m_pbaSelections->size();
        iNumSelected =  0;
        for( int i = 0; i < iSize; i++ ) {
            bSelected = m_pbaSelections->at(i);
            if( bSelected )
                iNumSelected++;
        }

        // Initial bitmap must have between 0 and m_iMaxMark 'marks'
        if( iNumSelected > m_iMaxMark && m_iMaxMark != -1 )
            return false;
    }

    // When mono-mark, sticky ctrl is not allowed
    if( m_iMaxMark == 1 && m_bHasStickyCtrl )
        return false;

    return true;
}



/////////////////////////////////////////////////////////////////////////////
// CSelectListCtrl

void CSelectListCtrl::Init() {
    int     i,j;

    if( ::IsWindow(m_hWnd) )
        DeleteAllItems();
    m_bIgnoreLimits = false;
    m_iMarked = 0;
    m_bHasPendingCtrl = false;  // flag to control the ficticious ctrl-key pressed
    m_baSelections.clear();
    m_pOptions = NULL;

    m_csKeyBuff = _T("");
    m_bIgnoreEnd = false;

    m_MarkedBkColor = RGB(0,155,0);
    m_MarkedTxtColor = ::GetSysColor(COLOR_HIGHLIGHTTEXT);

    for( i = 0; i < MAX_ITEMS; i++ )
        for( j = 0; j < MAX_COLUMNS; j++ )
            m_bFoundLight[i][j] = false;

    m_TextBkColor = ::GetSysColor(COLOR_WINDOW);
    m_TextColor = ::GetSysColor(COLOR_WINDOWTEXT);
    m_bCaseSensitiveSearch = false;
    m_bMenuBar = true;

    memset( &m_NormalLogFont, 0, sizeof(LOGFONT) );
    m_cxClient = 0;

    m_bIsMoving = false;

    m_iNRows = -1;
    m_iNCols = -1;

    m_pFont = NULL;
}

CSelectListCtrl::CSelectListCtrl()
{
    Init();
}

CSelectListCtrl::~CSelectListCtrl()
{
    EnableStickyControlKey( false );
}


BEGIN_MESSAGE_MAP(CSelectListCtrl, CListCtrl)
    //{{AFX_MSG_MAP(CSelectListCtrl)
    ON_WM_PAINT()
    ON_WM_SIZE()
    ON_WM_GETDLGCODE()
    ON_NOTIFY_REFLECT(LVN_ITEMCHANGING, OnItemchanging)
    ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnItemchanged)
    ON_WM_CHAR()
    ON_WM_KEYDOWN()
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONDOWN()
    ON_WM_RBUTTONDBLCLK()
    ON_WM_LBUTTONDBLCLK()
    ON_NOTIFY_REFLECT(NM_CLICK, OnClick)
    ON_MESSAGE(UWM::Capi::FinishSelectDialog, OnFinishedDialog)
    ON_WM_CREATE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectListCtrl message handlers
void CSelectListCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    DrawSelection(0, lpDrawItemStruct);

    if( m_pOptions->m_bHasStickyCtrl )
        EnableStickyControlKey( false );
}

void CSelectListCtrl::DrawFoundItem(int nItem)
{
    DrawSelection(nItem);
}

// Return TRUE if the item was selected
int CSelectListCtrl::DrawSelection(int nItem, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    //RHF Nov 13, 2006 CString csItemTxt;
    CString csItemTxt;
    CDC*    pDC=NULL;
    CRect   rcAllLabels;
    bool    bMustRelease=false;
    CFont*  pOldFont=NULL;

    // GHM 20100621 to allow for the dynamic setting of fonts for (old style) value sets
    UserDefinedFonts* pUserFonts = nullptr;
    AfxGetApp()->GetMainWnd()->SendMessage(WM_IMSA_GET_USER_FONTS, (WPARAM)&pUserFonts);

    if( pUserFonts != nullptr && pUserFonts->IsFontDefined(UserDefinedFonts::FontType::ValueSets) ) // user has defined a particular font
        m_pFont = pUserFonts->GetFont(UserDefinedFonts::FontType::ValueSets);

    else
        m_pFont = GetParent()->GetFont();// RHF Dec 11, 2002

    if (lpDrawItemStruct != NULL) {
        //pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
        pDC = GetDC();
        bMustRelease = true;

        if( m_pFont )
            pOldFont = pDC->SelectObject( m_pFont );

        nItem = lpDrawItemStruct->itemID;
    }
    else {
        pDC = GetDC();
        bMustRelease = true;

        if( m_pFont )
            pOldFont = pDC->SelectObject( m_pFont );

        GetItemRect(0, rcAllLabels, LVIR_BOUNDS);

        if(rcAllLabels.right < m_cxClient) {
            CRect rcClip;

            pDC->GetClipBox(rcClip);

            rcClip.left = std::min(rcAllLabels.right-1, rcClip.left);
            rcClip.right = m_cxClient;

            InvalidateRect(rcClip, TRUE);
        }
    }

    BOOL bFocus = (GetFocus() == this);

    csprochar szBuff[MAX_KEYLEN];

    LV_ITEM lvi;
    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lvi.iItem = nItem;
    lvi.iSubItem = 0;
    lvi.pszText = szBuff;
    lvi.cchTextMax = _countof(szBuff);
    lvi.stateMask = 0xFFFF;             // get all state flags
    GetItem(&lvi);

    bool bSelected = (bFocus || (GetStyle() & LVS_SHOWSELALWAYS)) && lvi.state & LVIS_SELECTED;

    // set colors if item is selected
    GetItemRect(nItem, rcAllLabels, LVIR_BOUNDS);

    CRect rcLabel;
    GetItemRect(nItem, rcLabel, LVIR_LABEL);

    COLORREF clrTextSave, clrBkSave;

    rcAllLabels.left = rcLabel.left;
    if( rcAllLabels.right < m_cxClient )
        rcAllLabels.right = m_cxClient;

    if( bSelected && m_bMenuBar ) {
        clrTextSave = pDC->SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
        clrBkSave = pDC->SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));

        pDC->FillRect(rcAllLabels, &CBrush(::GetSysColor(COLOR_HIGHLIGHT)));
    }
    else {
        //pDC->FillRect(rcAllLabels, &CBrush(::GetSysColor(COLOR_WINDOW)));
        pDC->FillRect(rcAllLabels, &CBrush(m_TextBkColor));

        COLORREF text_color;

        if( m_pOptions->m_rowTextColors != nullptr )
        {
            ASSERT((size_t)nItem < m_pOptions->m_rowTextColors->size());
            text_color = m_pOptions->m_rowTextColors->at(nItem).ToCOLORREF();
        }

        else
        {
            text_color = m_TextColor;
        }

        pDC->SetTextColor(text_color);
    }


   // draw item label

    CRect rcItem;

    GetItemRect(nItem, rcItem, LVIR_LABEL);

    int OFFSET_FIRST = 5;

    LPCTSTR pszText = szBuff;

    csItemTxt = pszText;
    csItemTxt.TrimRight();

    rcLabel = rcItem;
    rcLabel.left += OFFSET_FIRST;
    rcLabel.right -= OFFSET_FIRST;

    if (m_bFoundLight[nItem][0]) {
        COLORREF TextColorSave = pDC->SetTextColor(m_MarkedTxtColor);
        COLORREF BkColorSave = pDC->SetBkColor(m_MarkedBkColor);
        CSize TxtSize = pDC->GetTextExtent( csItemTxt );
        CRect TxtRect = rcLabel;

        TxtRect.right = TxtRect.left + TxtSize.cx;
        pDC->FillRect(TxtRect, &CBrush(m_MarkedBkColor));
        pDC->DrawText(csItemTxt,TxtRect,DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);
        pDC->SetTextColor(TextColorSave);
        pDC->SetBkColor(BkColorSave);
    }
    else {
        pDC->DrawText(csItemTxt,-1,rcLabel,DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);// RHF Dec 17, 2002
    }

    // draw labels for extra columns
    LV_COLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_WIDTH;

    for(int nColumn = 1; GetColumn(nColumn, &lvc); nColumn++) {
        rcItem.left = rcItem.right;
        rcItem.right += lvc.cx;

        int nRetLen = GetItemText(nItem, nColumn,
                                    szBuff, _countof(szBuff));
        if (nRetLen == 0)
            continue;

        int OFFSET_OTHER = 5;

        pszText = szBuff;

        csItemTxt = pszText;
        csItemTxt.TrimRight();

        UINT nJustify = DT_LEFT;

        if( _tcscmp( pszText, szBuff) == 0 ) {
            switch(lvc.fmt & LVCFMT_JUSTIFYMASK) {
                case LVCFMT_RIGHT:
                        nJustify = DT_RIGHT;
                        break;
                case LVCFMT_CENTER:
                        nJustify = DT_CENTER;
                        break;
                default:
                        break;
            }
        }

        rcLabel = rcItem;
        rcLabel.left += OFFSET_OTHER;
        rcLabel.right -= OFFSET_OTHER;

        if (m_bFoundLight[nItem][nColumn])
        {
            COLORREF TextColorSave = pDC->SetTextColor(m_MarkedTxtColor);
            COLORREF BkColorSave = pDC->SetBkColor(m_MarkedBkColor);
            CSize TxtSize = pDC->GetTextExtent( csItemTxt );
            CRect TxtRect = rcLabel;
            TxtRect.right = TxtRect.left + TxtSize.cx;
            pDC->FillRect(TxtRect, &CBrush(m_MarkedBkColor));
            pDC->DrawText( csItemTxt, TxtRect, nJustify | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);
            pDC->SetTextColor(TextColorSave);
            pDC->SetBkColor(BkColorSave);
        }
        else
            pDC->DrawText( csItemTxt, -1, rcLabel, // RHF Dec 17, 2002
            // RHF COM Dec 17, 2002 pDC->DrawText(pszText, -1, rcLabel,
                            nJustify | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);
    }

    // draw focus rectangle if item has focus
    if (lvi.state & LVIS_FOCUSED && bFocus && m_bMenuBar)
        pDC->DrawFocusRect(rcAllLabels);

    // set original colors if item was selected
    if (bSelected && m_bMenuBar) {
        pDC->SetTextColor(clrTextSave);
        pDC->SetBkColor(clrBkSave);
    }
    else
        bSelected = false;

    if( m_pFont != NULL )
        pDC->SelectObject( pOldFont );

    if( bMustRelease )
        ReleaseDC( pDC );

    return( bSelected );
}

void CSelectListCtrl::OnPaint()
{
    // in full row select mode, we need to extend the clipping region
    // so we can paint a selection all the way to the right
    if( (GetStyle() & LVS_TYPEMASK) == LVS_REPORT ) {
        CRect   rcAllLabels;

        GetItemRect(0, rcAllLabels, LVIR_BOUNDS);

        if(rcAllLabels.right < m_cxClient) {
            // need to call BeginPaint (in CPaintDC c-tor)
            // to get correct clipping rect
            CPaintDC dc(this);
            CFont*   pOldFont=NULL;

            m_pFont = GetParent()->GetFont();// RHF Dec 11, 2002
            if( m_pFont )
                pOldFont = dc.SelectObject( m_pFont );

            CRect rcClip;
            dc.GetClipBox(rcClip);

            rcClip.left = std::min(rcAllLabels.right-1, rcClip.left);
            rcClip.right = m_cxClient;

            InvalidateRect(rcClip, FALSE);

            if( m_pFont )
                dc.SelectObject( pOldFont );

            // EndPaint will be called in CPaintDC d-tor
        }
    }

    CListCtrl::OnPaint();
}

void CSelectListCtrl::OnSize(UINT nType, int cx, int cy)
{
    CListCtrl::OnSize(nType, cx, cy);
    m_cxClient = cx;
}

BOOL CSelectListCtrl::SetFullRowSel(BOOL bFullRowSel)
{
    // no painting during change
    LockWindowUpdate();

    BOOL m_bFullRowSel = bFullRowSel;

    BOOL bRet;

    if (m_bFullRowSel)
        bRet = ModifyStyle(0L, LVS_OWNERDRAWFIXED);
    else
        bRet = ModifyStyle(LVS_OWNERDRAWFIXED, 0L);

    // repaint window if we are not changing view type
    if (bRet && (GetStyle() & LVS_TYPEMASK) == LVS_REPORT)
        Invalidate();

    // repaint changes
    UnlockWindowUpdate();

    if (bRet)
        return (TRUE);
    else
        return (FALSE);
}

// iElem 0 based
// When we do toggle is possible that the m_iMaxMark be temporary reached. We use
// the m_bIgnoreMax in order to avoid this. Remember that the Invert Button is disable
/// when is not posible to do the invert.
void CSelectListCtrl::SetMarks( CSELLISTCTRL_OPTION iAction, int iElem /*=-1*/ )
{
    csprochar szBuff[MAX_KEYLEN];
    LV_ITEM     lvi;
    int         iLow, iHigh, iMaxElem;
    bool        bSelected;
    std::vector<bool> baSelections;

    // Toggle need all elements
    if( iAction == CSELLISTCTRL_TOGGLE && iElem != -1 )
        return;

    if( iElem >= 0 ) {
        iLow = iHigh = iElem;
        if( iElem >= GetItemCount() )
            return;
    }
    else {
        iLow = 0;
        iHigh = GetItemCount() - 1;
    }

    iMaxElem = iHigh - iLow + 1;

    if( iAction == CSELLISTCTRL_TOGGLE ) {
        baSelections.clear();
        baSelections.resize( iMaxElem );
    }

    m_bIgnoreLimits = true; // In order to avoid temporally more marked than the iMaxMark
    for( int i = iLow; i <= iHigh; i++ ) {
        lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
        lvi.iItem = i;
        lvi.iSubItem = 0;
        lvi.pszText = szBuff;
        lvi.cchTextMax = _countof(szBuff);
        lvi.stateMask = 0xFFFF;         // get all state flags
        GetItem(&lvi);

        if( iAction == CSELLISTCTRL_TOGGLE ) {
            bSelected = (GetStyle() & LVS_SHOWSELALWAYS) && (lvi.state & LVIS_SELECTED);

            baSelections[i] = ( bSelected ? 0 : 1 );
            lvi.state = 0; // Put Off in order to call OnItemChanged and m_iMarked work OK.
        }
        else if( iAction == CSELLISTCTRL_ON ) {
            lvi.state = LVIS_SELECTED;
        }
        else if( iAction == CSELLISTCTRL_OFF ) {
            lvi.state = 0;
        }
        else
            ASSERT(0);  // do nothing!!!!

        //if( iAction != CSELLISTCTRL_TOGGLE )
            SetItem(&lvi);
    }

    if( iAction == CSELLISTCTRL_TOGGLE ) {
        SetListStatus( &baSelections );
        m_bIgnoreEnd = false;
    }

    m_bIgnoreLimits = false;
}

int CSelectListCtrl::FindText(CString csTextToFind, BOOL /*bMark*/)
{
    int i, j, iOccursFound = 0, iFirstItemFound=-1;
    CString csItemText, csItemFoundText;
    bool bFound = false;

    csTextToFind.TrimRight();
    if (csTextToFind == _T(""))
        ClearSrchMarks();
    else {
        for (i = 0; i<GetItemCount(); i++) {
            for (j = 0; j<GetColCount(); j++) {
                csItemText = GetItemText(i, j);
                csItemText.TrimRight(); // RHF Dec 17, 2002
                if (!m_bCaseSensitiveSearch) {
                    csItemText.MakeLower();
                    csTextToFind.MakeLower();
                }

                if (csItemText.Find(LPCTSTR(csTextToFind)) >= 0) {
                    m_bFoundLight[i][j] = true;
                    iOccursFound++;
                    bFound = true;
                }
                else
                    m_bFoundLight[i][j] = false;
                csItemText.Empty();
            }
            if (bFound) {
                DrawFoundItem(i);
                if( iFirstItemFound == -1 )
                    iFirstItemFound = i;
                bFound = false;
            }
        }
    }

    if( iOccursFound == 0 )
        AfxMessageBox(_T("Search phrase '") + csTextToFind + _T("' was not found. Please try again."));


    if( iFirstItemFound >= 0 ) {
        SetItemState( iFirstItemFound, LVIS_FOCUSED, LVIS_FOCUSED );
        EnsureVisible( iFirstItemFound, TRUE );
    }

    return iOccursFound;
}


int CSelectListCtrl::GetColCount()
{
    LV_COLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_WIDTH;
    int nColumn = 0;
    do {
        nColumn++;
    }
    while (GetColumn(nColumn, &lvc));

    return nColumn;
}

void CSelectListCtrl::ClearSrchMarks()
{
    int     i, j;

    for (i = 0; i < GetItemCount(); i++)
        for (j = 0; j < GetColCount(); j++)
            if (m_bFoundLight[i][j]) {
                m_bFoundLight[i][j] = false;
                DrawFoundItem(i);
            }

}

void CSelectListCtrl::SetMultiSelect(bool bOnOff)
{
    if( bOnOff)
        ModifyStyle( LVS_SINGLESEL, 0);
    else
        ModifyStyle( 0, LVS_SINGLESEL);
}

void CSelectListCtrl::SetTextBkColor(COLORREF color)
{
    m_TextBkColor = color;
    UpdateWindow();
}

void CSelectListCtrl::SetMenuBar(BOOL OnOff)
{
    if (OnOff)
        m_bMenuBar = true;
    else
        m_bMenuBar = false;
    UpdateWindow();
}

void CSelectListCtrl::SetTextColor(COLORREF color)
{
    m_TextColor = color;
    UpdateWindow();
}


// Enable/disable the sticky control-key.
void CSelectListCtrl::EnableStickyControlKey( bool bOnOff ) {
    BYTE     KeyState[256];

    if( m_pOptions != NULL && !m_pOptions->m_bHasStickyCtrl ) // RHF Jan 21, 2000
        return;

    if( !bOnOff && !m_bHasPendingCtrl )
        return;

    // Simulate control key always down when a up,down or ctrl key is pressed
    if( GetKeyboardState( KeyState ) ) {
        KeyState[VK_CONTROL]  = bOnOff ? 0xFF : 0; // Turn on/off Control Key
        m_bHasPendingCtrl = bOnOff;
        SetKeyboardState( KeyState );
    }

}

UINT CSelectListCtrl::OnGetDlgCode() {
    return DLGC_WANTALLKEYS | DLGC_WANTARROWS | DLGC_WANTCHARS;
}


void CSelectListCtrl::CalculateColumnWidths(std::vector<int>* piaColumnWidths)
{
    CFont* pOldFont = NULL;
    CDC* pDC = GetDC();

    m_pFont = GetParent()->GetFont();

    if( m_pFont != NULL )
        pOldFont = pDC->SelectObject(m_pFont);

    for( int iCol = 0; iCol < m_iNCols; iCol++ )
    {
        int iMaxColumnWidth = 0;

        for( int iRow = ( m_pOptions->m_paColumnTitles ? -1 : 0 ); iRow < m_iNRows; iRow++ )
        {
            CString csText = ( iRow < 0 ) ? m_pOptions->m_paColumnTitles->at(iCol) : m_pOptions->m_paData->at(iRow)->at(iCol);

            iMaxColumnWidth = std::max(iMaxColumnWidth,(int)pDC->GetTextExtent(csText).cx);
        }

        piaColumnWidths->push_back(iMaxColumnWidth);
    }

    if( m_pFont != NULL )
        pDC->SelectObject(pOldFont);

    ReleaseDC(pDC);
}


void CSelectListCtrl::LoadItems()
{
    m_iNRows = m_pOptions->m_paData->size();
    m_iNCols = m_pOptions->m_paData->at(0)->size();

    ASSERT(m_iNCols > 0 && m_iNCols <= MAX_COLUMNS);
    ASSERT(m_iNRows > 0 && m_iNRows <= MAX_ITEMS);

    // get the column widths in pixels
    std::vector<int> iaColumnWidths;
    CalculateColumnWidths(&iaColumnWidths);

    // insert the columns
    for( int iCol = 0; iCol < m_iNCols; iCol++ )
    {
        LV_COLUMN lvc;
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvc.fmt = LVCFMT_LEFT;

        if( m_pOptions->m_bUseColTitle )
            lvc.pszText = const_cast<TCHAR*>(m_pOptions->m_paColumnTitles->at(iCol).GetString());

        else
            lvc.pszText = NULL;

        lvc.cx = iaColumnWidths.at(iCol) + 2;
        lvc.iSubItem = iCol;

        InsertColumn(iCol,&lvc);
    }

    // insert the items
    for( int iRow = 0; iRow < m_iNRows; iRow++ )
    {
        std::vector<CString>* paRowData = m_pOptions->m_paData->at(iRow);

        for( int iCol = 0; iCol < m_iNCols; iCol++ )
        {
            if( iCol == 0 )
            {
                LV_ITEM lvi;
                lvi.mask = LVIF_TEXT;
                lvi.iItem = iRow;
                lvi.iSubItem = 0;

                lvi.pszText = paRowData->at(iCol).GetBuffer();

                InsertItem(&lvi);
            }

            else
                SetItemText(iRow,iCol,paRowData->at(iCol));
        }
    }
}


// Return number of marked elements
int CSelectListCtrl::GetListStatus( std::vector<bool>* pbaSelections /* = NULL*/, CString* pcsMarkedOptions /*=NULL*/ )
{
    csprochar  szBuff[MAX_KEYLEN];
    LV_ITEM lvi;
    int     iNumRow, iNumMarked=0;
    bool    bSelected;


    iNumRow = GetItemCount();
    if( pbaSelections != NULL ) {
        pbaSelections->clear();
        if( iNumRow > 0 )
            pbaSelections->resize( iNumRow );
    }


    if( pcsMarkedOptions != NULL ) {
        lvi.mask = LVIF_TEXT | LVIF_STATE;
        lvi.pszText = szBuff;
        lvi.cchTextMax = _countof(szBuff);
    }
    else
        lvi.mask = LVIF_STATE;
    lvi.iSubItem = 0;

    lvi.stateMask = 0xFFFF;         // get all state flags

    for( int i = 0; i < iNumRow; i++ ) {
        lvi.iItem = i;
        GetItem(&lvi);

        bSelected = (GetStyle() & LVS_SHOWSELALWAYS) && (lvi.state & LVIS_SELECTED);
        // bSelected = bSelected || (lvi.state & LVIS_DROPHILITED);

        if( bSelected ) {
            iNumMarked++;
            if( pcsMarkedOptions != NULL )
                *pcsMarkedOptions += szBuff;
        }

        if( pbaSelections != NULL )
            (*pbaSelections)[i] = ( bSelected ? 1 : 0);
    }

    return( iNumMarked );
}

void CSelectListCtrl::SetListStatus(const std::vector<bool>* pbaSelections)
{
    csprochar szBuff[MAX_KEYLEN];
    int         iMaxElem, iMaxRow, iLastItemSelected=-1;
    bool        bSelected;

    m_bIgnoreEnd = true;
    iMaxRow = GetItemCount();
    m_baSelections.clear();
    if( iMaxRow > 0 )
        m_baSelections.resize(iMaxRow);

    iMaxElem = ( pbaSelections != NULL ) ? pbaSelections->size() : 0;

    for( int i = 0; i < iMaxRow; i++) {
        LV_ITEM lvi;
        lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
        lvi.iItem = i;
        lvi.iSubItem = 0;

        lvi.pszText = szBuff;
        lvi.cchTextMax = _countof(szBuff);
        lvi.stateMask = 0xFFFF;         // get all state flags
        GetItem(&lvi);

        if( pbaSelections != NULL && i < iMaxElem )
            bSelected = pbaSelections->at(i);
        else
            bSelected = false;

        m_baSelections[i] = ( bSelected ? 1 : 0 );

        lvi.state = bSelected ? LVIS_SELECTED : 0;

        if( bSelected ) {
            iLastItemSelected = i;
        }
        SetItem(&lvi);
    }

    // Focus the first item when there is nothing selected
    bool    bHighLight=false;
    if( iLastItemSelected < 0 ) {
        iLastItemSelected = 0;

        if( m_pOptions == NULL || m_pOptions->m_bHighLightFirst )
            bHighLight = true;
    }

    if( iLastItemSelected >= 0 ) {
        if( bHighLight )
            SetItemState( iLastItemSelected, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
        else
            SetItemState( iLastItemSelected, LVIS_FOCUSED, LVIS_FOCUSED );
        EnsureVisible( iLastItemSelected, TRUE );
    }

    // If m_bIgnoreEnd is setting to false here, some events
    //  ItemChanging are shooting and the SendMessage(IDOK) is shooting twice!!!!
    //m_bIgnoreEnd = false;
}

#define MAX_UNSCROLLROWS   20
// RHF INIC Feb 14, 2003
void  CSelectListCtrl::AdjustColWidth() {
    if( GetColCount() == 1 ) {
        int     iColWidth=GetColumnWidth(0);
        CRect   OrgCtrlRect;
        int     iFrame = GetSystemMetrics(SM_CYFRAME);
        int     iVertScrollBarWidth = GetSystemMetrics(SM_CXVSCROLL);

        GetWindowRect(&OrgCtrlRect);
        int     iFinalWidth=OrgCtrlRect.Width();

        int     iMinColWidth = iFinalWidth - 2*iFrame;
        if( GetItemCount() >= MAX_UNSCROLLROWS ) {
            iMinColWidth -= iVertScrollBarWidth;
        }

        if( iColWidth < iMinColWidth )
            SetColumnWidth( 0, iMinColWidth );
    }
}
// RHF END Feb 14, 2003

BOOL CSelectListCtrl::DoAutoWidth( CDC* pDC, int iMinWidth, int iMaxWidth, int iExtraHeightAvailable ) {
    CWnd        *pParentWnd;
    CWnd        *pGrandParentWnd;
    CString     csColText;

    pParentWnd = GetParent();
    pGrandParentWnd = ( m_pOptions->m_bUseParentWindowLimitsForSizing ) ? GetParent()->GetParent() : GetDesktopWindow();

    if (pParentWnd == NULL || pGrandParentWnd == NULL)
        return FALSE;

    HWND ParentWndHandle, GrandParentWndHandle;

    ParentWndHandle = pParentWnd->GetSafeHwnd();
    GrandParentWndHandle = pGrandParentWnd->GetSafeHwnd();

    if (GrandParentWndHandle == NULL || ParentWndHandle == NULL)
        return FALSE;


    LV_COLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    csprochar szBuff[MAX_KEYLEN];

    lvc.pszText = szBuff;
    lvc.cchTextMax = _countof(szBuff);

    int iFrame = GetSystemMetrics(SM_CYFRAME);

    // RHF INIC Dec 12, 2002
    CString csItemText;

    int     iHeigth=0;
    int     iSumWidth=0;

    for( int j = 0; j < GetColCount(); j++ ) {
        int     iColWidth=10;
        int     iColHeigth = 0;

        for( int i = -1; i < GetItemCount(); i++ ) {
            csItemText.Empty();

            if( i == -1 ) {
                if( !m_pOptions->m_bUseColTitle )// RHF Feb 13, 2003
                    continue; // RHF Feb 13, 2003
                if( GetColumn( j, &lvc) ) {
                    csItemText = lvc.pszText;
                }
            }
            else
                csItemText = GetItemText(i, j);

            csItemText.TrimRight();

            int     iNewColWidth = pDC->GetTextExtent(csItemText).cx; // + 20 ;

            // RHF COM Feb 13, 2003 iNewColWidth += 12;

            // RHF INIC Feb 13, 2003
            if( GetColCount() >= 2 )
                iNewColWidth += 12;
            else
                iNewColWidth += 8;
            // RHF END Feb 13, 2003

            // RHF COM Feb 13, 2003 iColHeigth += pDC->GetTextExtent(csItemText).cy + 2;
            iColHeigth += pDC->GetTextExtent(csItemText).cy + 1;// RHF Feb 13, 2003
            iColWidth =std::max( iColWidth, iNewColWidth );
        }

        iHeigth =std::max( iHeigth, iColHeigth );

        // RHF INIC Feb 13, 2003
        iSumWidth += iColWidth;

        // RHF END Feb 13, 2003

        SetColumnWidth( j, iColWidth );
    }
    // RHF END Dec 12, 2002

    int iVMin = 0;
    int iVMax = 0;


    GetScrollRange( SB_VERT, &iVMin, &iVMax);
    int     iVertScrollBarWidth = (iVMax > 0)?GetSystemMetrics(SM_CXVSCROLL):0;

    // RHF INIC Feb 13, 2003
    int     iTotalWidth = iSumWidth + 2*iFrame; // RHF Feb 13, 2003
    if( GetItemCount() >= MAX_UNSCROLLROWS )
        iTotalWidth += iVertScrollBarWidth;

    iHeigth += 2*iFrame;
    // RHF END Feb 13, 2003

    if (iTotalWidth == 0)
        return FALSE;

    iTotalWidth = std::min( iTotalWidth, iMaxWidth );
    iTotalWidth =std::max( iTotalWidth, iMinWidth );

    //Get Original dimentions of conrol
    CRect OrgCtrlRect;
    CRect ParentRect;
    CRect GrandParentRect;


    GetWindowRect(&OrgCtrlRect);
    pParentWnd->GetWindowRect(&ParentRect);
    pGrandParentWnd->GetWindowRect(&GrandParentRect);

    int xGrowth = iTotalWidth - OrgCtrlRect.Width();
    int yGrowth = iHeigth -  OrgCtrlRect.Height();

    if( xGrowth <= 0 )
        xGrowth = 0;

    if( yGrowth <= 0 )
        yGrowth = 0;

    yGrowth -= iExtraHeightAvailable;

    if( GrandParentRect.Width() < ParentRect.Width() + xGrowth ) {
        //90% of grand parent window size
        double dGrowth;
// RHF COM Dec 12, 2002        dGrowth = GrandParentRect.Width() * 0.90 - OrgCtrlRect.Width() - (ParentRect.Width() - OrgCtrlRect.Width());

        dGrowth = ( GrandParentRect.Width() - ParentRect.Width() );
        dGrowth *= 0.8; // RHF Dec 17, 2002
        xGrowth = (int) dGrowth;
    }

    if( GrandParentRect.Height() < ParentRect.Height() + yGrowth ) {
        //90% of grand parent window size
        double dGrowth;
// RHF COM Dec 12, 2002        dGrowth = GrandParentRect.Height() * 0.90 - OrgCtrlRect.Height() - (ParentRect.Height() - OrgCtrlRect.Height());

        dGrowth = ( GrandParentRect.Height() - ParentRect.Height() );

        dGrowth *= 0.8; // RHF Dec 17, 2002

        yGrowth = (int) dGrowth;
    }

    SetWindowPos( &wndTop, 0, 0, OrgCtrlRect.Width() + xGrowth, OrgCtrlRect.Height() + yGrowth, SWP_NOMOVE );
    pParentWnd->SetWindowPos( &wndTop, 0, 0, ParentRect.Width() + xGrowth, ParentRect.Height() + yGrowth, SWP_NOMOVE );

    // RHF INIC Feb 14, 2003
    // Fix column width
    AdjustColWidth();
    // RHF END Feb 14, 2003

    return TRUE;
}

void CSelectListCtrl::SetOptions( CSelectListCtrlOptions* pOptions ) {
    m_pOptions = pOptions;
}

void CSelectListCtrl::OnItemchanging(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    *pResult = 0;
}


void CSelectListCtrl::OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    bool    bOldSelected, bNewSelected, bSendMsg=false;

    int iStyle = GetStyle() & LVS_SHOWSELALWAYS;

    bOldSelected = iStyle && (pNMListView->uOldState & LVIS_SELECTED);

    bNewSelected = iStyle && (pNMListView->uNewState & LVIS_SELECTED);

    if( bOldSelected != bNewSelected ) {
        if( bNewSelected )
            m_iMarked++;
        else
            m_iMarked--;

        if( !m_bIgnoreLimits && m_pOptions->m_iMaxMark >= 0 ) {
            // Not check, When we are marking for first time.
            if( !m_bIgnoreEnd && m_pOptions->m_bEndOnLimits && m_iMarked >= m_pOptions->m_iMaxMark ) {
                bSendMsg = true;
            }

            if( m_iMarked > m_pOptions->m_iMaxMark ) {
                if( m_pOptions->m_iMaxMark == m_iMarked - 1 ) {
                    ::MessageBeep(MB_ICONASTERISK );
                    //CString csMsg;
                    //csMsg.Format( "Too many marked: max is %d (actual is %d)", m_pOptions->m_iMaxMark, m_iMarked );
                    //AfxMessageBox( csMsg );
                }

                SetMarks( CSELLISTCTRL_OFF, pNMListView->iItem );
                bNewSelected = !bNewSelected;
            }
        }

        m_baSelections[pNMListView->iItem] = ( bNewSelected ? 1 : 0 );
    }

    int     iSelected=GetSelectedCount();
    if( iSelected != m_iMarked ) {
        CString csMsg;
        csMsg.Format( _T("Invalid selection: %d != %d"), iSelected, m_iMarked );
        AfxMessageBox( csMsg );
    }
    *pResult = 0;

    // Special behaviour: when max of selection is 1, not finish when press up/down
    if( m_pOptions->m_iMaxMark == 1 && m_bIsMoving ) {
        bSendMsg = false;
    }

    if( bSendMsg )
        PostMessage(UWM::Capi::FinishSelectDialog, IDOK, 0);// Simulate IDOK
}


void CSelectListCtrl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) {
    if(nChar == VK_RETURN) {
        ((CSelectDlg*)GetParent())->OnOK();
    }
    else {
        m_bIgnoreEnd = false;

        if( m_pOptions->m_iKeyBuffMaxLen > 0 && nChar > _T(' ') && nChar <= 255 ) {
            CString     csText, csKeyBuffNew;
            int         i, iMaxElem, iLen;
            int         iFirstElemFound=-1;
            int         iNumElemFound=0;

            // concat new csprochar to current buffer
            csKeyBuffNew.Format( _T("%s%lc"), (LPCTSTR)m_csKeyBuff, nChar );

            iLen = m_csKeyBuff.GetLength();
            iMaxElem = m_pOptions->m_paData->size();

            // Search the prefix in list elements
            for( i = 0; iNumElemFound <= 1 && i < iMaxElem; i++ ) {
                csText = m_pOptions->m_paData->at(i)->at(0);

                if( csKeyBuffNew.CompareNoCase( csText.Left( iLen + 1 ) ) == 0 ) {
                    if( iFirstElemFound == -1 )
                        iFirstElemFound = i;
                    iNumElemFound++;
                }
            }

            if( iFirstElemFound >= 0 ) {
                m_csKeyBuff = csKeyBuffNew;

                ASSERT( m_pOptions->m_iKeyBuffMaxLen == 1 ); // RHF Apr 01, 2003

                bool    bOldMoving=m_bIsMoving; // RHF Apr 01, 2003
                if( iNumElemFound > 1 ) m_bIsMoving = true; // RHF Apr 01, 2003

                TRACE( _T("Keyboard Buff=(%s)\n"), (LPCTSTR)m_csKeyBuff );
                // RHF COM Apr 01, 2003 if( iNumElemFound == 1 || iLen + 1 >= m_pOptions->m_iKeyBuffMaxLen ) { // Reset buff
                if( true ) { // RHF Apr 01, 2003
                    SetItemState( iFirstElemFound, 0, LVIS_SELECTED ); // // RHF Apr 01, 2003 UnMark
                    SetItemState( iFirstElemFound, LVIS_SELECTED, LVIS_SELECTED ); // Mark
                    // RHF COM Apr 01, 2003 Only the first csprochar for accept! ResetKeyBuff();
                }

                m_csKeyBuff = _T(""); // RHF Apr 01, 2003
                if( iNumElemFound > 1 ) m_bIsMoving = bOldMoving; // RHF Apr 01, 2003

                SetItemState( iFirstElemFound, LVIS_FOCUSED, LVIS_FOCUSED ); // Move
                EnsureVisible( iFirstElemFound, TRUE );
            }
            return;

        }

        bool    bSpace=(nChar==' ' );
        if( m_pOptions->m_iKeyBuffMaxLen > 0 || bSpace&&m_pOptions->m_iMaxMark>=1 ) // RHF Jan 09, 2003
            CListCtrl::OnChar(nChar, nRepCnt, nFlags);
    }
}

void CSelectListCtrl::ResetKeyBuff() {
    m_csKeyBuff = _T("");
    SetMarks( CSELLISTCTRL_OFF, -1 );// RHF Jan 10, 2003
}

void CSelectListCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if(nChar == VK_RETURN) {
        return;
    }

    m_bIgnoreEnd = false;

    BYTE    KeyState[CSELLISTCTRLOPTIONS_KEYBOARDLEN];
    bool    bShift=false, bDown=false, bUp=false, bCtl=false, bSpace=false, bSendMsg=false, bReset;

    // Reset keyboard buff.
    bReset = ( nChar == VK_DELETE );
    if( bReset ) {
        ResetKeyBuff();
    }

    memset( KeyState, 0, CSELLISTCTRLOPTIONS_KEYBOARDLEN );
    if( GetKeyboardState( KeyState ) ) {
        bShift = ( KeyState[ VK_SHIFT   ] & 0x80 ) != 0;
        bDown  = ( KeyState[ VK_DOWN    ] & 0x80 ) != 0;
        bUp    = ( KeyState[ VK_UP      ] & 0x80 ) != 0;
        bCtl   = ( KeyState[ VK_CONTROL ] & 0x80 ) != 0;
        bSpace = ( KeyState[ VK_SPACE   ] & 0x80 ) != 0;

        // RHF INIC Dec 18, 2002
        bool    bPageUp =   ( KeyState[ VK_PRIOR   ] & 0x80 ) != 0;
        bool    bPageDown = ( KeyState[ VK_NEXT   ] & 0x80 ) != 0;
        bool    bHome =     ( KeyState[ VK_HOME   ] & 0x80 ) != 0;
        bool    bEnd =      ( KeyState[ VK_END   ] & 0x80 ) != 0;
        bool    bLeft =     ( KeyState[ VK_LEFT   ] & 0x80 ) != 0;
        bool    bRight =    ( KeyState[ VK_RIGHT   ] & 0x80 ) != 0;


        bUp = bUp || bPageUp || bHome;
        bDown = bDown || bPageDown || bEnd;
        // RHF END Dec 18, 2002

        m_bIsMoving = ( bUp || bDown || bLeft || bRight );
    }


    // these two keys were previously handled by the now-deleted class CKeyMgr
    int iRemappedKey = -1;

    if( KeyState[VK_RETURN] & 0x80 )
    {
        iRemappedKey = IDOK;
        bSendMsg = true;
    }

    else if( KeyState[VK_ESCAPE] & 0x80 )
    {
        iRemappedKey = IDCANCEL;
        bSendMsg = true;
    }


    if( m_pOptions->m_bHasStickyCtrl ) {
        if( ( bUp || bDown || bSpace ) && !bCtl && !bShift )
            EnableStickyControlKey( TRUE );
    }

    if( m_bIsMoving || bSpace || bReset ) // RHF Jan 09, 2003
        CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);

    if( bSpace && m_pOptions->m_iMaxMark>=1 )
        return; // RHF Jan 08, 2003

    if( bSendMsg )
        PostMessage(UWM::Capi::FinishSelectDialog, iRemappedKey, 0);

    else {
        CWnd* pMainWnd = AfxGetApp()->GetMainWnd();
        if( pMainWnd && IsWindow(pMainWnd->GetSafeHwnd()) ) {
            pMainWnd->PostMessage( WM_KEYDOWN, nChar, nRepCnt );
        }
    }
}


LRESULT CSelectListCtrl::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    return CListCtrl::WindowProc(message, wParam, lParam);
}

void CSelectListCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
    m_bIsMoving = false;
    m_bIgnoreEnd = false;

    CListCtrl::OnRButtonDown(nFlags, point);
}

void CSelectListCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
    m_bIsMoving = false;
    m_bIgnoreEnd = false;

    CListCtrl::OnLButtonDown(nFlags, point);
}

void CSelectListCtrl::OnRButtonDblClk(UINT nFlags, CPoint point)
{
    m_bIsMoving = false;
    m_bIgnoreEnd = false;

    CListCtrl::OnRButtonDblClk(nFlags, point);
}

void CSelectListCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    m_bIsMoving = false;
    m_bIgnoreEnd = false;

    CListCtrl::OnLButtonDblClk(nFlags, point);
}

void CSelectListCtrl::OnClick(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_LISTVIEW*    pNMListView = (NM_LISTVIEW*)pNMHDR;
    bool            bOldSelected, bSendMsg=false;

    int iStyle = GetStyle() & LVS_SHOWSELALWAYS;

    m_bIsMoving = false;
    m_bIgnoreEnd = false;

    bOldSelected = iStyle && (pNMListView->uOldState & LVIS_SELECTED);

    bool    bEndWhenClick=(m_pOptions->m_iMaxMark==1 || m_pOptions->m_bEndOnLimits);// RHF Jan 10, 2003
    bEndWhenClick=m_pOptions->m_bEndOnLimits; // RHF Sep 28, 2007. See CCapi::DoLabelsModeless

    // The cliked element is selected and the limits are reached
    if( !m_bIgnoreEnd && bOldSelected && pNMListView->uChanged==0 &&
        bEndWhenClick && m_iMarked >= m_pOptions->m_iMaxMark )
           bSendMsg = true;

    if( bSendMsg ) {
        PostMessage(UWM::Capi::FinishSelectDialog, IDOK, 0);
    }

    *pResult = 0;
}

LONG CSelectListCtrl::OnFinishedDialog(UINT wParam, LONG lParam)
{
    GetParent()->PostMessage(UWM::Capi::FinishSelectDialog, wParam, lParam );
    return 0;
}

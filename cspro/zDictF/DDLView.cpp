//***************************************************************************
//  File name: DDLView.cpp
//
//  Description:
//       Layout View application
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#include "StdAfx.h"
#include "DDLView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CDDLView, CScrollView)
IMPLEMENT_DYNAMIC(CItemPosInfo, CObject)
IMPLEMENT_DYNAMIC(CLayItemArray, CObject)

/////////////////////////////////////////////////////////////////////////////
//
//                       CItemPosInfo::CItemPosInfo
//
/////////////////////////////////////////////////////////////////////////////

CItemPosInfo::CItemPosInfo (void) {

    m_iLevel = 0;
    m_iRec = 0;
    m_iItem = 0;
    m_iOccs = 0;
    m_iStart = 0;
    m_iEnd = 0;
    m_iLine = 0;
    m_iIdLevel = NONE;
    m_iIdRec = NONE;
    m_type = LAYOUT_RECTYPE;
}

CItemPosInfo::CItemPosInfo (int iLevel, int iRec, int iItem, int iOccs, int iStart,
                            int iEnd, int iLine, int iIdLevel, int iIdRec, LayoutType type) {

    m_iLevel = iLevel;
    m_iRec = iRec;
    m_iItem = iItem;
    m_iOccs = iOccs;
    m_iStart = iStart;
    m_iEnd = iEnd;
    m_iLine = iLine;
    m_iIdLevel = iIdLevel;
    m_iIdRec = iIdRec;
    m_type = type;
    ASSERT(m_iRec == COMMON || (m_iIdLevel == NONE && m_iIdRec == NONE));
}

CItemPosInfo::CItemPosInfo (CItemPosInfo& ipi)  {

    m_iLevel = ipi.m_iLevel;
    m_iRec = ipi.m_iRec;
    m_iItem = ipi.m_iItem;
    m_iOccs = ipi.m_iOccs;
    m_iStart = ipi.m_iStart;
    m_iEnd = ipi.m_iEnd;
    m_iLine = ipi.m_iLine;
    m_iIdLevel = ipi.m_iIdLevel;
    m_iIdRec = ipi.m_iIdRec;
    m_type = ipi.m_type;
    ASSERT(m_iRec == COMMON || (m_iIdLevel == NONE && m_iIdRec == NONE));
}


/////////////////////////////////////////////////////////////////////////////
//
//                       CItemPosInfo::operator=
//
/////////////////////////////////////////////////////////////////////////////

void CItemPosInfo::operator= (CItemPosInfo& ipi) {

    m_iLevel = ipi.m_iLevel;
    m_iRec = ipi.m_iRec;
    m_iItem = ipi.m_iItem;
    m_iOccs = ipi.m_iOccs;
    m_iStart = ipi.m_iStart;
    m_iEnd = ipi.m_iEnd;
    m_iLine = ipi.m_iLine;
    m_iIdLevel = ipi.m_iIdLevel;
    m_iIdRec = ipi.m_iIdRec;
    m_type = ipi.m_type;
    ASSERT(m_iRec == COMMON || (m_iIdLevel == NONE && m_iIdRec == NONE));
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CLayItemArray
//
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
//
//                            CDDLView
//
/////////////////////////////////////////////////////////////////////////////

CDDLView::CDDLView() {

    m_ptContentsOrigin.x = NONE;
    m_ptContentsOrigin.y = NONE;
    SetScrollSizes(MM_TEXT, CSize(100,100));
    m_bBuilt = false;
    m_bInitialized = false;
    m_pFont = new CFont();
    m_pFontBold = new CFont();
}

CDDLView::~CDDLView() {

    ASSERT_VALID(m_pFont);
    delete m_pFont;
    delete m_pFontBold;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDLView::OnInitialUpdate
//
/////////////////////////////////////////////////////////////////////////////

void CDDLView::OnInitialUpdate() {

    Init();
    CScrollView::OnInitialUpdate();
    // we can't do stuff from Init() here; have to separate these out, since OnInitialUpdate() doesn't get called after the 1st instance
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDLView::Init
//
/////////////////////////////////////////////////////////////////////////////
//
// Stuff that would regularly be in OnInitialUpdate, but that was
// separated out since that function doesn't get called when creating
// splitter windows (except for the 1st time).
//CListBox
/////////////////////////////////////////////////////////////////////////////

void CDDLView::Init(void)  {

    ASSERT(!m_bInitialized);

    m_pFont->CreateFont (-14, 0, 0, 0, 400, FALSE, FALSE, 0, DEFAULT_CHARSET/*ANSI_CHARSET*/,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
        _T("Courier New"));
    m_pFontBold->CreateFont (-14, 0, 0, 0, 700, FALSE, FALSE, 0, DEFAULT_CHARSET/*ANSI_CHARSET*/,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
        _T("Courier New"));

    CClientDC dc(this);
    CFont* pOldFont = dc.SelectObject(m_pFont);
    m_iTextWidth = dc.GetTextExtent (_T("A"),1).cx;
    m_iTextHgt = dc.GetTextExtent (_T("A"),1).cy + 2;

    dc.SelectObject(pOldFont);

    EnableToolTips();
    m_bInitialized = true;
}

BEGIN_MESSAGE_MAP(CDDLView, CScrollView)
    //{{AFX_MSG_MAP(CDDLView)
    ON_WM_CONTEXTMENU()
    ON_WM_LBUTTONUP()
    ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
    ON_WM_ERASEBKGND()
    //}}AFX_MSG_MAP
    ON_MESSAGE(UWM::Dictionary::Find, OnFind)        // Find text
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDLView::OnUpdate
//
/////////////////////////////////////////////////////////////////////////////

void CDDLView::OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/) {

    Build();
    Invalidate();
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDLView::OnEraseBkgnd
//
/////////////////////////////////////////////////////////////////////////////

BOOL CDDLView::OnEraseBkgnd(CDC* pDC) {

    CBrush backBrush(LAYCOLOR_BACKGROUND);
    CBrush* pOldBrush = pDC->SelectObject(&backBrush);
    CRect rect;
    pDC->GetClipBox(&rect);
    pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(), PATCOPY);
    pDC->SelectObject(pOldBrush);
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDLView::OnDraw
//
/////////////////////////////////////////////////////////////////////////////

void CDDLView::OnDraw(CDC* pDC) {

    if (!m_bInitialized)  {
        Init();
    }
//    if (!m_bBuilt)  {
        Build();
        // m_bBuilt might still be FALSE here if we don't have a
        // fully loaded dictionary yet!
//    }

    if (m_bBuilt)  {
        COLORREF prevColor = pDC->SetBkColor(LAYCOLOR_BACKGROUND);
        CFont* pOldFont = pDC->SelectObject(m_pFont);
        LayoutRuler(pDC);
        LayoutNames(pDC);
        LayoutContents(pDC);
        pDC->SelectObject(pOldFont);
        pDC->SetBkColor(prevColor);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDLView::LayoutRuler
//
/////////////////////////////////////////////////////////////////////////////

void CDDLView::LayoutRuler (CDC* pDC) {

    COLORREF prevColor = pDC->SetBkColor(LAYCOLOR_RULER);
    pDC->ExtTextOut(m_ptContentsOrigin.x, LAY_BORDER, 0, NULL, m_csRuler, m_csRuler.GetLength(), NULL);
    pDC->SetBkColor(prevColor);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDLView::LayoutNames
//
/////////////////////////////////////////////////////////////////////////////

void CDDLView::LayoutNames(CDC* pDC)
{
    const CDataDict* pDict = GetDocument()->GetDict();
    CRect rcClip = CRect(0, 0, m_ptContentsOrigin.x, 10000);
    for( size_t level_number = 0; level_number < pDict->GetNumLevels(); ++level_number ) {
        // Output Level Label
        const DictLevel& dict_level = pDict->GetLevel(level_number);
        CString csLabel = dict_level.GetLabel();
        int iLine = GetRTLine(level_number, NONE);
        int iYPos = iLine * m_iTextHgt + m_ptContentsOrigin.y;
        CFont* pOldFont = pDC->SelectObject(m_pFontBold);
        pDC->ExtTextOut (LAY_BORDER + 4 * m_iTextWidth, iYPos, 0, NULL, (CString) csLabel, NULL);
        pDC->SelectObject(pOldFont);
        for (int r = 0 ; r < dict_level.GetNumRecords() ; r++) {
            // Output Record Label
            const CDictRecord* pRec = dict_level.GetRecord(r);
            csLabel = pRec->GetLabel();
            iLine = GetRTLine(level_number, r);
            iYPos = iLine * m_iTextHgt + m_ptContentsOrigin.y;
            pDC->ExtTextOut (LAY_BORDER, iYPos, ETO_CLIPPED, rcClip, csLabel, csLabel.GetLength(), NULL);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDLView::LayoutContents
//
/////////////////////////////////////////////////////////////////////////////

void CDDLView::LayoutContents (CDC* pDC) {

    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetDocument());
    // Prepare GDI objects
    CBrush brushRT(LAYCOLOR_RECTYPE);
    CBrush brushIDITEMBEF(LAYCOLOR_IDITEMBEF);
    CBrush brushIDITEM(LAYCOLOR_IDITEM);
    CBrush brushIDITEMAFT(LAYCOLOR_IDITEMAFT);
    CBrush brushITEM(LAYCOLOR_ITEM);
    CBrush brushSUBITEM(LAYCOLOR_SUBITEM);
    CBrush* pOldBrush = pDC->SelectObject(&brushITEM);
    pDC->SelectStockObject(BLACK_PEN);
    pDC->SetBkMode(TRANSPARENT);

    // For each item in the list
    for (int i = 0 ; i < m_aItemLay.GetSize() ; i++)  {
        CItemPosInfo* pIPI = m_aItemLay.GetItemPos(i);
        int iXPos1 = (pIPI->GetStart() - 1) * m_iTextWidth;
        int iYPos1 = pIPI->GetLine() * m_iTextHgt;
        int iXPos2 = (pIPI->GetEnd() - 1) * m_iTextWidth;
        int iYPos2 = iYPos1 + m_iTextHgt - 3;
        CRect rcIPI = CRect(iXPos1, iYPos1, iXPos2, iYPos2);
        rcIPI.OffsetRect(m_ptContentsOrigin);
        switch(pIPI->GetType()) {
            case LAYOUT_RECTYPE:
                pDC->SelectObject(&brushRT);
                break;
            case LAYOUT_IDITEMBEF:
                pDC->SelectObject(&brushIDITEMBEF);
                break;
            case LAYOUT_IDITEM:
                pDC->SelectObject(&brushIDITEM);
                break;
            case LAYOUT_IDITEMAFT:
                pDC->SelectObject(&brushIDITEMAFT);
                break;
            case LAYOUT_IDSUBITEM:
                pDC->SelectObject(&brushSUBITEM);
                break;
            case LAYOUT_RTITEM:
                pDC->SelectObject(&brushITEM);
                break;
            case LAYOUT_RTSUBITEM:
                pDC->SelectObject(&brushSUBITEM);
                break;
        }

        // Outline the item
        if (pIPI->GetLevel() == pDoc->GetLevel() &&
                pIPI->GetRec() == pDoc->GetRec() &&
                pIPI->GetItem() == pDoc->GetItem()) {
            pDC->SelectStockObject(WHITE_PEN);
            pDC->Rectangle(rcIPI);
            pDC->SelectStockObject(BLACK_PEN);
        }
        else {
            pDC->Rectangle(rcIPI);
        }

        // Draw tick marks
        for (int iTick = rcIPI.left + m_iTextWidth ; iTick <= rcIPI.right - m_iTextWidth ; iTick += m_iTextWidth)  {
            pDC->MoveTo(iTick, rcIPI.top);
            pDC->LineTo(iTick, rcIPI.top + 4);
            pDC->MoveTo(iTick, rcIPI.bottom - 1);
            pDC->LineTo(iTick, rcIPI.bottom - 5);
        }
    }
    pDC->SelectObject(pOldBrush);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDLView::Build
//
/////////////////////////////////////////////////////////////////////////////

void CDDLView::Build(void)
{
    const CDataDict* pDict = GetDocument()->GetDict();
    m_aItemLay.RemoveAllItems();

    CClientDC dc(this);
    CFont* pOldFont = dc.SelectObject(m_pFont);

    int iMaxLabelWidth = 0; // Maximum width of any record label (20111229 for unicode)
    int iMaxRecLen = 0;     // Maximum length on any record
    int iLineDD = 0;        // Line number for the whole layout
    for( size_t level_number = 0; level_number < pDict->GetNumLevels(); ++level_number ) {
	    const DictLevel& dict_level = pDict->GetLevel(level_number);
        iLineDD++;
        for (int r = 0 ; r < dict_level.GetNumRecords() ; r++)  {
            // Add Record Type Item
            int iRTStart = pDict->GetRecTypeStart();
            int iRTEnd = iRTStart + pDict->GetRecTypeLen();
            CItemPosInfo ipi(level_number, r, NONE, NONE, iRTStart, iRTEnd, iLineDD, NONE, NONE, LAYOUT_RECTYPE);
            m_aItemLay.AddItemPos(&ipi);
            // Add Id Items
            int iLinesId = 0;
            for( size_t id_level_number = 0; id_level_number < pDict->GetNumLevels(); ++id_level_number ) {
                iLinesId = std::max(iLinesId, BuildRec(id_level_number, COMMON, iLineDD, level_number, r));
            }
            // Add Record Items & check max label length
            int iLinesRec = BuildRec(level_number, r, iLineDD);
            iMaxRecLen = std::max(iMaxRecLen, dict_level.GetRecord(r)->GetRecLen());
            // Calculate line number for next record
            iLineDD += std::max(iLinesId, iLinesRec);

            // 20111229 for unicode: because we can't assume that all fonts are the same width (even with a courier new font), we need to get the text extent of each one
            LONG thisWidth = dc.GetTextExtent(dict_level.GetRecord(r)->GetLabel()).cx;
            iMaxLabelWidth = std::max(iMaxLabelWidth,(int) thisWidth);
        }
    }

    dc.SelectObject(pOldFont);

    // BuildRuler
    int iColSpacing = 5;
    m_csRuler.Empty();
    for (int i = 0 ; i < iMaxRecLen + iColSpacing ; i += iColSpacing)  {
        CIMSAString csNum;
        if (i + iColSpacing >= 1000)  {
            iColSpacing = 10;
        }
        csNum.Str(i + iColSpacing, iColSpacing, '.');
        m_csRuler += csNum;
    }
    m_csRuler = (CString) m_csRuler.Left(iMaxRecLen);

    // Set scroll sizes
    //CSize szTotal = CSize((iMaxRecLen + iMaxLabelLen + 5) * m_iTextWidth, (iLineDD + 2) * m_iTextHgt);
    CSize szTotal = CSize(iMaxLabelWidth + (iMaxRecLen + 5) * m_iTextWidth, (iLineDD + 2) * m_iTextHgt);
    CRect rcClient;
    GetClientRect(&rcClient);
    CSize szPage = rcClient.Size();
    CSize szLine = CSize(m_iTextWidth, m_iTextHgt);
    if (szPage.cy > 0) {
        SetScrollSizes(MM_TEXT, szTotal, szPage, szLine);
    }

    // Set contents origin
    //m_ptContentsOrigin.x = iMaxLabelLen * m_iTextWidth + LAY_BORDER * 2;    // Border + Max Label + Border
    m_ptContentsOrigin.x = iMaxLabelWidth + LAY_BORDER * 2;    // Border + Max Label + Border
    m_ptContentsOrigin.y = m_iTextHgt + LAY_BORDER * 2;                     // Border + Ruler + Border
    m_bBuilt = true;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDLView::BuildRec
//
/////////////////////////////////////////////////////////////////////////////

int CDDLView::BuildRec(int iLevel, int iRec, int iLineDD, int iIdLevel /*=NONE*/, int iIdRec /*=NONE*/)  {

    ASSERT(iRec == COMMON || (iIdLevel == NONE && iIdRec == NONE));
    const CDataDict* pDict = GetDocument()->GetDict();
    const DictLevel& dict_level = pDict->GetLevel(iLevel);
    const CDictRecord* pRec = dict_level.GetRecord(iRec);
    ASSERT(pRec);
    const CDictItem* pPrevItem;

    int iLineRT = 0;                // Number of lines needed to display this RT (returned by this function)
    int iLineItem = 0;              // Relative line number on which this item will appear within RT
    UINT uParentOccurs = 0;         // Number of occurrences of a SUBITEM's parent
    int iParentLen = 0;             // Length of a SUBITEM's parent

    for (int i = 0 ; i < pRec->GetNumItems() ; i++)  {
        // Get item position
        const CDictItem* pItem = pRec->GetItem(i);
        ASSERT(pItem);
        int iLen = pItem->GetLen();
        if (pItem->GetItemType() == ItemType::Item)  {
            iLineItem = 0;
            uParentOccurs = pItem->GetOccurs();
            iParentLen = pItem->GetLen();
        }
        else  {
            // if subitem, might need to drop to the next line
            ASSERT(i > 0);
            pPrevItem = pRec->GetItem(i - 1);
            if (pPrevItem->GetItemType() == ItemType::Item)  {
                // If first SUBITEM, drop to next line
                iLineItem = 1;
            }
            else  {
                // If there is prev subitem, see if overlap with it
                if (IsOverlap(pPrevItem, pItem))  {
                    // if overlap, drop to next line
                    iLineItem++;
                }
            }
        }

        // Add this item's info to the layout position array, accounting for occurrences
        ASSERT(pItem->GetOccurs() > 0);
        ASSERT(uParentOccurs > 0);
        for (UINT p = 0 ; p < uParentOccurs ; p++)  {
            for (UINT n = 0 ; n < pItem->GetOccurs() ; n++)  {
                int iOccs = 0;
                if (uParentOccurs > 1 || pItem->GetOccurs() > 1) {
                    iOccs = p + n + 1;
                }
                int iStart = pItem->GetStart() + n * iLen;
                int iEnd = iStart + iLen;
                if (pItem->GetItemType() == ItemType::Subitem)  {
                    // account for the SUBITEM's parents ... this does nested occurrences!
                    iStart += p * iParentLen;
                    iEnd += p * iParentLen;
                }
                ASSERT(iEnd >= iStart);
                LayoutType type;
                if (iRec == COMMON) {
                    if (pItem->GetItemType() == ItemType::Item) {
                        if (iIdLevel < iLevel) {
                            type = LAYOUT_IDITEMBEF;
                        }
                        else if (iLevel == iIdLevel) {
                            type = LAYOUT_IDITEM;
                        }
                        else {
                            type = LAYOUT_IDITEMAFT;
                        }
                    }
                    else {
                        type = LAYOUT_IDSUBITEM;
                    }

                }
                else  {
                    type = (pItem->GetItemType() == ItemType::Item ? LAYOUT_RTITEM : LAYOUT_RTSUBITEM);
                }
                if (pItem->GetItemType() != ItemType::Item || p == 0) {       // BMD  06 Jan 2002
                    CItemPosInfo ipi(iLevel, iRec, i, iOccs, iStart, iEnd, iLineItem + iLineDD, iIdLevel, iIdRec, type);
                    m_aItemLay.AddItemPos(&ipi);
                }
            }
        }
        iLineRT = std::max(iLineRT, iLineItem);
    }
/*
    #ifdef _DEBUG
        ASSERT(iLineRT >= 0);
        if (iLineDD == 0)  {
            ASSERT(iLevel == 0 && iRec == NONE);
        }
        else  {
            ASSERT((iRec>0 || iRec==COMMON && iCommonRT>0) && iLineDD>0);
        }
        if (pRec->GetNumItems() > 0)  {
            // we just built something
            if (type==LAYOUT_COMMONITEM || type==LAYOUT_COMMONSUBITEM)  {
                ASSERT(iRec==COMMON);
            }
            else  {
                ASSERT(iRec >= 0 && iRec < pDict->GetLevel(0)->GetNumRecords());
            }
        }
    #endif
*/
    return iLineRT + 1;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDLView::BuildRuler
//
/////////////////////////////////////////////////////////////////////////////

void CDDLView::BuildRuler (void) {

    int iColSpacing, iWindowWidth;
    CRect rcClient;

    GetClientRect(&rcClient);
    iWindowWidth = std::max((LONG) rcClient.Width(), GetTotalSize().cx)/m_iTextWidth;
    iColSpacing = 5;
    m_csRuler.Empty();
    for (int i = 0 ; i < iWindowWidth +iColSpacing ; i += iColSpacing)  {
        CIMSAString csNum;
        if (i+iColSpacing >= 1000)  {
            iColSpacing = 5;
        }
        csNum.Str(i+iColSpacing, iColSpacing, '.');
        m_csRuler += csNum;
    }
    m_csRuler += _T("..........");
    m_csRuler = (CString) m_csRuler.Right(m_csRuler.GetLength());
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDLView::GetRTLine
//
/////////////////////////////////////////////////////////////////////////////

int CDDLView::GetRTLine (int iLevel, int iRec) const {

    int iLine = 0;
    if (iRec < 0) {
        // Look for first record of level and subtract 1
        for (int i = 0 ; i < m_aItemLay.GetSize() ; i++)  {
            CItemPosInfo* pIPI = m_aItemLay.GetItemPos(i);
            if (pIPI->GetLevel() == iLevel && pIPI->GetRec() == 0) {
                iLine = pIPI->GetLine() - 1;
                break;
            }
        }
    }
    else {
        // Look for record and level
        for (int i = 0 ; i < m_aItemLay.GetSize() ; i++)  {
            CItemPosInfo* pIPI = m_aItemLay.GetItemPos(i);
            if (pIPI->GetLevel() == iLevel && pIPI->GetRec() == iRec) {
                iLine = pIPI->GetLine();
                break;
            }
        }
    }
    return iLine;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDLView::GetItemLine
//
/////////////////////////////////////////////////////////////////////////////
//
//  Returns the line number (local to the record type) where a particular
//  item will be displayed.
//
//  note: if iRec==COMMON, iCommonRT will signal the RT w/ which this set
//        of COMMON items are associated
//
/////////////////////////////////////////////////////////////////////////////

int CDDLView::GetItemLine (int iRec, int iItem, int iIdRec /*=NONE*/) {

    ASSERT(iIdRec == NONE || iRec == COMMON);

    int iRetVal = NONE;
    for (int i = 0 ; i < m_aItemLay.GetSize() ; i++)  {
        CItemPosInfo* pIpi = m_aItemLay.GetItemPos(i);
        if (pIpi->GetRec()==iRec && pIpi->GetItem()==iItem && pIpi->GetIdRec()== iIdRec)  {
            iRetVal = pIpi->GetLine();
            break;
        }
    }
    ASSERT(iRetVal >= 0);
    return iRetVal;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDLView::OnToolHitTest
//
/////////////////////////////////////////////////////////////////////////////

/*V*/ int CDDLView::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
    CItemPosInfo ipi;
    if (HitTest(point, ipi))  {
        const int iID = MAKELONG(point.x, point.y)+1;
        pTI->hwnd = m_hWnd;
        pTI->uId = iID;
        pTI->rect = CRect(point, CSize(1,1));

        CString csTTip;
        CIMSAString csTemp;
        const CDataDict* pDict = GetDocument()->GetDict();
        const CDictItem* pItem;
        switch (ipi.GetType())  {
        case LAYOUT_ON_NAME:
            csTTip = _T("Record label");
            break;
        case LAYOUT_RECTYPE:
            csTTip.Format(_T("Record type: Start=%d, Len=%d"), pDict->GetRecTypeStart(), pDict->GetRecTypeLen());
            break;
        case LAYOUT_IDITEMBEF:
        case LAYOUT_IDITEMAFT:
        case LAYOUT_IDITEM:
            ASSERT(ipi.GetRec()==COMMON);
            pItem = pDict->GetLevel(ipi.GetLevel()).GetRecord(ipi.GetRec())->GetItem(ipi.GetItem());
            if (ipi.GetOccs() > 0) {
                if (pItem->GetOccurrenceLabels().GetLabel(ipi.GetOccs() - 1).IsEmpty()) {
                    csTemp.Str(ipi.GetOccs());
                    csTemp = pItem->GetLabel() + _T("(") + csTemp + _T(")");
                }
                else {
                    csTemp = pItem->GetOccurrenceLabels().GetLabel(ipi.GetOccs() - 1);
                }
            }
            else {
                csTemp = pItem->GetLabel();
            }
            csTTip.Format(_T("%s: Start=%d, Len=%d"), (LPCTSTR)csTemp, ipi.GetStart(), pItem->GetLen());
            break;
        case LAYOUT_IDSUBITEM:
            ASSERT(ipi.GetRec()==COMMON);
            pItem = pDict->GetLevel(ipi.GetLevel()).GetRecord(ipi.GetRec())->GetItem(ipi.GetItem());
            if (ipi.GetOccs() > 0) {
                csTemp.Str(ipi.GetOccs());
                csTemp = pItem->GetLabel() + _T("(") + csTemp + _T(")");
            }
            else {
                csTemp = pItem->GetLabel();
            }
            csTTip.Format(_T("%s: Start=%d, Len=%d"), (LPCTSTR)csTemp, ipi.GetStart(), pItem->GetLen());
            break;
        case LAYOUT_RTITEM:
            pItem = pDict->GetLevel(ipi.GetLevel()).GetRecord(ipi.GetRec())->GetItem(ipi.GetItem());
            if (ipi.GetOccs() > 0) {
                if (pItem->GetOccurrenceLabels().GetLabel(ipi.GetOccs() - 1).IsEmpty()) {
                    csTemp.Str(ipi.GetOccs());
                    csTemp = pItem->GetLabel() + _T("(") + csTemp + _T(")");
                }
                else {
                    csTemp = pItem->GetOccurrenceLabels().GetLabel(ipi.GetOccs() - 1);
                }
            }
            else {
                csTemp = pItem->GetLabel();
            }
            csTTip.Format(_T("%s: Start=%d, Len=%d"), (LPCTSTR)csTemp, ipi.GetStart(), pItem->GetLen());
            break;
        case LAYOUT_RTSUBITEM:
            pItem = pDict->GetLevel(ipi.GetLevel()).GetRecord(ipi.GetRec())->GetItem(ipi.GetItem());
            ASSERT(pItem->GetItemType()==ItemType::Subitem);
            if (ipi.GetOccs() > 0) {
                if (pItem->GetOccurrenceLabels().GetLabel(ipi.GetOccs() - 1).IsEmpty()) {
                    csTemp.Str(ipi.GetOccs());
                    csTemp = pItem->GetLabel() + _T("(") + csTemp + _T(")");
                }
                else {
                    csTemp = pItem->GetOccurrenceLabels().GetLabel(ipi.GetOccs() - 1);
                }
            }
            else {
                csTemp = pItem->GetLabel();
            }
            csTTip.Format(_T("%s: Start=%d, Len=%d"), (LPCTSTR)csTemp, ipi.GetStart(), pItem->GetLen());
            break;
        default:
//            ASSERT(false);
            break;
        }
        csTTip = (CString) csTTip.Left(99);
        pTI->lpszText = _tcsdup((const TCHAR*)csTTip);   // MFC will free this memory
        return iID;
    }
    return NONE;
}



/////////////////////////////////////////////////////////////////////////////
//
//                        CDDLView::HitTest
//
/////////////////////////////////////////////////////////////////////////////
//
//   Determines whether or not a point (given in client coordinates)
//   intersects a dictionary item.  If so, the item's number and record are
//   assigned appropriately.
//
/////////////////////////////////////////////////////////////////////////////

bool CDDLView::HitTest (CPoint pt, CItemPosInfo& ipi) const {

    CItemPosInfo* pIpi;
    CPoint ptCurrPos;  // in character position, origin at top left of item area

    int i = 0;
    bool bRetVal = false;
//    CDataDict* pDict = GetDocument()->GetDict();
    CPoint ptScroll = GetScrollPosition();
    ptScroll += pt;
    ptCurrPos.x = (ptScroll.x-m_ptContentsOrigin.x)/m_iTextWidth+1;
    ptCurrPos.y = (ptScroll.y-m_ptContentsOrigin.y)/m_iTextHgt;
    CItemPosInfo ipiTmp(NONE, NONE, NONE, NONE, ptCurrPos.x, ptCurrPos.x+1, ptCurrPos.y, NONE, NONE, LAYOUT_NOHIT);
    ipi = ipiTmp;
    if (ptCurrPos.x < 0) {
/*
        // we're to the left of the items, move us to an RT name
        while (GetRTLine(i) < ptCurrPos.y && i < pDict->GetLevel(0)->GetNumRecords()-1)  {
            i++;
        }
        if (GetRTLine(i) != ptCurrPos.y && i > 0 && ptCurrPos.y < GetRTLine(pDict->GetLevel(0)->GetNumRecords()-1))  {
            // we are between RT names in the dlg box, move up to the previous one
            i--;
        }
*/
//        CItemPosInfo ipiTmp2(NONE, i, NONE, NONE, NONE, GetRTLine(i), NONE, LAYOUT_ON_NAME);
//        ipi = ipiTmp2;
//        bRetVal = true;
    }
    else  {
        for (i = 0 ; i < m_aItemLay.GetSize() ; i++)  {
            pIpi = m_aItemLay.GetItemPos(i);
            if (pIpi->GetLine() == ptCurrPos.y && pIpi->GetStart() <= ptCurrPos.x && pIpi->GetEnd() > ptCurrPos.x)  {
                ipi = *pIpi;   // for speed
                bRetVal = true;
                break;
            }
        }
    }
    return bRetVal;
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CDDLView::OnContextMenu
//
/////////////////////////////////////////////////////////////////////////////

void CDDLView::OnContextMenu(CWnd* /*pWnd*/, CPoint point) {

//    CMenu menu;
//    menu.LoadMenu(IDR_CONTEXT_MENU);
//    CMenu* pPopup = menu.GetSubMenu(3);   // specific to layout right-click
//    ASSERT(pPopup != NULL);

    CItemPosInfo ipi;
    CPoint ptClient = point;
    ScreenToClient(&ptClient);
    if (HitTest(ptClient, ipi))  {
        CDataDict* pDict = GetDocument()->GetDict();
        CDictItem* pItem;
        CStringArray acsInfo;
        CString cs;
        switch (ipi.GetType())  {
        case LAYOUT_ON_NAME:
            acsInfo.Add(_T("Record label"));
            break;
        case LAYOUT_RECTYPE:
            acsInfo.Add(_T("Record type indicator"));
            cs.Format(_T("Start = %d"), pDict->GetRecTypeStart());
            acsInfo.Add(cs);
            cs.Format(_T("Len = %d"),pDict->GetRecTypeLen());
            acsInfo.Add(cs);
            break;
        case LAYOUT_IDITEMBEF:
        case LAYOUT_IDITEM:
        case LAYOUT_IDITEMAFT:
            ASSERT(ipi.GetRec()==COMMON);
            pItem = pDict->GetLevel(ipi.GetLevel()).GetRecord(ipi.GetRec())->GetItem(ipi.GetItem());
            acsInfo.Add(pItem->GetLabel());
            acsInfo.Add(_T("(Common item)"));
            cs.Format(_T("Start = %d"), pItem->GetStart());
            acsInfo.Add(cs);
            cs.Format(_T("Len = %d"),pItem->GetLen());
            acsInfo.Add(cs);
            break;
        case LAYOUT_IDSUBITEM:
            ASSERT(ipi.GetRec()==COMMON);
            pItem = pDict->GetLevel(ipi.GetLevel()).GetRecord(ipi.GetRec())->GetItem(ipi.GetItem());
            acsInfo.Add(pItem->GetLabel());
            acsInfo.Add(_T("(Common subitem)"));
            cs.Format(_T("Start = %d"), pItem->GetStart());
            acsInfo.Add(cs);
            cs.Format(_T("Len = %d"),pItem->GetLen());
            acsInfo.Add(cs);
            break;
        case LAYOUT_RTITEM:
            pItem = pDict->GetLevel(ipi.GetLevel()).GetRecord(ipi.GetRec())->GetItem(ipi.GetItem());
            acsInfo.Add(pItem->GetLabel());
            acsInfo.Add(_T("(Item)"));
            cs.Format(_T("Start = %d"), pItem->GetStart());
            acsInfo.Add(cs);
            cs.Format(_T("Len = %d"),pItem->GetLen());
            acsInfo.Add(cs);
            break;
        case LAYOUT_RTSUBITEM:
            pItem = pDict->GetLevel(ipi.GetLevel()).GetRecord(ipi.GetRec())->GetItem(ipi.GetItem());
            ASSERT(pItem->GetItemType()==ItemType::Subitem);
            acsInfo.Add(pItem->GetLabel());
            acsInfo.Add(_T("(Subitem)"));
            cs.Format(_T("Start = %d"), pItem->GetStart());
            acsInfo.Add(cs);
            cs.Format(_T("Len = %d"),pItem->GetLen());
            acsInfo.Add(cs);
            break;
        default:
//            ASSERT(false);
            break;
        }
//        pPopup->InsertMenu(0, MF_BYPOSITION|MF_SEPARATOR);
//        for (int i=acsInfo.GetSize()-1 ; i>=0 ; i--)  {
//            pPopup->InsertMenu(0, MF_BYPOSITION|MF_STRING, 0, acsInfo.GetAt(i));
//        }
//        GetParentFrame()->ActivateFrame();
//        pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, AfxGetMainWnd()); // use main window for cmds
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDLView::OnLButtonUp
//
/////////////////////////////////////////////////////////////////////////////

void CDDLView::OnLButtonUp(UINT /*nFlags*/, CPoint point)
{
    CItemPosInfo ipi;
    if (HitTest(point, ipi)) {
        CDDDoc* pDoc = assert_cast<CDDDoc*>(GetDocument());
        CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
        POSITION pos = pDoc->GetFirstViewPosition();
        ASSERT(pos != NULL);
        CDDGView* pView = (CDDGView*)pDoc->GetNextView(pos);
        DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
        int iLevel = ipi.GetLevel();
        int iRec = ipi.GetRec();
        int iItem = ipi.GetItem();
        if ((iLevel != pDoc->GetLevel() ||
                iRec != pDoc->GetRec() ||
                iItem != pDoc->GetItem()) || iItem == NONE)  {
            pDoc->SetLevel(iLevel);
            pDoc->SetRec(iRec);
            pDoc->SetItem(iItem);

            pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, iLevel, iRec);
            pView->m_iGrid = DictionaryGrid::Record;
            pView->m_gridRecord.SetRedraw(FALSE);
            pView->m_gridRecord.Update(pDoc->GetDict(), iLevel, iRec);
            pView->ResizeGrid();
            pView->m_gridRecord.ClearSelections();
            if (iItem == NONE) {
                pView->m_gridRecord.GotoCell(CRecordGrid::GetStartColumn(), 0);
            }
            else {
                pView->m_gridRecord.GotoRow(pView->m_gridRecord.GetRow(iItem));
            }
            pView->m_gridRecord.SetRedraw(TRUE);
            pView->m_gridRecord.InvalidateRect(NULL);
            pView->m_gridRecord.SetFocus();
        }
        else {
            pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, iLevel, iRec, iItem);
            pView->m_iGrid = DictionaryGrid::Item;
            pView->m_gridItem.SetRedraw(FALSE);
            pView->m_gridItem.Update(pDoc->GetDict(), iLevel, iRec, iItem, 0);
            pView->ResizeGrid();
            pView->m_gridItem.ClearSelections();
            if (pView->m_gridItem.GetNumberRows() > 0) {
                pView->m_gridItem.GotoRow(0);
            }
            pView->m_gridItem.SetRedraw(TRUE);
            pView->m_gridItem.InvalidateRect(NULL);
            pView->m_gridItem.SetFocus();
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDLView::OnFind
//
/////////////////////////////////////////////////////////////////////////////

LRESULT CDDLView::OnFind(WPARAM /*wParam*/, LPARAM /*lParam*/) {

    GetParentFrame()->PostMessage(UWM::Dictionary::Find);
    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CDDLView::OnFilePrint
//
/////////////////////////////////////////////////////////////////////////////

void CDDLView::OnFilePrint() {

    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetDocument());
    POSITION pos = pDoc->GetFirstViewPosition();
    ASSERT(pos != NULL);
    CDDGView* pView = (CDDGView*)pDoc->GetNextView(pos);
    pView->SendMessage(WM_COMMAND, ID_FILE_PRINT);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDLView  Diagnostics
//
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
void CDDLView::AssertValid() const {

    CView::AssertValid();
}

void CDDLView::Dump(CDumpContext& dc) const {

    CView::Dump(dc);
}
#endif //_DEBUG

BOOL CDDLView::OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll) // BMD 20 Mar 2002
{
    SCROLLINFO info;
    info.cbSize = sizeof(SCROLLINFO);
    info.fMask = SIF_TRACKPOS;

    if (LOBYTE(nScrollCode) == SB_THUMBTRACK) {
        GetScrollInfo(SB_HORZ, &info);
        nPos = info.nTrackPos;
    }
    if (HIBYTE(nScrollCode) == SB_THUMBTRACK) {
        GetScrollInfo(SB_VERT, &info);
        nPos = info.nTrackPos;
    }
    return CScrollView::OnScroll(nScrollCode, nPos, bDoScroll);
}

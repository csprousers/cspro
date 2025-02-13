#include "StdAfx.h"
#include "GridCell.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridCell::CGridCell()
//
// 2 ctors
///////////////////////////////////////////////////////////////////////////////////////////////////
CGridCell::CGridCell() : m_rcClient(0,0,0,0), m_szMinH(0,0), m_szMinV(0,0)
{
    m_bSelected=false;
    COLORREF rColor = GetSysColor(COLOR_3DFACE);
    int r = GetRValue(rColor);
    int g = GetGValue(rColor);
    int b = GetBValue(rColor);
    if (r == 216 && g == 208 && b == 200) {
        m_colorB = RGB(237,233,227);
    }
    else if (r == 212 && g == 208 && b == 200) {
        m_colorB = RGB(234,232,227);
    }
    else {
        m_colorB = RGB(GetRValue(rColor)/2+128, GetGValue(rColor)/2+128, GetBValue(rColor)/2+128);
    }
    m_colorF = rgbFCellDefault;
    m_colorBSel = GetSysColor(COLOR_3DFACE);     // BMD  17 Apr 2001
    m_colorFSel = rgbFSelDefault;
}

CGridCell::CGridCell(const CGridCell& cell)
    :   m_boxes(cell.m_boxes),
        m_texts(cell.m_texts)
{
    for (int i=0 ; i<cell.GetNumFields(); i++)  {
        CCellField val = cell.GetField(i);
        AddField(val);
    }

    m_rcClient = cell.GetRect();
    m_colorB = cell.GetBColor();
    m_colorF = cell.GetFColor();
    m_colorBSel = cell.GetBSelColor();
    m_colorFSel = cell.GetFSelColor();
    m_bSelected = cell.IsSelected();
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridCell::operator=()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
const CGridCell& CGridCell::operator=(const CGridCell& cell)
{
    m_boxes = cell.m_boxes;
    m_texts = cell.m_texts;

    m_aField.RemoveAll();

    for (int i=0 ; i<cell.GetNumFields(); i++)  {
        CCellField val = cell.GetField(i);
        AddField(val);
    }

    m_rcClient = cell.GetRect();
    m_bSelected = cell.IsSelected();
    m_colorB = cell.GetBColor();
    m_colorF = cell.GetFColor();
    m_colorBSel = cell.GetBSelColor();
    m_colorFSel = cell.GetFSelColor();

    return *this;
}


void CGridCell::AddBoxSet(CDEBoxSet& box_set)
{
    const auto& new_boxes = box_set.GetBoxes().GetSharedPointerVector();
    m_boxes.insert(m_boxes.end(), new_boxes.cbegin(), new_boxes.cend());
}


void CGridCell::AddTextSet(CDETextSet& text_set)
{
    const auto& new_texts = text_set.GetTexts().GetSharedPointerVector();
    m_texts.insert(m_texts.end(), new_texts.cbegin(), new_texts.cend());
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridCell::CalcMinSize()
//
// There are 2 min sizes: one for vertical positioning/sizing and one for horizontal.
// Horizontal min gives the minimum possible width that we can position this cell to;
// Veritical min gives the current height of the cell.  (Vertical can't give minimum
// possible height because CDC::DrawText w/ DT_CALCRECT adjusts rect height as a function
// of given width, and not vice versa).  Note that CGridWnd::RecalcLayout will adjust
// min vertical sizes for headers.
// Savy for optimimzation- changed CalcMinSize to send an additional parameter szFieldFontTextExt passed as reference
// to be used to compute minsize. If textExtent is not known send szFieldFontTextExt(NONE,NONE)
// this method updates the reference with the new textExtent that should be use.
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridCell::CalcMinSize(CDC* pDC, CSize& szFieldFontTextExt)
{
    int iSaveDC = pDC->SaveDC();

    m_szMinH = m_szMinV = CSize(0,0);

    for( const CDEText& text : GetTexts() ) {
        pDC->SelectObject(text.GetFont().GetCFont());

        CString cs = text.GetText();
        int iRepl = cs.Replace(_T("@"), _T("%d"));
        ASSERT(iRepl==0 || iRepl==1);
        if (cs.Find(_T("%d"))!=NONE)  {
            cs.Format(CString(cs),999);
        }

        // determine min sizes, if squished horizontally or vertically
        CSize sz = CIMSAString::GetLongestWordSize(pDC, cs);
        CRect rcMinH(0,0,sz.cx,0), rcMinV(0,0,0,sz.cy);

        pDC->DrawText(cs, &rcMinV, DT_CALCRECT|DT_LEFT|DT_WORDBREAK);

        // WINBUG: sometimes CDC::CalcRect with DT_RIGHT and DT_WORDBREAK doesn't
        // wrap correctly; seems to always work OK with DT_LEFT, so we'll isolate the
        // problem based on that.   CSC 10/21/00

        CRect rcRight(rcMinH), rcLeft(rcMinH);
        pDC->DrawText(cs, &rcLeft, DT_CALCRECT|DT_LEFT|DT_WORDBREAK);
        pDC->DrawText(cs, &rcRight, DT_CALCRECT|DT_RIGHT|DT_WORDBREAK);
        if (rcLeft.Height()<rcRight.Height() || rcLeft.Width()!=rcRight.Width())  {
            rcRight.right += pDC->GetTextExtent(SPACE).cx;
        }
        rcMinH = rcRight;
        pDC->SelectStockObject(OEM_FIXED_FONT);

        m_szMinH.cx = __max(rcMinH.Width(),m_szMinH.cx);
        m_szMinH.cy = __max(rcMinH.Height(),m_szMinH.cy);
        m_szMinV.cx = __max(rcMinV.Width(),m_szMinV.cx);
        m_szMinV.cy = __max(rcMinV.Height(),m_szMinV.cy);
    }

    // fields don't distinguish between horizontal and vertical sizing
    CSize szChar(NONE,NONE);
    bool bFontInit = false;   // set to true after we've retrieved a font for the first CDEField
    for (int i=0 ; i<GetNumFields() ; i++)  {
        const CDEField* pFld = GetField(i).GetDEField();
        if (!bFontInit && szFieldFontTextExt == CSize(NONE,NONE))  {
            // first time we've seen a CDEField ... set font and csprochar size info
            bFontInit = true;
            pDC->SelectObject(pFld->GetFont().GetCFont());
            szChar = pDC->GetTextExtent(_T("0"),1);
            szFieldFontTextExt = szChar;
            pDC->SelectStockObject(OEM_FIXED_FONT);
        }
        else {
            szChar = szFieldFontTextExt;
        }

        const CDictItem* pDictItem= pFld->GetDictItem();
        if(!pDictItem)//SAVY added this to prevent crash 02/27/01
            continue;
        ASSERT(NULL!=pDictItem);
        int iLength = pDictItem->GetLen();
        if(!pDictItem->GetDecChar()  && pDictItem->GetDecimal() != 0){
          iLength++; //The length does not account for the decimal character in this case
        }

        int iXB = GetSystemMetrics(SM_CXBORDER); //Border width
        int iYB = GetSystemMetrics(SM_CYBORDER); //Border height
        int iFldWidth = szChar.cx * iLength + (iLength-1)*GRIDSEP_SIZE + 2*iLength + 2*iXB; // 1 for outside border
        int iFldHeight = szChar.cy + 2*iYB; // 1 for outside border
        if(pFld->UseUnicodeTextBox()){
             pFld->GetDims().Height() > 0 ? iFldHeight = pFld->GetDims().Height() : iFldHeight =iFldHeight;
             pFld->GetDims().Width() > 0 ? iFldWidth = pFld->GetDims().Width() : iFldWidth =iFldWidth;
        }


        /* removed csc 2/23/01; handle this in recalc layout
        // update field rect ... csc 2/13/01
        CRect rcFld = pFld->GetDims();
        if (rcFld.Width()!=iFldWidth || rcFld.Height()!=iFldHeight)  {
            rcFld.BottomRight() = rcFld.TopLeft()+CPoint(iFldWidth, iFldHeight);
            pFld->SetDims(rcFld);
        }
        */
        if (GetNumFields()==1)  {
            // set min size to accomodate "auto-sliding" ...
            m_szMinH.cx = __max(m_szMinH.cx, iFldWidth);
            m_szMinH.cy = __max(m_szMinH.cy, iFldHeight);
            m_szMinV.cx = __max(m_szMinV.cx, iFldWidth);
            m_szMinV.cy = __max(m_szMinV.cy, iFldHeight);
        }
        else  {
            // when more than 1 field is present, don't allow "auto-sliding" ... csc 12/27/00
            const CPoint ptFldOrigin(pFld->GetDims().TopLeft());
            m_szMinH.cx = __max(m_szMinH.cx, ptFldOrigin.x + iFldWidth);
            m_szMinH.cy = __max(m_szMinH.cy, ptFldOrigin.y + iFldHeight);
            m_szMinV.cx = __max(m_szMinV.cx, ptFldOrigin.x + iFldWidth);
            m_szMinV.cy = __max(m_szMinV.cy, ptFldOrigin.y + iFldHeight);
        }
    }

    pDC->RestoreDC(iSaveDC);
}

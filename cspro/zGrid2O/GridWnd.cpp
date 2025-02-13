#include "StdAfx.h"
#include "GridWnd.h"


BEGIN_MESSAGE_MAP(CGridWnd, CWnd)
    //{{AFX_MSG_MAP(CGridWnd)
    ON_WM_PAINT()
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_WM_GETMINMAXINFO()
    ON_WM_LBUTTONUP()
    ON_WM_KEYDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_SETCURSOR()
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONDOWN()
    ON_WM_RBUTTONUP()
    ON_WM_ERASEBKGND()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGridWnd

int CGridWnd::m_iId = 1001;
__int64 CGridWnd::m_lNumColors = NONE;


namespace
{
    UINT GetHorzDTFlag(const CDEText& text)
    {
        switch( text.GetHorizontalAlignmentOrDefault() )
        {
        case HorizontalAlignment::Left:
            return DT_LEFT;
        case HorizontalAlignment::Center:
            return DT_CENTER;
        case HorizontalAlignment::Right:
        default:
            return DT_RIGHT;
        }
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                                      CGridWnd::CGridWnd()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
CGridWnd::CGridWnd() : m_rcScroll(0,0,0,0), m_rcTotal(0,0,0,0), m_ptCorner(1,1), m_szHeader(0,0), m_decimalChar(_T('.'))
{
    m_bInitialized = FALSE;
    m_bResizing = FALSE;
    m_bSwapping = FALSE;
    m_bPreSwapping = FALSE;
    m_pRoster = NULL;
    m_iActiveCol = m_iActiveRow = m_iActiveField = NONE;
    m_ptActiveCell = CPoint(NONE,NONE);
    m_iResizingCol = NONE;
    m_iResizingRow = NONE;
    m_pEdit = NULL;
    m_dwStyle = WS_BORDER|WS_CHILD|WS_VISIBLE;
    m_bQueue = false;
    m_cGridbkcolor = ::GetSysColor(COLOR_BTNFACE);

    m_iTextBoxHeightMin = -1;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                                      CGridWnd::OnPaint()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::OnPaint()
{
    CPaintDC dc(this); // device context for painting
    int iSaveDC = dc.SaveDC();

    CPoint ptOffset(GetScrollPos());
    CRect rcDraw, rcClient;
    int iMaxCol, iMaxRow, iMinRow, iMinCol, iRow, iCol;
    //    UINT uDrawFormat=0;
    bool bFontInit = false;   // set to true after we've retrieved a font for the first CDEField

    dc.SetMapMode(MM_TEXT);

    CSize szChar(NONE,NONE);

    CRect rcClientNoScrollBar;
    GetClientRect(&rcClientNoScrollBar);
    GetClientRect(&rcClient);
    rcClient.BottomRight() += CSize(::GetSystemMetrics(SM_CXVSCROLL),::GetSystemMetrics(SM_CYHSCROLL));
    dc.GetClipBox(&rcDraw);
    dc.SetBkMode(TRANSPARENT);

    // right justify roster in window for right to left
    ptOffset.x -= GetRightJustifyOffset();

   // start csc 4/30/01
    iMaxCol=GetNumCols();
    iMinCol=0;
    iMaxRow=GetNumRows();
    iMinRow=0;
    for (iCol=1 ; iCol<iMaxCol ; iCol++)  {

        CGridCell& cell = GetHeaderCell(iCol);
        CRect rc = cell.GetRect();
        rc.OffsetRect(CPoint(-ptOffset.x,0));
        if (rc.left > rcDraw.right)  {
            iMaxCol = iCol;
            break;
        }
        if (rc.right <= rcDraw.left)  {
            continue;
        }
        if (iMinCol==0)  {

            iMinCol=iCol;
        }
    }
    for (iRow=1 ; iRow<iMaxRow ; iRow++)  {
        CGridCell& cell = GetStubCell(iRow);
        CRect rc = cell.GetRect();
        rc.OffsetRect(CPoint(0,-ptOffset.y));

        if (rc.top > rcDraw.bottom)  {
            iMaxRow = iRow;
            break;
        }
        if (rc.bottom <= rcDraw.top)  {
            continue;
        }
        if (iMinRow==0)  {
            iMinRow=iRow;
        }
    }
    // end special 4/30/01 csc section


    // draw cells
    for (iRow=iMinRow ; iRow<iMaxRow ; iRow++)  {
        CGridRow& row=GetRow(iRow);
        for (iCol=iMinCol ; iCol<iMaxCol ; iCol++)  {
            CGridCell& cell = row.GetCell(iCol);
            CRect rcCell = cell.GetRect();
            rcCell.OffsetRect(-ptOffset);
            dc.FillSolidRect(&rcCell, cell.IsSelected()?cell.GetBSelColor():cell.GetBColor());
            rcCell.DeflateRect(GRIDCELL_BORDER, GRIDCELL_BORDER);

            // draw with an offset...
            CPoint offset = rcCell.TopLeft();

            // text
            for( const CDEText& text : cell.GetTexts() ) {
                int iSaveDC3 = dc.SaveDC();
                dc.SelectObject(text.GetFont().GetCFont());
                CRect rcTxt=text.GetDims();
                rcTxt.OffsetRect(offset);
                CPen penCell(PS_SOLID, 1, cell.GetFColor());
                dc.SelectObject(&penCell);
                dc.SetTextColor(text.GetColor().ToCOLORREF()); // Test Chirag
                dc.DrawText(text.GetText(), rcTxt, DT_LEFT|DT_WORDBREAK);
                dc.RestoreDC(iSaveDC3);
            }

            // boxes
            for( const CDEBox& box : cell.GetBoxes() ) {
                box.Draw(&dc, &offset);
            }

            // fields
            for (int i = 0 ; i<cell.GetNumFields() ; i++)  {
                int iSaveDC4 = dc.SaveDC();
                CCellField& fld = cell.GetField(i);
                CDEField* pFld = fld.GetDEField();
                //Set the color for protected SAVY 02/12/01 added the gray color for protected
                if(pFld->IsProtected() || pFld->IsMirror()){
                    fld.SetBColor(m_cGridbkcolor);//RGB(192,192,192) Modified by chirag while changing color
                }
                CRect rcFld = fld.GetDEField()->GetDims();
                rcFld.OffsetRect(offset);


                // paint mimicking Savy's new CDEEdit;
                // *****************************************************************************************
                // IF YOU CHANGE THIS, BE SURE TO UPDATE CDEEdit and CFormScrollView::DrawField() !!!!!
                // *****************************************************************************************

                CBrush brush(fld.IsSelected()?fld.GetBSelColor():fld.GetBColor());
                dc.SelectObject(pFld->GetFont().GetCFont());
                if (!bFontInit)  {
                    // first time we've seen a CDEField ... create font
                    bFontInit = true;
                    szChar = dc.GetTextExtent(_T("0"), 1);
                }
                int iYB = GetSystemMetrics(SM_CYBORDER); //Border height; //Border height
                m_iTextBoxHeightMin = szChar.cy + 2*iYB;
                dc.SelectObject(&brush);

                // RHF INIC Nov 22, 2002
                if( pFld->IsHidden() )
                    dc.SetTextColor(fld.IsSelected()?fld.GetBSelColor():fld.GetBColor());
                else
                    // RHF END Nov 22, 2002
                dc.SetTextColor(fld.IsSelected()?fld.GetFSelColor():fld.GetFColor());
                dc.SetBkColor(fld.IsSelected()?fld.GetBSelColor():fld.GetBColor());

                const CDictItem* pDictItem = fld.GetDEField()->GetDictItem();

                bool bUsingControl = false;
                HPEN hBlueBorderPen = NULL; // GHM 20110509 not deleting the pen was creating problems when moving blocks on and off the screen

                if( pDictItem != nullptr && !_tcsstr(AfxGetApp()->m_pszExeName, _T("ntry")) ) // GHM 20120131 removed the E because the case after the build sometimes differed
                {
                    // GHM 20100608 if the user is using a control draw the border in a different color, and thicker
                    CaptureType evaluated_capture_type = pFld->GetEvaluatedCaptureInfo().IsSpecified() ?
                        pFld->GetEvaluatedCaptureInfo().GetCaptureType() : CaptureInfo::GetBaseCaptureType(*pDictItem);

                    bUsingControl = ( evaluated_capture_type != CaptureType::TextBox );

                    if( bUsingControl )
                    {
                        // GHM 20130417 use a dark aqua for the number pad
                        COLORREF color = ( evaluated_capture_type == CaptureType::NumberPad ) ? RGB(95, 150, 160) :
                                                                                                RGB(0, 0, 128);

                        hBlueBorderPen = CreatePen(PS_SOLID, 2, color);

                        dc.SelectObject(hBlueBorderPen); // blue border, two pixels wide
                        rcFld.left++;
                        rcFld.top++;
                    }
                }

                dc.MoveTo(rcFld.left,rcFld.top+1 );
                dc.LineTo(rcFld.right-1, rcFld.top+1);
                dc.LineTo(rcFld.right-1, rcFld.bottom);
                dc.LineTo(rcFld.left, rcFld.bottom);

                if( bUsingControl )
                    dc.LineTo(rcFld.left,rcFld.top);
                else
                    dc.LineTo(rcFld.left,rcFld.top+1);

                CRect rect(rcFld.left+1,rcFld.top+2 ,rcFld.right-1,rcFld.bottom);

                if( bUsingControl ) // GHM 20100608 the fill rectangle should be slightly smaller due to the larger border
                {
                    rect.right--;
                    rect.bottom--;
                    DeleteObject(hBlueBorderPen);
                }

                dc.FillSolidRect(&rect, fld.IsSelected()?fld.GetBSelColor():fld.GetBColor());

                ASSERT(NULL!=fld.GetDEField());

                if(!pDictItem)//SAVY added this to prevent crash 02/27/01
                    continue;

                if( fld.GetDEField()->UseUnicodeTextBox() ||
                    ( pDictItem->GetContentType() == ContentType::Alpha && fld.GetDEField()->GetFont().IsArabic() ) ) {
                    // for alpha fields with arabic fonts don't draw tick marks
                    // and use single call to textout with whole string to get ligatures
                    CString sTrim = fld.GetVal();

                    if(!fld.GetDEField()->UseUnicodeTextBox() && fld.GetDEField()->GetFont().IsArabic()){
                        dc.SetTextAlign(TA_RIGHT | TA_BOTTOM | TA_RTLREADING);
                        sTrim.TrimRight();
                        CSize sizeStr = dc.GetTextExtent(sTrim);
                        dc.TextOut(rect.right, rect.bottom, sTrim);
                    }
                    else {
                        CString& csTemp =sTrim;   //Add CRLF for multiline
                        int p = csTemp.Find(_T("\\n"));
                        while (p >= 0) {
                            csTemp.SetAt(p++,'\r');
                            csTemp.SetAt(p,'\n');
                            p = csTemp.Find(_T("\\n"),p);
                        }
                        if(fld.GetDEField()->AllowMultiLine()){
                            dc.DrawText(csTemp, rect, DT_EDITCONTROL|DT_WORDBREAK);
                        }
                        else{
                            dc.DrawText(csTemp, rect, DT_EDITCONTROL);
                        }
                    }

                }
                else {
                    int iLength = pDictItem->GetLen();
                    if(!pDictItem->GetDecChar()  && pDictItem->GetDecimal() != 0){
                        iLength++; //The length does not account for the decimal character in this case
                    }
                    int iLeft = rcFld.left+1; //account for the left border
                    int iBottom = rcFld.bottom-1; //account for the bottom border
                    int iHeight = rcFld.Height() -2 ;//account for the border

                    for (int iIndex=0; iIndex < iLength-1; iIndex ++) {

                        //if(m_iTickWidth != 1) {//Savy for showing the tick marks in Vista and upwards
                        //  BOOL bRet = FALSE;
                        //  CPen penLine(PS_SOLID, m_iTickWidth, rgbCellLines);
                        //  CPen* pOldPen = dc.SelectObject(&penLine);
                        //  dc.MoveTo(iLeft + szChar.cx * (iIndex+1) +(iIndex+1)*2 +iIndex*GRIDSEP_SIZE,iBottom);
                        //  bRet =  dc.LineTo(iLeft + szChar.cx * (iIndex+1)+(iIndex+1)*2+iIndex*GRIDSEP_SIZE ,iBottom-iHeight/4);
                        //  dc.SelectObject(pOldPen);
                        //  ASSERT(bRet);
                        //}
                        //else
                        { //use default pen
                            dc.MoveTo(iLeft + szChar.cx * (iIndex+1) +(iIndex+1)*2 +iIndex*GRIDSEP_SIZE,iBottom);
                            dc.LineTo(iLeft + szChar.cx * (iIndex+1)+(iIndex+1)*2+iIndex*GRIDSEP_SIZE ,iBottom-iHeight/4);
                        }
                    }

                    if(pDictItem->GetDecimal() != 0) {//put the decimal character
                        for (int iIndex =0 ; iIndex <iLength ; iIndex++) {
                            //int iX = rect.left + sizeChar.cx * (iIndex) +(2*iIndex + 1)+iIndex*SEP_SIZE;
                            int iX = iLeft + szChar.cx * (iIndex) +(2*iIndex + 1)+iIndex*GRIDSEP_SIZE - 1;
                            int iY = iBottom - szChar.cy+1;
                            if(iIndex == (int)(iLength  - pDictItem->GetDecimal() -1)){
                                //dc.TextOut(iX,iY,'.');//SAVY&& get the decimal character from sytem metrics
                                dc.TextOut(iX,iY,m_decimalChar);//SAVY&& get the decimal character from sytem metrics
                                break;
                            }
                        }
                    }


                    if(!fld.GetVal().IsEmpty()) {
                        for(int iIndex =0 ; iIndex <fld.GetVal().GetLength(); iIndex++) {
                            int iX = iLeft + szChar.cx * (iIndex) +(2*iIndex + 1)+iIndex*GRIDSEP_SIZE - 1;
                            int iY = iBottom - szChar.cy+1;

                            if(fld.GetVal().GetAt(iIndex) != m_decimalChar)
                                dc.TextOut(iX,iY,fld.GetVal().GetAt(iIndex));
                        }
                    }
                }

                dc.SelectStockObject(OEM_FIXED_FONT);  // need to prevserve fontEdit
                dc.RestoreDC(iSaveDC4);
            }
        }
   }
        // draw lines
        CPen penLine(PS_SOLID, 1, rgbCellLines);
        dc.SelectObject(&penLine);
        CPoint pt;
        for (iCol=std::max(1, iMinCol); iCol<iMaxCol ; iCol++)  {
            CRect& rc = GetHeaderCell(iCol).GetRect();
            pt.x = rc.left - ptOffset.x;
            pt.y = rcDraw.top;
            dc.MoveTo(pt);
            dc.LineTo(pt.x, rcDraw.bottom);
        }

        // if left side of min col is visible, draw it (only happens when stubs are on right
        // and you can have dead space on left)
        if (iMinCol == 0) {
            CRect& rc = GetHeaderCell(0).GetRect();
            pt.x = rc.left - ptOffset.x;
            pt.y = rcDraw.top;
            dc.MoveTo(pt);
            dc.LineTo(pt.x, rcDraw.bottom);
        }

        if (iMinRow>0)  {
            for (iRow=iMinRow-1 ; iRow<iMaxRow ; iRow++)  {
                CRect& rc = GetStubCell(iRow).GetRect();
                pt.x = rcDraw.left;
                pt.y = rc.bottom - ptOffset.y;
                dc.MoveTo(pt);
                dc.LineTo(rcDraw.right, pt.y);
            }
        }

        // draw headers
        //      iMaxCol=GetNumCols();
        //      iMinCol=0;
        CString csStub;
        //      for (iCol=1 ; iCol<iMaxCol ; iCol++)  {
        //              CGridCell& cell = GetHeaderCell(iCol);
        //              CRect rc = cell.GetRect();
        //              rc.OffsetRect(CPoint(-ptOffset.x,0));
        //
        //              if (rc.left > rcDraw.right)  {
        //                      iMaxCol = iCol;
        //                      break;
        //              }
        //              if (rc.right <= rcDraw.left)  {
        //                      continue;
        //              }
        //              if (iMinCol==0)  {
        //                      iMinCol=iCol;
        //              }

        for (iCol=iMinCol ; iCol<iMaxCol ; iCol++)  {
            CGridCell& cell = GetHeaderCell(iCol);
            CRect rc = cell.GetRect();

            rc.OffsetRect(CPoint(-ptOffset.x,0));

            if (cell.IsSelected())  {
                dc.FillSolidRect(&rc, cell.GetBSelColor());
                dc.Draw3dRect(&rc, rgbBlack, rgbWhite);
            }
            else  {
                dc.FillSolidRect(&rc, cell.GetBColor());
                dc.DrawEdge(&rc, EDGE_ETCHED, BF_RIGHT|BF_BOTTOM);
            }
            dc.SetTextColor(cell.IsSelected()?cell.GetFSelColor():cell.GetFColor());

            for( const CDEText& text : cell.GetTexts() ) {
                CRect rcTxt = text.GetDims();
                rcTxt.OffsetRect(rc.TopLeft());
                rcTxt.DeflateRect(GRIDCELL_BORDER, GRIDCELL_BORDER);
                csStub = text.GetText();
                if (csStub.Find(_T("%d"))!=NONE)  {
                    csStub.Format(CString(csStub),iCol);
                }
                int iSaveDC1 = dc.SaveDC();
                dc.SelectObject(text.GetFont().GetCFont());
                dc.SetTextColor(text.GetColor().ToCOLORREF()); // Test Chirag
                dc.DrawText(csStub, &rcTxt, GetHorzDTFlag(text) | DT_WORDBREAK);
                dc.RestoreDC(iSaveDC1);
            }
        }

        // draw stubs
        //      iMaxRow=GetNumRows();
        //      iMinRow=0;
        //      for (iRow=1 ; iRow<iMaxRow ; iRow++)  {
        //              CGridCell& cell = GetStubCell(iRow);
        //              CRect rc = cell.GetRect();
        //              rc.OffsetRect(CPoint(0,-ptOffset.y));
        //
        //              if (rc.top > rcDraw.bottom)  {
        //                      iMaxRow = iRow;
        //                      break;
        //              }
        //              if (rc.bottom <= rcDraw.top)  {/
        //                      continue;
        //              }
        //              if (iMinRow==0)  {
        //                      iMinRow=iRow;
        //              }


        for (iRow=iMinRow ; iRow<iMaxRow ; iRow++)  {
            CGridCell& cell = GetStubCell(iRow);
            CRect rc = cell.GetRect();
            rc.OffsetRect(CPoint(0,-ptOffset.y));

            // right justify stubs in window for right to left
            if (m_pRoster->GetRightToLeft()) {
                rc.OffsetRect(rcClientNoScrollBar.right - rc.right, 0);
            }

            if (cell.IsSelected())  {
                dc.FillSolidRect(&rc, cell.GetBSelColor());
                dc.Draw3dRect(&rc, rgbBlack, rgbWhite);
            }
            else  {
                dc.FillSolidRect(&rc, cell.GetBColor());
                if (m_pRoster->GetRightToLeft()) {
                    dc.DrawEdge(&rc, EDGE_ETCHED, BF_LEFT|BF_BOTTOM);
                }
                else {
                    dc.DrawEdge(&rc, EDGE_ETCHED, BF_RIGHT|BF_BOTTOM);
                }
            }
            dc.SetTextColor(cell.IsSelected()?cell.GetFSelColor():cell.GetFColor());

            //              rc.DeflateRect(GRIDCELL_BORDER, GRIDCELL_BORDER);
            for( const CDEText& text : cell.GetTexts() ) {
                int iSaveDC2 = dc.SaveDC();
                CRect rcTxt = text.GetDims();
                rcTxt.OffsetRect(rc.TopLeft());
                rcTxt.DeflateRect(GRIDCELL_BORDER, GRIDCELL_BORDER);
                csStub = text.GetText();
                if (csStub.Find(_T("%d"))!=NONE)  {
                    csStub.Format(CString(csStub),iRow);
                }

                dc.SelectObject(text.GetFont().GetCFont());
                dc.SetTextColor(text.GetColor().ToCOLORREF()); // Test Chirag
                dc.DrawText(csStub, &rcTxt, GetHorzDTFlag(text) | DT_WORDBREAK);
                dc.RestoreDC(iSaveDC2);
            }
        }

        // draw dead areas above vert sb and to left of horiz sb
        CGridCell& cell = GetHeaderCell(0);
        CRect rc;

        rc.SetRect(0,rcClient.Height()-::GetSystemMetrics(SM_CYHSCROLL),m_szHeader.cx,rcClient.bottom);
        dc.FillSolidRect(&rc, GetSysColor(COLOR_3DFACE));     // BMD 17 Apr 2001
        rc.SetRect(rcClient.Width()-::GetSystemMetrics(SM_CXVSCROLL),0,rcClient.right,m_szHeader.cy);
        dc.FillSolidRect(&rc, GetSysColor(COLOR_3DFACE));     // BMD 17 Apr 2001

        // draw dead area in upper left corner
        rc = cell.GetRect();
        // right justify stubs in window for right to left
        if (m_pRoster->GetRightToLeft()) {
            rc.OffsetRect(rcClientNoScrollBar.right - rc.right, 0);
        }
        dc.FillSolidRect(&rc, cell.GetBColor());
        if (m_pRoster->GetRightToLeft()) {
            dc.DrawEdge(&rc, EDGE_ETCHED, BF_LEFT|BF_BOTTOM);
        }
        else {
            dc.DrawEdge(&rc, EDGE_ETCHED, BF_RIGHT|BF_BOTTOM);
        }
        ASSERT(cell.GetNumTexts()==1);
        //SAVY 01/29 this comes up in sequential fields ASSERT(cell.GetText(0)->GetText().IsEmpty());
        ASSERT(cell.GetNumBoxes()==0);
        ASSERT(cell.GetNumFields()==0);

        // draw resizing indicators, if applicable
        if (IsResizing() || IsSwapping())  {
            ASSERT(m_iResizingCol==NONE || m_iResizingRow==NONE);  // one or the other, can't do both at the same time!
            CRect rcResizing;
            if (m_iResizingCol != NONE)  {
                rcResizing.SetRect(m_iCurResizePos-1,0,m_iCurResizePos+1,rcDraw.bottom);
                rcResizing.OffsetRect(-ptOffset.x,0);
            }
            else  {
                rcResizing.SetRect(0,m_iCurResizePos-1,rcDraw.right,m_iCurResizePos+1);
                rcResizing.OffsetRect(0,-ptOffset.y);
            }
            dc.DrawFocusRect(&rcResizing);
        }

        dc.RestoreDC(iSaveDC);

        // draw rect trackers
        for (int i = 0 ; i<this->GetNumTrackers() ; i++)  {
            GetTracker(i).Draw(&dc);
        }

        // update edit ctrl, if present
        if (m_pEdit->GetSafeHwnd())  {
            m_pEdit->RedrawWindow();
        }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::RedrawCell()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::RedrawCell(int iRow, int iCol, bool bUpdate /*=false*/)
{
    CGridCell& cell = GetCell(iRow, iCol);
    CRect rcCell = cell.GetRect();
    if (m_pRoster->GetRightToLeft() && iCol == 0) {
        // stubs in r to l are special case since they are always at right side of window
        CRect rcClient;
        GetClientRect(&rcClient);
        rcCell.OffsetRect(rcClient.right - rcCell.right, -GetScrollPos().y);
    }
    else {
        rcCell.OffsetRect(-GetScrollPos().x + GetRightJustifyOffset(), -GetScrollPos().y);
    }
    InvalidateRect(rcCell);
    if (bUpdate)  {
        UpdateWindow();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//              CGridWnd::Create(const CRect& rcPage, CWnd* pParentWnd, CDERoster* pRoster)
//      CGridWnd::Create(const CPoint& ptOrigin, int iNumRows, int iNumCols, CWnd* pParentWnd, CDERoster* pRoster)
//
// rcPage -- the bounding viewport rectangle, in parent client coordinates
// ptOrigin -- viewport origin, in parent client coordinates
// iNumRows -- number of rows (incl header row) to display in viewport (NONE means size for all rows; no vertical scroll)
// iNumCols -- number of cols (incl stub col) to display in viewport (NONE means size for all cols; no horiz scroll)
// pParentWnd -- parent window, we are a child window of pParentWnd
// pRoster -- CDERoster object that we are rendering
// bShrinkToFit -- will shrink rcPage so that rows and cols are completely displayed
///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CGridWnd::Create(const CRect& rcPage, CWnd* pParentWnd, CDERoster* pRoster, bool bShrinkToFit /*=false*/)
{
    ASSERT_VALID(pParentWnd);
    ASSERT_VALID(pRoster);
    m_pRoster = pRoster;

    // create ourselves
    //m_dwStyle  = WS_BORDER|WS_CHILD|WS_VISIBLE SAVY made this into member variable  to create a grid
    //in hidden mode
    if (!CWnd::Create(NULL, _T("gridwnd"), m_dwStyle, rcPage, pParentWnd, CGridWnd::m_iId++))  {
        TRACE(_T("Error creating grid window\n"));
        return FALSE;
    }

    // create horizontal sb
    if (!m_sbHorz.Create(SBS_HORZ|WS_CHILD|SBS_BOTTOMALIGN, CRect(0,0,0,0), this, CGridWnd::m_iId++))  {
        TRACE(_T("Error creating horizontal scroll bar\n"));
        return FALSE;
    }

    // create vertical sb
    if (!m_sbVert.Create(SBS_VERT|WS_CHILD|SBS_RIGHTALIGN, CRect(0,0,0,0), this, CGridWnd::m_iId++))  {
        TRACE(_T("Error creating vertical scroll bar\n"));
        return FALSE;
    }

    // cover dead space at intersection of scroll bars
    if (!m_wndCorner.Create(_T("STATIC"), NULL, WS_CHILD|WS_VISIBLE, CRect(0,0,0,0), this, CGridWnd::m_iId++))  {
        TRACE(_T("Error creating corner dead-space window\n"));
        return FALSE;
    }

    m_bInitialized = TRUE;
    BuildGrid();

    CSize szPage(rcPage.Size());
    if (bShrinkToFit)  {
        RecalcLayout(CSize(-1,-1), false);
        int iRow, iCol, iNumCols, iNumRows;
        iNumRows=GetNumRows();
        iNumCols=GetNumCols();

        bool bSBHorz = (GetHeaderCell(iNumCols-1).GetRect().right>rcPage.Width());
        bool bSBVert = (GetStubCell(iNumRows-1).GetRect().bottom>rcPage.Height());
        CSize szSB(::GetSystemMetrics(SM_CXVSCROLL), ::GetSystemMetrics(SM_CYHSCROLL));  // scrollbar sizes

        if (bSBVert && !bSBHorz)  {
            bSBHorz = (GetHeaderCell(iNumCols-1).GetRect().right>rcPage.Width() - szSB.cx);
        }
        if (!bSBVert && bSBHorz)  {
            bSBVert = (GetStubCell(iNumRows-1).GetRect().bottom>rcPage.Height()-szSB.cy);
        }

        for (iCol=0 ; iCol<iNumCols ; iCol++)  {
            if (GetHeaderCell(iCol).GetRect().right>rcPage.Width() -  (bSBVert?::GetSystemMetrics(SM_CXVSCROLL):0))  {
                break;
            }
        }
        for (iRow=0 ; iRow<iNumRows ; iRow++)  {
            if (GetStubCell(iRow).GetRect().bottom>rcPage.Height() - (bSBHorz?::GetSystemMetrics(SM_CYHSCROLL):0))  {
                break;
            }
        }

        if (iCol>1)  {
            iCol--;
            szPage.cx = GetHeaderCell(iCol).GetRect().right + (bSBVert?::GetSystemMetrics(SM_CXVSCROLL):0);
        }
        if (iRow>1)  {
            iRow--;
            szPage.cy = GetStubCell(iRow).GetRect().bottom + (bSBHorz?::GetSystemMetrics(SM_CYHSCROLL):0);
        }
    }
    RecalcLayout(szPage);
    return TRUE;
}


BOOL CGridWnd::Create(const CPoint& ptOrigin, int iNumRows, int iNumCols, CWnd* pParentWnd, CDERoster* pRoster)
{
    ASSERT_VALID(pParentWnd);
    ASSERT_VALID(pRoster);
    m_pRoster = pRoster;
    //m_dwStyle  = WS_BORDER|WS_CHILD|WS_VISIBLE SAVY made this into member variable  to create a grid
    //in hidden mode
    // create ourselves
    if (!CWnd::Create(NULL, _T("gridwnd"), m_dwStyle, CRect(ptOrigin, CSize(200,200)), pParentWnd, CGridWnd::m_iId++))  {
        TRACE(_T("Error creating grid window\n"));
        return FALSE;
    }

    // create horizontal sb
    if (!m_sbHorz.Create(SBS_HORZ|WS_CHILD|SBS_BOTTOMALIGN, CRect(0,0,0,0), this, CGridWnd::m_iId++))  {
        TRACE(_T("Error creating horizontal scroll bar\n"));
        return FALSE;
    }

    // create vertical sb
    if (!m_sbVert.Create(SBS_VERT|WS_CHILD|SBS_RIGHTALIGN, CRect(0,0,0,0), this, CGridWnd::m_iId++))  {
        TRACE(_T("Error creating vertical scroll bar\n"));
        return FALSE;
    }

    // cover dead space at intersection of scroll bars
    if (!m_wndCorner.Create(_T("STATIC"), NULL, WS_CHILD|WS_VISIBLE, CRect(0,0,0,0), this, CGridWnd::m_iId++))  {
        TRACE(_T("Error creating corner dead-space window\n"));
        return FALSE;
    }

    m_bInitialized = TRUE;
    BuildGrid();

    if (iNumRows==NONE)  {
        iNumRows = GetNumRows();
    }
    if (iNumCols==NONE)  {
        iNumCols = GetNumCols();
    }

    ASSERT(iNumRows>0 && iNumRows<=GetNumRows());
    ASSERT(iNumCols>0 && iNumCols<=GetNumCols());

    bool bSBHorz = (iNumCols<GetNumCols()-1);
    bool bSBVert = (iNumRows<GetNumRows()-1);
    CSize szPage;

    RecalcLayout(CSize(100,100), false);
    szPage.cx = GetHeaderCell(iNumCols-1).GetRect().right + (bSBVert?::GetSystemMetrics(SM_CXVSCROLL):0);
    m_pRoster->GetRightToLeft() ?  szPage.cx = GetHeaderCell(0).GetRect().right : szPage.cx ;
    szPage.cy = GetStubCell(iNumRows-1).GetRect().bottom + (bSBHorz?::GetSystemMetrics(SM_CYHSCROLL):0);
    RecalcLayout(szPage);

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                                      CGridWnd::OnHScroll()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::OnHScroll(UINT nSBCode, UINT, CScrollBar* pScrollBar)
{
    long lCurPos, lPrevPos, lX;   // csc 1/6/04, changed int to long so we can accommodate very large grids (size >32767); see bug 1289
    int iNumCols;
    bool bHit;
    CRect rcClient;
    SCROLLINFO si;

    GetClientRect(&rcClient);
    si.cbSize=sizeof(SCROLLINFO);
    si.fMask=SIF_ALL;
    pScrollBar->GetScrollInfo(&si);
//    iCurPos = iPrevPos = pScrollBar->GetScrollPos();
    lCurPos = lPrevPos = (long)si.nPos;
    iNumCols = GetNumCols();

    switch (nSBCode)  {
    case SB_LINELEFT:
        if (m_ptCorner.x>1)  {
            lCurPos = GetHeaderCell(--m_ptCorner.x).GetRect().left-GetLeftHeaderMargin();
        }
        else {
            lCurPos = 0; // scroll all the way to the left in case previous scroll
                         // keft us with a little space to the left even though m_ptCorner is at 1
        }
        break;
    case SB_LINERIGHT:
        if (m_ptCorner.x<iNumCols-1)  {
            lCurPos = GetHeaderCell(++m_ptCorner.x).GetRect().left-GetLeftHeaderMargin();
        }
        else {
            lCurPos = (long)si.nMax-(long)si.nPage+1;
        }
        break;
    case SB_PAGELEFT:
        ASSERT(m_ptCorner.x>=0 && m_ptCorner.x < iNumCols);
        if (GetHeaderCell(m_ptCorner.x).GetRect().Width() > rcClient.Width()-m_szHeader.cx) {
            // handle cells wider than window
            if (m_ptCorner.x > 0) {
                lCurPos = GetHeaderCell(--m_ptCorner.x).GetRect().left-GetLeftHeaderMargin();
            }
            else {
                lCurPos = si.nMin;
            }
        }
        else {
            lX=GetHeaderCell(m_ptCorner.x).GetRect().right-rcClient.Width()+m_szHeader.cx;
            bHit=false;
            while (m_ptCorner.x>0 && !bHit)  {
                bHit = (GetHeaderCell(m_ptCorner.x--).GetRect().left < lX);
                ASSERT(m_ptCorner.x>=0 && m_ptCorner.x < iNumCols);
            }
            lCurPos = GetHeaderCell(++m_ptCorner.x).GetRect().left-GetLeftHeaderMargin();
        }
        break;
    case SB_PAGERIGHT:
        ASSERT(m_ptCorner.x>=0 && m_ptCorner.x < iNumCols);
        if (GetHeaderCell(m_ptCorner.x).GetRect().Width() > rcClient.Width()-m_szHeader.cx) {
            // handle cells wider than window
            if (m_ptCorner.x < iNumCols - 1) {
                lCurPos = GetHeaderCell(++m_ptCorner.x).GetRect().left-GetLeftHeaderMargin();
            }
            else {
                lCurPos = si.nMax;
            }
        }
        else {
            lX=GetHeaderCell(m_ptCorner.x).GetRect().left+rcClient.Width()-m_szHeader.cx;
            bHit=false;
            while (m_ptCorner.x<iNumCols-1 && !bHit)  {
                bHit = (GetHeaderCell(m_ptCorner.x++).GetRect().right > lX);
                ASSERT(m_ptCorner.x>=0 && m_ptCorner.x < iNumCols);
            }
            lCurPos = GetHeaderCell(--m_ptCorner.x).GetRect().left-GetLeftHeaderMargin();
        }
        break;
    case SB_LEFT:
        lCurPos = (long)si.nMin;
        break;
    case SB_RIGHT:
        lCurPos = (long)si.nMax-(long)si.nPage;
        break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        if ((long)si.nTrackPos>=(long)si.nMax-(long)si.nPage)  {
            // special case, set to right side
            lCurPos=(long)si.nMax-si.nPage+1;
        }
        else  {
            lX = (long)si.nTrackPos + GetLeftHeaderMargin();
            bHit = false;
            if ((long)si.nTrackPos<lCurPos)  {
                // moving left
                while (m_ptCorner.x>0 && !bHit)  {
                    bHit = (GetHeaderCell(m_ptCorner.x--).GetRect().left < lX);
                }
                lCurPos = GetHeaderCell(++m_ptCorner.x).GetRect().left-GetLeftHeaderMargin();
            }
            else  {
                // moving right
                while (m_ptCorner.x<iNumCols-1 && !bHit)  {
                    bHit = (GetHeaderCell(m_ptCorner.x++).GetRect().right > lX);
                }
                lCurPos = GetHeaderCell(--m_ptCorner.x).GetRect().left-GetLeftHeaderMargin();
            }
        }
        break;
    case SB_ENDSCROLL:
        return;
        break;
    }

    ASSERT(m_ptCorner.x >0 && m_ptCorner.x < iNumCols);
    if (lCurPos<si.nMin)  {
        lCurPos=si.nMin;
        m_ptCorner.x = 1;
    }
    if (lCurPos>(long)si.nMax-(int)si.nPage)  {
        lCurPos=(long)si.nMax-si.nPage;
        m_ptCorner.x = GetMaxCorner().x;
    }
    ASSERT(m_ptCorner.x >0 && m_ptCorner.x < iNumCols);

    // turn off trackers (to be consistent with forms designer)
    DeselectTrackers();

    pScrollBar->SetScrollPos(lCurPos);
    int iDelta = lPrevPos-lCurPos;
    ScrollWindow(iDelta,0, m_rcScroll, m_rcScroll);
    InvalidateRect(CRect(m_rcScroll.right+iDelta, m_rcScroll.top, m_rcScroll.right, m_rcScroll.bottom));

    // scroll header
    CRect rcHeader(GetLeftHeaderMargin(),0,m_rcScroll.right,m_szHeader.cy);

    ScrollWindow(iDelta,0, rcHeader, rcHeader);
    InvalidateRect(CRect(rcHeader.right+iDelta, rcHeader.top, rcHeader.right, rcHeader.bottom));
    UpdateWindow();

    // reposition edit control, if active
    if (m_pEdit->GetSafeHwnd())  {
        CRect rc, rcTest;
        m_pEdit->GetWindowRect(&rc);
        rc.OffsetRect(iDelta,0);
        GetParent()->ScreenToClient(&rc);

        rcTest = CRect(GetLeftHeaderMargin(), m_szHeader.cy, rcClient.right - GetRightHeaderMargin(), rcClient.bottom);  // only include cell areas
        ClientToScreen(&rcTest);
        GetParent()->ScreenToClient(&rcTest);

        // hide the rect if outside of viewport area
        CRect rcIntersect;
        rcIntersect.IntersectRect(rc, rcTest);
        if (rcIntersect != rc)  {
            m_pEdit->ShowWindow(SW_HIDE);
        }
        else  {
            m_pEdit->ShowWindow(SW_SHOWNORMAL);
        }
        m_pEdit->MoveWindow(&rc);
        m_pEdit->RedrawWindow();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                                      CGridWnd::OnVScroll()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::OnVScroll(UINT nSBCode, UINT, CScrollBar* pScrollBar)
{
    long lCurPos, lPrevPos, lY;   // csc 1/6/04, changed int to long so we can accommodate very large grids (size >32767); see bug 1289
    int iNumRows;
    bool bHit;
    CRect rcClient;
    SCROLLINFO si;

    GetClientRect(&rcClient);
    si.cbSize=sizeof(SCROLLINFO);
    si.fMask=SIF_ALL;
    pScrollBar->GetScrollInfo(&si);
//    iCurPos = iPrevPos = pScrollBar->GetScrollPos();
    lCurPos = lPrevPos = (long)si.nPos;
    iNumRows=GetNumRows();

    switch (nSBCode)  {
    case SB_LINEUP:
        if (m_ptCorner.y>1)  {
            lCurPos = GetStubCell(--m_ptCorner.y).GetRect().top-m_szHeader.cy;
        }
        break;
    case SB_LINEDOWN:
        if (m_ptCorner.y<GetNumRows()-1)  {
            lCurPos = GetStubCell(++m_ptCorner.y).GetRect().top-m_szHeader.cy;
        }
        else {
            lCurPos = (long)si.nMax-(long)si.nPage+1;
        }
        break;
    case SB_PAGEUP:
        lY=GetStubCell(m_ptCorner.y).GetRect().bottom-rcClient.Height()+m_szHeader.cy;
        bHit=false;
        while (m_ptCorner.y>0 && !bHit)  {
            bHit = (GetStubCell(m_ptCorner.y--).GetRect().top < lY);
        }
        lCurPos = GetStubCell(++m_ptCorner.y).GetRect().top-m_szHeader.cy;
        break;
    case SB_PAGEDOWN:
        lY=GetStubCell(m_ptCorner.y).GetRect().top+rcClient.Height()-m_szHeader.cy;
        bHit=false;
        while (m_ptCorner.y<iNumRows-1 && !bHit)  {
            bHit = (GetStubCell(m_ptCorner.y++).GetRect().bottom > lY);
        }
        lCurPos = GetStubCell(--m_ptCorner.y).GetRect().top-m_szHeader.cy;
        break;
    case SB_TOP:
        lCurPos = (long)si.nMin;
        break;
    case SB_BOTTOM:
        lCurPos = (long)si.nMax-(long)si.nPage;
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        if ((long)si.nTrackPos>=(long)si.nMax-(long)si.nPage)  {
            // special case, set to bottom
            lCurPos=(long)si.nMax-si.nPage+1;
        }
        else  {
            lY = (long)si.nTrackPos + m_szHeader.cy;
            bHit = false;
            if ((long)si.nTrackPos<lCurPos)  {
                // moving up
                while (m_ptCorner.y>0 && !bHit)  {
                    bHit = (GetStubCell(m_ptCorner.y--).GetRect().top < lY);
                }
                lCurPos = GetStubCell(++m_ptCorner.y).GetRect().top-m_szHeader.cy;
            }
            else  {
                // moving down
                while (m_ptCorner.y<iNumRows-1 && !bHit)  {
                    bHit = (GetStubCell(m_ptCorner.y++).GetRect().bottom > lY);
                }
                lCurPos = GetStubCell(--m_ptCorner.y).GetRect().top-m_szHeader.cy;
            }
        }
        break;
    case SB_ENDSCROLL:
        return;
        break;
    }

    if (lCurPos<si.nMin)  {
        lCurPos=si.nMin;
        m_ptCorner.y = 1;
    }
    if (lCurPos>(long)si.nMax-(int)si.nPage)  {
        lCurPos = (long)si.nMax-si.nPage;
        m_ptCorner.y = GetMaxCorner().y;
    }

    // turn off trackers (to be consistent with forms designer)
    DeselectTrackers();

    pScrollBar->SetScrollPos(lCurPos);
    int iDelta = lPrevPos-lCurPos;
    ScrollWindow(0,iDelta,m_rcScroll, m_rcScroll);
    InvalidateRect(CRect(m_rcScroll.left, m_rcScroll.bottom+iDelta, m_rcScroll.right, m_rcScroll.bottom));

    // scroll stubs
    CRect rcStub;
    if (m_pRoster->GetRightToLeft()) {
        rcStub = CRect(m_rcScroll.right,m_szHeader.cy, m_rcScroll.right + m_szHeader.cx, m_rcScroll.bottom);
    }
    else {
        rcStub = CRect(0,m_szHeader.cy, m_szHeader.cx, m_rcScroll.bottom);
    }
    ScrollWindow(0,iDelta,rcStub,rcStub);
    InvalidateRect(CRect(rcStub.left, rcStub.bottom+iDelta, rcStub.right, rcStub.bottom));
    UpdateWindow();

    // reposition edit control, if active
    if (m_pEdit->GetSafeHwnd())  {
        CRect rc, rcTest;
        m_pEdit->GetWindowRect(&rc);
        rc.OffsetRect(0, iDelta);
        GetParent()->ScreenToClient(&rc);

        rcTest = CRect(m_szHeader.cx, m_szHeader.cy, rcClient.right, rcClient.bottom);  // only include cell areas
        ClientToScreen(&rcTest);
        GetParent()->ScreenToClient(&rcTest);

        // hide the rect if outside of viewport area
        CRect rcIntersect;
        rcIntersect.IntersectRect(rc, rcTest);
        if (rcIntersect != rc)  {
            m_pEdit->ShowWindow(SW_HIDE);
        }
        else  {
            m_pEdit->ShowWindow(SW_SHOWNORMAL);
        }
        m_pEdit->MoveWindow(&rc);
        m_pEdit->RedrawWindow();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                          CGridWnd::ScrollTo()
//
// puts the cell (iRow,iCol) into upper left corner (if possible)
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::ScrollTo(int iRow, int iCol)
{
    int iNumRows, iNumCols;
    CPoint ptCurPos, ptPrevPos;
    CRect rcClient;
    SCROLLINFO si;

    iNumRows = GetNumRows();
    iNumCols = GetNumCols();
    //    ASSERT(iRow>=1 && iRow<iNumRows && iCol>=1 && iCol<iNumCols);
    ASSERT(iRow>=1 && iRow<=iNumRows && iCol>=1 && iCol<=iNumCols);   // csc 2/14/01
    // iRow==iNumRows or iCol==iNumCols means scroll to max

    // prep
    GetClientRect(&rcClient);
    si.cbSize=sizeof(SCROLLINFO);
    si.fMask=SIF_ALL;
    m_ptCorner = CPoint(iCol, iRow);
    if (iCol==iNumCols)  {
        ptCurPos.x=GetHeaderCell(iCol-1).GetRect().right-GetLeftHeaderMargin();
    }
    else  {
        ptCurPos.x=GetHeaderCell(iCol).GetRect().left-GetLeftHeaderMargin();
    }
    if (iRow==iNumRows)  {
        ptCurPos.y=GetStubCell(iRow-1).GetRect().bottom-m_szHeader.cy;
    }
    else  {
        ptCurPos.y=GetStubCell(iRow).GetRect().top-m_szHeader.cy;
    }

    // process horizontal
    m_sbHorz.GetScrollInfo(&si);
    ptPrevPos.x = si.nPos;
    if (ptCurPos.x<si.nMin)  {
        ptCurPos.x=si.nMin;
        m_ptCorner.x=1;
    }
    if (ptCurPos.x>si.nMax-(int)si.nPage)  {
        ptCurPos.x=si.nMax-si.nPage;
        m_ptCorner.x=GetMaxCorner().x;
    }
    m_sbHorz.SetScrollPos(ptCurPos.x);

    // process vertical
    m_sbVert.GetScrollInfo(&si);
    ptPrevPos.y = si.nPos;
    if (ptCurPos.y<si.nMin)  {
        ptCurPos.y=si.nMin;
        m_ptCorner.y=1;
    }
    if (ptCurPos.y>si.nMax-(int)si.nPage)  {
        ptCurPos.y=si.nMax-si.nPage;
        m_ptCorner.y=GetMaxCorner().y;
    }
    m_sbVert.SetScrollPos(ptCurPos.y);

    if (ptCurPos != ptPrevPos)  {
        //        // turn off trackers (to be consistent with forms designer)
        //        DeselectTrackers();
        RedrawWindow();
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                          CGridWnd::EnsureVisible()
//
// ensures that the cell (iRow, iCol) is visible
// scrolls minimally
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::EnsureVisible(int iRow, int iCol)
{
/*
removed csc and savy 1/23/00
// do nothing if user has scrolled so that edit ctrl is outside of currently visible area...
if (m_pEdit->GetSafeHwnd() != NULL)  {
if (!m_pEdit->IsWindowVisible())  {
return;
}
}
    */

    int iNumRows, iNumCols;
    CRect rcClient, rcVisible;

    iNumRows = GetNumRows();
    iNumCols = GetNumCols();
    ASSERT(iRow>=1 && iRow<iNumRows && iCol>=1 && iCol<iNumCols);

    // prep
    GetClientRect(&rcClient);
    rcVisible = m_rcScroll;
    rcVisible.OffsetRect(GetScrollPos());

    CRect rcCell = GetCell(iRow, iCol).GetRect();

    // right justify cells for r to left
    rcCell.OffsetRect(CPoint(GetRightJustifyOffset(), 0));

    while (rcVisible.left > rcCell.left)  {
        // scroll left if needed
        m_bUpdateFlag = false;
        SendMessage(WM_HSCROLL, SB_LINELEFT, LPARAM(m_sbHorz.GetSafeHwnd()));
        rcVisible = m_rcScroll;
        rcVisible.OffsetRect(GetScrollPos());
    }
    if (rcCell.Width() < rcVisible.Width())  {
        // avoid special case of very wide cells
        while (rcVisible.right+2< rcCell.right)  {   // account for borders
            // scroll right if needed
            ASSERT(m_ptCorner.x<iNumCols-1);
            m_bUpdateFlag = false;
            SendMessage(WM_HSCROLL, SB_LINERIGHT, LPARAM(m_sbHorz.GetSafeHwnd()));
            rcVisible = m_rcScroll;
            rcVisible.OffsetRect(GetScrollPos());
        }
    }
    while (rcVisible.top > rcCell.top)  {
        // scroll up if needed
        ASSERT(m_ptCorner.y>1);
        m_bUpdateFlag = false;
        SendMessage(WM_VSCROLL, SB_LINEUP, LPARAM(m_sbVert.GetSafeHwnd()));
        rcVisible = m_rcScroll;
        rcVisible.OffsetRect(GetScrollPos());
    }
    if (rcCell.Height() < rcVisible.Height())  {
        // avoid special case of very high cells
        while (rcVisible.bottom+2 < rcCell.bottom)  {  // account for borders
            // scroll down if needed
            ASSERT(m_ptCorner.y<iNumRows-1);
            m_bUpdateFlag = false;
            SendMessage(WM_VSCROLL, SB_LINEDOWN, LPARAM(m_sbVert.GetSafeHwnd()));
            rcVisible = m_rcScroll;
            rcVisible.OffsetRect(GetScrollPos());
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                                      CGridWnd::GetMaxCorner()
//
// returns the upper left corner of the furthest scroll position possible
// (.x is cell column, .y is cell stub)
///////////////////////////////////////////////////////////////////////////////////////////////////
CPoint CGridWnd::GetMaxCorner(void) const
{
    CPoint ptRet(1,1);
    CRect rcClient;

    GetClientRect(&rcClient);

    //      int iMaxY=m_rcTotal.Height() - rcClient.Height() + m_szHeader.cy ;
    int iMaxY=m_rcTotal.Height() - rcClient.Height() + m_szHeader.cy - 2*::GetSystemMetrics(SM_CYBORDER);  // csc 1/3/01
    for (int iRow=GetNumRows()-1 ; iRow>=0 ; iRow--)  {
        const CGridCell& cell = GetStubCell(iRow);
        if (cell.GetRect().top<iMaxY)  {
            ptRet.y=iRow+1 ;
            break;
        }
    }

    //      int iMaxX=m_rcTotal.Width() - rcClient.Width() + m_szHeader.cx;
   // int iMaxX=m_rcTotal.Width() - rcClient.Width() + m_szHeader.cx - 2*::GetSystemMetrics(SM_CXBORDER);  // csc 1/3/01
    // JH 11/5/05 iMax needs to be same as scrollbar max, otherwise
    // you can set max corner outside max scroll and then get infinite loop
    // trying to scroll
    SCROLLINFO si;
    si.cbSize=sizeof(SCROLLINFO);
    si.fMask=SIF_ALL;
    const_cast<CGridWnd*>(this)->m_sbHorz.GetScrollInfo(&si); // Why is GetScrollInfo non-const!!!!!!
    int iMaxX= si.nMax - si.nPage;

    for (int iCol=GetNumCols()-1 ; iCol>=0 ; iCol--)  {
        const CGridCell& cell = GetHeaderCell(iCol);
        if (cell.GetRect().left<iMaxX)  {
            ptRet.x=iCol+1 ;
            break;
        }
    }

    // brute force fix for bug where this gets out of range if you scroll all way to right
    // and then page back.  JH 6/05.
    ptRet.x = std::min(ptRet.x, (LONG) GetNumCols()-1);
    ptRet.y = std::min(ptRet.y, (LONG) GetNumRows()-1);

    return ptRet;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                                      CGridWnd::HitTest()
//
// ptTest is a point to test in client coords
// hitOb is the object that was hit (has a point with cell coords, and an int that gives the field)
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
GRID_HITTEST CGridWnd::HitTest(const CPoint& ptTest, CHitOb& hitOb) const
{
    int iNumRows, iNumCols, iDelta1, iDelta2;
    bool bBorderDistance, bHorz, bVert;
    CPoint ptRet, ptOffset(ptTest + GetScrollPos());

    CRect rcClient;
    GetClientRect(&rcClient);

    // take into account right justification of roster in window for right to left
    if (m_pRoster->GetRightToLeft()) {
        const CRect& rcStub = GetCell(0,0).GetRect();
        if (rcClient.right >= rcStub.right) {
            ptOffset.x -= rcClient.right - rcStub.right;
        }
    }

    iNumRows=GetNumRows();
    iNumCols=GetNumCols();
    ptRet=CPoint(NONE,NONE);
    hitOb.SetCell(CPoint(NONE,NONE));
    hitOb.SetField(NONE);
    bBorderDistance = bHorz = bVert = false;

    // see if we hit a tracker
    for (int i = 0 ; i<GetNumTrackers() ; i++)  {
        const CGridRectTracker& t = GetTracker(i);
        if (t.HitTest(ptTest)!=CRectTracker::hitNothing)  {
            hitOb = t.GetHitOb();
            switch(t.GetTrackObject())  {
            case CGridRectTracker::trackField:
                return TRACK_FIELD;
            case CGridRectTracker::trackBox:
                return TRACK_BOX;
            case CGridRectTracker::trackText:
                return TRACK_TEXT;
            default:
                ASSERT(FALSE);
            }
        }
    }

    // test non-scrolling header row and stub col first (do not use offset for this) ...
    if (ptTest.y < m_szHeader.cy)  {
        ptRet.y=0;

        // see if we're close to a border
        iDelta1 = m_szHeader.cy - ptTest.y;
        //        iDelta2 = ptTest.y - GetStubCell(0).GetRect().top;
        iDelta2 = ptTest.y - const_cast<CGridWnd*>(this)->GetStubCell(0).GetRect().top;
        if (iDelta1<HITTEST_BORDER || iDelta2<HITTEST_BORDER)  {
            bHorz = bBorderDistance = true;
        }
    }

    if (m_pRoster->GetRightToLeft()) {
        // stub is on right
        int iStubLeft = rcClient.right - GetHeaderCell(0).GetRect().Width();
        if (ptTest.x > iStubLeft)  {
            ptRet.x=0;

            // see if we're close to a border
            iDelta1 = rcClient.right - ptTest.x;
            iDelta2 = ptTest.x - iStubLeft;
            if (iDelta1<HITTEST_BORDER || iDelta2<HITTEST_BORDER)  {
                bVert = bBorderDistance = true;
            }
        }
    }
    else {
        // stub is on left
        if (ptTest.x < m_szHeader.cx)  {
            ptRet.x=0;

            // see if we're close to a border
            iDelta1 = m_szHeader.cx - ptTest.x;
            //        iDelta2 = ptTest.x - GetHeaderCell(0).GetRect().left;
            iDelta2 = ptTest.x - const_cast<CGridWnd*>(this)->GetHeaderCell(0).GetRect().left;
            if (iDelta1<HITTEST_BORDER || iDelta2<HITTEST_BORDER)  {
                bVert = bBorderDistance = true;
            }
        }
    }

    // make sure we're within client area (up to scroll bars)
    if (ptTest.x>rcClient.right && ptTest.y>m_szHeader.cy && ptTest.y>rcClient.bottom && (m_pRoster->GetRightToLeft() || ptTest.x>m_szHeader.cx))  {
        // user clicked on small box between horz and vert scroll bars
        return HIT_CORNER;
    }
    if (ptTest.x>rcClient.right && ptTest.y>m_szHeader.cy)  {
        return NO_HIT;
    }
    if (ptTest.y>rcClient.bottom && (m_pRoster->GetRightToLeft() || ptTest.x>m_szHeader.cx))  {
        return NO_HIT;
    }

    // test scrollable cells (use offset here) ...
    if (ptRet.y==NONE)  {
        for (int iRow=1 ; iRow<iNumRows ; iRow++)  {
            const CGridCell& cell = const_cast<CGridWnd*>(this)->GetStubCell(iRow);   // avoid spurious calls to copy ctor
            if (ptOffset.y < cell.GetRect().bottom)  {
                ptRet.y=iRow;

                // see if we're close to a border
                //                iDelta1 = GetStubCell(iRow).GetRect().bottom - ptOffset.y;
                //                iDelta2 = ptOffset.y - GetStubCell(iRow).GetRect().top;
                iDelta1 = cell.GetRect().bottom - ptOffset.y;
                iDelta2 = ptOffset.y - cell.GetRect().top;
                ASSERT(iDelta1>=0);
                ASSERT(iDelta2>=0);
                if (iDelta1<HITTEST_BORDER || iDelta2<HITTEST_BORDER)  {
                    bHorz = bBorderDistance = true;
                }
                break;
            }
        }
    }
    if (ptRet.x==NONE)  {

        for (int iCol=1 ; iCol<iNumCols ; iCol++)  {
            const CGridCell& cell = const_cast<CGridWnd*>(this)->GetHeaderCell(iCol);   // avoid spurious calls to copy ctor
            if (ptOffset.x < cell.GetRect().right)  {

                if (iCol == 1 && m_pRoster->GetRightToLeft() && ptOffset.x < cell.GetRect().left) {
                    // with rt to left can have empty space to right of first cell
                    break;
                }
                ptRet.x=iCol;

                // see if we're close to a border
                //                iDelta1 = GetHeaderCell(iCol).GetRect().right - ptOffset.x;
                //                iDelta2 = ptOffset.x - GetHeaderCell(iCol).GetRect().left;
                iDelta1 = cell.GetRect().right - ptOffset.x;
                iDelta2 = ptOffset.x - cell.GetRect().left;
                ASSERT(iDelta1>=0);
                ASSERT(iDelta2>=0);
                if (iDelta1<HITTEST_BORDER || iDelta2<HITTEST_BORDER)  {
                    bVert = bBorderDistance = true;
                }
                break;
            }
        }
    }

    // process hit results and determine return code ...
    hitOb.SetCell(ptRet);
    if (bVert && bHorz)  {
        // handle case of being close to corner border as vertical
        bVert = bHorz = false;
    }
    if (ptRet.x>0 && ptRet.y>0)  {
        // hit a cell ... see if we hit a field too
        //        CGridCell cell = GetCell(ptRet.y, ptRet.x);
        const CGridCell& cell = const_cast<CGridWnd*>(this)->GetCell(ptRet.y, ptRet.x);   // avoid spurious calls to copy ctor

        for (int i = 0 ; i<cell.GetNumFields() ; i++)  {
            const CCellField& fld = cell.GetField(i);
            CRect rc = fld.GetDEField()->GetDims();
            rc.OffsetRect(cell.GetRect().TopLeft() + CPoint(GRIDCELL_BORDER,GRIDCELL_BORDER));
            if (rc.PtInRect(ptOffset))  {
                hitOb.SetField(i);
                return HIT_CELLFIELD;
            }
        }

        for( size_t i = 0; i<cell.GetNumTexts(); ++i ) {
            const CDEText& text = cell.GetText(i);
            CRect rc = text.GetDims();
            rc.OffsetRect(cell.GetRect().TopLeft() + CPoint(GRIDCELL_BORDER,GRIDCELL_BORDER));
            if (rc.PtInRect(ptOffset))  {
                hitOb.SetText(i);
                return HIT_CELLTEXT;
            }
        }

        for (size_t i = 0; i < cell.GetNumBoxes(); ++i ) {
            CRect rc = cell.GetBox(i).GetDims();
            rc.OffsetRect(cell.GetRect().TopLeft() + CPoint(GRIDCELL_BORDER,GRIDCELL_BORDER));
            if (rc.PtInRect(ptOffset))  {
                hitOb.SetBox(i);
                return HIT_CELLBOX;
            }
        }

        if (!bBorderDistance)  {
            return HIT_CELL;
        }
        else  {
            return (bHorz ? BORDER_CELLH : BORDER_CELLV);
        }
    }

    if (ptRet.x==0 && ptRet.y>0)  {
        if (!bBorderDistance)  {
            return HIT_STUB;
        }
        else  {
            return (bHorz ? BORDER_STUBH : BORDER_STUBV);
        }
    }

    if (ptRet.x>0 && ptRet.y==0)  {
        if (!bBorderDistance)  {
            return HIT_HEADER;
        }
        else  {
            return (bHorz ? BORDER_HEADERH : BORDER_HEADERV);
        }
    }

    if (ptRet.x==0 && ptRet.y==0)  {
        if (!bBorderDistance)  {
            return HIT_CORNER;
        }
        else  {
            return (bHorz ? BORDER_CORNERH : BORDER_CORNERV);
        }
    }

    return NO_HIT;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                          CGridWnd::OnGetMinMaxInfo()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
    ASSERT(GetNumCols()>1);
    ASSERT(GetNumRows()>1);
    CPoint pt = (CPoint) GetMinSize();
    lpMMI->ptMaxPosition=pt;
    lpMMI->ptMinTrackSize=pt;

    //      lpMMI->ptMaxPosition=CPoint(500,500);
    //      lpMMI->ptMinTrackSize=CPoint(30,30);
    CWnd::OnGetMinMaxInfo(lpMMI);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                          CGridWnd::Resize()
//
// note: 2 versions of this method
// rcNewPage is viewport size and position (relative to parent client); includes areas for scrollbar(s), if needed
// iRows, iCols give the number of rows/columns to show (use NONE to leave unchanged)
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::Resize(int iRows, int iCols, bool bTestOnly/*=false*/,  CRect* pTestRect/*=NULL*/)
{
    CRect rcNewPage;
    GetWindowRect(&rcNewPage);
    GetParent()->ScreenToClient(&rcNewPage);

    // adjust grid window based on number of rows/cols
    if (iRows!=NONE)  {
        ASSERT(iRows>1);
        ASSERT(iRows<=GetNumRows());
        rcNewPage.bottom = rcNewPage.top + GetStubCell(iRows-1).GetRect().bottom;
    }

    int rightColIndex = 0;
    if (iCols!=NONE)  {
        ASSERT(iCols>1);
        ASSERT(iCols<=GetNumCols());
        rightColIndex = m_pRoster->GetRightToLeft() ? 0 : iCols - 1;
        rcNewPage.right = rcNewPage.left + GetHeaderCell(rightColIndex).GetRect().right;
    }

    // see if we need to adjust more because of scroll bars
    if (iRows!=NONE)  {
        if (rcNewPage.Width()<GetHeaderCell(rightColIndex).GetRect().right)  {
            // horizontal scroll bar will be present
            rcNewPage.bottom += ::GetSystemMetrics(SM_CYHSCROLL);
        }
    }
    if (iCols!=NONE)  {
        if (rcNewPage.Height()<GetStubCell(GetNumRows()-1).GetRect().bottom)  {
            // vertical scroll bar will be present
            rcNewPage.right += ::GetSystemMetrics(SM_CXVSCROLL);
        }
    }
        //FABN Dec 9, 2002
    if(!bTestOnly){
        Resize(rcNewPage);
    } else {

        ASSERT(pTestRect);

        pTestRect->left     = rcNewPage.left;
        pTestRect->top      = rcNewPage.top;

        pTestRect->right    = rcNewPage.right;
        pTestRect->bottom   = rcNewPage.bottom;
    }

}


void CGridWnd::Resize(const CRect& rcNewPage)
{
    // prevent recursive calls ...
    static BOOL bResizing=FALSE;
    if (bResizing)  {
        return;
    }
    if (!m_bInitialized) {
        return;
    }

    bResizing=TRUE;

    bool bSBVert = (m_rcTotal.Height()>rcNewPage.Height());
    bool bSBHorz = (m_rcTotal.Width()>rcNewPage.Width());

    CSize szSB(::GetSystemMetrics(SM_CXVSCROLL), ::GetSystemMetrics(SM_CYHSCROLL));  // scrollbar sizes
    CSize szBorder(::GetSystemMetrics(SM_CXBORDER), ::GetSystemMetrics(SM_CYBORDER));  // border sizes

    if (bSBVert && !bSBHorz)  {
        bSBHorz = (m_rcTotal.Width()>rcNewPage.Width()-szSB.cx);
    }
    if (!bSBVert && bSBHorz)  {
        bSBVert = (m_rcTotal.Height()>rcNewPage.Height()-szSB.cy);
    }

    // calc sizes
    CRect rcsbHorz(0,0,0,0), rcsbVert(0,0,0,0), rcwndCorner(0,0,0,0);
    m_szHeader = CSize(GetStubCell(0).GetRect().Width(), GetHeaderCell(0).GetRect().Height());
    if (bSBHorz)  {
        rcsbHorz = CRect(m_szHeader.cx,0,rcNewPage.Width()-szBorder.cx-1, rcNewPage.Height()-szBorder.cy-1);
        if (m_pRoster->GetRightToLeft()) {
            rcsbHorz.left = 0;
        }
        rcsbHorz.top = rcsbHorz.bottom-szSB.cy;
        if (bSBVert)  {
            rcsbHorz.right -= szSB.cx;
        }
    }
    if (bSBVert)  {
        rcsbVert = CRect(0,m_szHeader.cy,rcNewPage.Width()-szBorder.cx-1, rcNewPage.Height()-szBorder.cy-1);
        rcsbVert.left = rcsbVert.right-szSB.cx;
        if (bSBHorz)  {
            rcsbVert.bottom -= szSB.cy;
        }
    }
    if (bSBVert && bSBHorz)  {
        rcwndCorner = CRect(rcsbVert.left, rcsbHorz.top, rcsbVert.right, rcsbHorz.bottom);
    }

    if (m_pRoster->GetRightToLeft()) {
        m_rcScroll = CRect(0, m_szHeader.cy, rcNewPage.Width()-m_szHeader.cx, rcNewPage.Height());
    }
    else {
        m_rcScroll = CRect(m_szHeader.cx, m_szHeader.cy, rcNewPage.Width(), rcNewPage.Height());
    }

    if (bSBVert) {
        m_rcScroll.right = rcsbVert.left;
        if (m_pRoster->GetRightToLeft()) {
            m_rcScroll.right -= m_szHeader.cx; //make room for stubs on right
        }
    }
    if (bSBHorz)  {
        m_rcScroll.bottom = rcsbHorz.top;
    }

    m_rcTotal.OffsetRect(rcNewPage.TopLeft() - m_rcTotal.TopLeft());

    // move the windows
    MoveWindow(&rcNewPage);
    m_sbHorz.MoveWindow(&rcsbHorz);
    m_sbVert.MoveWindow(&rcsbVert);
    m_wndCorner.MoveWindow(&rcwndCorner);

    // set scroll ranges, positions
    CRect rcClient;
    GetClientRect(&rcClient);

    SCROLLINFO si;
    si.cbSize=sizeof(SCROLLINFO);
    si.fMask=SIF_ALL;
    si.nMin=0;
    si.nTrackPos=0;

    si.nMax=m_rcTotal.Width()-rcNewPage.Width()+(bSBVert?szSB.cx:0)+rcClient.Width();
    si.nPage=rcClient.Width();
    si.nPos= 0;
    if (m_pRoster->GetRightToLeft()) {
        // set initial scroll pos all the way to the right for right to left reading order
        si.nPos = (long)si.nMax-si.nPage;
    }
    m_sbHorz.SetScrollInfo(&si);

    si.nMax=m_rcTotal.Height()-rcNewPage.Height()+(bSBHorz?szSB.cy:0)+rcClient.Height();
    si.nPage=rcClient.Height();
    si.nPos=0;
    m_sbVert.SetScrollInfo(&si);

    m_ptCorner=CPoint(1,1);
    if (m_pRoster->GetRightToLeft()) {
        // set initial scroll pos all the way to the right for right to left reading order
        m_ptCorner.x = GetMaxCorner().x;
    }
    GetRoster()->SetDims(rcNewPage+CPoint(GetParent()->GetScrollPos(SB_HORZ), GetParent()->GetScrollPos(SB_VERT)));

    // display scroll bars if appropriate
    m_sbHorz.ShowScrollBar(bSBHorz);
    m_sbVert.ShowScrollBar(bSBVert);
    m_wndCorner.ShowWindow((bSBHorz && bSBVert) ? SW_SHOWNORMAL : SW_HIDE);

    RedrawWindow();
    bResizing=FALSE;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                          CGridWnd::Move()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::Move(const CPoint& ptNewOrigin)
{
    // prevent recursive calls ...
    static BOOL bMoving=FALSE;
    if (bMoving)  {
        return;
    }
    if (!m_bInitialized) {
        return;
    }

    bMoving=TRUE;

    CPoint ptOffset;
    ptOffset = ptNewOrigin - m_rcTotal.TopLeft();
    m_rcTotal.OffsetRect(ptOffset);

    // move the grid
    CRect rc;
    GetWindowRect(&rc);
    GetParent()->ScreenToClient(&rc);
    rc.OffsetRect(ptOffset);
    MoveWindow(&rc);

    // move horizontal scroll bar
    m_sbHorz.GetWindowRect(&rc);
    ScreenToClient(&rc);
    m_sbHorz.MoveWindow(&rc);

    // move vertical scroll bar
    m_sbVert.GetWindowRect(&rc);
    ScreenToClient(&rc);
    m_sbVert.MoveWindow(&rc);

    // move corner (dead-space) window
    m_wndCorner.GetWindowRect(&rc);
    ScreenToClient(&rc);
    m_wndCorner.MoveWindow(&rc);

    // update roster coords
    GetRoster()->SetDims(m_rcTotal+CPoint(GetParent()->GetScrollPos(SB_HORZ), GetParent()->GetScrollPos(SB_VERT)));

    RedrawWindow();
    bMoving=FALSE;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::SelectColumn()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
/*V*/ void CGridWnd::SelectColumn(int iCol, bool bMulti /*=false*/, bool bRange /*=false*/, bool bRedraw /*=true*/, COLORREF rgbBSel /*=rgbBSelDefault*/, COLORREF rgbFSel /*=rgbFSelDefault*/)
{
    ASSERT(iCol!=NONE);
    ASSERT(iCol>=0 && iCol<GetNumCols());

    int i, iRow, iNumRows=GetNumRows();

    DeselectRows();
    DeselectCells();
    DeselectTrackers();
    if (!bMulti && !bRange)  {
        // no multiple selection
        DeselectColumns();
    }

    // always select the column indicated by iCol
    bool bSelect = IsColSelected(iCol);

    GetHeaderCell(iCol).Select(!bSelect);
    RedrawCell(0,iCol);

    for (iRow=1 ; iRow<iNumRows ; iRow++)  {
        CGridCell& cell = GetCell(iRow, iCol);
        cell.SetBSelColor(rgbBSel);
        cell.SetFSelColor(rgbFSel);
        cell.Select(!bSelect);
        for (i = 0 ; i<cell.GetNumFields() ; i++)  {
            CCellField& fld = cell.GetField(i);
            fld.SetBSelColor(rgbBSel);
            fld.SetFSelColor(rgbFSel);
            fld.Select(!bSelect);
        }
        RedrawCell(iRow, iCol);
    }
    if (!IsColSelected(iCol))  {
        m_aiSelCol.Add(iCol);
    }
    else  {
        for (i = 0 ; i<m_aiSelCol.GetSize() ; i++)  {
            if (m_aiSelCol[i]==iCol)  {
                m_aiSelCol.RemoveAt(i--);
            }
        }
    }

    if (bRange && m_iActiveCol!=NONE)  {
        // need to select all columns between m_iActiveCol and iCol...
        int j, iFrom, iTo;
        iFrom = __min(m_iActiveCol, iCol);
        iTo = __max(m_iActiveCol, iCol);
        for (i=iFrom ; i<=iTo ; i++)  {
            GetHeaderCell(i).Select();
            RedrawCell(0,i);
            for (iRow=1 ; iRow<iNumRows ; iRow++)  {
                CGridCell& cell = GetCell(iRow, i);
                cell.SetBSelColor(rgbBSel);
                cell.SetFSelColor(rgbFSel);
                cell.Select();
                for (j=0 ; j<cell.GetNumFields() ; j++)  {
                    CCellField& fld = cell.GetField(j);
                    fld.SetBSelColor(rgbBSel);
                    fld.SetFSelColor(rgbFSel);
                    fld.Select();
                }
                RedrawCell(iRow, i);
            }
            if (!IsColSelected(i))  {
                m_aiSelCol.Add(i);
            }
        }
    }

    m_iActiveCol=iCol;
    if (bRedraw)  {
        UpdateWindow();
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::DeselectColumns()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::DeselectColumns(void)
{
    if (m_iActiveCol == NONE)  {
        ASSERT(m_aiSelCol.GetSize()==0);
        return;
    }

    int iRow, iCol;
    const int iNumRows=GetNumRows();
    const int iNumCols = GetNumCols();
    for (int i = 0 ; i<m_aiSelCol.GetSize() ; i++)  {
        iCol = m_aiSelCol[i];
        if (iCol < iNumCols) { // after a join, you can have sel cols that no longer exist JH 6/4/07
            GetHeaderCell(iCol).Select(false);
            RedrawCell(0,iCol);
            for (iRow=1 ; iRow<iNumRows ; iRow++)  {
                if (!IsRowSelected(iRow) && !IsCellSelected(iRow, iCol))  {
                    CGridCell& cell = GetCell(iRow, iCol);
                    cell.Select(false);
                    for (int j=0 ; j<cell.GetNumFields() ; j++)  {
                        cell.GetField(j).Select(false);
                    }
                }
                RedrawCell(iRow, iCol);
            }
        }
    }
    m_aiSelCol.RemoveAll();
    m_iActiveCol = NONE;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::SelectRow()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::SelectRow(int iRow, bool bMulti /*=false*/, bool bRange /*=false*/, bool bRedraw /*=true*/, COLORREF rgbBSel /*=rgbBSelDefault*/, COLORREF rgbFSel /*=rgbFSelDefault*/)
{
    ASSERT(iRow!=NONE);
    ASSERT(iRow>=0 && iRow<GetNumRows());

    int iNumCols = GetNumCols();

    DeselectColumns();
    DeselectCells();
    DeselectTrackers();
    if (!bMulti && !bRange)  {
        // no multiple selection
        DeselectRows();
    }

    // always select the row indicated by iRow
    bool bSelect = IsRowSelected(iRow);

    GetStubCell(iRow).Select(!bSelect);
    RedrawCell(iRow,0);

    int iCol = 1;
    for (; iCol<iNumCols ; iCol++)  {
        CGridCell& cell = GetCell(iRow, iCol);
        cell.SetBSelColor(rgbBSel);
        cell.SetFSelColor(rgbFSel);
        cell.Select(!bSelect);
        for (int i = 0 ; i<cell.GetNumFields() ; i++)  {
            CCellField& fld = cell.GetField(i);
            fld.SetBSelColor(rgbBSel);
            fld.SetFSelColor(rgbFSel);
            fld.Select(!bSelect);
        }
        RedrawCell(iRow, iCol);
    }
    if (!IsRowSelected(iRow))  {
        m_aiSelRow.Add(iRow);
    }
    else  {
        for (int i = 0 ; i<m_aiSelCol.GetSize() ; i++)  {
            if (m_aiSelCol[i]==iCol)  {
                m_aiSelCol.RemoveAt(i--);
            }
        }
    }

    if (bRange && m_iActiveRow!=NONE)  {
        // need to select all columns between m_iActiveCol and iCol...
        int i, j, iFrom, iTo;
        iFrom = __min(m_iActiveRow, iRow);
        iTo = __max(m_iActiveRow, iRow);
        for (i=iFrom ; i<=iTo ; i++)  {
            GetStubCell(i).Select();
            RedrawCell(i,0);
            for (iCol=1 ; iCol<iNumCols ; iCol++)  {
                CGridCell& cell = GetCell(i, iCol);
                cell.SetBSelColor(rgbBSel);
                cell.SetFSelColor(rgbFSel);
                cell.Select();
                for (j=0 ; j<cell.GetNumFields() ; j++)  {
                    CCellField& fld = cell.GetField(j);
                    fld.SetBSelColor(rgbBSel);
                    fld.SetFSelColor(rgbFSel);
                    fld.Select();
                }
                RedrawCell(i, iCol);
            }
            if (!IsRowSelected(i))  {
                m_aiSelRow.Add(i);
            }
        }
    }

    m_iActiveRow=iRow;
    if (bRedraw)  {
        UpdateWindow();
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::IsColSelected()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGridWnd::IsColSelected(int iCol) const
{
    for (int i = 0 ; i<m_aiSelCol.GetSize() ; i++)  {
        if (m_aiSelCol[i]==iCol)  {
            return true;
        }
    }
    return false;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::IsRowSelected()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGridWnd::IsRowSelected(int iRow) const
{
    for (int i = 0 ; i<m_aiSelRow.GetSize() ; i++)  {
        if (m_aiSelRow[i]==iRow)  {
            return true;
        }
    }
    return false;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::IsCellSelected()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGridWnd::IsCellSelected(int iRow, int iCol) const
{
    for (int i = 0 ; i<m_aptSelCell.GetSize() ; i++)  {
        const CPoint& pt=m_aptSelCell[i];
        if (pt.x==iCol && pt.y==iRow)  {
            return true;
        }
    }
    return false;
}

bool CGridWnd::IsCellSelected(const CPoint& ptCell) const
{
    return IsCellSelected(ptCell.y, ptCell.x);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::IsFieldSelected()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGridWnd::IsFieldSelected(int iRow, int iCol, int iFld) const
{
    for (int i = 0 ; i<GetNumTrackers() ; i++)  {
        const CHitOb& h = GetTracker(i).GetHitOb();
        if (h.GetCell()==CPoint(iCol, iRow) && h.GetField()==iFld)  {
            return true;
        }
    }
    return false;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::IsBoxSelected()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGridWnd::IsBoxSelected(int iRow, int iCol, int iBox) const
{
    for (int i = 0 ; i<GetNumTrackers() ; i++)  {
        const CHitOb& h = GetTracker(i).GetHitOb();
        if (h.GetCell()==CPoint(iCol, iRow) && h.GetBox()==iBox)  {
            return true;
        }
    }
    return false;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::IsTextSelected()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGridWnd::IsTextSelected(int iRow, int iCol, int iTxt) const
{
    for (int i = 0 ; i<GetNumTrackers() ; i++)  {
        const CHitOb& h = GetTracker(i).GetHitOb();
        if (h.GetCell()==CPoint(iCol, iRow) && h.GetText()==iTxt)  {
            return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::GetNumTrackers()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
int CGridWnd::GetNumTrackers(CGridRectTracker::TrackObject trackOb) const
{
    int i, iCount=0;
    for (i = 0 ; i<GetNumTrackers() ; i++)  {
        if (GetTracker(i).GetTrackObject()==trackOb)  {
            iCount++;
        }
    }
    return iCount;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::DeselectRows()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::DeselectRows(void)
{
    if (m_iActiveRow == NONE)  {
        ASSERT(m_aiSelRow.GetSize()==0);
        return;
    }

    int iRow, iCol;
    const int iNumRows=GetNumRows();
    const int iNumCols = GetNumCols();

    for (int i = 0 ; i<m_aiSelRow.GetSize() ; i++)  {
        iRow = m_aiSelRow[i];

        if( iRow < iNumRows ) // GHM 20111115, copied this: after a join, you can have sel rows that no longer exist JH 6/4/07
        {
            GetStubCell(iRow).Select(false);
            RedrawCell(iRow,0);
            for (iCol=1 ; iCol<iNumCols ; iCol++)  {
                if (!IsColSelected(iCol) && !IsCellSelected(iRow, iCol))  {
                    CGridCell& cell = GetCell(iRow, iCol);
                    cell.Select(false);
                    for (int j=0 ; j<cell.GetNumFields() ; j++)  {
                        cell.GetField(j).Select(false);
                    }
                }
                RedrawCell(iRow, iCol);
            }
        }
    }
    m_aiSelRow.RemoveAll();
    m_iActiveRow = NONE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::SelectCell()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::SelectCell(int iRow, int iCol, bool bMulti /*=false*/, bool bRange /*=false*/, bool bRedraw /*=true*/, COLORREF rgbBSel /*=rgbBSelDefault*/, COLORREF rgbFSel /*=rgbFSelDefault*/)
{
    ASSERT(iCol!=NONE);
    ASSERT(iCol>=0 && iCol<GetNumCols());
    ASSERT(iRow!=NONE);
    ASSERT(iRow>=0 && iRow<GetNumRows());

    CPoint pt(iCol, iRow);

    DeselectColumns();
    DeselectRows();
    DeselectTrackers();
    if (!bMulti && !bRange)  {
        // no multiple selection
        DeselectCells();
    }

    // always select the cell indicated by iRow/iCol
    bool bSelect = IsCellSelected(iRow,iCol);
    CGridCell& cell = GetCell(iRow, iCol);
    if (iCol>0 && iRow>0)  {
        cell.SetBSelColor(rgbBSel);
        cell.SetFSelColor(rgbFSel);
    }
    cell.Select(!bSelect);
    for (int i = 0 ; i<cell.GetNumFields() ; i++)  {
        CCellField& fld = cell.GetField(i);
        fld.SetBSelColor(rgbBSel);
        fld.SetFSelColor(rgbFSel);
        fld.Select(!bSelect);
    }
    RedrawCell(iRow, iCol);

    if (!IsCellSelected(pt))  {
        m_aptSelCell.Add(pt);
    }
    else  {
        for (int i = 0 ; i<m_aptSelCell.GetSize() ; i++)  {
            if (m_aptSelCell[i]==pt)  {
                m_aptSelCell.RemoveAt(i--);
            }
        }
    }


    if (bRange && m_ptActiveCell!=CPoint(NONE,NONE))  {
        // need to select all cells within rectangle formed by m_ptActiveCell and CPoint(iRow,iCol)...
        int i, j, k, iFromRow, iFromCol, iToRow, iToCol;
        iFromRow = __min(m_ptActiveCell.y, iRow);
        iFromCol = __min(m_ptActiveCell.x, iCol);
        iToRow = __max(m_ptActiveCell.y, iRow);
        iToCol = __max(m_ptActiveCell.x, iCol);
        for (i=iFromRow ; i<=iToRow ; i++)  {
            for (j=iFromCol ; j<=iToCol ; j++)  {
                CGridCell& c = GetCell(i,j);
                if (i>0 && j>0)  {
                    c.SetBSelColor(rgbBSel);
                    c.SetFSelColor(rgbFSel);
                }
                c.Select();
                for (k=0 ; k<c.GetNumFields() ; k++)  {
                    CCellField& fld = c.GetField(k);
                    fld.SetBSelColor(rgbBSel);
                    fld.SetFSelColor(rgbFSel);
                    fld.Select();
                }
                RedrawCell(i,j);
                if (!IsCellSelected(i,j))  {
                    CPoint ptNew(j,i);
                    m_aptSelCell.Add(ptNew);
                }
            }
        }
    }

    m_ptActiveCell=pt;
    if (bRedraw)  {
        UpdateWindow();
    }
}

void CGridWnd::SelectCell(const CPoint& ptCell, bool bMulti /*=false*/, bool bRange /*=false*/, bool bRedraw /*=true*/, COLORREF rgbBSel /*=rgbBSelDefault*/, COLORREF rgbFSel /*=rgbFSelDefault*/)
{
    SelectCell(ptCell.y, ptCell.x, bMulti, bRange, bRedraw, rgbBSel, rgbFSel);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::DeselectCells()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::DeselectCells(void)
{
    if (m_ptActiveCell == CPoint(NONE,NONE))  {
        ASSERT(m_aptSelCell.GetSize()==0);
        return;
    }

    for (int i = 0 ; i<m_aptSelCell.GetSize() ; i++)  {
        const CPoint& pt = m_aptSelCell[i];
        if (!IsColSelected(pt.x) && !IsRowSelected(pt.y))  {
            CGridCell& cell = GetCell(pt.y, pt.x);
            cell.Select(false);
            for (int j=0 ; j<cell.GetNumFields() ; j++)  {
                cell.GetField(j).Select(false);
            }
        }
        RedrawCell(pt.y, pt.x);
    }
    m_aptSelCell.RemoveAll();
    m_ptActiveCell=CPoint(NONE,NONE);
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::DeselectTracker()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::DeselectTracker(int i)  {
    CRect rc;
    ASSERT(i>=0 && i<GetNumTrackers());
    GetTracker(i).GetTrueRect(&rc);
    InvalidateRect(&rc);
    RemoveTrackerAt(i);
    UpdateWindow();
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::DeselectTrackers()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::DeselectTrackers(void)  {
    CRect rc;
    for (int i = 0 ; i<GetNumTrackers() ; i++)  {
        GetTracker(i).GetTrueRect(&rc);
        InvalidateRect(&rc);
    }
    RemoveAllTrackers();
    m_iActiveField = NONE; //SAVY added this to fix
    //Glenn's bug :Multiple selection delete failed after rb click properties of field in a a cell.
    UpdateWindow();
}




///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::UnionTrackers()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
CRect CGridWnd::UnionTrackers(void) const {
    CRect rc;
    rc.SetRectEmpty();
    for (int i = 0 ; i<GetNumTrackers() ; i++)  {
        rc.UnionRect(rc, GetTracker(i).m_rect);
    }
    return rc;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::SelectFldTxtBox
//
// selects all occurrences of this field in the grid, and puts up a tracker for movement
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::SelectFldTxtBox(int iRow, int iCol, int iOb, CGridRectTracker::TrackObject trackOb, bool bMulti /*=false*/, bool bNoTrack /*=false*/)
{
    bool bNewTextBox = false;
    bool bMultiLineTextBox = false;

    int i,iNumCols=GetNumCols();
    int iNumRows=GetNumRows();
    CGridRectTracker t;

    ASSERT(iCol!=NONE);
    ASSERT(iCol>=0 && iCol<iNumCols);
    ASSERT(iRow!=NONE);
    ASSERT(iRow>=0 && iRow<iNumRows);
    ASSERT(iOb>=0);
    ASSERT(trackOb==CGridRectTracker::trackField || trackOb==CGridRectTracker::trackBox || trackOb==CGridRectTracker::trackText);
    CGridCell& cell = GetCell(iRow, iCol);

#ifdef _DEBUG
    if (trackOb==CGridRectTracker::trackField)  {
        ASSERT(iOb<cell.GetNumFields());
    }
    else if (trackOb==CGridRectTracker::trackBox)  {
        ASSERT(iOb<(int)cell.GetNumBoxes());
    }
    else  {
        ASSERT(iOb<(int)cell.GetNumTexts());
    }
#endif

    CPoint pt;
    GetCursorPos(&pt);
    ScreenToClient(&pt);

    // deselect other stuff
    DeselectRows();
    DeselectCells();
    DeselectColumns();

    // logic for multiple selections
    bool bWasPreviouslySelected = false;
    int iPreviousIndex=NONE;
    for (i = 0 ; i<GetNumTrackers() ; i++)  {
        CGridRectTracker& tDeselect = GetTracker(i);
        if (tDeselect.GetHitOb().GetCell() == CPoint(iCol, iRow))  {
            if (tDeselect.GetTrackObject()==trackOb)  {
                if ((trackOb==CGridRectTracker::trackField && tDeselect.GetHitOb().GetField()==iOb) || (trackOb==CGridRectTracker::trackBox && tDeselect.GetHitOb().GetBox()==iOb) || (trackOb==CGridRectTracker::trackText && tDeselect.GetHitOb().GetText()==iOb))  {
                    bWasPreviouslySelected = true;
                    iPreviousIndex = i;
                    break;
                }
            }
        }
    }

    for (i = 0 ; i<GetNumTrackers() ; i++)  {
        CGridRectTracker& tDeselect = GetTracker(i);
        if (tDeselect.GetHitOb().GetCell() != CPoint(iCol, iRow))  {
            // always deselect trackers that aren't in our cell
            DeselectTracker(i--);
        }
        else  {
            if (!bMulti)  {
                if (!bWasPreviouslySelected)  {
                    DeselectTracker(i--);
                }
            }
            else  {
                if (bWasPreviouslySelected && i==iPreviousIndex)  {
                    // in this case the user is turn off a tracker; remove it and exit
                    DeselectTracker(i--);
                    return;
                }
            }
        }
    }
    bool bClearOthersIfNoMovement=(bWasPreviouslySelected && !bMulti);

    EnsureVisible(iRow, iCol);

    // let the tracker know about the bounding cell rect
    CRect rcObject1, rcCell;
    CHitOb hitOb;
    hitOb.SetCell(CPoint(iCol,iRow));
    rcCell = cell.GetRect();
    rcCell.DeflateRect(GRIDCELL_BORDER, GRIDCELL_BORDER);
    rcCell.OffsetRect(-GetScrollPos().x+GetRightJustifyOffset(), -GetScrollPos().y);
    t.SetBoundingRect(rcCell);

    // prep tracker based on obj characteristics
    t.RemoveNoOverlapRects();

    if (trackOb==CGridRectTracker::trackField)  {
        // fields ... let the tracker know about other fields in the cell
        for (i = 0 ; i<cell.GetNumFields() ; i++)  {
            CCellField& fld = cell.GetField(i);
            rcObject1 = fld.GetDEField()->GetDims();
            rcObject1.OffsetRect(rcCell.TopLeft());
            if (i==iOb)  {
                t.m_rect = rcObject1;
                bNewTextBox = fld.GetDEField()->UseUnicodeTextBox() && fld.GetDEField()->GetDictItem()->GetContentType() == ContentType::Alpha;
                bMultiLineTextBox =  fld.GetDEField()->AllowMultiLine() && fld.GetDEField()->UseUnicodeTextBox() && fld.GetDEField()->GetDictItem()->GetContentType() == ContentType::Alpha;
                if(bNewTextBox){
                    t.SetSizingOverrideFlag(true);
                }
            }
            else  {
                t.AddNoOverlapRect(rcObject1);
            }
        }
        hitOb.SetField(iOb);
        if(bNewTextBox){
            t.m_nStyle = CRectTracker::hatchedBorder | CRectTracker::resizeOutside;
        }
        else{
            t.m_nStyle = CRectTracker::hatchedBorder;
        }
    }

    else if (trackOb==CGridRectTracker::trackBox)  {
        // boxes
        rcObject1 = cell.GetBox(iOb).GetDims();
        rcObject1.OffsetRect(rcCell.TopLeft());
        t.m_rect = rcObject1;
        hitOb.SetBox(iOb);
        t.m_nStyle = CRectTracker::hatchedBorder | CRectTracker::resizeOutside;
        t.m_sizeMin = CSize(10,10);
    }

    else if (trackOb==CGridRectTracker::trackText)  {
        // texts
        rcObject1 = cell.GetText(iOb).GetDims();
        rcObject1.OffsetRect(rcCell.TopLeft());
        t.m_rect = rcObject1;
        hitOb.SetText(iOb);
        t.m_nStyle = CRectTracker::hatchedBorder | CRectTracker::resizeOutside;
        t.m_sizeMin = CSize(10,10);
    }

    else {
        ASSERT(FALSE);
    }

    t.SetHitOb(hitOb);
    t.SetTrackObject(trackOb);
    t.GetTrueRect(&rcObject1);

    // see if this field is already being tracked...
    bool bFound=false;
    for (i = 0 ; i<GetNumTrackers() && !bFound ; i++)  {
        if (GetTracker(i).GetHitOb() == hitOb)  {
            bFound=true;
        }
    }
    if (!bFound)  {
        if(bNewTextBox){
            t.SetSizingOverrideFlag(true);
        }
        AddTracker(t);
    }

    // fix up display
    InvalidateRect(&rcObject1);
    UpdateWindow();

    // process tracking (won't let the user drag outside of the current cell)
    CRect rcUnion = UnionTrackers();
    CGridRectTracker trackUnion(rcUnion, CRectTracker::solidLine);
    trackUnion.SetBoundingRect(rcCell);
    for (i = 0 ; i<t.GetNumNoOverlapRects() ; i++)  {
        trackUnion.AddNoOverlapRect(t.GetNoOverlapRect(i));
    }
    CHitOb hitUnion;
    hitUnion.SetCell(CPoint(iCol, iRow));
    if (GetNumTrackers()==1)  {
        // just tracking the object solo; allow for special properties (fields won't overlap, texts/boxes can be resized) ...
        trackUnion.SetTrackObject(t.GetTrackObject());
        trackUnion.m_nStyle=t.m_nStyle;
        hitUnion.SetField(hitOb.GetField());
        hitUnion.SetBox(hitOb.GetBox());
        hitUnion.SetText(hitOb.GetText());
        ASSERT(trackUnion.m_rect==t.m_rect);
    }
    else  {
        trackUnion.SetTrackObject(CGridRectTracker::trackMultiple);
        trackUnion.m_nStyle = CRectTracker::hatchedBorder;
    }
    trackUnion.SetHitOb(hitUnion);

    // do the tracking
    bool bTrackedOK = false;
    if (!bNoTrack)  {
        trackUnion.SetFirstTime();  // regain virginity
        if(bNewTextBox){
            trackUnion.SetSizingOverrideFlag(true);
        }
        bTrackedOK = (trackUnion.Track(this,pt)==0?false:true);
    }
    if (bTrackedOK)  {
        // apply changes to this field (CDEField*), then rebuild to cause the changes to other rows
        CRect rcEnd = trackUnion.m_rect;
        CPoint ptOffset = rcEnd.TopLeft() - rcUnion.TopLeft();

        if (trackUnion.GetTrackObject()==CGridRectTracker::trackField && !bNewTextBox)  {
            CRect rcTest;
            trackUnion.GetTrueRect(&rcTest);
            if (rcObject1.Size()!=rcTest.Size())  {
                // bug: for some reason, you can sometimes resize a field.  Make sure that field dims correspond to dictionary...
                trackUnion.m_rect.BottomRight() = trackUnion.m_rect.TopLeft() + cell.GetField(iOb).GetDEField()->GetDims().Size();
                rcEnd = trackUnion.m_rect;

            }
            ASSERT(rcUnion.Size()==rcEnd.Size());
        }

        CPoint ptCorner(m_ptCorner);

        // possibly resize objects
        if (GetNumTrackers()==1)  {
            // user might have resized if just 1 tracker (text and box only; not fields!)
            switch(t.GetTrackObject())  {
            case CGridRectTracker::trackText:
            case CGridRectTracker::trackBox:
                GetTracker(0).m_rect = rcEnd - ptOffset;  // this handles resize, movement comes in switch stmt below
                break;
            case CGridRectTracker::trackField:
                if(bNewTextBox){

                    GetTracker(0).m_rect = rcEnd - ptOffset;  // this handles resize, movement comes in switch stmt below
                }
                break;
            }
        }

        // move all objs being tracked...
        for (i = 0 ; i<GetNumTrackers() ; i++)  {
            CGridRectTracker& tMove = GetTracker(i);
            CRect rcObject2 = tMove.m_rect;
            rcObject2.OffsetRect(-rcCell.TopLeft());
            rcObject2.OffsetRect(ptOffset);

            switch (tMove.GetTrackObject())  {
            case CGridRectTracker::trackField:
                ASSERT(tMove.GetHitOb().GetField()>=0 && tMove.GetHitOb().GetField()<cell.GetNumFields());
                if(bNewTextBox && !bMultiLineTextBox){//do not change the height
                    CRect rectField = cell.GetField(tMove.GetHitOb().GetField()).GetDEField()->GetDims();
                    rcObject2.bottom = rectField.bottom;
                    cell.GetField(tMove.GetHitOb().GetField()).GetDEField()->SetDims(rcObject2);
                }
                else {
                    //Do not change the height if it is less than the min height
                        CDEField* pFld = cell.GetField(tMove.GetHitOb().GetField()).GetDEField();
                        CRect currRect = pFld->GetDims();
                        if(m_iTextBoxHeightMin != -1  && rcObject2.Height() < m_iTextBoxHeightMin){ //Field has been resized to less than the min height
                            //Set the height back to the min
                            CRect newRect(rcObject2);
                            if(newRect.top != currRect.top) {//user sized from the top . Make the height back to min
                                newRect.top -= m_iTextBoxHeightMin - rcObject2.Height();
                            }
                            if(newRect.bottom != currRect.bottom) {//user sized from the bottom . Make the height back to min
                                newRect.bottom += m_iTextBoxHeightMin - rcObject2.Height();
                            }
                            pFld->SetDims(newRect);
                        }
                        else {
                            cell.GetField(tMove.GetHitOb().GetField()).GetDEField()->SetDims(rcObject2);
                        }
                }
                OnFieldMoved(iRow,iCol,iOb);
                break;

            case CGridRectTracker::trackBox:
                ASSERT(tMove.GetHitOb().GetBox()>=0 && tMove.GetHitOb().GetBox()<(int)cell.GetNumBoxes());
                cell.GetBox(tMove.GetHitOb().GetBox()).SetDims(rcObject2);
                OnBoxMoved(iRow,iCol,iOb);
                break;

            case CGridRectTracker::trackText:
                ASSERT(tMove.GetHitOb().GetText()>=0 && tMove.GetHitOb().GetText()<(int)cell.GetNumTexts());
                cell.GetText(tMove.GetHitOb().GetText()).SetDims(rcObject2);
                OnTextMoved(iRow,iCol,iOb);
                break;

            default:
                ASSERT(FALSE);
            }
        }

        // calc min sizes for all cells down this column and across this row ...  csc 12/27/00
        CClientDC dc(this);
        dc.SetMapMode(MM_TEXT);
        //        GetHeaderCell(iCol).CalcMinSize(&dc);
        //        GetStubCell(iRow).CalcMinSize(&dc);
        CSize szFieldFontTextExt(NONE, NONE);
        for (i = 0 ; i<iNumCols ; i++)  {
            GetCell(iRow,i).CalcMinSize(&dc, szFieldFontTextExt);
        }
        for (i = 0 ; i<iNumRows ; i++)  {
            GetCell(i,iCol).CalcMinSize(&dc, szFieldFontTextExt);
        }

        GetHeaderCell(iCol).CalcMinSize(&dc, szFieldFontTextExt);
        GetStubCell(iRow).CalcMinSize(&dc, szFieldFontTextExt);

        RecalcLayout();
        ScrollTo(ptCorner.y, ptCorner.x);

        // special case with text: need to reset tracker size, since the CRect for the text
        // object will be shrunk to "just fix" the text ...
        if (t.GetTrackObject()==CGridRectTracker::trackText && GetNumTrackers()==1)  {
            CRect rcDraw;
            t.GetTrueRect(&rcDraw);
            rcObject1.OffsetRect(rcCell.TopLeft());
            t.m_rect = rcObject1;
            t.GetTrueRect(&rcObject1);
            rcDraw.UnionRect(rcDraw, rcObject1);
            InvalidateRect(rcDraw);
            UpdateWindow();
        }
    }
    else  {
        // tracking was not successful; the user probably just clicked w/o dragging the mouse
        if (bClearOthersIfNoMovement)  {
            for (int it=0 ; it<GetNumTrackers() ; it++)  {
                CGridRectTracker& tDeselect = GetTracker(it);
                ASSERT(tDeselect.GetHitOb().GetCell() == CPoint(iCol, iRow));
                if (tDeselect.GetTrackObject()==trackOb)  {
                    if (trackOb==CGridRectTracker::trackField && tDeselect.GetHitOb().GetField()==iOb)  {
                        continue;
                    }
                    if (trackOb==CGridRectTracker::trackBox && tDeselect.GetHitOb().GetBox()==iOb)  {
                        continue;
                    }
                    if (trackOb==CGridRectTracker::trackText && tDeselect.GetHitOb().GetText()==iOb)  {
                        continue;
                    }
                }
                DeselectTracker(it--);
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::Deselect()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::Deselect(void)
{
    DeselectTrackers();
    DeselectColumns();
    DeselectRows();
    DeselectCells();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::OnKeyDown()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    bool bProcessed=true;

    switch(nChar)  {
    case VK_UP:
        if (IsTrackerActive())  {
            MoveTrackerObject(VK_UP);
        }
        else  {
            SendMessage(WM_VSCROLL, SB_LINEUP, LPARAM(m_sbVert.GetSafeHwnd()));
        }
        break;
    case VK_DOWN:
        if (IsTrackerActive())  {
            MoveTrackerObject(VK_DOWN);
        }
        else  {
            SendMessage(WM_VSCROLL, SB_LINEDOWN, LPARAM(m_sbVert.GetSafeHwnd()));
        }
        break;
        break;
    case VK_LEFT:
        if (IsTrackerActive())  {
            MoveTrackerObject(VK_LEFT);
        }
        else  {
            SendMessage(WM_HSCROLL, SB_LINELEFT, LPARAM(m_sbHorz.GetSafeHwnd()));
        }
        break;
        break;
    case VK_RIGHT:
        if (IsTrackerActive())  {
            MoveTrackerObject(VK_RIGHT);
        }
        else  {
            SendMessage(WM_HSCROLL, SB_LINERIGHT, LPARAM(m_sbHorz.GetSafeHwnd()));
        }
        break;
        break;
    case VK_HOME:
        SendMessage(WM_VSCROLL, SB_TOP, LPARAM(m_sbVert.GetSafeHwnd()));
        SendMessage(WM_HSCROLL, SB_LEFT, LPARAM(m_sbHorz.GetSafeHwnd()));
        break;
    case VK_END:
        SendMessage(WM_VSCROLL, SB_BOTTOM, LPARAM(m_sbVert.GetSafeHwnd()));
        SendMessage(WM_HSCROLL, SB_RIGHT, LPARAM(m_sbHorz.GetSafeHwnd()));
        break;
    case VK_PRIOR:
        SendMessage(WM_VSCROLL, SB_PAGEUP, LPARAM(m_sbVert.GetSafeHwnd()));
        break;
    case VK_NEXT:
        SendMessage(WM_VSCROLL, SB_PAGEDOWN, LPARAM(m_sbVert.GetSafeHwnd()));
        break;
    case VK_DELETE:
        if (IsTrackerActive())  {
            bProcessed=false;
        }
        else if (GetCurCol()!=NONE)  {
            // trying to delete a column
            if (OnCanDeleteCol(GetCurCol()))  {
                GetParent()->PostMessage(WM_KEYDOWN, VK_DELETE);
            }
        }
        else  {
            // trying to delete something else ... maybe the whole grid?
            GetParent()->PostMessage(WM_KEYDOWN, VK_DELETE);
        }
        break;
    case VK_ESCAPE:
        Deselect();
        break;
    default:
        bProcessed=false;
    }
    CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
    OnKeyDown(&nChar, bProcessed);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::MoveTrackerObject()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::MoveTrackerObject(int iCode)
{
    ASSERT(IsTrackerActive());
    int iRow = GetTracker(0).GetHitOb().GetCell().y;
    int iCol = GetTracker(0).GetHitOb().GetCell().x;
    ASSERT(iCol>=0 && iCol<GetNumCols());
    ASSERT(iRow>=0 && iRow<GetNumRows());
    CGridCell& cell = GetCell(iRow,iCol);
    CRect rcCell = cell.GetRect();  // can't move outside its boundaries
    rcCell.DeflateRect(GRIDCELL_BORDER, GRIDCELL_BORDER);
    rcCell.OffsetRect(-rcCell.TopLeft());

    CSize szMove(0,0);
    switch (iCode)  {
    case VK_RIGHT:
        szMove.cx=1;
        break;
    case VK_LEFT:
        szMove.cx=-1;
        break;
    case VK_UP:
        szMove.cy=-1;
        break;
    case VK_DOWN:
        szMove.cy=1;
        break;
    default:
        ASSERT(FALSE);
    }

    bool bOKToMove=true; // all tracked objects must be able to move, otherwise none of them move
    CRect rcNew;
    for (int i = 0 ; i<GetNumTrackers() && bOKToMove ; i++)  {
        CGridRectTracker& t = GetTracker(i);
        const CHitOb& h = t.GetHitOb();

        switch (t.GetTrackObject())  {
        case CGridRectTracker::trackField:
        {
            const CCellField& f = cell.GetField(h.GetField());
            const CDEField* pFld = f.GetDEField();
            rcNew = pFld->GetDims()+szMove;
            if ((rcNew | rcCell) != rcCell)  {
                bOKToMove = false;
            }
            // special case for fields: make sure we don't overlap with another field
            for (int j=0 ; j<cell.GetNumFields() ; j++)  {
                const CCellField& fl = cell.GetField(j);
                if (fl.GetDEField()!=pFld)  {
                    CRect rcOverlap = fl.GetDEField()->GetDims();
                    if ((rcNew & rcOverlap) != CRect(0,0,0,0))  {
                        bOKToMove=false;
                    }
                }
            }
            break;
        } 

        case CGridRectTracker::trackText:
        {
            const CDEText& text = cell.GetText(h.GetText());
            rcNew = text.GetDims()+szMove;
            if ((rcNew | rcCell) != rcCell)  {
                bOKToMove = false;
            }
            break;
        }

        case CGridRectTracker::trackBox:
        {
            const CDEBox& box = cell.GetBox(h.GetBox());
            rcNew = box.GetDims()+szMove;
            if ((rcNew | rcCell) != rcCell)  {
                bOKToMove = false;
            }
            break;
        } 

        default:
            ASSERT(FALSE);
        }
    }


    if (bOKToMove)  {
        // now move them
        for (int i = 0 ; i<GetNumTrackers() && bOKToMove ; i++)  {
            CGridRectTracker& t = GetTracker(i);
            const CHitOb& h = t.GetHitOb();

            switch (t.GetTrackObject())  {
            case CGridRectTracker::trackField:
            {
                CCellField& fld = cell.GetField(h.GetField());
                CDEField* pFld = fld.GetDEField();
                rcNew = pFld->GetDims()+szMove;
                pFld->SetDims(rcNew);
                t.m_rect += szMove;
                OnFieldMoved(iRow,iCol,h.GetField());
                break;
            }

            case CGridRectTracker::trackText:
            {
                CDEText& text = cell.GetText(h.GetText());
                CRect rc(text.GetDims()+szMove);
                text.SetDims(rc);
                t.m_rect += szMove;
                OnTextMoved(iRow,iCol,h.GetText());
                break;
            }

            case CGridRectTracker::trackBox:
            {
                CDEBox& box = cell.GetBox(h.GetBox());
                CRect rc(box.GetDims()+szMove);
                box.SetDims(rc);
                t.m_rect += szMove;
                OnBoxMoved(iRow,iCol,h.GetBox());
                break;
            }  

            default:
                ASSERT(FALSE);
            }
        }

        // refresh view
        if (m_pRoster->GetOrientation() == RosterOrientation::Horizontal)  {
            for (int i=1 ; i<GetNumRows() ; i++)  {
                RedrawCell(i,iCol);
            }
        }
        else  {
            for (int i=1 ; i<GetNumCols() ; i++)  {
                RedrawCell(iRow,i);
            }
        }
        UpdateWindow();
    }
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::OnSetCursor()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CGridWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    CPoint ptTest,pt;
    CHitOb h;
    bool bTrapped = false;

    GetCursorPos(&ptTest);
    ScreenToClient(&ptTest);
    if (nHitTest==HTCLIENT && pWnd==this)  {
        GRID_HITTEST ht = HitTest(ptTest, h);
        pt = h.GetCell();
        switch (ht)  {
        case BORDER_HEADERV:
        case BORDER_CORNERV:
            if (ptTest.x<HITTEST_BORDER)  {
                // user is at left side of corner box ... don't handle this
                break;
            }
            if (abs(GetHeaderCell(pt.x).GetRect().left - ptTest.x - m_sbHorz.GetScrollPos())<HITTEST_BORDER)  {
                pt.x--;
            }
            if (pt.x>=0)  {
                if (OnCanSizeCol(pt.x))  {
                    ::SetCursor(::LoadCursor(NULL, IDC_SIZEWE));
                    bTrapped=true;
                }
            }
            break;
        case BORDER_STUBH:
        case BORDER_CORNERH:
            if (ptTest.y<HITTEST_BORDER)  {
                // user is at top of corner box ... don't handle this
                break;
            }
            if (abs(GetStubCell(pt.y).GetRect().top - ptTest.y - m_sbVert.GetScrollPos())<HITTEST_BORDER)  {
                pt.y--;
            }
            if (pt.y>=0)  {
                if (OnCanSizeRow(pt.y))  {
                    ::SetCursor(::LoadCursor(NULL, IDC_SIZENS));
                    bTrapped=true;
                }
            }
            break;
        }
    }

    if (!bTrapped)  {
        // see if the tracker wants to set the cursor
        if (pWnd==this)  {
            for (int i = 0 ; i<GetNumTrackers() ; i++)  {
                if (GetTracker(i).SetCursor(this, nHitTest, (GetNumTrackers()>1)))  {
                    bTrapped = true;
                }
            }
        }
    }

    if (!bTrapped)  {
        return CWnd::OnSetCursor(pWnd, nHitTest, message);
    }
    else  {
        return TRUE;
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::OnMouseMove()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::OnMouseMove(UINT nFlags, CPoint point)
{
    CPoint ptCurPos(GetScrollPos());
    GRID_HITTEST ht;
    CPoint pt;
    CHitOb h;
    CRect rcClient;

    CWnd::OnMouseMove(nFlags, point);
    /*if(GetEdit()){
     GetEdit()->SendMessage(WM_MOUSEMOVE,(WPARAM)nFlags,MAKELONG(point.x,point.y));
    }*/
    ht = HitTest(point, h);
    pt = h.GetCell();
    GetClientRect(&rcClient);

    // right justify roster in window for right to left
    ptCurPos.x -= GetRightJustifyOffset();

    point += ptCurPos;

    if (IsResizing())  {
        ASSERT(m_iResizingCol==NONE || m_iResizingRow==NONE); // one or the other, but not both at the same time!
        CRect rcOld, rcNew;
        if (m_iResizingCol != NONE)  {
            // resizing a column
            rcOld.SetRect(m_iCurResizePos-1,0,m_iCurResizePos+1,rcClient.bottom);
            rcOld.OffsetRect(-ptCurPos.x,0);

            // put up new resize bar
            m_iCurResizePos = point.x;
            if (m_pRoster->GetRightToLeft()) {
                if (m_iCurResizePos>m_iMinResizePos)  {
                    m_iCurResizePos=m_iMinResizePos;
                }
            }
            else {
                if (m_iCurResizePos<m_iMinResizePos)  {
                    m_iCurResizePos=m_iMinResizePos;
                }
            }
            rcNew.SetRect(m_iCurResizePos-1,0,m_iCurResizePos+1,rcClient.bottom);
            rcNew.OffsetRect(-ptCurPos.x,0);
        }
        else {
            // resizing a row
            rcOld.SetRect(0,m_iCurResizePos-1,rcClient.right,m_iCurResizePos+1);
            rcOld.OffsetRect(0,-ptCurPos.y);

            // put up new resize bar
            m_iCurResizePos = point.y;
            if (m_iCurResizePos<m_iMinResizePos)  {
                m_iCurResizePos=m_iMinResizePos;
            }
            rcNew.SetRect(0,m_iCurResizePos-1,rcClient.right,m_iCurResizePos+1);
            rcNew.OffsetRect(0,-ptCurPos.y);
        }

        // make them paint!
        if (rcOld!=rcNew)  {
            InvalidateRect(&rcOld);
            InvalidateRect(&rcNew);
            UpdateWindow();
        }
    }
    else if (IsPreSwapping())  {
        // user has started to click+drag, but hasn't reached a border area yet (so don't show swap bar yet)
        if (ht==BORDER_HEADERV)  {
            if (OnCanSwapCol() && pt.x!=0)  {  // can't swap 0th column
                ASSERT(m_iResizingCol!=NONE);
                StartSwapCol(m_iResizingCol);
            }
        }
        else if (ht==BORDER_STUBH)  {
            if (OnCanSwapRow() && pt.y!=0)  {
                ASSERT(m_iResizingRow!=NONE);
                StartSwapRow(m_iResizingRow);
            }
        }
    }
    else if (IsSwapping())  {
        ASSERT(m_iResizingCol==NONE || m_iResizingRow==NONE); // one or the other, but not both at the same time!
        CRect rcOld, rcNew;
        if (m_iResizingCol != NONE)  {
            // swapping columns ... remove previous resize bar
            rcOld.SetRect(m_iCurResizePos-1,0,m_iCurResizePos+1,rcClient.bottom);
            rcOld.OffsetRect(-ptCurPos.x,0);
            ht = HitTest(CPoint(point.x-ptCurPos.x,5), h);  // just test horizontal plane
            pt = h.GetCell();

            // figure out which border we're closest to
            if (m_pRoster->GetRightToLeft())
            {
                if (pt.x == NONE) {
                    pt.x = m_ptCorner.x;
                }
            }
            else {
                if (pt.x == NONE) {
                    pt.x = GetNumCols();
                }
                if (pt.x == 0) {
                    pt.x = m_ptCorner.x;
                }
            }
            ASSERT(pt.x>=0 && pt.x<=GetNumCols());
            int iMaxPos = GetHeaderCell(GetNumCols()-1).GetRect().right-2;
            if (pt.x==GetNumCols())  {
                m_iCurResizePos = iMaxPos;
            }
            else  {
                const CRect& rc = GetHeaderCell(pt.x).GetRect();
                if (abs(point.x - rc.left) < abs(point.x - rc.right))  {
                    m_iCurResizePos = __max(m_ptCorner.x,rc.left);
                }
                else {
                    m_iCurResizePos = __min(rc.right,iMaxPos);
                }
            }
            if (m_iCurResizePos > rcClient.right+ptCurPos.x+2*::GetSystemMetrics(SM_CXBORDER))  {
                CPoint ptRightSideMax;
                CHitOb hRight;
                HitTest(CPoint(rcClient.right,5), hRight);
                ptRightSideMax = hRight.GetCell();
                ASSERT(ptRightSideMax.x>=0 && ptRightSideMax.x<GetNumCols());
                m_iCurResizePos = GetHeaderCell(ptRightSideMax.x).GetRect().left;
            }

            // put up new resize bar
            rcNew.SetRect(m_iCurResizePos-1,0,m_iCurResizePos+1,rcClient.bottom);
            rcNew.OffsetRect(-ptCurPos.x,0);
        }
        else  {
            // swapping rows
            rcOld.SetRect(0,m_iCurResizePos-1,rcClient.right, m_iCurResizePos+1);
            rcOld.OffsetRect(0,-ptCurPos.y);
            ht = HitTest(CPoint(5,point.y-ptCurPos.y), h);  // just test vertical plane
            pt = h.GetCell();

            // figure out which border we're closest to
            if (pt.y==NONE)  {
                pt.y = GetNumRows();
            }
            if (pt.y==0)  {
                pt.y = m_ptCorner.y;
            }
            ASSERT(pt.y>=0 && pt.y<=GetNumRows());
            int iMaxPos = GetStubCell(GetNumRows()-1).GetRect().bottom-2;
            if (pt.y==GetNumRows())  {
                m_iCurResizePos = iMaxPos;
            }
            else  {
                const CRect& rc = GetStubCell(pt.y).GetRect();
                if (abs(point.y - rc.top) < abs(point.y - rc.bottom))  {
                    m_iCurResizePos = __max(rc.top,m_ptCorner.y);
                }
                else {
                    m_iCurResizePos = __min(rc.bottom,iMaxPos);
                }
            }
            if (m_iCurResizePos > rcClient.bottom+ptCurPos.y+2*::GetSystemMetrics(SM_CYBORDER))  {
                CPoint ptBottomMax;
                CHitOb hBottom;
                HitTest(CPoint(5,rcClient.bottom), hBottom);
                ptBottomMax = hBottom.GetCell();
                ASSERT(ptBottomMax.y>=0 && ptBottomMax.y<GetNumRows());
                m_iCurResizePos = GetStubCell(ptBottomMax.y).GetRect().top;
            }

            // put up new resize bar
            rcNew.SetRect(0,m_iCurResizePos-1,rcClient.right,m_iCurResizePos+1);
            rcNew.OffsetRect(0,-ptCurPos.y);
        }
        // make them paint!
        if (rcOld!=rcNew)  {
            InvalidateRect(&rcOld);
            InvalidateRect(&rcNew);
            UpdateWindow();
        }
    }

}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::OnLButtonDown()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
    CWnd::OnLButtonDown(nFlags, point);
    //if(GetEdit()){
    // GetEdit()->SendMessage(WM_LBUTTONDOWN,(WPARAM)nFlags,MAKELONG(point.x,point.y));
    //}
    ASSERT(!IsResizing());
    ASSERT(!IsSwapping());
    ASSERT(!IsPreSwapping());
    ASSERT(m_iResizingRow==NONE);
    ASSERT(m_iResizingCol==NONE);

    CPoint pt;
    CHitOb h;
    GRID_HITTEST ht;

    bool bCtrl = OnCanMultiSelect()?(GetKeyState(VK_CONTROL)<0):false;
    bool bShift = OnCanMultiSelect()?(GetKeyState(VK_SHIFT)<0):false;

    ht = HitTest(point, h);
    pt = h.GetCell();
    switch (ht)  {
    case BORDER_CORNERV:
        if (point.x>HITTEST_BORDER)  {
            if (OnCanSizeCol(0))  {
                StartResizeCol(0);
            }
        }
        else  {
            if (!OnCanDrawBox(pt.y, pt.x))  {  // csc 1/30/01
                OnCB_LClicked(point);   // csc 1/15/01
            }
        }
        break;
    case BORDER_HEADERV:
        {
        // see if user wants to resize a column ...
        // see which column border we are closest to

        CPoint ptCurPos(point + GetScrollPos());

        // right justify roster in window for right to left
        ptCurPos.x -= GetRightJustifyOffset();

        if (abs(GetHeaderCell(pt.x).GetRect().left - ptCurPos.x)<HITTEST_BORDER)  {
            pt.x--;
        }
        ASSERT(pt.x>=0);
        if (pt.x<m_ptCorner.x && !m_pRoster->GetRightToLeft())  {
            // user is really trying to resize the stub column
            pt.x=0;
        }

        if (m_pRoster->GetRightToLeft()) {
            // for right to left, we resize with left edge of cell, not right edge
            // so need to shift over one column
            pt.x += 1;
            if (pt.x == GetNumCols()) {
                pt.x = 0; // last column is stubs (stubs are on right for right to left)
            }
        }
        if (OnCanSizeCol(pt.x))  {
            StartResizeCol(pt.x);
        }
        }
        break;
    case BORDER_CORNERH:
        if (point.y>HITTEST_BORDER)  {
            if (OnCanSizeRow(0))  {
                StartResizeRow(0);
            }
        }
        break;
    case BORDER_STUBH:
        // see if user wants to resize a row ...
        if (abs(GetStubCell(pt.y).GetRect().top - point.y - m_sbVert.GetScrollPos())<HITTEST_BORDER)  {
            pt.y--;
        }
        ASSERT(pt.y>=0);
        if (pt.y<m_ptCorner.y)  {
            // user is really trying to resize the header row
            pt.y=0;
        }
        if (OnCanSizeRow(pt.y))  {
            StartResizeRow(pt.y);
        }
        break;
    case HIT_STUB:
    case BORDER_STUBV:
        if (!OnCanDrawBox(pt.y, pt.x))  {   // csc 1/30/01
            if (OnCanSelectRow(pt.y))  {
                SetCapture();
                m_bPreSwapping=TRUE;
                m_iResizingRow=pt.y;
            }
            OnSH_LClicked(pt.y);
        }
        break;
    case HIT_HEADER:
    case BORDER_HEADERH:
        if (!OnCanDrawBox(pt.y, pt.x))  {    // csc 1/30/01
            if (OnCanSelectCol(pt.x))  {
                SetCapture();
                m_bPreSwapping=TRUE;
                m_iResizingCol=pt.x;
            }
            OnTH_LClicked(pt.x);
        }
        break;
    case HIT_CELL:
    case BORDER_CELLH:
    case BORDER_CELLV:
        if (OnCanSelectCell(pt.y, pt.x))  {
            //            SelectCell(pt, bCtrl, bShift);
            SelectCell(pt, bCtrl, bShift, true, GetSysColor(COLOR_3DFACE));   // BMD 17 Apr 2001
        }
        else  {
            if (OnCanDrawBox(pt.y, pt.x))  {
                DrawBox(pt.y, pt.x, point);
            }
            else  {
                OnCell_LClicked(point);
            }
        }
        break;
    case HIT_CELLFIELD:
    case TRACK_FIELD:
        if (OnCanDrawBox(pt.y, pt.x))  {    // csc 1/30/01
            DrawBox(pt.y, pt.x, point);
        }
        else  {
            CDEField* pFld = GetCell(pt.y, pt.x).GetField(h.GetField()).GetDEField();
            int iOcc = (m_pRoster->GetOrientation() == RosterOrientation::Horizontal?pt.y:pt.x);
            OnCellField_LClicked(pFld, iOcc);
        }
        break;
    case HIT_CELLBOX:
    case TRACK_BOX:
        if (OnCanDrawBox(pt.y, pt.x))  {    // csc 1/30/01
            DrawBox(pt.y, pt.x, point);
        }
        else  {
            OnCellBox_LClicked(h);
        }
        break;
    case HIT_CELLTEXT:
    case TRACK_TEXT:
        if (OnCanDrawBox(pt.y, pt.x))  {    // csc 1/30/01
            DrawBox(pt.y, pt.x, point);
        }
        else  {
            OnCellText_LClicked(h);
        }
        break;
    case HIT_CORNER:
    case NO_HIT:
        if (!OnCanDrawBox(pt.y, pt.x))  {    // csc 1/30/01
            OnCB_LClicked(point);
        }
        break;
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::OnLButtonUp()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
    CWnd::OnLButtonUp(nFlags, point);
    /*if(GetEdit()){
     GetEdit()->SendMessage(WM_LBUTTONUP,(WPARAM)nFlags,MAKELONG(point.x,point.y));
    }*/
    CRect rcOld, rcClient;
    CPoint ptCurPos(GetScrollPos());

    GetClientRect(&rcClient);

    // right justify roster in window for right to left
    ptCurPos.x -= GetRightJustifyOffset();

    ReleaseCapture();
    if (IsResizing())  {
        ASSERT(m_iResizingCol==NONE || m_iResizingRow==NONE);
        if (m_iResizingCol != NONE)  {
            // resizing a column
            rcOld.SetRect(m_iCurResizePos-1,0,m_iCurResizePos+1,rcClient.bottom);
            rcOld.OffsetRect(-ptCurPos.x,0);
            InvalidateRect(&rcOld);
            UpdateWindow();
            m_bResizing=FALSE;
            m_iCurResizePos = point.x + ptCurPos.x;
            if (m_pRoster->GetRightToLeft()) {
                if (m_iCurResizePos>m_iMinResizePos)  {
                    m_iCurResizePos = m_iMinResizePos;
                }
            }
            else {
                if (m_iCurResizePos<m_iMinResizePos)  {
                    m_iCurResizePos = m_iMinResizePos;
                }
            }

            // adjust column width
            ASSERT(m_iResizingCol>=0 && m_iResizingCol<GetNumCols());
            int iWidth;
            if (m_iResizingCol==0)  {
                // user is sizing the stub column!
                if (m_pRoster->GetRightToLeft()) {
                    iWidth = (rcClient.right + ptCurPos.x) - m_iCurResizePos;
                }
                else {
                    iWidth = m_iCurResizePos - ptCurPos.x;
                }
                SetColWidth(iWidth, 0);
            }
            else  {
                // VERTICAL orientation implies that all columns must have same width!
                if (m_pRoster->GetRightToLeft()) {
                    iWidth = GetHeaderCell(m_iResizingCol).GetRect().right - m_iCurResizePos;
                }
                else {
                    iWidth = m_iCurResizePos - GetHeaderCell(m_iResizingCol).GetRect().left;
                }
                SetColWidth(iWidth, m_pRoster->GetOrientation() == RosterOrientation::Horizontal?m_iResizingCol:NONE);
            }
            DeselectTrackers();
            OnResizedCol(m_iResizingCol);
            m_iResizingCol = NONE;
        }
        else  {
            // resizing a row
            rcOld.SetRect(0,m_iCurResizePos-1,rcClient.right,m_iCurResizePos+1);
            rcOld.OffsetRect(0,-ptCurPos.y);
            InvalidateRect(&rcOld);
            UpdateWindow();
            m_bResizing=FALSE;
            m_iCurResizePos = point.y + ptCurPos.y;
            if (m_iCurResizePos<m_iMinResizePos)  {
                m_iCurResizePos = m_iMinResizePos;
            }

            // adjust row width
            ASSERT(m_iResizingRow>=0 && m_iResizingRow<GetNumRows());
            int iHeight;
            if (m_iResizingRow==0)  {
                // user is sizing the header row!
                iHeight = m_iCurResizePos - ptCurPos.y;
                SetRowHeight(iHeight, 0);
            }
            else  {
                // HORIZONTAL orientation implies that all rows much have the same height!
                iHeight = m_iCurResizePos - GetStubCell(m_iResizingRow).GetRect().top;
                SetRowHeight(iHeight, (m_pRoster->GetOrientation() == RosterOrientation::Horizontal?NONE:m_iResizingRow));
            }
            DeselectTrackers();
            OnResizedRow(m_iResizingRow);
            m_iResizingRow = NONE;
        }

    }
    else if (IsPreSwapping()) {
        m_bPreSwapping = FALSE;
        m_iResizingCol=m_iResizingRow=NONE;
    }
    else if (IsSwapping())  {
        ASSERT(m_iResizingCol==NONE || m_iResizingRow==NONE);
        if (m_iResizingCol != NONE)  {
            // swapping a column ... remove previous resize bar
            rcOld.SetRect(m_iCurResizePos-1,0,m_iCurResizePos+1,rcClient.bottom);
            rcOld.OffsetRect(-ptCurPos.x,0);
            InvalidateRect(&rcOld);
            UpdateWindow();
            m_bSwapping=FALSE;

            // move the cols!
            CPoint pt;
            CHitOb h;

            if (m_iCurResizePos==GetHeaderCell(GetNumCols()-1).GetRect().right-2)  {
                // special position to show selection bar on right hand side
                m_iCurResizePos += 2;
            }
            HitTest(CPoint(m_iCurResizePos-ptCurPos.x,5), h);
            pt = h.GetCell();

            // right to left are dragging other edge of cell so need to
            // shift over by 1
            if (m_pRoster->GetRightToLeft()) {
                if (pt.x == NONE) {
                    pt.x = 0; // over left edge, use last col
                }
                else if (pt.x == 0)
                {
                    // Stub
                    pt.x = GetNumCols() - 1;
                }
                else
                {
                    pt.x = pt.x - 1;
                }
            }
            else {
                if (pt.x==0)  {
                    // special case of user swapping into the stub area; just place the col before the topmost displayed col
                    pt.x = m_ptCorner.x;
                }
                if (pt.x==NONE)  {
                    // special case swapping past right-side
                    pt.x = GetNumCols();
               }
            }

            ASSERT(pt.x>=0 && pt.x<=GetNumCols());


            if (m_iResizingCol != pt.x)  {
                OnSwappedCol(std::vector<int> {m_pRoster->AdjustColIndexForRightToLeft(m_iResizingCol)}, m_pRoster->AdjustColIndexForRightToLeft(pt.x));
            }
            m_iResizingCol=NONE;
        }
        else  {
            // swapping a row ... remove previous resize bar
            rcOld.SetRect(0,m_iCurResizePos-1,rcClient.right, m_iCurResizePos+1);
            rcOld.OffsetRect(0,-ptCurPos.y);
            InvalidateRect(&rcOld);
            UpdateWindow();
            m_bSwapping=FALSE;

            // move the rows!
            CPoint pt;
            CHitOb h;

            if (m_iCurResizePos==GetStubCell(GetNumRows()-1).GetRect().bottom-2)  {
                // special position to show selection bar on bottom
                m_iCurResizePos += 2;
            }
            HitTest(CPoint(5,m_iCurResizePos-ptCurPos.y), h);
            pt = h.GetCell();
            if (pt.y==0)  {
                // special case of user swapping into the header area; just place the row before the topmost displayed row
                pt.y = m_ptCorner.y;
            }
            if (pt.y==NONE)  {
                // special case swapping past bottom
                pt.y = GetNumRows();
            }
            ASSERT(pt.y>0 && pt.y<=GetNumRows());
            if (m_iResizingRow != pt.y)  {
                OnSwappedRow(std::vector<int> {m_iResizingRow}, pt.y);
            }
            m_iResizingRow=NONE;
        }

        // rebuild and scroll back to previous position
        CPoint ptCorner(m_ptCorner);
        int iPrevSelRow = GetCurRow();
        int iPrevSelCol = GetCurCol();
        BuildGrid();
        RecalcLayout();
        if (iPrevSelCol!=NONE)  {
            //            SelectColumn(iPrevSelCol);
            SelectColumn(iPrevSelCol, false, false, true, GetSysColor(COLOR_3DFACE));  // BMD 17 Apr 2001
        }
        if (iPrevSelRow!=NONE)  {
            //            SelectRow(iPrevSelRow);
            SelectRow(iPrevSelRow, false, false, true, GetSysColor(COLOR_3DFACE));  // BMD 17 Apr 2001
        }
        ScrollTo(ptCorner.y, ptCorner.x);
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::OnRButtonDown()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::OnRButtonDown(UINT nFlags, CPoint point)
{
    CWnd::OnRButtonDown(nFlags, point);

    // cancel any resizing or swapping operations ...
    m_bPreSwapping=FALSE;
    if (IsResizing() || IsSwapping())  {
        ASSERT(m_iResizingCol==NONE || m_iResizingRow==NONE);
        CRect rcOld, rcClient;
        CPoint ptCurPos(GetScrollPos());
        GetClientRect(&rcClient);
        if (m_iResizingCol != NONE)  {
            // was resizing a column
            rcOld.SetRect(m_iCurResizePos-1,0,m_iCurResizePos+1,rcClient.bottom);
            rcOld.OffsetRect(-ptCurPos.x,0);
        }
        else  {
            rcOld.SetRect(0,m_iCurResizePos-1,rcClient.right,m_iCurResizePos+1);
            rcOld.OffsetRect(0,-ptCurPos.y);
        }
        m_bResizing=m_bSwapping=FALSE;
        m_iResizingRow=m_iResizingCol=NONE;
        InvalidateRect(&rcOld);
        UpdateWindow();
    }
    m_iResizingCol=m_iResizingRow=NONE;

    CPoint pt;
    CHitOb h;
    GRID_HITTEST ht = HitTest(point, h);
    pt = h.GetCell();
    switch (ht)  {
    case HIT_CELL:
    case BORDER_CELLH:
    case BORDER_CELLV:
        break;
    case HIT_CELLBOX:
    case TRACK_BOX:
        break;
    case HIT_CELLTEXT:
    case TRACK_TEXT:
        break;
    case HIT_CELLFIELD:
    case TRACK_FIELD:
        break;
    case HIT_STUB:
    case BORDER_STUBH:
    case BORDER_STUBV:
        break;
    case HIT_HEADER:
    case BORDER_HEADERH:
    case BORDER_HEADERV:
        break;
    case HIT_CORNER:
    case BORDER_CORNERH:
    case BORDER_CORNERV:
    case NO_HIT:
        break;
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::OnRButtonUp()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::OnRButtonUp(UINT nFlags, CPoint point)
{
    CWnd::OnRButtonUp(nFlags, point);

    ASSERT(!IsResizing());
    SetFocus();
    ReleaseCapture();

    bool bCtrl = OnCanMultiSelect()?(GetKeyState(VK_CONTROL)<0):false;
    bool bShift = OnCanMultiSelect()?(GetKeyState(VK_SHIFT)<0):false;

    CPoint pt;
    CHitOb h;
    GRID_HITTEST ht = HitTest(point, h);
    pt = h.GetCell();

    if (bCtrl || bShift)  {
        if (m_iActiveCol!=NONE)  {
            ASSERT(m_iActiveRow==NONE && m_ptActiveCell==CPoint(NONE,NONE));
            pt.x = m_iActiveCol;
            ht = HIT_HEADER;
        }
        else if (m_iActiveRow!=NONE)  {
            ASSERT(m_iActiveCol==NONE && m_ptActiveCell==CPoint(NONE,NONE));
            pt.y = m_iActiveRow;
            ht = HIT_STUB;
        }
        else if (m_ptActiveCell!=CPoint(NONE,NONE))  {
            ASSERT(m_iActiveCol==NONE && m_iActiveRow==NONE);
            pt = m_ptActiveCell;
            if (pt.x==0)  {
                // user is clicking on a stub
                ht = HIT_STUB;
            }
            else if (pt.y==0)  {
                // user is clicking on a header
                ht = HIT_HEADER;
            }
            else  {
                ht = HIT_CELL;
            }
        }
    }

    switch (ht)  {
    case HIT_CELL:
    case BORDER_CELLH:
    case BORDER_CELLV:
        OnCell_RClicked(pt.y, pt.x);
        break;
    case HIT_CELLBOX:
    case TRACK_BOX:
        OnCellBox_RClicked(h);
        break;
    case HIT_CELLTEXT:
    case TRACK_TEXT:
        OnCellText_RClicked(h);
        break;
    case HIT_CELLFIELD:  {
    case TRACK_FIELD:
        CDEField* pFld = GetCell(pt.y, pt.x).GetField(h.GetField()).GetDEField();
        int iOcc = (m_pRoster->GetOrientation() == RosterOrientation::Horizontal?pt.y:pt.x);
        OnCellField_RClicked(pFld, iOcc);
                         }
        break;
    case HIT_STUB:
    case BORDER_STUBH:
    case BORDER_STUBV:
        OnSH_RClicked(pt.y);
        break;
    case HIT_HEADER:
    case BORDER_HEADERH:
    case BORDER_HEADERV:
        OnTH_RClicked(pt.x);
        break;
    case HIT_CORNER:
    case BORDER_CORNERH:
    case BORDER_CORNERV:
    case NO_HIT:
        OnCB_RClicked(point);
        break;
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::StartResizeCol()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::StartResizeCol(int iCol)
{
    ASSERT(!IsResizing());
    ASSERT(!IsSwapping());

    CRect rcClient;
    int iCurPos;

    SetCapture();
    DeselectTrackers();
    GetClientRect(&rcClient);
    iCurPos = m_sbHorz.GetScrollPos();

    // right justify roster in window for right to left
    iCurPos -= GetRightJustifyOffset();
    rcClient.OffsetRect(iCurPos, 0);

    m_bResizing = TRUE;
    m_iResizingCol=iCol;

    // set minimum resizing position
    m_iMinResizePos=0;
    if (iCol>0)  {
        // resize min considers header (for text) and 1st row (for fields)
        CGridCell& cellHeader = GetHeaderCell(iCol);
        m_iMinResizePos = cellHeader.GetMinSizeH().cx;
        for (int iRow=1 ; iRow<GetNumRows() ; iRow++)  {
            m_iMinResizePos=__max(m_iMinResizePos, GetCell(iRow,iCol).GetMinSizeH().cx);
        }
        if (m_pRoster->GetRightToLeft()) {
            m_iMinResizePos = cellHeader.GetRect().right - m_iMinResizePos - GRIDCELL_BORDER*2;
            m_iCurResizePos = GetHeaderCell(iCol).GetRect().left;
        }
        else {
            m_iMinResizePos = cellHeader.GetRect().left + m_iMinResizePos + GRIDCELL_BORDER*2;
            m_iCurResizePos = GetHeaderCell(iCol).GetRect().right;
        }
    }
    else  {
        // on corner button
        m_iMinResizePos = 0;
        for (int iRow=1 ; iRow<GetNumRows() ; iRow++)  {
            int iMinWidth = GetStubCell(iRow).GetMinSizeH().cx;
            m_iMinResizePos = __max(iMinWidth,m_iMinResizePos);
        }
        if (m_pRoster->GetRightToLeft()) {
            m_iMinResizePos = rcClient.right - m_iMinResizePos - GRIDCELL_BORDER*2;
            m_iCurResizePos = GetHeaderCell(iCol).GetRect().left;
        }
        else {
            m_iMinResizePos += GRIDCELL_BORDER*2 + iCurPos;
            m_iCurResizePos = GetHeaderCell(iCol).GetRect().right;
        }
    }

    CRect rc(m_iCurResizePos-1,0,m_iCurResizePos+1,rcClient.bottom);
    rc.OffsetRect(-iCurPos,0);
    InvalidateRect(&rc);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::StartResizeRow()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::StartResizeRow(int iRow)
{
    ASSERT(!IsResizing());
    ASSERT(!IsSwapping());

    CRect rcClient;
    int iCurPos;

    SetCapture();
    DeselectTrackers();
    GetClientRect(&rcClient);
    iCurPos = m_sbVert.GetScrollPos();
    rcClient.OffsetRect(0,iCurPos);

    m_bResizing = TRUE;
    m_iResizingRow=iRow;

    // set minimum resizing position
    if (iRow>0)  {
        CGridCell& cellStub = GetStubCell(iRow);
        m_iMinResizePos = cellStub.GetMinSizeV().cy;
        for (int iCol=0 ; iCol<GetNumCols() ; iCol++)  {
            m_iMinResizePos = __max(m_iMinResizePos,GetCell(iRow,iCol).GetMinSizeV().cy);
        }
        m_iMinResizePos = cellStub.GetRect().top + m_iMinResizePos + GRIDCELL_BORDER*2;
    }
    else  {
        // on corner button
        m_iMinResizePos = 0;
        int iNumCols = GetNumCols();
        for (int iCol=1 ; iCol<iNumCols ; iCol++)  {
            m_iMinResizePos = __max(GetHeaderCell(iCol).GetMinSizeV().cy,m_iMinResizePos);
            //            CGridCell& cell = GetHeaderCell(iCol);
            //            if (cell.GetMinSize().cy + GRIDCELL_BORDER*2 > m_iMinResizePos)  {
            //                m_iMinResizePos = cell.GetMinSize().cy + GRIDCELL_BORDER*2;
            //            }
        }
        m_iMinResizePos += GRIDCELL_BORDER*2;
    }

    // set maximum resizing position
    if (iRow<GetNumRows()-1)  {
        m_iMaxResizePos = GetStubCell(iRow+1).GetRect().bottom;
        if (m_iMaxResizePos>rcClient.bottom+iCurPos)  {
            m_iMaxResizePos = rcClient.bottom+iCurPos;
        }
    }
    else  {
        m_iMaxResizePos = GetStubCell(iRow).GetRect().bottom+100;  // next release, we need to handle this better
    }

    m_iCurResizePos = GetStubCell(iRow).GetRect().bottom;

    CRect rc(0,m_iCurResizePos-1,rcClient.right, m_iCurResizePos+1);
    rc.OffsetRect(0,-iCurPos);
    InvalidateRect(&rc);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::StartSwapCol()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::StartSwapCol(int iCol)
{
    ASSERT(!IsResizing());
    ASSERT(!IsSwapping());

    CRect rcClient;
    CPoint ptCurPos, pt;
    int iCurPos;

    GetClientRect(&rcClient);
    GetCursorPos(&ptCurPos);
    ScreenToClient(&ptCurPos);
    iCurPos = m_sbHorz.GetScrollPos();
    rcClient.OffsetRect(iCurPos, 0);

    m_bSwapping = TRUE;
    m_bPreSwapping = FALSE;

    m_iResizingCol=iCol;
    m_iMinResizePos = GetHeaderCell(0).GetRect().left;
    m_iMaxResizePos = GetHeaderCell(GetNumCols()-1).GetRect().right;

    ptCurPos.x += iCurPos;
    CRect& rc = GetHeaderCell(iCol).GetRect();
    if (abs(ptCurPos.x - rc.left) < abs(ptCurPos.x - rc.right))  {
        m_iCurResizePos = rc.left;
    }
    else {
        m_iCurResizePos = rc.right;
    }

    CRect rcDraw(m_iCurResizePos-1,0,m_iCurResizePos+1,rcClient.bottom);
    rcDraw.OffsetRect(-iCurPos,0);
    InvalidateRect(&rcDraw);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::StartSwapRow()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::StartSwapRow(int iRow)
{
    ASSERT(!IsResizing());
    ASSERT(!IsSwapping());

    CRect rcClient;
    CPoint ptCurPos, pt;
    int iCurPos;

    GetClientRect(&rcClient);
    GetCursorPos(&ptCurPos);
    ScreenToClient(&ptCurPos);
    iCurPos = m_sbVert.GetScrollPos();
    rcClient.OffsetRect(0,iCurPos);

    m_bSwapping = TRUE;
    m_bPreSwapping = FALSE;

    m_iResizingRow=iRow;
    m_iMinResizePos = GetStubCell(0).GetRect().top;
    m_iMaxResizePos = GetStubCell(GetNumRows()-1).GetRect().bottom;

    ptCurPos.y += iCurPos;
    CRect& rc = GetStubCell(iRow).GetRect();
    if (abs(ptCurPos.y - rc.top) < abs(ptCurPos.y - rc.bottom))  {
        m_iCurResizePos = rc.top;
    }
    else {
        m_iCurResizePos = rc.bottom;
    }

    CRect rcDraw(0,m_iCurResizePos-1,rcClient.right,m_iCurResizePos+1);
    rcDraw.OffsetRect(0, -iCurPos);
    InvalidateRect(&rcDraw);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::OnEraseBkgnd()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CGridWnd::OnEraseBkgnd(CDC* pDC)
{
    CBrush brush(m_cGridbkcolor); // GHM 20120606 removing above code to use the form background color instead
    CBrush* pOldBrush = pDC->SelectObject(&brush);
    CRect rc;
    pDC->GetClipBox(&rc);     // Erase the area needed.
    pDC->PatBlt(rc.left, rc.top, rc.Width(), rc.Height(), PATCOPY);
    pDC->SelectObject(pOldBrush);

    return TRUE;
}

//FABN Dec 12, 2002
COLORREF CGridWnd::GetGridBkColor()
{
    return m_cGridbkcolor;
}

CSize CGridWnd::GetHeaderSize()
{
    return m_szHeader;
}

int CGridWnd::GetRightJustifyOffset() const
{
    if (m_pRoster->GetRightToLeft()) {
        // using right to left layout - only need to right justify
        // if the client rect is big enough to fit all the cells i.e.
        // if there is no horiz scroll bar.  This is true if the stub
        // header cell (cell 0,0) has its right edge inside the client rect.
        // When the client rect can fit all the cells and has extra space, we
        // want to shift all the cells over to the right so that the extra space
        // gets put on the left of the window.
        CRect rcClient;
        GetClientRect(&rcClient);
        const CRect& rcStub = GetCell(0,0).GetRect();
        if (rcClient.right >= rcStub.right) {
            return rcClient.right - rcStub.right; // offset cells by difference btwn
                                                  // client rect and rt edge of cells
        }
    }

    return 0; // no offset
}

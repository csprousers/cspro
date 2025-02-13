// GridLay.cpp : implementation file
//
// layout stuff for CGridWnd

#include "StdAfx.h"
#include "GridWnd.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::RecalcLayout
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::RecalcLayout(CSize szNewPage /*=CSize(-1,-1)*/, bool bRedraw /*=true*/)
{
    ASSERT_VALID(m_pRoster);
    int iRow, iCol, iNumCols, iNumRows;
    UINT uDrawFormat=0;
    bool bFontInit = false;   // set to true after we've retrieved a font for the first CDEField

    CClientDC dc(this);
    int iSaveDC= dc.SaveDC();
    CRect rcClient;
    CSize szChar(NONE,NONE);
    CFont fontEdit;

    GetClientRect(&rcClient);
    iNumCols=GetNumCols();
    iNumRows=GetNumRows();

    // determine 1 square inch in LP
    CSize szInch(100,100);
    dc.SetMapMode(MM_LOENGLISH);
    dc.DPtoLP(&szInch);
    dc.SetMapMode(MM_TEXT);

    // measure headers
    uDrawFormat = DT_WORDBREAK;     // csc 12/22/00
    for (iCol=0 ; iCol<iNumCols ; iCol++)  {
        CGridCell& cell = GetHeaderCell(iCol);
        CRect rcCell = cell.GetRect();
#ifdef _DEBUG
        if(rcCell.Width() > 1024) {
            CString sMsg = _T("STOP! THIS IS THE 'BIG COLUMN BUG'\n\n");
            sMsg.AppendFormat(_T("Header Cell Width is %d for '%s'\n"), rcCell.right, (LPCTSTR)cell.GetText(0).GetText());
            sMsg += _T("Think about what you just did and see if you can reproduce it\n\n");
            sMsg += _T("CSPro will now reset the width to something reasonable.");

            AfxMessageBox(sMsg);
            rcCell.right = rcCell.left + 60;
            cell.SetRect(rcCell);
        }
#endif
        rcCell.OffsetRect(-rcCell.TopLeft()); // move it back to 0,0 origin
        ASSERT(cell.GetNumBoxes()==0);
        ASSERT(cell.GetNumTexts()==0 || cell.GetNumTexts()==1);
        if (cell.GetNumTexts()>0)  {
            CDEText& text = cell.GetText(0);
            CRect rcTxt = text.GetDims();
            if (text.GetText().IsEmpty())  {
                // no text to display
                rcTxt.SetRectEmpty();
            }
            else  {
                // we have some text, see if the area allocated is too small
                CRect rcOld(rcTxt);
                CString csStub = text.GetText();
                int iRepl = csStub.Replace(_T("@"), _T("%d"));
                ASSERT(iRepl==0 || iRepl==1);
                if (csStub.Find(_T("%d"))!=NONE)  {
                    text.SetText(csStub);
                    csStub.Format(CString(csStub),999);
                }
                if (rcTxt.IsRectNull())  {
                    // no CRect for txt; make a default
//                  rcTxt.SetRect(0,0, szInch.cx, szInch.cy);
                    rcTxt.SetRect(0,0, std::max((int) szInch.cx,rcCell.Width()), szInch.cy);   // csc 29 Sept 2003; bug report #1190
                }
                rcTxt.BottomRight() -= CPoint(2*GRIDCELL_BORDER, 2*GRIDCELL_BORDER);
                rcTxt.OffsetRect(-rcTxt.TopLeft());

                // WINBUG: sometimes CDC::CalcRect with DT_RIGHT and DT_WORDBREAK doesn't
                // wrap correctly; seems to always work OK with DT_LEFT, so we'll isolate the
                // problem based on that.

                dc.SelectObject(text.GetFont().GetCFont());

                CRect rcLeft(rcTxt), rcRight(rcTxt), rcMin;
                dc.DrawText(csStub, &rcLeft, DT_CALCRECT|DT_LEFT|uDrawFormat);
                dc.DrawText(csStub, &rcRight, DT_CALCRECT|DT_RIGHT|uDrawFormat);
                if (rcLeft.Height()<rcRight.Height() || rcLeft.Width()!=rcRight.Width())  {
                    rcRight.right += dc.GetTextExtent(SPACE).cx;
                    rcRight.bottom = rcLeft.bottom;   // csc 12/27/00
                }

                dc.SelectStockObject(OEM_FIXED_FONT);
                rcTxt = rcMin = rcRight;
                cell.GetMinSizeV() = rcTxt.Size();  // OK since cell doesn't have any fields!
                rcTxt.BottomRight() += CPoint(2*GRIDCELL_BORDER, 2*GRIDCELL_BORDER);

                // rcMin is for drawing the text
                rcMin.BottomRight() += CPoint(2*GRIDCELL_BORDER, 2*GRIDCELL_BORDER);
            //header color  background color of grid
                cell.SetBColor(m_cGridbkcolor);

                // if previously allocated area was larger, then use that (maybe user resized it on purpose)
                if (rcTxt.Width()<rcOld.Width())  {
                    rcTxt.right = rcTxt.left + rcOld.Width();
                }
                if (rcTxt.Height()<rcOld.Height())  {
                    rcTxt.bottom = rcTxt.top + rcOld.Height();
                }

                // adjust cell to accomodate the txt's rect
                rcCell.UnionRect(&rcCell, &rcTxt);
                text.SetDims(rcMin);
            }
            cell.SetRect(rcCell);
        }
    }

    // measure stubs
    CString csStub;
    uDrawFormat = DT_WORDBREAK;      // csc 12/22/00
    for (iRow=1 ; iRow<iNumRows ; iRow++)  {
        CGridCell& cell = GetStubCell(iRow);
        CRect rcCell = cell.GetRect();
        rcCell.OffsetRect(-rcCell.TopLeft()); // move it back to 0,0 origin
        if (cell.GetNumTexts()>0)  {
            CDEText& text = cell.GetText(0);
            CRect rcTxt = text.GetDims();
            if (text.GetText().IsEmpty())  {
                // no text to display
                rcTxt.SetRectEmpty();
            }
            else  {
                // we have some text
                CRect rcOld(rcTxt), rcMin;
                if (rcTxt.IsRectNull())  {
                    // no CRect for txt; make a default
                    int iGoodGuessWidth=__max(m_pRoster->GetStubColWidth(),szInch.cx);  // consider stub col width; csc 12/26/00
                    if (m_pRoster->GetOrientation() == RosterOrientation::Vertical)  {
                        // for vertical only ... try to guess what the ultimate width
                        // will be, based on widest word in the grid; csc 10/21/00
                        for (int i=1;i<iNumRows;i++)  {
                            ASSERT(GetStubCell(i).GetMinSizeH().cx>0);
                            iGoodGuessWidth = __max(GetStubCell(i).GetMinSizeH().cx,iGoodGuessWidth);
                        }
                    }
                rcTxt.SetRect(0,0, iGoodGuessWidth, szInch.cy);
                }
                csStub = text.GetText();
                int iRepl = csStub.Replace(_T("@"), _T("%d"));
                ASSERT(iRepl==0 || iRepl==1);
                if (csStub.Find(_T("%d"))!=NONE)  {
                    text.SetText(csStub);
                    csStub.Format(CString(csStub),999);
                }
                rcTxt.BottomRight() -= CPoint(2*GRIDCELL_BORDER, 2*GRIDCELL_BORDER);
                rcTxt.OffsetRect(-rcTxt.TopLeft());

                dc.SelectObject(text.GetFont().GetCFont());
                dc.DrawText(csStub, &rcTxt, DT_CALCRECT|DT_LEFT|uDrawFormat);
                dc.SelectStockObject(OEM_FIXED_FONT);

                cell.GetMinSizeV() = rcTxt.Size();
                // Side row 1 color
                cell.SetBColor(m_cGridbkcolor);

                rcTxt.BottomRight() += CPoint(2*GRIDCELL_BORDER, 2*GRIDCELL_BORDER);
                rcMin = rcTxt;

                // if previously allocated area was larger, then use that (maybe user resized it on purpose)
                bool bUserResized = false;
                if (rcTxt.Width()<rcOld.Width())  {
                    rcTxt.right = rcTxt.left + rcOld.Width();
                    bUserResized = true;
                }
                if (rcTxt.Height()<rcOld.Height())  {
                    rcTxt.bottom = rcTxt.top + rcOld.Height();
                    bUserResized = true;
                }

                // adjust cell to accomodate the txt's rect
                rcCell.UnionRect(&rcCell, &rcTxt);
                rcTxt.OffsetRect(-rcCell.TopLeft());
                //text.SetDims(rcMin);     // was (rcTxt);      csc 12/21/
                //Savy 08/13/15 for retaining ths stubwidths changed by the user.
                // To Fix the grid stub width was getting resized while changing languages
                bUserResized ? text.SetDims(rcTxt) : text.SetDims(rcMin);

                m_pRoster->SetStubColWidth(__max(m_pRoster->GetStubColWidth(), rcTxt.Width()));
            }
            cell.SetRect(rcCell);
        }
    }

    // apply previous row height (horizontal) or col width (vertical) settings
    if (m_pRoster->GetOrientation() == RosterOrientation::Horizontal)  {
        ASSERT(m_pRoster->GetFieldRowHeight()>=0);
        for (iRow=1 ; iRow<iNumRows ; iRow++)  {
            CGridCell& cell = GetStubCell(iRow);
            CRect& rcCell = cell.GetRect();
            if (rcCell.Height()<m_pRoster->GetFieldRowHeight())  {
                rcCell.bottom = rcCell.top + m_pRoster->GetFieldRowHeight();
            }
        }
        ASSERT(m_pRoster->GetHeadingRowHeight()>=0);
        for (iCol=0 ; iCol<iNumCols ; iCol++)  {
            CGridCell& cell = GetHeaderCell(iCol);
            CRect& rcCell = cell.GetRect();
            if (rcCell.Height()<m_pRoster->GetHeadingRowHeight())  {
                rcCell.bottom = rcCell.top + m_pRoster->GetHeadingRowHeight();
            }
        }
    }
    else  {
        // vertical
        ASSERT(m_pRoster->GetColWidth()>=0);
        ASSERT(m_pRoster->GetHeadingRowHeight()>=0);
        for (iCol=1 ; iCol<iNumCols ; iCol++)  {
            CGridCell& cell = GetHeaderCell(iCol);
            CRect& rcCell = cell.GetRect();
            if (rcCell.Width()<m_pRoster->GetColWidth())  {
                rcCell.right = rcCell.left + m_pRoster->GetColWidth();
            }
            if (rcCell.Height()<m_pRoster->GetHeadingRowHeight())  {
                rcCell.bottom = rcCell.top + m_pRoster->GetHeadingRowHeight();
            }
        }
        ASSERT(m_pRoster->GetStubColWidth()>=0);
        for (iRow=1 ; iRow<iNumRows ; iRow++)  {
            CGridCell& cell = GetStubCell(iRow);
            CRect& rcCell = cell.GetRect();
            if (rcCell.Width()<m_pRoster->GetStubColWidth())  {
                rcCell.right = rcCell.left + m_pRoster->GetStubColWidth();
            }
        }
    }

    // measure cells
    for (iRow=1 ; iRow<iNumRows ; iRow++)  {
        CGridRow& row = m_aRow[iRow];   //1
        for (iCol=1 ; iCol<iNumCols ; iCol++)  {
            CGridCell& cell = row.GetCell(iCol);
            CRect rcCell(0,0,0,0);
            CPoint ptNextObject(0,0);  // used to line up text and fields when no dims available; text down the left, fields down the right
            for( CDEText& text : cell.GetTexts() ) {
                CRect rcTxt = text.GetDims();
                int iMaxWidth, iMaxHeight;
                iMaxWidth = GetHeaderCell(iCol).GetRect().Width();
                iMaxHeight = GetStubCell(iRow).GetRect().Height();
                if (rcTxt.IsRectNull())  {
                    rcTxt.SetRect(0,0, iMaxWidth, iMaxHeight);
                }
                dc.SelectObject(text.GetFont().GetCFont());
                dc.DrawText(text.GetText(), &rcTxt, DT_CALCRECT|DT_LEFT|DT_WORDBREAK);
                dc.SelectStockObject(OEM_FIXED_FONT);

                text.SetDims(rcTxt);
                ptNextObject.y = rcTxt.bottom+GRIDCELL_BORDER;
            }
            ptNextObject = CPoint(rcCell.right,0);
            for (int i = 0 ; i<cell.GetNumFields() ; i++)  {
                CDEField* pFld = cell.GetField(i).GetDEField();
                CRect rcFld = pFld->GetDims();
                if (rcFld.IsRectNull())  {
                    // position below prev flds (default!)
                    rcFld.TopLeft()=ptNextObject;
                    cell.GetField(i).SetFldPos(posAlignRight);
                }
                else  {
                    cell.GetField(i).SetFldPos(posNone);
                }

                if (!bFontInit)  {
                    // first time we've seen a CDEField ... set font and csprochar size info
                    //Savy for grid optimization- Field font is fixed for the all fields o the form file. GetTextExtent only one time
                    //as there is performance overhead for this call
                    bFontInit = true;
                    dc.SelectObject(pFld->GetFont().GetCFont());
                    szChar = dc.GetTextExtent(_T("0"), 1);
                    dc.RestoreDC(iSaveDC);
                }
                //int iLength = pFld->GetLength();

                const CDictItem* pDictItem = pFld->GetDictItem();
                if(!pDictItem)//SAVY added this to prevent crash 02/27/01
                    continue;
                ASSERT(NULL!=pDictItem);
                int iLength = pDictItem->GetLen();
                if(!pDictItem->GetDecChar()  && pDictItem->GetDecimal() != 0){
                    iLength++; //The length does not account for the decimal character in this case
                }

                ASSERT(szChar.cx>0 && szChar.cy>0);

                int iXB = GetSystemMetrics(SM_CXBORDER); //Border width
                int iYB = GetSystemMetrics(SM_CYBORDER); //Border height

                //int iXB =1 ; //border Width //get from system metrics
                //int iYB =1 ; //border Width //get from system metrics
                int iFldWidth = szChar.cx * iLength + 2*iXB;
                // add space for tick marks for all but alpha fields with arabic font
                CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");
                if(pDictItem->GetContentType() != ContentType::Alpha || !pFld->GetFont().IsArabic()) {
                    iFldWidth += (iLength-1)*GRIDSEP_SIZE + 2*iLength;
                }
                int iFldHeight = szChar.cy+2*iYB;
                if (rcFld.Width() != iFldWidth)  {
                    // changed csc 2/23/01
                    rcFld.right = rcFld.left +iFldWidth;
                }
                if (rcFld.Height() != iFldHeight){
                    rcFld.bottom = rcFld.top +iFldHeight;
                }

                bool bNewTextBox = pDictItem->GetContentType() == ContentType::Alpha && pFld->UseUnicodeTextBox() ;
                if(!bNewTextBox || pFld->GetDims() ==  CRect(0,0,0,0)){//do not change the dims for the new text box. Use the user give dims
                    pFld->SetDims(rcFld);
                }
                else {
                    //change the height back to default if the bNewText Box changed from multiline to single line
                    if(!pFld->AllowMultiLine()){
                        CRect currRect = pFld->GetDims();
                        if(currRect.Height() != rcFld.Height()){
                            currRect.bottom = rcFld.bottom;
                            pFld->SetDims(currRect);
                        }
                    }
                    if(pFld->GetDims().IsRectNull() && pFld->GetUnicodeTextBoxSize()!= CSize(0,0)){
                        rcFld.right = pFld->GetUnicodeTextBoxSize().cx;
                        rcFld.bottom = pFld->GetUnicodeTextBoxSize().cy;
                        pFld->SetUnicodeTextBoxSize(CSize(0,0));
                        pFld->SetDims(rcFld);
                    }
                }
                ptNextObject.y = rcFld.bottom+GRIDCELL_BORDER;
                rcFld.bottom += 2;
                rcFld.right += 2;
                rcCell.UnionRect(&rcCell, &rcFld);
            }
            cell.SetRect(rcCell);
        }
    }

    // make sure that headers are as wide as widest cell
    for (iCol=0 ; iCol<iNumCols ; iCol++)  {
        CGridCell& cellHeader = GetHeaderCell(iCol);
        for (iRow=1 ; iRow<iNumRows ; iRow++)  {
            CGridCell& cell = GetCell(iRow,iCol);
            for (int i = 0 ; i<cell.GetNumFields() ; i++)  {
                CDEField* pFld = cell.GetField(i).GetDEField();

                // csc 10/3/00 to fix prob: r-click relationship; chng txt to Rel; fld doesn't show
                if (cellHeader.GetRect().right < pFld->GetDims().right + GRIDCELL_BORDER*2)  {
                    cellHeader.GetRect().right = pFld->GetDims().right + GRIDCELL_BORDER*2;
                }
            }
        }
    }

    // make sure that stubs are as high as highest cell
    if (iNumCols>1)  {
        for (iRow=0 ; iRow<iNumRows ; iRow++)  {
            CGridCell& cellStub = GetStubCell(iRow);
            for (iCol=0 ; iCol<iNumCols ; iCol++)  {
                CGridCell& cell = GetCell(iRow, iCol);
                for (int i = 0 ; i<cell.GetNumFields() ; i++)  {
                    CDEField* pFld = cell.GetField(i).GetDEField();
                    if (cellStub.GetRect().bottom < pFld->GetDims().bottom + 2*GRIDCELL_BORDER)  {
                        cellStub.GetRect().bottom = pFld->GetDims().bottom + 2*GRIDCELL_BORDER;
                    }
                }
            }
        }
    }

    // give all headers same height
    int iMaxHeight=0;
    for (iCol=0 ; iCol<iNumCols ; iCol++)  {
        if (GetHeaderCell(iCol).GetRect().Height()>iMaxHeight)  {
            iMaxHeight = GetHeaderCell(iCol).GetRect().Height();
        }
    }
    for (iCol=0 ; iCol<iNumCols ; iCol++)  {
        CGridCell& cell = GetHeaderCell(iCol);
        CRect& rcCell = cell.GetRect();
        if (rcCell.Height()<iMaxHeight)  {
            rcCell.bottom = rcCell.top + iMaxHeight;
        }
    }

    // give all stubs same width
    int iMaxWidth=0;
    for (iRow=0 ; iRow<iNumRows ; iRow++)  {
        if (GetStubCell(iRow).GetRect().Width()>iMaxWidth)  {
            iMaxWidth = GetStubCell(iRow).GetRect().Width();
        }
    }
    for (iRow=0 ; iRow<iNumRows ; iRow++)  {
        CRect& rc = GetStubCell(iRow).GetRect();
        if (rc.Width()<iMaxWidth)  {
            rc.right = rc.left + iMaxWidth;
        }
    }

    // align headers and stub (left/center/right, top/middle/bottom)...  csc 12/21/00
    for (iCol=0 ; iCol<iNumCols ; iCol++)  {
        for (iRow=0 ; iRow<iNumRows ; iRow++)  {
            if (iRow!=0 && iCol!=0)  {
                continue;
            }
            CGridCell& cell = GetCell(iRow,iCol);
            CRect& rcCell = cell.GetRect();

            ASSERT(cell.GetNumTexts()>0);
            CDEText& text = cell.GetText(0);
            CRect rcTxt = text.GetDims();
            ASSERT(cell.GetRect().TopLeft()==CPoint(0,0));
            switch (text.GetHorizontalAlignmentOrDefault())  {
            case HorizontalAlignment::Left:
                rcTxt.OffsetRect(-rcTxt.left,0);
                break;
            case HorizontalAlignment::Center:
                rcTxt.OffsetRect((rcCell.right-rcTxt.Width())/2-rcTxt.left,0);
                break;
            case HorizontalAlignment::Right:
                rcTxt.OffsetRect(rcCell.right-rcTxt.Width()-rcTxt.left,0);
                break;
            }
            switch (text.GetVerticalAlignmentOrDefault())  {
            case VerticalAlignment::Top:
                rcTxt.OffsetRect(0,-rcTxt.top);
                break;
            case VerticalAlignment::Middle:
                rcTxt.OffsetRect(0,(rcCell.bottom-rcTxt.Height())/2-rcTxt.top);
                break;
            case VerticalAlignment::Bottom:
                rcTxt.OffsetRect(0,rcCell.bottom-rcTxt.Height()-rcTxt.top);
                break;
            }
            text.SetDims(rcTxt);
        }
    }

    // line up header across the top
    CPoint ptOffset(0,0);
    int iStartCol = m_pRoster->GetRightToLeft() ? 1 : 0; // for right to left, stub (in col 0) is drawn
                                                         // on right so start w. col 1
    for (iCol=iStartCol ; iCol<iNumCols ; iCol++)  {
        CRect& rc = GetHeaderCell(iCol).GetRect();
        rc.OffsetRect(-rc.TopLeft());
        rc.OffsetRect(ptOffset);
        ptOffset.x = rc.right;
    }

    // line up stubs down the left (or right if right to left is on)
    if (!m_pRoster->GetRightToLeft()) {
        ptOffset.x = 0; // for r to l, use curr value of ptOffset.x which is rt of last hdr cell
    }
    for (iRow=0 ; iRow<iNumRows ; iRow++)  {
        CRect& rc = GetStubCell(iRow).GetRect();
        rc.OffsetRect(-rc.TopLeft());
        rc.OffsetRect(ptOffset);
        ptOffset.y = rc.bottom;
    }

    // line up cells
    for (iRow=0 ; iRow<iNumRows ; iRow++)  {
        for (iCol=0 ; iCol<iNumCols ; iCol++)  {
            CRect& rcHeader = GetHeaderCell(iCol).GetRect();
            CRect& rcStub = GetStubCell(iRow).GetRect();
            CRect& rcCell = GetCell(iRow,iCol).GetRect();
            rcCell.SetRect(rcHeader.left, rcStub.top, rcHeader.right, rcStub.bottom);
        }
    }

    // remember column widths (or row heights if vertical)
    if (m_pRoster->GetOrientation() == RosterOrientation::Horizontal)  {
        for (iCol=1 ; iCol<iNumCols ; iCol++)  {
            // JH 12/05 fix bug where roster created w. non arabic has
            // columns set to wrong size when run in DE, in DE unlike in designer, this gets
            // called and then the grid gets rebuilt (grid is rebuilt twice) so we need to swap
            // cols when we save off the data so that it isn't reversed again the second time.
            int iRosterCol = iCol;
            if (m_pRoster->GetRightToLeft()) {
                // need to swap col if r to l
                iRosterCol = iNumCols - iRosterCol;
            }
            m_pRoster->GetCol(iRosterCol)->SetWidth(GetHeaderCell(iCol).GetRect().Width());   // csc 12/22/00
        }
    }
    else  {
        for (iRow=1 ; iRow<iNumRows ; iRow++)  {
            m_pRoster->GetCol(iRow)->SetWidth(GetStubCell(iRow).GetRect().Height());   // csc 12/22/00
        }
    }


    // position fields within cells
    // note: if orientation is horizontal,the we line up the pDEField*'s across the top
    //       if orientation is vertical,the we line up the pDEField*'s down the left
    if( m_pRoster->GetOrientation() == RosterOrientation::Horizontal ) {
        for (iCol=0 ; iCol<iNumCols ; iCol++)  {
            CGridCell& cell = GetCell(1, iCol);
            CRect& rcCell = cell.GetRect();
            for (int i = 0 ; i<cell.GetNumFields() ; i++)  {
                CCellField& fld = cell.GetField(i);
                CRect rcFld = fld.GetDEField()->GetDims();
                CPoint ptFldOffset(0,0);
                switch (fld.GetFldPos())  {
                case posBottomRight:
                    ptFldOffset = CPoint(rcCell.Width()-rcFld.Width()-2*GRIDCELL_BORDER, rcCell.Height()-rcFld.Height()-2*GRIDCELL_BORDER);
                    break;
                case posTopRight:
                    ptFldOffset = CPoint(rcCell.Width()-rcFld.Width()-2*GRIDCELL_BORDER, 0);
                    break;
                case posCenterRight:
                    ptFldOffset = CPoint(rcCell.Width()-rcFld.Width()-2*GRIDCELL_BORDER, rcCell.Height()-rcCell.Height()/2);
                    break;
                case posBottomLeft:
                    ptFldOffset = CPoint(0, rcCell.bottom-rcFld.Height()-2*GRIDCELL_BORDER);
                    break;
                case posCenterLeft:
                    ptFldOffset = CPoint(0, rcCell.top+rcCell.Height()/2);
                    break;
                case posAlignRight:
                    ptFldOffset = CPoint(rcCell.Width()-rcFld.Width()-2*GRIDCELL_BORDER,0);
                    break;
                case posAlignLeft:
                    ptFldOffset = CPoint(0,0);
                    break;
                case posTopLeft:
                case posNone:
                    ptFldOffset = CPoint(0,0);
                    break;
                default:
                    ASSERT(FALSE);
                }
                rcFld.OffsetRect(ptFldOffset);
                fld.GetDEField()->SetDims(rcFld);
            }
        }
    }
    else { // vertical
        for (iRow=0 ; iRow<iNumRows ; iRow++)  {
            CGridCell& cell = GetCell(iRow, 1);
            CRect& rcCell = cell.GetRect();
            for (int i = 0 ; i<cell.GetNumFields() ; i++)  {
                CCellField& fld = cell.GetField(i);
                CRect rcFld = fld.GetDEField()->GetDims();
                CPoint ptTopLeft(0,0);
                switch (fld.GetFldPos())  {
                case posBottomRight:
                    ptTopLeft = CPoint(rcCell.Width()-rcFld.Width()-2*GRIDCELL_BORDER, rcCell.Height()-rcFld.Height()-2*GRIDCELL_BORDER);
                    break;
                case posTopRight:
                    ptTopLeft = CPoint(rcCell.Width()-rcFld.Width()-2*GRIDCELL_BORDER, 0);
                    break;
                case posCenterRight:
                    ptTopLeft = CPoint(rcCell.Width()-rcFld.Width()-2*GRIDCELL_BORDER, rcCell.Height()-rcCell.Height()/2);
                    break;
                case posBottomLeft:
                    ptTopLeft = CPoint(0, rcCell.bottom-rcFld.Height()-2*GRIDCELL_BORDER);
                    break;
                case posCenterLeft:
                    ptTopLeft = CPoint(0, rcCell.top+rcCell.Height()/2);
                    break;
                case posAlignRight:
                    ptTopLeft = CPoint(rcCell.Width()-rcFld.Width()-2*GRIDCELL_BORDER,0);
                    break;
                case posAlignLeft:
                    ptTopLeft = CPoint(0,0);
                    break;
                case posTopLeft:
                case posNone:
                    ptTopLeft = CPoint(0,0);
                    break;
                default:
                    ASSERT(FALSE);
                }
                rcFld.OffsetRect(ptTopLeft);
                fld.GetDEField()->SetDims(rcFld);
            }
        }
    }

    // make sure that trackers are correctly positioned (if they're active)...
    for (int iTracker=0 ; iTracker<GetNumTrackers() ; iTracker++)  {
        CGridRectTracker& t = GetTracker(iTracker);
        CHitOb h = t.GetHitOb();
        CGridCell& cell = GetCell(h.GetCell().y, h.GetCell().x);
        CRect rc;
        t.GetTrueRect(&rc);
        InvalidateRect(rc);
        switch(t.GetTrackObject())  {
        case CGridRectTracker::trackField:
            ASSERT(h.GetField()!=NONE);
            rc = cell.GetField(h.GetField()).GetDEField()->GetDims();
            break;

        case CGridRectTracker::trackBox:
            ASSERT(h.GetBox()!=NONE);
            rc = cell.GetBox(h.GetBox()).GetDims();
            break;

        case CGridRectTracker::trackText:
            ASSERT(h.GetText()!=NONE);
            rc = cell.GetText(h.GetText()).GetDims();
            break;
        }
        rc.OffsetRect(cell.GetRect().TopLeft() - GetScrollPos() + CPoint(GetRightJustifyOffset(),0));
        rc.OffsetRect(GRIDCELL_BORDER, GRIDCELL_BORDER);

        t.m_rect = rc;
        t.GetTrueRect(&rc);
        InvalidateRect(&rc);
    }

    // resize scrollbars, etc
    CRect rcWnd;
    CSize szMin(GetMinSize(true)); // csc 1/31/01
    GetWindowRect(&rcWnd);
    GetParent()->ScreenToClient(&rcWnd);
    CPoint ptExtent(GetHeaderCell(iNumCols-1).GetRect().right, GetStubCell(iNumRows-1).GetRect().bottom);
    if (m_pRoster->GetRightToLeft()) {
        ptExtent.x = GetHeaderCell(0).GetRect().right; // for r to l, stub is on right
    }
    m_rcTotal.SetRect(0,0,ptExtent.x, ptExtent.y);
    m_rcTotal.OffsetRect(rcWnd.TopLeft());
    if (szNewPage!=CSize(-1,-1))  {
        rcWnd.BottomRight() = rcWnd.TopLeft()+szNewPage;
    }

    if (rcWnd.Size().cx<szMin.cx)  {
        rcWnd.right = rcWnd.left + szMin.cx;
    }
    if (rcWnd.Size().cy<szMin.cy)  {
        rcWnd.bottom = rcWnd.top + szMin.cy;
    }

    CSize szSB(::GetSystemMetrics(SM_CXVSCROLL), ::GetSystemMetrics(SM_CYHSCROLL));  // scrollbar sizes
    if (rcWnd.Size().cx<szMin.cx+szSB.cx && rcWnd.Height()<m_rcTotal.Height())  {
        rcWnd.right += szSB.cx;
    }
    if (rcWnd.Size().cy<szMin.cy+szSB.cy && rcWnd.Width()<m_rcTotal.Width())  {
        rcWnd.bottom += szSB.cy;
    }

    GetRoster()->SetDims(rcWnd+CPoint(GetParent()->GetScrollPos(SB_HORZ), GetParent()->GetScrollPos(SB_VERT)));


    if (bRedraw)  {
        Resize(rcWnd);
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::BuildGrid
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::BuildGrid()
{
    ASSERT(m_pRoster);

    m_aRow.RemoveAll();

    CClientDC dc(this);
    dc.SetMapMode(MM_TEXT);

    if (m_lNumColors==NONE)  {
        m_lNumColors = (__int64)1 << ( dc.GetDeviceCaps(BITSPIXEL) * dc.GetDeviceCaps(PLANES) );
    }

    // build header row
    CGridRow rowHead;
    for (int iCol=0 ; iCol<m_pRoster->GetNumCols() ; iCol++)  {
        CDECol* pCol = m_pRoster->GetCol(iCol);
        CGridCell cell;
        //Savy for MultiLanguage Support
        if(pCol->GetNumFields() == 1){
            pCol->GetHeaderText().SetLabel(pCol->GetField(0)->GetCDEText().GetLabel());
        }
        cell.AddText(pCol->GetSharedHeaderText());
        cell.SetBColor(m_cGridbkcolor);             // BMD  17 Apr 2001 // GetSysColor(COLOR_3DFACE)
        cell.SetFColor(rgbFHeaderDefault);
        cell.SetBSelColor(GetSysColor(COLOR_3DSHADOW));         // BMD  17 Apr 2001
        cell.SetFSelColor(rgbWhite);
        rowHead.AddCell(std::move(cell));
    }
    m_aRow.Add(rowHead);

    // build each row of cells
    CGridRow rowBody;

    // add cells (w/o stub text)...
    for (int iCol=0 ; iCol<m_pRoster->GetNumCols() ; iCol++)  {
        CDECol* pCol = m_pRoster->GetCol(iCol);
        CGridCell cell;

        cell.AddBoxSet(pCol->GetColumnCell().GetBoxSet());
        cell.AddTextSet(pCol->GetColumnCell().GetTextSet());

        for (int i = 0 ; i<pCol->GetNumFields() ; i++)  {
            CDEField* pFld = pCol->GetField(i);
            CCellField fld;
            fld.SetFldPos(posBottomRight);
            fld.SetDEField(pFld);
            cell.AddField(fld);
        }

        if (iCol==0)  {
            cell.SetBColor(m_cGridbkcolor);//GetSysColor(COLOR_3DFACE)              // BMD  17 Apr 2001
            cell.SetFColor(rgbFStubDefault);
            cell.SetBSelColor(GetSysColor(COLOR_3DSHADOW));         // BMD  17 Apr 2001
            cell.SetFSelColor(rgbWhite);
        }

        else // 20120606 adding color to the cells themselves
        {
            const double multiplicationFactor = 1.2;
            COLORREF slightlyLighterColor = RGB(std::min(255,(int)(GetRValue(m_cGridbkcolor) * multiplicationFactor)),
                                                std::min(255,(int) (GetGValue(m_cGridbkcolor) * multiplicationFactor)),
                                                std::min(255,(int) (GetBValue(m_cGridbkcolor) * multiplicationFactor)));
            cell.SetBColor(slightlyLighterColor);
        }

        rowBody.AddCell(std::move(cell));
    }

    for (int iRow=0 ; iRow<m_pRoster->GetMaxLoopOccs() ; iRow++)  {
        // set stub text (the only thing that differs btwn rows)
        CGridCell& stub = rowBody.GetCell(0);
        stub.RemoveAllTexts();
        stub.AddText(m_pRoster->GetStubTextSet().GetTexts().GetSharedPointerVector()[iRow]);
        m_aRow.Add(rowBody);
    }

    int iNumCols = GetNumCols();
    int iNumRows = GetNumRows();

    // add free cell texts and boxes
    for( CDEFreeCell& free_cell : m_pRoster->GetFreeCells() )
    {
        ASSERT(free_cell.GetColumn() > 0 && free_cell.GetColumn() < iNumCols);
        ASSERT(free_cell.GetRow() > 0 && free_cell.GetRow() < iNumRows);

        CGridCell& grid_cell = GetCell(free_cell.GetRow(), free_cell.GetColumn());

        grid_cell.AddBoxSet(free_cell.GetBoxSet());
        grid_cell.AddTextSet(free_cell.GetTextSet());
    }

    // handle orientation
    if (m_pRoster->GetOrientation() == RosterOrientation::Vertical)  {
        Transpose(RosterOrientation::Vertical);
        iNumCols = GetNumCols();
        iNumRows = GetNumRows();
    }

    // handle left to right/right to left
    if (m_pRoster->GetRightToLeft()) {
        // copy to backup
        CArray<CGridRow, CGridRow&> aOld;
        for (int iRow=0 ; iRow<iNumRows ; iRow++)  {
            aOld.Add(m_aRow[iRow]);
        }
        m_aRow.RemoveAll();

        // add with swapped cols
        for (int iRow=0 ; iRow<iNumRows ; iRow++)  {
            CGridRow row;
            row.AddCell(aOld[iRow].GetCell(0));
            for (int iCol=iNumCols-1 ; iCol>0 ; iCol--)  {
                row.AddCell(aOld[iRow].GetCell(iCol));
            }
            m_aRow.Add(row);
       }
    }

    // restore persisted col width (or row heights if vertical)
    if (m_pRoster->GetOrientation() == RosterOrientation::Horizontal)  {
        for (int iCol=1 ; iCol<iNumCols; iCol++)  {
            CGridCell& cell = GetHeaderCell(iCol);

            // JH 11/05/05 fix bug where roster created w. non arabic has
            // columns set to wrong size, wasn't swapping col index when
            // using saved col sizes from file
            int iRosterCol = iCol;
            if (m_pRoster->GetRightToLeft()) {
                // need to swap col if r to l
                iRosterCol = iNumCols - iRosterCol;
            }
            cell.SetRect(CRect(0,0,m_pRoster->GetCol(iRosterCol)->GetWidth(),1));   // csc 12/21/00
        }
    }
    else  {
        for (int iRow=1 ; iRow<iNumRows ; iRow++)  {
            CGridCell& cell = GetStubCell(iRow);
            cell.SetRect(CRect(0,0,1,m_pRoster->GetCol(iRow)->GetWidth()));   // csc 12/21/00
        }
    }

    // calculate minimum sizes for headers and stubs ... we need to do for all cells to accomodate horizontal/vertical combinations
    CSize szFieldFontTextExt(NONE, NONE);
    for (int iCol=0 ; iCol<iNumCols ; iCol++)  {
        //iNumRows = 1;
        for (int iRow=0 ; iRow<iNumRows ; iRow++)  {
            GetCell(iRow,iCol).CalcMinSize(&dc, szFieldFontTextExt);
        }
    }
}

void CGridWnd::RefreshOccLabelsStubText()
{
    if( m_pRoster != nullptr && m_pRoster->GetUseOccurrenceLabels() )
    {
        for( int iRow=0; iRow < m_pRoster->GetMaxLoopOccs(); ++iRow )
        {
            // set stub text (the only thing that differs btwn rows)
            CGridCell& stub = GetStubCell(iRow);
            stub.GetText(0).SetText(m_pRoster->GetStubTextSet().GetText(iRow).GetText());
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::SetColWidth()
//
// iWidth -- new width for the col
// iCol -- col to adjust; NONE to change *all* cols
// bRedraw -- true to force redraw
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::SetColWidth(int iWidth, int iCol /*=NONE*/, bool bRedraw /*=true*/)
{
    int iNumCols = GetNumCols();
    int iNumRows = GetNumRows();
    if (iCol==NONE)  {
        // adjust all cols
        ASSERT(GetRoster()->GetOrientation() == RosterOrientation::Vertical);
        GetRoster()->SetColWidth(iWidth);
        for (int i=1 ; i<iNumCols ; i++)  {
            SetColWidth(iWidth, i, false);
        }
    }
    else if (iCol==0) {
        // adjusting a stub column
        GetRoster()->SetStubColWidth(iWidth);
        for (int iRow=0 ; iRow<iNumRows ; iRow++)  {
            CGridCell& cell = GetCell(iRow, iCol);
            if (cell.GetNumTexts()>0)  {
                ASSERT(cell.GetNumTexts()==1);
                CDEText& text = cell.GetText(0);

                CRect rc = text.GetDims();
                rc.right = rc.left + iWidth;
//                rc.bottom = rc.top;            csc 12/22/00
                text.SetDims(rc);
                rc = cell.GetRect();
                rc.right = rc.left + iWidth;
//                rc.bottom = rc.top;             csc 12/22/00
                cell.SetRect(rc);
            }
        }
    }
    else  {
        CGridCell& cell = GetHeaderCell(iCol);
        if (cell.GetNumTexts()>0)  {
            ASSERT(cell.GetNumTexts()==1);
            CDEText& text = cell.GetText(0);
            CRect rc = text.GetDims();
            rc.right = rc.left + iWidth;
            text.SetDims(rc);
            rc = cell.GetRect();
            rc.right = rc.left + iWidth;
            cell.SetRect(rc);
            if (m_pRoster->GetOrientation() == RosterOrientation::Horizontal)  {
                ASSERT(iCol>=0 && iCol<iNumCols);
                m_pRoster->GetCol(iCol)->SetWidth(iWidth);  // csc 12/21/00
            }
        }

        for (int iRow=1 ; iRow<iNumRows ; iRow++)  {
            CGridCell& cellFld = GetCell(iRow,iCol);
            for (int iFld=0 ; iFld<cellFld.GetNumFields() ; iFld++)  {
                CDEField* pFld = cellFld.GetField(iFld).GetDEField();
                CRect rcFld = pFld->GetDims();
                ASSERT(rcFld.Width()+2*GRIDCELL_BORDER <= iWidth);
                if (rcFld.right + GRIDCELL_BORDER*2 > iWidth)  {
                    rcFld.OffsetRect(iWidth - rcFld.right - 2*GRIDCELL_BORDER,0);
                }
                pFld->SetDims(rcFld);
            }
        }
    }

    if (bRedraw)  {
        CPoint ptCorner(m_ptCorner);
        RecalcLayout();
        ScrollTo(ptCorner.y, ptCorner.x);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::SetRowHeight()
//
// iHeight -- new height for the row
// iRow -- row to adjust; NONE to change *all* rows
// bRedraw -- true to force redraw
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::SetRowHeight(int iHeight, int iRow /*=NONE*/, bool bRedraw /*=true*/)
{
    int iNumRows = GetNumRows();
    int iNumCols = GetNumCols();
    if (iRow==NONE)  {
        // adjust all rows
        ASSERT(GetRoster()->GetOrientation() == RosterOrientation::Horizontal);
        GetRoster()->SetFieldRowHeight(iHeight);
        for (int i=1 ; i<iNumRows ; i++)  {
            SetRowHeight(iHeight, i, false);
        }
    }
    else  if (iRow==0)  {
        // user is resizing the header row
        GetRoster()->SetHeadingRowHeight(iHeight);
        for (int iCol=0 ; iCol<iNumCols ; iCol++)  {
            CGridCell& cell = GetCell(iRow, iCol);
            if (cell.GetNumTexts()>0)  {
                ASSERT(cell.GetNumTexts()==1);
                CDEText& text = cell.GetText(0);
                CRect rc = text.GetDims();
                rc.bottom = rc.top + iHeight;
                text.SetDims(rc);
                rc = cell.GetRect();
                rc.bottom = rc.top + iHeight;
                cell.SetRect(rc);
            }
        }
    }
    else  {
        CGridCell& cell = GetStubCell(iRow);
        if (cell.GetNumTexts()>0)  {
            ASSERT(cell.GetNumTexts()==1);
            CDEText& text = cell.GetText(0);

            CRect rc = text.GetDims();
            rc.bottom = rc.top + iHeight;
            text.SetDims(rc);
            rc = cell.GetRect();
            rc.bottom = rc.top + iHeight;
            cell.SetRect(rc);
            if (m_pRoster->GetOrientation() == RosterOrientation::Vertical)  {
                ASSERT(iRow>=0 && iRow<iNumRows);
                m_pRoster->GetCol(iRow)->SetWidth(iHeight);  // csc 12/21/00
            }
        }

        for (int iCol=1 ; iCol<iNumCols ; iCol++)  {
            CGridCell& cellFld = GetCell(iRow,iCol);
            for (int iFld=0 ; iFld<cellFld.GetNumFields() ; iFld++)  {
                CDEField* pFld = cellFld.GetField(iFld).GetDEField();
                CRect rcFld = pFld->GetDims();
                ASSERT(rcFld.Height()+2*GRIDCELL_BORDER <= iHeight);
                if (rcFld.bottom + GRIDCELL_BORDER*2 > iHeight)  {
                    rcFld.OffsetRect(0,iHeight - rcFld.bottom - 2*GRIDCELL_BORDER);
                }
                pFld->SetDims(rcFld);
            }
        }
    }

    if (bRedraw)  {
        CPoint ptCorner(m_ptCorner);
        RecalcLayout();
        ScrollTo(ptCorner.y, ptCorner.x);
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::GetMinSize()
//
// min size is (2,2)
///////////////////////////////////////////////////////////////////////////////////////////////////
CSize CGridWnd::GetMinSize(bool bExcludeScrollBars /*=false*/) const
{
    ASSERT(GetNumCols()>1);
    ASSERT(GetNumRows()>1);
    CSize szRet(GetHeaderCell(1).GetRect().right, GetStubCell(1).GetRect().bottom);
    CSize szSB(::GetSystemMetrics(SM_CXVSCROLL), ::GetSystemMetrics(SM_CYHSCROLL));  // scrollbar sizes
    if (!bExcludeScrollBars)  {
        if (GetNumCols()>2)  {
            // need horz sb
            szRet.cy += szSB.cy;
        }
        if (GetNumRows()>2)  {
            // need vert sb
            szRet.cx += szSB.cx;
        }
    }
    return szRet;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CGridWnd::Transpose()
//
// usage: when user decides to transpose a grid, call Transpose().
// CFormGrid's orientation will be changed by this method.
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::Transpose(RosterOrientation orientation, bool bResetPositions /*=false*/)
{
    int iRow, iCol, iNumCols, iNumRows;
    CArray<CGridRow, CGridRow&> aOld;
    iNumRows = GetNumRows();
    iNumCols = GetNumCols();

    // copy to backup
    for (iRow=0 ; iRow<iNumRows ; iRow++)  {
        aOld.Add(m_aRow[iRow]);
    }
    m_aRow.RemoveAll();

    // add with swapped rows/cols
    for (iCol=0 ; iCol<iNumCols ; iCol++)  {
        CGridRow row;
        for (iRow=0 ; iRow<iNumRows ; iRow++)  {
            row.AddCell(aOld[iRow].GetCell(iCol));
        }
        m_aRow.Add(row);
    }
    ASSERT(GetNumCols()==iNumRows);
    ASSERT(GetNumRows()==iNumCols);

    if (bResetPositions)  {
        // reset rectangles, since measurements are all screwed up right now...
        iNumRows = GetNumRows();
        iNumCols = GetNumCols();
        for (iRow=0 ; iRow<iNumRows ; iRow++)  {
            CGridRow& row = m_aRow[iRow];
            for (iCol=0 ; iCol<iNumCols ; iCol++)  {
                CGridCell& cell = row.GetCell(iCol);
                cell.SetRectEmpty();
                for( CDEText& text : cell.GetTexts() ) {
                    text.SetDims(0,0,0,0);
                }
                for (int i = 0 ; i<cell.GetNumFields() ; i++)  {
                    CCellField& fld = cell.GetField(i);
                    fld.GetDEField()->SetDims(0,0,0,0);
                }
            }
        }
        for (iCol=0 ; iCol<m_pRoster->GetNumCols() ; iCol++)  {
            m_pRoster->GetCol(iCol)->SetWidth(0);
        }
    }

    // toggle orientation
    m_pRoster->SetOrientation(orientation);
}



/////////////////////////////////////////////////////////////////////////////////
//
//                  void CGridWnd::DrawBox()
//
// iRow, iCol indicate the cell where the drawing will occur
// pt is the point offset within that cell where the drawing starts
/////////////////////////////////////////////////////////////////////////////////
/*V*/ void CGridWnd::DrawBox(int iRow, int iCol, const CPoint& pt)
{
    CGridRectTracker trackBox;

    CRect rcCell(GetCell(iRow,iCol).GetRect());
    rcCell.DeflateRect(GRIDCELL_BORDER, GRIDCELL_BORDER);
    rcCell.OffsetRect(-GetScrollPos() + CPoint(GetRightJustifyOffset(),0));
    trackBox.SetBoundingRect(rcCell);
    CHitOb hitOb;
    hitOb.SetCell(CPoint(iCol,iRow));
    trackBox.SetHitOb(hitOb);
    trackBox.m_nStyle = CRectTracker::hatchedBorder | CRectTracker::resizeOutside;
    trackBox.SetTrackObject(CGridRectTracker::trackBox);
    RemoveAllTrackers();
    Deselect();

    CPoint ptTrack(pt);
    if (ptTrack.x<rcCell.left)  {
        ptTrack.x = rcCell.left;
    }
    if (ptTrack.y<rcCell.top)  {
       ptTrack.y = rcCell.top;
    }
    if (ptTrack.x>rcCell.right)  {
        ptTrack.x = rcCell.right;
    }
    if (ptTrack.y>rcCell.bottom)  {
        ptTrack.y = rcCell.bottom;
    }
    if (trackBox.TrackRubberBand(this, ptTrack, FALSE))  {
        CRect rc(trackBox.m_rect);
        rc.OffsetRect(-rcCell.TopLeft());
        OnAddBox(iRow, iCol, rc);
    }
}



/////////////////////////////////////////////////////////////////////////////////
//
//                  void CGridWnd::ChangeFont()
//
// sets all text objects in the grid to a specified font
/////////////////////////////////////////////////////////////////////////////////
void CGridWnd::ChangeFont(const PortableFont& font)
{
    CClientDC dc(this);
    dc.SetMapMode(MM_TEXT);

    bool bRecalc=false;
    int iNumCols = GetNumCols();
    int iNumRows = GetNumRows();
    CSize szFieldFontTextExt(NONE, NONE);

    for (int iCol=0 ; iCol<iNumCols ; iCol++)  {
        for (int iRow=0 ; iRow<iNumRows ; iRow++)  {
            CGridCell& cell = GetCell(iRow, iCol);
            if (cell.GetNumTexts() > 0)  {
                for( CDEText& text : cell.GetTexts() ) {
                    text.SetFont(font);
                }
                cell.CalcMinSize(&dc, szFieldFontTextExt);
                bRecalc=true;
            }
        }
    }

    if (bRecalc)  {
        // something changed!
        RecalcLayout();
    }
}










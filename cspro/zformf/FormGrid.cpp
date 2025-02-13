#include "StdAfx.h"
#include "FormGrid.h"
#include "ColumnPropertiesDlg.h"
#include "GridDlgs.h"
#include <zUtilO/BCMenu.h>


BEGIN_MESSAGE_MAP(CFormGrid, CGridWnd)
    ON_WM_PAINT()
    ON_COMMAND(ID_EDIT_TEXTPROP, OnEditTextProp)
    ON_COMMAND(ID_ADD_TEXT, OnAddText)
    ON_COMMAND(ID_EDIT_BOXPROP, OnEditBoxProp)
    ON_COMMAND(ID_EDIT_FIELDPROP, OnEditFieldProp)
    ON_COMMAND(ID_EDIT_MULTIPLE_FIELDPROP, OnEditMultipleFieldProperties)
    ON_COMMAND(ID_EDIT_COLUMNPROP, OnEditColumnProp)
    ON_COMMAND(ID_EDIT_STUBPROP, OnEditStubProp)
    ON_COMMAND(ID_ADD_FORM, OnAddForm)
    ON_COMMAND(ID_EDIT_GRIDPROP, OnEditGridProp)
    ON_COMMAND(ID_EDIT_GRID_AUTOFIT, OnEditGridAutoFit)
    ON_COMMAND(ID_VVIEW_LOGIC, OnViewLogic)
    ON_COMMAND(ID_VQSF_EDITOR, OnQSFEditor)
    ON_COMMAND(ID_DELETE_ITEM, OnDeleteItem)
    ON_COMMAND(ID_DELETE_COLUMN, OnDeleteColumn)
    ON_COMMAND(ID_SHOW_BOXTOOLBAR, OnShowBoxToolbar)
    ON_COMMAND(ID_EDIT_JOIN, OnEditJoin)
    ON_COMMAND(ID_EDIT_SPLIT, OnEditSplit)
    ON_COMMAND_RANGE(ID_LAYOUT_ALIGN_LEFT, ID_LAYOUT_ALIGN_BOTTOM, OnLayoutAlign)
    ON_COMMAND(ID_EDIT_PASTEF, OnEditPaste)
    ON_COMMAND(ID_EDIT_COPYF, OnEditCopy)
END_MESSAGE_MAP()


CFormGrid::CFormGrid(CDERoster* pRoster)
    :   m_pRoster(pRoster)
{
}


CFormScrollView* CFormGrid::GetFormView()
{
    return assert_cast<CFormScrollView*>(GetParent());
}

CFormDoc* CFormGrid::GetFormDoc()
{
    return GetFormView()->GetDocument();
}


/////////////////////////////////////////////////////////////////////////////
// CFormGrid message handlers


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormGrid::DeselectScrollViewItems()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CFormGrid::DeselectScrollViewItems()
{
    CFormScrollView* pView = GetFormView();
    pView->RemoveAllTrackers ();
    pView->RedrawWindow();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormGrid::OnTH_LClicked()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnTH_LClicked(int iCol)
{
    if (!OnCanSelectCol(iCol)) {
        return;
    }

    bool bCtrl = OnCanMultiSelect()?(GetKeyState(VK_CONTROL)<0):false;
    bool bShift = OnCanMultiSelect()?(GetKeyState(VK_SHIFT)<0):false;

    GetFormView()->RemoveAllTrackersAndRefresh();
    GetFormView()->RemoveSelectionsFromOtherRosters(this); // 20111114

    // highlight the column ONLY if the hdg txt is arranged across top
    if (GetRoster()->GetOrientation() ==  RosterOrientation::Horizontal) {
        SelectColumn(iCol, bCtrl, bShift, true, GetSysColor(COLOR_3DFACE));  // BMD 17 Apr 2001  RGB(128,255,200) chk 23 dec 2002
    }
    else {
        SelectCell(0,iCol,bCtrl,bShift, true, GetSysColor(COLOR_3DFACE));  // BMD 17 Apr 2001 RGB(128,255,200) chk 23 dec 2002
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CFormGrid::OnSH_LClicked()
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void CFormGrid::OnSH_LClicked(int iRow)
{
    if (!OnCanSelectRow(iRow)) {
        return;
    }

    bool bCtrl = OnCanMultiSelect()?(GetKeyState(VK_CONTROL)<0):false;
    bool bShift = OnCanMultiSelect()?(GetKeyState(VK_SHIFT)<0):false;

    CFormScrollView* pFV = GetFormView();
    pFV->SetCurItem (NULL);
    pFV->RemoveAllTrackersAndRefresh();
    GetFormView()->RemoveSelectionsFromOtherRosters(this); // 20111114


    // highlight the column ONLY if the hdg txt is arranged across top
    if (GetRoster()->GetOrientation() == RosterOrientation::Vertical) {
        SelectRow(iRow, bCtrl, bShift, true, GetSysColor(COLOR_3DFACE));  // BMD 17 Apr 2001
    }
    else {
        SelectCell(iRow,0,bCtrl,bShift, true, GetSysColor(COLOR_3DFACE));   // BMD 17 Apr 2001
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormGrid::OnCB_LClicked()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnCB_LClicked(const CPoint& pt)
{
    Deselect();

    CPoint point(pt), ptCorner(m_ptCorner);
    ClientToScreen(&point);
    GetParent()->ScreenToClient(&point);
    GetParent()->SendMessage(WM_LBUTTONDOWN, 0, MAKELONG(point.x ,point.y));
    ScrollTo(ptCorner.y, ptCorner.x);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormGrid::OnLClicked()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnCell_LClicked(int iRow, int iCol)
{
    OnCell_LClicked(CPoint(iCol, iRow));  // csc 10/15/00
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormGrid::OnCell_LClicked()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnCell_LClicked(const CPoint& pt)
{
    Deselect();
    GetFormView()->RemoveAllTrackersAndRefresh();

    // inform view to select entire grid ... CSC 10/15/00
    CPoint point(pt);
    ClientToScreen(&point);
    GetParent()->ScreenToClient(&point);
    GetParent()->SendMessage(WM_LBUTTONDOWN, 0, MAKELONG(point.x ,point.y));
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormGrid::OnRClicked()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
// this gets invoked when user clicks on open real estate w/in grid;
// i.e., not on a text, box, or field

void CFormGrid::OnCell_RClicked(int iRow, int iCol)
{
    CPoint pt;
    GetCursorPos(&pt);
    GetParent()->ScreenToClient(&pt);

    Deselect();
    CFormScrollView* pView = GetFormView();
    if (pView->SelectSingleItem (pView->GetDC(), pt)) {
        pView->SetCurItem();
    }

    pView->m_pRightClickItem = m_pRoster;
    m_hitOb.SetCell(CPoint(iCol, iRow));

    BCMenu popMenu;
    popMenu.CreatePopupMenu();
    popMenu.AppendMenu(MF_STRING, ID_EDIT_GRIDPROP, _T("&Properties"));
    popMenu.AppendMenu(MF_STRING, ID_EDIT_GRID_AUTOFIT, _T("AutoFit"));
    popMenu.AppendMenu(MF_STRING, ID_DELETE_ITEM, _T("&Delete"));
    popMenu.AppendMenu(MF_SEPARATOR);
    popMenu.AppendMenu(MF_STRING, ID_ADD_TEXT, _T("Add &Text"));
    popMenu.AppendMenu(MF_STRING, ID_SHOW_BOXTOOLBAR, _T("Add &Boxes"));
    popMenu.AppendMenu(MF_STRING, ID_ADD_FORM, _T("Add &Form"));

    popMenu.AppendMenu(MF_SEPARATOR);
    popMenu.AppendMenu(MF_STRING, ID_VVIEW_LOGIC, _T("View &Logic"));
    if (((CFormChildWnd*)(pView->GetParentFrame()))->GetUseQuestionText()) {
        popMenu.AppendMenu(MF_STRING, ID_VQSF_EDITOR, _T("View CAPI &Question"));
    }

    GetParent()->ClientToScreen(&pt);
    popMenu.LoadToolbar(IDR_FORM_FRAME);
    popMenu.TrackPopupMenu(TPM_RIGHTBUTTON, pt.x, pt.y, this);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormGrid::OnCellText_LClicked()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnCellText_LClicked(CHitOb& hitOb)
{
    int iRow, iCol, iTxt;
    bool bCtrl = OnCanMultiSelect()?(GetKeyState(VK_CONTROL)<0):false;
    bool bShift = OnCanMultiSelect()?(GetKeyState(VK_SHIFT)<0):false;
    iRow = hitOb.GetCell().y;
    iCol = hitOb.GetCell().x;
    iTxt = hitOb.GetText();

    // sanity checks
    if (m_pRoster->GetOrientation() == RosterOrientation::Horizontal) {
        ASSERT(iRow>=0 && iRow<= m_pRoster->GetMaxLoopOccs());  // smg 2003-10-07
        ASSERT(iCol>=0 && iCol<  m_pRoster->GetNumCols());
    }
    else {
        ASSERT(iCol>=0 && iCol<= m_pRoster->GetMaxLoopOccs());  // smg 2003-10-07
        ASSERT(iRow>=0 && iRow<  m_pRoster->GetNumCols());
    }
    CGridCell& cell = GetCell(iRow, iCol);
    ASSERT(iTxt>=0 && iTxt<(int)cell.GetNumTexts());

    GetFormView()->RemoveAllTrackersAndRefresh();
    if (OnCanSelectText(iRow, iCol, iTxt)) {
        SelectFldTxtBox(iRow, iCol, iTxt, CGridRectTracker::trackText, (bCtrl||bShift));
    }
    else {
        Deselect();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormGrid::OnCellBox_LClicked()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnCellBox_LClicked(CHitOb& hitOb)
{
    int iRow, iCol, iBox;
    bool bCtrl = OnCanMultiSelect()?(GetKeyState(VK_CONTROL)<0):false;
    bool bShift = OnCanMultiSelect()?(GetKeyState(VK_SHIFT)<0):false;
    iRow = hitOb.GetCell().y;
    iCol = hitOb.GetCell().x;
    iBox = hitOb.GetBox();

    // sanity checks
    if (m_pRoster->GetOrientation() == RosterOrientation::Horizontal) {
        ASSERT(iRow>=0 && iRow<=m_pRoster->GetMaxLoopOccs());    // csc 2003-10-7
        ASSERT(iCol>=0 && iCol<m_pRoster->GetNumCols());
    }
    else {
        ASSERT(iCol>=0 && iCol<=m_pRoster->GetMaxLoopOccs());    // csc 2003-10-7
        ASSERT(iRow>=0 && iRow<m_pRoster->GetNumCols());
    }
    CGridCell& cell = GetCell(iRow, iCol);
    ASSERT(iBox>=0 && iBox<(int)cell.GetNumBoxes());

    GetFormView()->RemoveAllTrackersAndRefresh();
    if (OnCanSelectBox(iRow, iCol, iBox)) {
        SelectFldTxtBox(iRow, iCol, iBox, CGridRectTracker::trackBox, (bCtrl||bShift));
    }
    else {
        Deselect();
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormGrid::OnCellField_RClicked()
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void CFormGrid::OnCellField_RClicked(CDEField* pFld, int iOcc)
{
    CPoint pt;
    GetCursorPos(&pt);
    GetParent()->ScreenToClient(&pt);

    // work backwards to find row/col...
    int i, j, iFld=NONE, iMax, iRow=NONE, iCol=NONE;
    iMax = ( m_pRoster->GetOrientation() == RosterOrientation::Horizontal ) ? GetNumCols() : GetNumRows();
    for (i = 0; i<iMax && iFld==NONE; ++i) {
        CGridCell& cell = ( m_pRoster->GetOrientation() == RosterOrientation::Horizontal ) ? GetCell(1,i) : GetCell(i,1);
        for (j = 0 ; j<cell.GetNumFields() ; j++) {
            if (cell.GetField(j).GetDEField()==pFld) {
                iFld = j;
                break;
            }
        }
    }
    i--;
    ASSERT(iFld!=NONE);
    if (m_pRoster->GetOrientation() == RosterOrientation::Horizontal) {
        iRow=iOcc;
        iCol=i;
    }
    else {
        iCol=iOcc;
        iRow=i;
    }

    GetFormView()->RemoveAllTrackersAndRefresh();
    if (OnCanSelectField(iRow, iCol, iFld)) {
        bool bMulti = IsFieldSelected(iRow, iCol, iFld);   // idiosyncracies of right-clicking logic
        if (bMulti) {
            bool bOK=false;
            for (i = 0; i < GetNumTrackers(); ++i) {
                CGridRectTracker &t = GetTracker(i);
                if (t.GetHitOb().GetCell()==CPoint(iCol,iRow) && t.GetHitOb().GetField()==iFld && t.GetTrackObject()==CGridRectTracker::trackField) {
                    // idiosyncracies of right-clicking logic
                    DeselectTracker(i);
                    bOK=true;
                    break;
                }
            }
            ASSERT(bOK);
        }
        SelectFldTxtBox(iRow, iCol, iFld, CGridRectTracker::trackField, bMulti, true);  // won't track!
        m_hitOb.SetCell(CPoint(iCol, iRow));
        m_hitOb.SetField(iFld);

        // decide whether or not we should grey out properties menu
        ASSERT(GetNumTrackers(CGridRectTracker::trackField)>0);
//        bool bActive = (GetNumTrackers(CGridRectTracker::trackBox)+GetNumTrackers(CGridRectTracker::trackText)==0);
        bool bActive = (GetNumTrackers(CGridRectTracker::trackBox)+GetNumTrackers(CGridRectTracker::trackText)==0) && GetNumTrackers(CGridRectTracker::trackField)==1;  // csc 2/2/01

        BCMenu popMenu;
        popMenu.CreatePopupMenu();
        popMenu.AppendMenu((bActive?MF_STRING:MF_GRAYED), ID_EDIT_FIELDPROP, _T("Field &Properties"));
        popMenu.AppendMenu(MF_STRING, ID_DELETE_ITEM, _T("&Delete"));
        popMenu.AppendMenu(MF_SEPARATOR);
        popMenu.AppendMenu(MF_STRING, ID_ADD_TEXT, _T("Add &Text"));
        popMenu.AppendMenu(MF_STRING, ID_SHOW_BOXTOOLBAR, _T("Add &Boxes"));
        popMenu.AppendMenu(MF_STRING, ID_ADD_FORM, _T("Add &Form"));

        popMenu.AppendMenu(MF_SEPARATOR);
        popMenu.AppendMenu(MF_STRING, ID_VVIEW_LOGIC, _T("View &Logic"));
        if (((CFormChildWnd*)(GetFormView()->GetParentFrame()))->GetUseQuestionText()) {
            popMenu.AppendMenu(MF_STRING, ID_VQSF_EDITOR, _T("View CAPI &Question"));
        }

        GetParent()->ClientToScreen(&pt);
        popMenu.LoadToolbar(IDR_FORM_FRAME);
        popMenu.TrackPopupMenu(TPM_RIGHTBUTTON, pt.x, pt.y, this);

        GetFormView()->SetCurItem(pFld);
    }
    else {
        Deselect();
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormGrid::OnCellText_RClicked()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnCellText_RClicked(CHitOb& hitOb)
{
    CPoint pt;
    GetCursorPos(&pt);
    GetParent()->ScreenToClient(&pt);

    int i, iCol, iRow, iTxt;
    iCol = hitOb.GetCell().x;
    iRow = hitOb.GetCell().y;
    iTxt = hitOb.GetText();
    ASSERT(iTxt>=0 && iTxt<(int)GetCell(iRow, iCol).GetNumTexts());

    CFormScrollView* pFV = GetFormView();

    pFV->RemoveAllTrackersAndRefresh();
    if (OnCanSelectText(iRow, iCol, iTxt)) {
        bool bMulti = IsTextSelected(iRow, iCol, iTxt);   // idiosyncracies of right-clicking logic
        if (bMulti) {
            bool bOK=false;
            for (i = 0; i < GetNumTrackers(); ++i) {
                CGridRectTracker &t = GetTracker(i);
                if (t.GetHitOb().GetCell()==CPoint(iCol,iRow) && t.GetHitOb().GetText()==iTxt && t.GetTrackObject()==CGridRectTracker::trackText) {
                    // idiosyncracies of right-clicking logic
                    DeselectTracker(i);
                    bOK=true;
                    break;
                }
            }
            ASSERT(bOK);
        }
        SelectFldTxtBox(iRow, iCol, iTxt, CGridRectTracker::trackText, bMulti, true);  // won't track!
        m_hitOb.SetCell(CPoint(iCol, iRow));
        m_hitOb.SetText(iTxt);

        // decide whether or not we should grey out properties menu
        ASSERT(GetNumTrackers(CGridRectTracker::trackText)>0);
        bool bActive = (GetNumTrackers(CGridRectTracker::trackField)+GetNumTrackers(CGridRectTracker::trackBox)==0);

        BCMenu popMenu;
        popMenu.CreatePopupMenu();
        popMenu.AppendMenu((bActive?MF_STRING:MF_GRAYED), ID_EDIT_TEXTPROP, _T("&Properties"));
        popMenu.AppendMenu(MF_STRING, ID_DELETE_ITEM, _T("&Delete"));
        popMenu.AppendMenu(MF_SEPARATOR);
        popMenu.AppendMenu(MF_STRING, ID_ADD_TEXT, _T("Add &Text"));
        popMenu.AppendMenu(MF_STRING, ID_SHOW_BOXTOOLBAR, _T("Add &Boxes"));
        popMenu.AppendMenu(MF_STRING, ID_ADD_FORM, _T("Add &Form"));

        popMenu.AppendMenu(MF_SEPARATOR);
        popMenu.AppendMenu(MF_STRING, ID_VVIEW_LOGIC, _T("View &Logic"));
        if (((CFormChildWnd*)(GetFormView()->GetParentFrame()))->GetUseQuestionText()) {
            popMenu.AppendMenu(MF_STRING, ID_VQSF_EDITOR, _T("View CAPI &Question"));
        }

        GetParent()->ClientToScreen(&pt);
        popMenu.LoadToolbar(IDR_FORM_FRAME);
        popMenu.TrackPopupMenu(TPM_RIGHTBUTTON, pt.x, pt.y, this);
    }
    else {
        Deselect();
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormGrid::OnCellBox_RClicked()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnCellBox_RClicked(CHitOb& hitOb)
{
    CPoint pt;
    GetCursorPos(&pt);
    GetParent()->ScreenToClient(&pt);

    int i, iCol, iRow, iBox;
    iCol = hitOb.GetCell().x;
    iRow = hitOb.GetCell().y;
    iBox = hitOb.GetBox();
    ASSERT(iCol>=0 && GetNumCols());
    ASSERT(iBox>=0 && iBox<(int)GetCell(iRow, iCol).GetNumBoxes());

    GetFormView()->RemoveAllTrackersAndRefresh();
    if (OnCanSelectBox(iRow, iCol, iBox)) {
        bool bMulti = IsBoxSelected(iRow, iCol, iBox);   // idiosyncracies of right-clicking logic
        if (bMulti) {
            bool bOK=false;
            for (i = 0; i < GetNumTrackers(); ++i) {
                CGridRectTracker &t = GetTracker(i);
                if (t.GetHitOb().GetCell()==CPoint(iCol,iRow) && t.GetHitOb().GetBox()==iBox && t.GetTrackObject()==CGridRectTracker::trackBox) {
                    // idiosyncracies of right-clicking logic
                    DeselectTracker(i);
                    bOK=true;
                    break;
                }
            }
            ASSERT(bOK);
        }
        SelectFldTxtBox(iRow, iCol, iBox, CGridRectTracker::trackBox, bMulti, true);  // won't track!
        m_hitOb.SetCell(CPoint(iCol, iRow));
        m_hitOb.SetBox(iBox);

        // decide whether or not we should grey out properties menu
        ASSERT(GetNumTrackers(CGridRectTracker::trackBox)>0);
        bool bActive = (GetNumTrackers(CGridRectTracker::trackField)+GetNumTrackers(CGridRectTracker::trackText)==0);

        BCMenu popMenu;
        popMenu.CreatePopupMenu();
        popMenu.AppendMenu((bActive?MF_STRING:MF_GRAYED), ID_EDIT_BOXPROP, _T("&Properties"));
        popMenu.AppendMenu(MF_STRING, ID_DELETE_ITEM, _T("&Delete"));
        popMenu.AppendMenu(MF_SEPARATOR);
        popMenu.AppendMenu(MF_STRING, ID_ADD_TEXT, _T("Add &Text"));
        popMenu.AppendMenu(MF_STRING, ID_SHOW_BOXTOOLBAR, _T("Add &Boxes"));
        popMenu.AppendMenu(MF_STRING, ID_ADD_FORM, _T("Add &Form"));

        popMenu.AppendMenu(MF_SEPARATOR);
        popMenu.AppendMenu(MF_STRING, ID_VVIEW_LOGIC, _T("View &Logic"));
        if (((CFormChildWnd*)(GetFormView()->GetParentFrame()))->GetUseQuestionText()) {
            popMenu.AppendMenu(MF_STRING, ID_VQSF_EDITOR, _T("View CAPI &Question"));
        }

        GetParent()->ClientToScreen(&pt);
        popMenu.LoadToolbar(IDR_FORM_FRAME);
        popMenu.TrackPopupMenu(TPM_RIGHTBUTTON, pt.x, pt.y, this);
    }
    else {
        Deselect();
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormGrid::OnCellField_LClicked()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnCellField_LClicked(CDEField* pFld, int iOcc)
{
    bool bCtrl = OnCanMultiSelect()?(GetKeyState(VK_CONTROL)<0):false;
    bool bShift = OnCanMultiSelect()?(GetKeyState(VK_SHIFT)<0):false;

    // work backwards to find row/col...
    int i, j, iFld=NONE, iMax, iRow=NONE, iCol=NONE;
    iMax = ( m_pRoster->GetOrientation() == RosterOrientation::Horizontal ) ? GetNumCols() : GetNumRows();
    for (i = 0; i<iMax && iFld==NONE; ++i) {
        CGridCell& cell = ( m_pRoster->GetOrientation() == RosterOrientation::Horizontal ) ? GetCell(1,i) : GetCell(i,1);
        for (j = 0 ; j<cell.GetNumFields() ; j++) {
            if (cell.GetField(j).GetDEField()==pFld) {
                iFld = j;
                break;
            }
        }
    }
    i--;
    ASSERT(iFld!=NONE);
    if (m_pRoster->GetOrientation() == RosterOrientation::Horizontal) {
        iRow=iOcc;
        iCol=i;
    }
    else {
        iCol=iOcc;
        iRow=i;
    }

    GetFormView()->RemoveAllTrackersAndRefresh();
    GetFormView()->UnselectRosterRowOrCol(); // 20120626 cells on other rosters weren't getting unselected

    if (OnCanSelectField(iRow, iCol, iFld)) {
        SelectFldTxtBox(iRow, iCol, iFld, CGridRectTracker::trackField, (bCtrl||bShift));
    }
    else {
        Deselect();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CFormGrid::OnSH_RClicked()
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void CFormGrid::OnSH_RClicked(int iRow)
{
    CFormScrollView* pFV = GetFormView();
    pFV->RemoveAllTrackersAndRefresh();

    if (OnCanSelectRow(iRow)) {
        if (!IsRowSelected(iRow) && !IsCellSelected(iRow,0)) {
            if (GetRoster()->GetOrientation() == RosterOrientation::Vertical)    {
                DeselectRows();
                SelectRow(iRow, true, false, true, GetSysColor(COLOR_3DFACE));  // BMD 17 Apr 2001
            }
            else {
                DeselectCells();
                SelectCell(iRow, 0, true, false, true, GetSysColor(COLOR_3DFACE));  // BMD 17 Apr 2001
            }
        }
    }

    BCMenu popMenu;
    popMenu.CreatePopupMenu();
    bool bIsVert = (m_pRoster->GetOrientation() == RosterOrientation::Vertical);
    bool bActive = (m_aiSelRow.GetSize()>0 || m_aptSelCell.GetSize()>0);

    if (bIsVert) {
        pFV->m_pRightClickRoster = m_pRoster;
        pFV->m_pRightClickItem = m_pRoster->GetItem (this->GetCurRow()-1);
        popMenu.AppendMenu(bActive?MF_STRING:MF_GRAYED, ID_EDIT_COLUMNPROP, _T("Row &Properties"));

        if( m_aiSelRow.GetSize() > 1 ) // 20120612
            popMenu.AppendMenu(bActive ? MF_STRING : MF_GRAYED,ID_EDIT_MULTIPLE_FIELDPROP,_T("&Field Properties"));

        popMenu.AppendMenu(MF_STRING, ID_DELETE_COLUMN, _T("&Delete"));
    }
    else {
        popMenu.AppendMenu(bActive?MF_STRING:MF_GRAYED, ID_EDIT_STUBPROP, _T("Row &Properties"));
        popMenu.AppendMenu(MF_GRAYED, ID_DELETE_COLUMN, _T("&Delete"));
    }
        popMenu.AppendMenu(MF_SEPARATOR);

    if (bIsVert)    // don't want these menu items showing up at all if not vert orientation
    {
        if (m_aiSelRow.GetSize() == 1) {
            popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_EDIT_JOIN, _T("&Join"));
            ASSERT(m_aiSelRow[0]==GetCurRow());
            CGridCell& cell = GetCell(GetCurRow(),1);
            ASSERT(cell.GetNumFields() > 0);
            if (cell.GetNumFields() > 1)
                popMenu.AppendMenu(MF_STRING, ID_EDIT_SPLIT, _T("&Split"));
            else
                popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_EDIT_SPLIT, _T("&Split"));
        }
        else {
            popMenu.AppendMenu(MF_STRING, ID_EDIT_JOIN, _T("&Join"));
            popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_EDIT_SPLIT, _T("&Split"));
        }
        popMenu.AppendMenu(MF_SEPARATOR);
    }
    popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_ADD_TEXT, _T("Add &Text"));
    popMenu.AppendMenu(MF_STRING, ID_SHOW_BOXTOOLBAR, _T("Add &Boxes"));
    popMenu.AppendMenu(MF_STRING, ID_ADD_FORM, _T("Add &Form"));

    popMenu.AppendMenu(MF_SEPARATOR);
    popMenu.AppendMenu(MF_STRING, ID_VVIEW_LOGIC, _T("View &Logic"));
    if (((CFormChildWnd*)(GetFormView()->GetParentFrame()))->GetUseQuestionText()) {
        popMenu.AppendMenu(MF_STRING, ID_VQSF_EDITOR, _T("View CAPI &Question"));
    }

    CHitOb h;
    h.SetCell(CPoint(iRow,NONE));
    m_hitOb = h;  // signals that nothing was hit

    CPoint pt;
    GetCursorPos(&pt);
    popMenu.LoadToolbar(IDR_FORM_FRAME);
    popMenu.TrackPopupMenu(TPM_RIGHTBUTTON, pt.x, pt.y, this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CFormGrid::OnTH_RClicked()
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void CFormGrid::OnTH_RClicked(int iCol)
{
    CFormScrollView* pFV = GetFormView();

    pFV->RemoveAllTrackersAndRefresh();

    if (OnCanSelectCol(iCol)) {
        if (!IsColSelected(iCol) && !IsCellSelected(0,iCol)) {
            if (GetRoster()->GetOrientation() == RosterOrientation::Horizontal) {
                DeselectColumns();
                SelectColumn(iCol, true, false, true, GetSysColor(COLOR_3DFACE) );  // BMD 17 Apr 2001 RGB(128,255,200) chk 23/12/02
            }
            else {
                DeselectCells();
                SelectCell(0,iCol, true, false, true, GetSysColor(COLOR_3DFACE));  // BMD 17 Apr 2001 RGB(128,255,200) chk 23/12/02
            }
        }
    }

    BCMenu popMenu;
    popMenu.CreatePopupMenu();
    bool bActive = ( m_aiSelCol.GetSize() > 0 || m_aptSelCell.GetSize() > 0 );
    bool bIsHorz = ( m_pRoster->GetOrientation() == RosterOrientation::Horizontal );

    if (bIsHorz) {
        pFV->m_pRightClickItem = m_pRoster;
        pFV->m_iRosterColIndex = iCol;         // smg: who uses this? don't think anyone
        popMenu.AppendMenu(bActive?MF_STRING:MF_GRAYED, ID_EDIT_COLUMNPROP, _T("Column &Properties"));

        if( m_aiSelCol.GetSize() > 1 ) // 20120612
            popMenu.AppendMenu(bActive ? MF_STRING : MF_GRAYED,ID_EDIT_MULTIPLE_FIELDPROP,_T("&Field Properties"));

        popMenu.AppendMenu(MF_STRING, ID_DELETE_COLUMN, _T("&Delete"));
    }
    else {
        popMenu.AppendMenu(bActive?MF_STRING:MF_GRAYED, ID_EDIT_STUBPROP, _T("Column &Properties"));
        popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_DELETE_COLUMN, _T("&Delete"));
    }
    popMenu.AppendMenu(MF_SEPARATOR);

    if (bIsHorz) {  // if the orientation isn't horz, don't want these to show up at all

        if (m_aiSelCol.GetSize() == 1) {
            popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_EDIT_JOIN, _T("&Join"));

            ASSERT(m_aiSelCol[0]==GetCurCol());
            CGridCell& cell = GetCell(1,GetCurCol());
            ASSERT(cell.GetNumFields()>0);
            if (cell.GetNumFields() > 1)
                popMenu.AppendMenu(MF_STRING, ID_EDIT_SPLIT, _T("&Split"));
            else
                popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_EDIT_SPLIT, _T("&Split"));
        }
        else {
            popMenu.AppendMenu(MF_STRING, ID_EDIT_JOIN, _T("&Join"));
            popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_EDIT_SPLIT, _T("&Split"));
        }
        popMenu.AppendMenu(MF_SEPARATOR);
    }

    popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_ADD_TEXT, _T("Add &Text"));
    popMenu.AppendMenu(MF_STRING, ID_SHOW_BOXTOOLBAR, _T("Add &Boxes"));
    popMenu.AppendMenu(MF_STRING, ID_ADD_FORM, _T("Add &Form"));

    popMenu.AppendMenu(MF_SEPARATOR);
    popMenu.AppendMenu(MF_STRING, ID_VVIEW_LOGIC, _T("View &Logic"));
    if (((CFormChildWnd*)(GetFormView()->GetParentFrame()))->GetUseQuestionText()) {
        popMenu.AppendMenu(MF_STRING, ID_VQSF_EDITOR, _T("View CAPI &Question"));
    }
    CHitOb h;
    h.SetCell(CPoint(iCol, NONE));
    m_hitOb = h;  // signals that nothing was hit

    CPoint pt;
    GetCursorPos(&pt);
    popMenu.LoadToolbar(IDR_FORM_FRAME);
    popMenu.TrackPopupMenu(TPM_RIGHTBUTTON, pt.x, pt.y, this);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                      CFormGrid::OnCB_RClicked()
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void CFormGrid::OnCB_RClicked(const CPoint& pt)
{
    Deselect();

    CPoint point(pt);
    ClientToScreen(&point);
    GetParent()->ScreenToClient(&point);

    CFormScrollView* pView = GetFormView();
    ASSERT(pView->okToSelectItems());
    if (pView->SelectSingleItem (pView->GetDC(), point)) {
        pView->SetCurItem();
    }

    pView->m_pRightClickItem = m_pRoster;

    BCMenu popMenu;
    popMenu.CreatePopupMenu();
    popMenu.AppendMenu(MF_STRING, ID_EDIT_GRIDPROP, _T("Roster &Properties"));
    popMenu.AppendMenu(MF_STRING, ID_EDIT_GRID_AUTOFIT, _T("AutoFit"));
    popMenu.AppendMenu(MF_STRING, ID_DELETE_ITEM, _T("&Delete"));
    popMenu.AppendMenu(MF_SEPARATOR);
    popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_ADD_TEXT, _T("Add &Text"));
    popMenu.AppendMenu(MF_STRING, ID_SHOW_BOXTOOLBAR, _T("Add &Boxes"));
    popMenu.AppendMenu(MF_STRING, ID_ADD_FORM, _T("Add &Form"));

    popMenu.AppendMenu(MF_SEPARATOR);
    popMenu.AppendMenu(MF_STRING, ID_VVIEW_LOGIC, _T("View &Logic"));
    if (((CFormChildWnd*)(pView->GetParentFrame()))->GetUseQuestionText()) {
        popMenu.AppendMenu(MF_STRING, ID_VQSF_EDITOR, _T("View CAPI &Question"));
    }

    GetParent()->ClientToScreen(&point);
    popMenu.LoadToolbar(IDR_FORM_FRAME);
    popMenu.TrackPopupMenu(TPM_RIGHTBUTTON, point.x, point.y, this);
}


/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnKeyDown()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnKeyDown(UINT* piKey, bool bProcessed)
{
    if (!bProcessed) {
        if (IsTrackerActive() && *piKey==VK_DELETE) {
            CGridRectTracker& t = GetTracker(0);
            m_hitOb = t.GetHitOb();
            SendMessage(WM_COMMAND, ID_DELETE_ITEM);
        }
    }
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormGrid::OnCanSelectCol()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CFormGrid::OnCanSelectCol(int) const
{
    return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormGrid::OnCanSelectRow()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CFormGrid::OnCanSelectRow(int) const
{
    return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormGrid::OnCanSelectCell()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CFormGrid::OnCanSelectCell(int, int) const
{
    return false;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormGrid::OnCanDrawBox()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CFormGrid::OnCanDrawBox(int /*iRow*/, int /*iCol*/)
{
    return GetFormView()->okToDrawBox();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormGrid::OnCanSwapCol()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CFormGrid::OnCanSwapCol() const
{
    return ( m_pRoster->GetOrientation() == RosterOrientation::Horizontal );
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormGrid::OnCanSwapRow()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CFormGrid::OnCanSwapRow() const
{
    return ( m_pRoster->GetOrientation() == RosterOrientation::Vertical );
}


bool CFormGrid::OnCanSizeCol(int /*iCol*/) const
{
    return true;

}

bool CFormGrid::OnCanSizeRow(int /*iRow*/) const
{
    return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormGrid::OnSwappedCol()
//
//  the user has swapped the positions of two columns in the grid; now i need to
//  reflect that change in the roster, so the two are in sync (or, conversely,
//  do all changes to the grid, and on destruction of the grid update the roster
//  w/its changes)
// csc 8/28/00
///////////////////////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnSwappedCol(const std::vector<int>& dragItemIndexes, int iDropItem)
{
    OnSwappedRowOrCol(dragItemIndexes, iDropItem);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CFormGrid::OnSwappedRow()
//
// Remember, even though we're swapping rows, since our orientation is vertical we need to move
// the corresponding CDECol ptrs.
// csc 8/28/00
///////////////////////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnSwappedRow(const std::vector<int>& dragItemIndexes, int iDropItem)
{
    OnSwappedRowOrCol(dragItemIndexes, iDropItem);
}

void CFormGrid::OnSwappedRowOrCol(const std::vector<int>& dragItemIndexes, int iDropItem)
{
    CDERoster* pRoster = GetRoster();

    CDECol* pDropColumn = iDropItem < pRoster->GetNumCols() ? pRoster->GetCol(iDropItem) : nullptr;
    std::vector<CDECol*> dragColumns;
    std::transform(dragItemIndexes.begin(),
        dragItemIndexes.end(),
        std::back_inserter(dragColumns),
        [pRoster](int i) { return pRoster->GetCol(i); });

    for (CDECol* pDragCol : dragColumns) {
        pRoster->RemoveCol(pDragCol);
        for (int iColFld = 0; iColFld < pDragCol->GetNumFields(); ++iColFld)
        {
            CDEField* pDragFld = pDragCol->GetField(iColFld);
            CDEBlock* pBlock = pRoster->GetBlock(pDragFld);
            if (pBlock)
                pBlock->RemoveField();
            pRoster->RemoveItemAt(pRoster->GetItemIndex(pDragFld));
        }
    }

    int iDropColumn = pDropColumn ? pRoster->GetColIndex(pDropColumn) : pRoster->GetNumCols();
    int iGroupFieldInsertIndex = pRoster->GetGroupInsertIndexForCol(iDropColumn);

    for (CDECol* pDragCol : dragColumns)
    {
        pRoster->InsertColAt(pDragCol, iDropColumn++);
        for (int iColFld = 0; iColFld < pDragCol->GetNumFields(); ++iColFld)
        {
            CDEField* pDragFld = pDragCol->GetField(iColFld);
            pRoster->InsertItemAt(pDragFld, iGroupFieldInsertIndex);
            CDEBlock* pBlock = pRoster->GetBlockForFieldAt(iGroupFieldInsertIndex);
            if (pBlock)
                pBlock->AddField();
            ++iGroupFieldInsertIndex;
        }
    }

    GetFormDoc()->GetFormTreeCtrl()->ReBuildTree(pRoster->GetFormNum(), pRoster);
    GetFormDoc()->SetModifiedFlag();

    if (pRoster->GetOrientation() == RosterOrientation::Horizontal) {
        for (CDECol* pDragCol : dragColumns)
        {
            SelectColumn(pRoster->AdjustColIndexForRightToLeft(pRoster->GetColIndex(pDragCol)), pDragCol != dragColumns.front(), false, pDragCol == dragColumns.back(), GetSysColor(COLOR_3DFACE));
        }
    }
    else {
        for (CDECol* pDragCol : dragColumns)
        {
            SelectRow(pRoster->GetColIndex(pDragCol), pDragCol != dragColumns.front(), false, pDragCol == dragColumns.back(), GetSysColor(COLOR_3DFACE));
        }
    }

}

/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnResizedCol()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnResizedCol(int /*iCol*/)
{
    GetFormDoc()->SetModifiedFlag();
}



/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnResizedCol()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnResizedRow(int /*iRow*/)
{
    GetFormDoc()->SetModifiedFlag();
}


/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnBoxMoved()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnBoxMoved(int, int, int)
{
    GetFormDoc()->SetModifiedFlag();
}


/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnTextMoved()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnTextMoved(int /*iRow*/, int /*iCol*/, int /*iTxt*/)
{
    GetFormDoc()->SetModifiedFlag();
}


/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnFieldMoved()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnFieldMoved(int /*iRow*/, int /*iCol*/, int /*iFld*/)
{
    GetFormDoc()->SetModifiedFlag();
}


/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnEditTextProp()
//
/////////////////////////////////////////////////////////////////////////////////
struct EditTextInfo
{
    // this local class lets us store info on each CDEText when >1 are selected...
    CDEText& text;                    // the CDEText in question
    CDEFreeCell* free_cell = nullptr; // * to the CDEText's free cell, null if not free standing
};


void CFormGrid::OnEditTextProp()
{
    CDEFormFile* pFF = GetFormView()->GetFormFile();

    std::vector<EditTextInfo> aTxtInfo;

    LOGFONT lfNULL;
    memset((void*)&lfNULL,0,sizeof(LOGFONT));

    int iCol = GetTracker(0).GetHitOb().GetCell().x;
    int iRow = GetTracker(0).GetHitOb().GetCell().y;
    int iRosterCol = iCol;
    int iRosterRow = iRow;
    if (m_pRoster->GetRightToLeft()) {
        iRosterCol = GetNumCols() - iRosterCol;
    }
    if (m_pRoster->GetOrientation() == RosterOrientation::Vertical) {
        int tmp = iRosterCol;
        iRosterCol = iRosterRow;
        iRosterRow = tmp;
    }
    CDECol* pCol = m_pRoster->GetCol(iRosterCol);

    // gather information about the selected CDETexts...
    for( int i = 0; i < GetNumTrackers(); ++i )
    {
        const CGridRectTracker& t = GetTracker(i);
        ASSERT(t.GetTrackObject()==CGridRectTracker::trackText);
        int iTxt = t.GetHitOb().GetText();
        CDEText& text = GetCell(iRow, iCol).GetText(iTxt);

        auto find_text = [&](const CDETextSet& text_set)
        {
            for( const CDEText& this_text : text_set.GetTexts() )
            {
                if( &this_text == &text )
                    return true;
            }

            return false;
        };

        // figure out if this CDEText is free standing or occurs in every row/col
        if( find_text(pCol->GetColumnCell().GetTextSet()) )
        {
            aTxtInfo.emplace_back(EditTextInfo { text, nullptr });
        }

        else
        {
            for( CDEFreeCell& free_cell : m_pRoster->GetFreeCells() )
            {
                if( free_cell.GetRow() == iRow && free_cell.GetColumn() == iCol && find_text(free_cell.GetTextSet()) )
                {
                    aTxtInfo.emplace_back(EditTextInfo { text, &free_cell });
                    break;
                }
            }
        }
    }

    // prep the dlg box ...
    CGridTextDlg dlg;

    CString csTxt = aTxtInfo.front().text.GetText();
    int iDefFontFlag = aTxtInfo.front().text.GetUseDefaultFont()?0:1;
    int iMulti=-1;
    int iFreeStandingCount= ( aTxtInfo.front().free_cell != nullptr  ) ? 1 : 0;
    COLORREF color = aTxtInfo.front().text.GetColor().ToCOLORREF();              // csc 4/14/2004
    bool bCustom=false;
    LOGFONT lfCustom;
    if (aTxtInfo.front().text.GetUseDefaultFont()) {
        memset((void*)&lfCustom,NULL,sizeof(LOGFONT));
    }
    else {
        lfCustom = aTxtInfo.front().text.GetFont();
        bCustom=true;
    }

    for (size_t i = 1 ; i<aTxtInfo.size(); ++i) {
        if (!csTxt.IsEmpty() && csTxt!=aTxtInfo[i].text.GetText()) {
            csTxt.Empty();
            dlg.m_bDisableTxt=true;
        }
        if (iDefFontFlag!=-1 && iDefFontFlag!=(aTxtInfo[i].text.GetUseDefaultFont()?0:1)) {
            iDefFontFlag = -1;
        }
        if (!aTxtInfo[i].text.GetUseDefaultFont()) {
            // find similarities among custom fonts
            if (!bCustom) {
                lfCustom = aTxtInfo[i].text.GetFont();
                bCustom=true;
            }
            else {
                LOGFONT custom_lf = aTxtInfo[i].text.GetFont();
                if (custom_lf.lfHeight!=lfCustom.lfHeight) {
                    lfCustom.lfHeight=0;
                }
                if (custom_lf.lfWidth!=lfCustom.lfWidth) {
                    lfCustom.lfWidth=0;
                }
                if (custom_lf.lfEscapement!=lfCustom.lfEscapement) {
                    lfCustom.lfEscapement=0;
                }
                if (custom_lf.lfOrientation!=lfCustom.lfOrientation) {
                    lfCustom.lfOrientation=0;
                }
                if (custom_lf.lfWeight!=lfCustom.lfWeight) {
                    lfCustom.lfWeight=0;
                }
                if (custom_lf.lfItalic!=lfCustom.lfItalic) {
                    lfCustom.lfItalic=0;
                }
                if (custom_lf.lfUnderline!=lfCustom.lfUnderline) {
                    lfCustom.lfUnderline=0;
                }
                if (custom_lf.lfStrikeOut!=lfCustom.lfStrikeOut) {
                    lfCustom.lfStrikeOut=0;
                }
                if (custom_lf.lfCharSet!=lfCustom.lfCharSet) {
                    lfCustom.lfCharSet=0;
                }
                if (custom_lf.lfOutPrecision!=lfCustom.lfOutPrecision) {
                    lfCustom.lfOutPrecision=0;
                }
                if (custom_lf.lfClipPrecision!=lfCustom.lfClipPrecision) {
                    lfCustom.lfClipPrecision=0;
                }
                if (custom_lf.lfQuality!=lfCustom.lfQuality) {
                    lfCustom.lfQuality=0;
                }
                if (custom_lf.lfPitchAndFamily!=lfCustom.lfPitchAndFamily) {
                    lfCustom.lfPitchAndFamily=0;
                }
                if (_tcscmp(custom_lf.lfFaceName,lfCustom.lfFaceName)!=0) {
                    _tcscpy(lfCustom.lfFaceName,_T(""));
                }
            }
        }
        if (aTxtInfo[i].free_cell != nullptr) {
            iFreeStandingCount++;
        }
    }
    dlg.m_csTxt=csTxt;
    dlg.m_lfDefault = pFF->GetDefaultTextFont();
    dlg.m_iFont=iDefFontFlag;
    dlg.m_color=color;
    if (bCustom) {
        dlg.m_lfCustom = lfCustom;
    }
    else {
        dlg.m_lfCustom = dlg.m_lfDefault;
    }
    dlg.m_csMsg1.Format(IDS_GRIDTEXTMSG1, _T("text"), ( m_pRoster->GetOrientation() == RosterOrientation::Horizontal ) ? _T("column") : _T("row"));
    dlg.m_csRadio1.Format(IDS_GRIDTEXTMSG2, _T("text"), ( m_pRoster->GetOrientation() == RosterOrientation::Horizontal ) ? _T("column") : _T("row"));
    dlg.m_csRadio2.Format(IDS_GRIDTEXTMSG3, _T("text"));
    if (iFreeStandingCount==0) {
        dlg.m_iMulti=iMulti = 0;
    }
    else if (iFreeStandingCount==(int)aTxtInfo.size()) {
        dlg.m_iMulti=iMulti=1;
    }
    else {
        dlg.m_iMulti=iMulti=-1;
    }

    // show the dialog box ...
    if (dlg.DoModal()!=IDOK) {
        return;
    }

    bool bUpdate = (color==dlg.m_color || csTxt!=dlg.m_csTxt || iDefFontFlag!=dlg.m_iFont || iMulti!=dlg.m_iMulti || memcmp((void*)&lfCustom, (void*)&dlg.m_lfCustom, sizeof(LOGFONT))!=0 || dlg.m_applytoall);
    if (bUpdate) {
        PortableColor portable_color = PortableColor::FromCOLORREF(dlg.m_color);
        if (dlg.m_applytoall) {
            FormFileIterator::ForeachCDEText(*pFF, [&](CDEText& text) { text.SetColor(portable_color); });
        }
        DeselectTrackers();

        for( EditTextInfo& e : aTxtInfo )
        {
            if (!dlg.m_bDisableTxt) {
                e.text.SetText(dlg.m_csTxt);
            }
            if (dlg.m_iFont!=-1) {
                e.text.SetUseDefaultFont(dlg.m_iFont==0?true:false);
                if (!e.text.GetUseDefaultFont()) {
                    e.text.SetFont(dlg.m_lfCustom);
                }
                else {
                    e.text.SetFont(pFF->GetDefaultTextFont());
                }
            }
            ASSERT(static_cast<LOGFONT>(e.text.GetFont()).lfHeight!=0);
            e.text.SetColor(portable_color);

            if (dlg.m_iMulti!=-1) {
                // make either free-standing or repeating ...
                bool bFound;
                if (e.free_cell != nullptr && dlg.m_iMulti==0) {
                    // swap from free standing to all cells
                    ASSERT(pCol!=NULL);
                    auto text_copy = std::make_shared<CDEText>(e.text);
                    LOGFONT lfTxt = text_copy->GetFont();
                    if(memcmp((void*)&lfTxt,(void*)&lfNULL,sizeof(LOGFONT)) == 0) {
                        text_copy->SetFont(pFF->GetDefaultTextFont());
                        e.text.SetFont(pFF->GetDefaultTextFont());
                    }
                    pCol->GetColumnCell().GetTextSet().AddText(std::move(text_copy));
                    bFound=false;
                    for (size_t j = 0 ; j<e.free_cell->GetTextSet().GetNumTexts() ; j++) {
                        if (&e.free_cell->GetTextSet().GetText(j)==&e.text) {
                            e.free_cell->GetTextSet().RemoveText(j);
                            bFound=true;
                            break;
                        }
                    }
                    ASSERT(bFound);
                }
                if (e.free_cell == nullptr && dlg.m_iMulti==1) {
                    // swap from all cells to free-standing
                    ASSERT(pCol!=NULL);
                    CDEFreeCell& free_cell = m_pRoster->GetOrCreateFreeCell(iRosterRow, iRosterCol);
                    auto text_copy = std::make_shared<CDEText>(e.text);
                    LOGFONT lfTxt = text_copy->GetFont();
                    if(memcmp((void*)&lfTxt,(void*)&lfNULL,sizeof(LOGFONT)) == 0) {
                        text_copy->SetFont(pFF->GetDefaultTextFont());
                        e.text.SetFont(pFF->GetDefaultTextFont());
                    }
                    free_cell.GetTextSet().AddText(std::move(text_copy));
                    bFound=false;
                    for (size_t j = 0 ; j<pCol->GetColumnCell().GetTextSet().GetNumTexts() ; j++) {
                        if (&pCol->GetColumnCell().GetTextSet().GetText(j)==&e.text) {
                            pCol->GetColumnCell().GetTextSet().RemoveText(j);
                            bFound=true;
                            break;
                        }
                    }
                    ASSERT(bFound);
                }
            }
        }
        GetFormDoc()->SetModifiedFlag();
        CPoint pt = m_ptCorner;
        BuildGrid();
        RecalcLayout();
        ScrollTo(pt.y, pt.x);
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnAddText()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnAddText()
{
    CDEFormFile* pFF = GetFormView()->GetFormFile();

    static int iPlacementChoice=0; // persists for duration of the run, choice of "this cell only" or "put in all rows/cols"

    int iCol = m_hitOb.GetCell().x;
    int iRow = m_hitOb.GetCell().y;

    if (m_pRoster->GetRightToLeft()) {
        iCol = GetNumCols() - iCol;
    }
    if (m_pRoster->GetOrientation() == RosterOrientation::Vertical) {
        int iTmp = iCol;
        iCol = iRow;
        iRow = iTmp;
    }

    // prep dlg
    CGridTextDlg dlg;
    dlg.m_csMsg1.Format(IDS_GRIDTEXTMSG1, _T("text"), ( m_pRoster->GetOrientation() == RosterOrientation::Horizontal ) ? _T("column") : _T("row"));
    dlg.m_csRadio1.Format(IDS_GRIDTEXTMSG2, _T("text"), ( m_pRoster->GetOrientation() == RosterOrientation::Horizontal ) ? _T("column") : _T("row"));
    dlg.m_csRadio2.Format(IDS_GRIDTEXTMSG3, _T("text"));
    dlg.m_csTxt=_T("text");
    dlg.m_lfCustom = dlg.m_lfDefault = GetFormDoc()->GetFormFile().GetDefaultTextFont();
    dlg.m_iMulti=iPlacementChoice;
    dlg.m_iFont=0;
    dlg.m_color=0;

    // do dlg and process results
    if( dlg.DoModal() != IDOK )
        return;

    PortableColor portable_color = PortableColor::FromCOLORREF(dlg.m_color);

    if (dlg.m_applytoall) {
        FormFileIterator::ForeachCDEText(*pFF, [&](CDEText& text) { text.SetColor(portable_color); });
    }

    // make a new text object
    auto text = std::make_shared<CDEText>(dlg.m_csTxt);
    text->SetColor(portable_color);
    text->SetUseDefaultFont(dlg.m_iFont==0);
    text->SetFont(text->GetUseDefaultFont()?dlg.m_lfDefault:dlg.m_lfCustom);

    iPlacementChoice = dlg.m_iMulti;
    if (dlg.m_iMulti==0) {
        // add to all cells
        CDECol* pCol = m_pRoster->GetCol(iCol);
        pCol->GetColumnCell().GetTextSet().AddText(std::move(text));
    }
    else {
        // add as free standing text
        CDEFreeCell& free_cell = m_pRoster->GetOrCreateFreeCell(iRow, iCol);
        free_cell.GetTextSet().AddText(std::move(text));
    }

    CPoint pt = m_ptCorner;
    BuildGrid();
    RecalcLayout();
    ScrollTo(pt.y,pt.x);
    GetFormDoc()->SetModifiedFlag();
}



/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnDrawBox()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormGrid::DrawBox(int iRow, int iCol, const CPoint& pt)
{
    Deselect();
    GetFormView()->RemoveAllTrackersAndRefresh();
    CGridWnd::DrawBox(iRow, iCol, pt);
}


/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnEditBoxProp()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnEditBoxProp()
{
    int iCol = m_hitOb.GetCell().x;
    int iRow = m_hitOb.GetCell().y;
    int iBox = m_hitOb.GetBox();  // this is the box w/in the CGridCell, not w/in a CDECol or CDEFreeText

    // isolate the CDEBox object the user is working with
    CGridCell& cell = GetCell(iRow, iCol);
    const CDEBox& source_box = cell.GetBox(iBox);

    if (m_pRoster->GetRightToLeft()) {
        iCol = GetNumCols() - iCol;
    }
    if (m_pRoster->GetOrientation() == RosterOrientation::Vertical) {
        // col, row need to swapped since CDERoster doesn't change structure when orientation is VERTICAL
        std::swap(iRow, iCol);
    }

    // figure out if the box is part of a column or free-standing
    CDEBoxSet* source_box_set = nullptr;
    bool originally_box_was_in_every_cell = false;

    auto find_box = [&](CDEBoxSet& box_set)
    {
        for( const CDEBox& box : box_set.GetBoxes() )
        {
            if( &box == &source_box )
            {
                source_box_set = &box_set;
                return true;
            }
        }

        return false;
    };

    CDECol* pCol = m_pRoster->GetCol(iCol);

    if( find_box(pCol->GetColumnCell().GetBoxSet()) )
    {
        originally_box_was_in_every_cell = true;
    }

    else
    {
        for( CDEFreeCell& free_cell : m_pRoster->GetFreeCells() )
        {
            if( free_cell.GetRow() == iRow && free_cell.GetColumn() == iCol && find_box(free_cell.GetBoxSet()) )
                break;
        }
    }

    ASSERT(source_box_set != nullptr);

    CGridBoxDlg dlg(m_pRoster->GetOrientation(), originally_box_was_in_every_cell);

    if( dlg.DoModal() != IDOK || dlg.GetBoxInEveryCell() == originally_box_was_in_every_cell )
        return;

    std::shared_ptr<CDEBox> removed_box = source_box_set->RemoveBox(source_box);
    ASSERT(removed_box != nullptr);

    // swap from free standing to all cells
    if( !originally_box_was_in_every_cell )
    {
        pCol->GetColumnCell().GetBoxSet().AddBox(removed_box);
    }

    // swap from all cells to free-standing
    else
    {
        m_pRoster->GetOrCreateFreeCell(iRow, iCol).GetBoxSet().AddBox(removed_box);
    }

    GetFormDoc()->SetModifiedFlag();

    CPoint pt = m_ptCorner;
    BuildGrid();
    RecalcLayout();
    ScrollTo(pt.y, pt.x);
}


/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnDeleteItem()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnDeleteItem()
{
    int iCol = m_hitOb.GetCell().x;
    int iRow = m_hitOb.GetCell().y;

    if (GetFormView()->GetCurItem() != NULL &&
            GetFormView()->GetCurItem()->GetItemType() == CDEFormBase::Roster)
    {
        CFormScrollView* pView = GetFormView();
        pView->SetFocus();              // view nds to have the focus for delete to work prop
        pView->SendMessage (WM_COMMAND, ID_DELETE_ITEM);
        return;
    }

    int iRosterCol = iCol;
    int iRosterRow = iRow;
    if (m_pRoster->GetRightToLeft()) {
        iRosterCol = GetNumCols() - iRosterCol;
    }
    if (m_pRoster->GetOrientation() == RosterOrientation::Vertical) {
        int tmp = iRosterCol;
        iRosterCol = iRosterRow;
        iRosterRow = tmp;
    }

    int iTracker, iMax;
    bool bRebuild=false;

    // sort the texts, boxes, and fields the user wants to delete (so that indexes don't get screwed up as we nuke them)
    CArray<int,int>  aiBox, aiTxt, aiFld, aiTmp;

    // sort CDETexts...
    for (iTracker=0 ; iTracker<GetNumTrackers() ; iTracker++) {
        CGridRectTracker& t = GetTracker(iTracker);
        if (t.GetTrackObject()==CGridRectTracker::trackText) {
            ASSERT(t.GetHitOb().GetText()!=NONE);
            aiTmp.Add(t.GetHitOb().GetText());
        }
    }
    while (aiTmp.GetSize()>0) {
        iMax=0;
        for (int i = 1 ;i<aiTmp.GetSize() ;i++) {
            if (aiTmp[i]>aiTmp[iMax]) {
                iMax=i;
            }
        }
        aiTxt.Add(aiTmp[iMax]);
        aiTmp.RemoveAt(iMax);
    }

    // sort CDEBoxes...
    ASSERT(aiTmp.GetSize()==0);
    for (iTracker=0 ; iTracker<GetNumTrackers() ; iTracker++) {
        CGridRectTracker& t = GetTracker(iTracker);
        if (t.GetTrackObject()==CGridRectTracker::trackBox) {
            ASSERT(t.GetHitOb().GetBox()!=NONE);
            aiTmp.Add(t.GetHitOb().GetBox());
        }
    }
    while (aiTmp.GetSize()>0) {
        iMax=0;
        for (int i = 1 ;i<aiTmp.GetSize() ;i++) {
            if (aiTmp[i]>aiTmp[iMax]) {
                iMax=i;
            }
        }
        aiBox.Add(aiTmp[iMax]);
        aiTmp.RemoveAt(iMax);
    }

    // sort CDEFields...
    ASSERT(aiTmp.GetSize()==0);
    for (iTracker=0 ; iTracker<GetNumTrackers() ; iTracker++) {
        CGridRectTracker& t = GetTracker(iTracker);
        if (t.GetTrackObject()==CGridRectTracker::trackField) {
            ASSERT(t.GetHitOb().GetField()!=NONE);
            aiTmp.Add(t.GetHitOb().GetField());
        }
    }
    while (aiTmp.GetSize()>0) {
        iMax=0;
        for (int i = 1 ;i<aiTmp.GetSize() ;i++) {
            if (aiTmp[i]>aiTmp[iMax]) {
                iMax=i;
            }
        }
        aiFld.Add(aiTmp[iMax]);
        aiTmp.RemoveAt(iMax);
    }
    ASSERT(aiTxt.GetSize()+aiBox.GetSize()+aiFld.GetSize()==GetNumTrackers());

    // nuke selected CDETexts ...
    for (int i = 0; i<aiTxt.GetSize(); ++i) {
        // figure out if the text is part of a column or free-standing
        int iTxt = aiTxt[i];
        ASSERT(iTxt!=NONE);
        CDEText& text = GetCell(iRow, iCol).GetText(iTxt);

        bool bFreeStanding=true;
        bool bOK=false;
        CDECol* pCol = m_pRoster->GetCol(iRosterCol);
        for (size_t j = 0 ; j<pCol->GetColumnCell().GetTextSet().GetNumTexts() ; j++) {
            if (&pCol->GetColumnCell().GetTextSet().GetText(j)==&text) {
                bFreeStanding=false;
                bOK=true;
                pCol->GetColumnCell().GetTextSet().RemoveText(j);
                break;
            }
        }

        if (bFreeStanding)
        {
            for( CDEFreeCell& free_cell : m_pRoster->GetFreeCells() )
            {
                if( free_cell.GetRow() == iRosterRow && free_cell.GetColumn() == iRosterCol )
                {
                    for( size_t j = 0 ; j < free_cell.GetTextSet().GetNumTexts(); ++j )
                    {
                        if( &free_cell.GetTextSet().GetText(j) == &text )
                        {
                            free_cell.GetTextSet().RemoveText(j);
                            bOK = true;
                            break;
                        }
                    }
                }
            }
        }

        ASSERT(bOK);
        bRebuild=true;
    }

    // nuke selected CDEBoxes ...
    for (int i = 0; i<aiBox.GetSize(); ++i) {
        // figure out if the box is part of a column or free-standing
        int iBox = aiBox[i];
        ASSERT(iBox!=NONE);

        const CDEBox& box = GetCell(iRow, iCol).GetBox(iBox);

        CDECol* pCol = m_pRoster->GetCol(iRosterCol);
        bool removed = ( pCol->GetColumnCell().GetBoxSet().RemoveBox(box) != nullptr );

        if( !removed )
        {
            for( CDEFreeCell& free_cell : m_pRoster->GetFreeCells() )
            {
                if( free_cell.GetRow() == iRosterRow && free_cell.GetColumn() == iRosterCol)
                {
                    if( free_cell.GetBoxSet().RemoveBox(box) != nullptr )
                    {
                        removed = true;
                        break;
                    }
                }
            }
        }

        ASSERT(removed);
        bRebuild=true;
    }

    // we have to build/recalc before deleting any fields, since a WM_PAINT message might be sent before the field is completely deleted
    CPoint ptCorner = m_ptCorner;
    if (bRebuild) {
        GetFormDoc()->SetModifiedFlag();
        BuildGrid();
        DeselectTrackers();
        RecalcLayout();
    }

    // nuke selected CDEFields ...
    for (int i = 0; i<aiFld.GetSize(); ++i)
    {
        // figure out if the text is part of a column or free-standing
        int iFld = aiFld[i];
        ASSERT(iFld!=NONE);
        CDEField* pFld = GetCell(iRow, iCol).GetField(iFld).GetDEField();
        ASSERT(NULL!=pFld);

        m_iActiveCol = iRosterCol;
        m_iActiveRow = iRosterRow;
        m_iActiveField = iFld;
        RemoveAllTrackers();
        if (GetCell(iRow,iCol).GetNumFields()>1)
        {
            GetParent()->SendMessage(WM_COMMAND, ID_DELETE_ITEM);
        }
        else
        {
            // post on last fld deletion since we'll be deleting ourselves
            GetParent()->PostMessage(WM_COMMAND, ID_DELETE_ITEM);
        }
    }

    ScrollTo(ptCorner.y, ptCorner.x);
}

/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnDeleteColumn()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnDeleteColumn()
{
    GetParent()->PostMessage(WM_KEYDOWN, VK_DELETE);
}

/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnAddForm()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnAddForm()
{
    GetParent()->SendMessage(WM_COMMAND, ID_ADD_FORM);
}

/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnEditFieldProp()
//
/////////////////////////////////////////////////////////////////////////////////

void CFormGrid::OnEditFieldProp()
{
    int iSavedValues[3] = { m_iActiveRow, m_iActiveCol, m_iActiveField };

    m_iActiveRow=m_hitOb.GetCell().y;
    m_iActiveCol=m_hitOb.GetCell().x;
    m_iActiveField = m_hitOb.GetField();
    ASSERT(m_iActiveRow!=NONE);
    ASSERT(m_iActiveCol!=NONE);
    ASSERT(m_iActiveField!=NONE);

    CGridCell& cell = GetCell(m_iActiveRow, m_iActiveCol);
    CDEField* pFld = cell.GetField(m_iActiveField).GetDEField();
    ASSERT_VALID(pFld);
    GetFormView()->m_pRightClickItem=pFld;
    GetFormView()->m_pRightClickRoster = m_pRoster;

    GetParent()->SendMessage(WM_COMMAND, ID_EDIT_FIELDPROP);
    CPoint pt = m_ptCorner;
    BuildGrid();
    RecalcLayout();
    ScrollTo(pt.y, pt.x);

    m_iActiveRow = iSavedValues[0];
    m_iActiveCol = iSavedValues[1];
    m_iActiveField = iSavedValues[2];
}



void CFormGrid::OnEditMultipleFieldProperties() // 20120612
{
    std::vector<CDEField*> fields;

    // create a list of all the fields that are selected

    bool bHoriz = ( m_pRoster->GetOrientation() == RosterOrientation::Horizontal );
    int selectionSize = bHoriz ? m_aiSelCol.GetSize() : m_aiSelRow.GetSize();

    for( int i = 0; i < selectionSize; i++ )
    {
        CGridCell& cell = GetCell(bHoriz ? 1 : m_aiSelRow[i],bHoriz ? m_aiSelCol[i] : 1);

        for( int j = 0; j < cell.GetNumFields(); j++ )
            fields.emplace_back(cell.GetField(j).GetDEField());
    }

    CFormScrollView* pParent = (CFormScrollView *)GetParent();
    pParent->OnEditMultipleFieldProperties(fields);

    // refresh the grid (code copied from the above function)
    CPoint pt = m_ptCorner;
    BuildGrid();
    RecalcLayout();
    ScrollTo(pt.y, pt.x);
}


/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnEditColumnProp()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnEditColumnProp()
{
    // build array of CDETexts that are selected ...
    CArray<CDEText*, CDEText*> apTxt;
    CArray<CGridCell*, CGridCell*> apCell;
    CDEField* pColField = NULL;

    if (m_pRoster->GetOrientation() == RosterOrientation::Horizontal) {
        ASSERT(m_aiSelCol.GetSize()>0);
        ASSERT(m_aptSelCell.GetSize()==0 && m_aiSelRow.GetSize()==0);
        for (int i = 0; i<m_aiSelCol.GetSize(); ++i) {
            int iCol = m_aiSelCol[i];
            if (m_pRoster->GetRightToLeft()) {
                iCol = GetNumCols() - iCol;
            }
            ASSERT(iCol>=0 && iCol<m_pRoster->GetNumCols());
            CDECol* pCol = m_pRoster->GetCol(iCol);
            apTxt.Add(&pCol->GetHeaderText());
            apCell.Add(&GetHeaderCell(iCol));
            if(pCol->GetNumFields() == 1){
                pColField = pCol->GetField(0);
            }
        }
    }
    else {
        ASSERT(m_aiSelRow.GetSize()>0);
        ASSERT(m_aiSelCol.GetSize()==0 && m_aptSelCell.GetSize()==0);
        for (int i = 0; i<m_aiSelRow.GetSize(); ++i) {
            int iCol = m_aiSelRow[i];
            if (m_pRoster->GetRightToLeft()) {
                iCol = GetNumCols() - iCol;
            }
            ASSERT(iCol>=0 && iCol<m_pRoster->GetNumCols());
            CDECol* pCol = m_pRoster->GetCol(iCol);
            apTxt.Add(&pCol->GetHeaderText());
            apCell.Add(&GetStubCell(iCol));
            if(pCol->GetNumFields() == 1){
                pColField = pCol->GetField(0);
            }
        }
    }
    ASSERT(apTxt.GetSize()>0);

    // run properties dlg for the selected CDETexts...
    if (OnEditColStubProperties(apTxt)) {
        GetFormDoc()->GetFormTreeCtrl()->ReBuildTree(GetFormView()->GetFormIndex());
        GetFormDoc()->SetModifiedFlag();
        if(apTxt.GetSize() == 1){//if only one field for the column, then set the field text to the column header text
            CString sChangedLabel = apTxt.GetAt(0)->GetLabel();

            if(sChangedLabel.Compare(pColField->GetCDEText().GetLabel()) != 0) {
                pColField->SetFieldLabelType(FieldLabelType::Custom);
            }
            pColField->SetCDEText(*apTxt.GetAt(0));
        }

        // update sizing info...
        CClientDC dc(this);
        int iSaveDC = dc.SaveDC();
        dc.SetMapMode(MM_TEXT);
        CSize szFieldFontTextExt(NONE, NONE);
        for (int i = 0; i<apCell.GetSize(); ++i) {
            apCell[i]->CalcMinSize(&dc, szFieldFontTextExt);
        }
        dc.RestoreDC(iSaveDC);

        CPoint pt = m_ptCorner;
        RecalcLayout();
        ScrollTo(pt.y, pt.x);
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnEditStubProp()
//
/////////////////////////////////////////////////////////////////////////////////
// Function HAS TO BE CHECKED PROBLY MAY LEAK MEMORY

void CFormGrid::OnEditStubProp()
{
    // build array of CDETexts that are selected ...
    CArray<CDEText*, CDEText*> apTxt;
    CArray<CGridCell*, CGridCell*> apCell;

    if (m_pRoster->GetOrientation() == RosterOrientation::Vertical) {
        ASSERT(m_aptSelCell.GetSize()>0);
        ASSERT(m_aiSelCol.GetSize()==0 && m_aiSelRow.GetSize()==0);
        for (int i = 0; i<m_aptSelCell.GetSize(); ++i) {
            int iStubTxt = m_aptSelCell[i].x-1;
            ASSERT(iStubTxt>=0 && iStubTxt<(int)m_pRoster->GetStubTextSet().GetNumTexts());
            apTxt.Add(&m_pRoster->GetStubTextSet().GetText(iStubTxt));
            apCell.Add(&GetHeaderCell(iStubTxt));
        }
    }
    else {
        ASSERT(m_aptSelCell.GetSize()>0);
        ASSERT(m_aiSelCol.GetSize()==0 && m_aiSelRow.GetSize()==0);
        for (int i = 0; i<m_aptSelCell.GetSize(); ++i) {
            int iStubTxt = m_aptSelCell[i].y-1;
            ASSERT(iStubTxt>=0 && iStubTxt<(int)m_pRoster->GetStubTextSet().GetNumTexts());
            apTxt.Add(&m_pRoster->GetStubTextSet().GetText(iStubTxt));
            apCell.Add(&GetStubCell(iStubTxt));
        }
    }
    ASSERT(apTxt.GetSize()>0);

    // run properties dlg for the selected CDETexts...
    if (OnEditColStubProperties(apTxt)) {
        GetFormDoc()->GetFormTreeCtrl()->ReBuildTree(GetFormView()->GetFormIndex());
        GetFormDoc()->SetModifiedFlag();

        // update sizing info...
        CClientDC dc(this);
        int iSaveDC = dc.SaveDC();
        dc.SetMapMode(MM_TEXT);
        CSize szFieldFontTextExt(NONE, NONE);
        for (int i = 0; i<apCell.GetSize(); ++i) {
            apCell[i]->CalcMinSize(&dc, szFieldFontTextExt);
        }
        dc.RestoreDC(iSaveDC);
        dc.RestoreDC(iSaveDC);

        CPoint pt = m_ptCorner;
        RecalcLayout();
        ScrollTo(pt.y, pt.x);
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//              bool CFormGrid::OnEditColStubProperties()
//
// common stuff for editing column or stub properties
// csc 12/22/00
/////////////////////////////////////////////////////////////////////////////////
bool CFormGrid::OnEditColStubProperties(CArray<CDEText*, CDEText*>& apTxt)
{
    ASSERT(apTxt.GetSize()>0);
    CDEFormFile* pFF = GetFormView()->GetFormFile();

    CColumnProp dlg(this);

    // determine common settings (or not, when doing multiple selections)...
    int iDefFontFlag = apTxt[0]->GetUseDefaultFont()?0:1;
    bool bCustom=false;
    LOGFONT lfCustom;
    CString csTxt = apTxt[0]->GetText();
    std::optional<HorizontalAlignment> h = apTxt[0]->GetHorizontalAlignmentOrDefault();
    std::optional<VerticalAlignment> v = apTxt[0]->GetVerticalAlignmentOrDefault();
    if (apTxt[0]->GetUseDefaultFont()) {
        memset((void*)&lfCustom,_T('\0'),sizeof(LOGFONT));
    }
    else {
        lfCustom = apTxt[0]->GetFont();
        bCustom=true;
    }
    dlg.m_color = apTxt[0]->GetColor().ToCOLORREF();

    for (int i = 1 ; i<apTxt.GetSize(); ++i) {
        if( h.has_value() && h != apTxt[i]->GetHorizontalAlignmentOrDefault() ) {
            h.reset();
        }
        if( v.has_value() && v != apTxt[i]->GetVerticalAlignmentOrDefault() ) {
            v.reset();
        }
        if (!csTxt.IsEmpty() && csTxt!=apTxt[i]->GetText()) {
            csTxt.Empty();
            dlg.m_bDisableTxt=true;
        }
        if (iDefFontFlag!=-1 && iDefFontFlag!=(apTxt[i]->GetUseDefaultFont()?0:1)) {
            iDefFontFlag = -1;
        }
        if (!apTxt[i]->GetUseDefaultFont()) {
            // find similarities among custom fonts
            if (!bCustom) {
                lfCustom = apTxt[i]->GetFont();
                bCustom=true;
            }
            else {
                LOGFONT custom_lf = apTxt[i]->GetFont();
                if (custom_lf.lfHeight!=lfCustom.lfHeight) {
                    lfCustom.lfHeight=0;
                }
                if (custom_lf.lfWidth!=lfCustom.lfWidth) {
                    lfCustom.lfWidth=0;
                }
                if (custom_lf.lfEscapement!=lfCustom.lfEscapement) {
                    lfCustom.lfEscapement=0;
                }
                if (custom_lf.lfOrientation!=lfCustom.lfOrientation) {
                    lfCustom.lfOrientation=0;
                }
                if (custom_lf.lfWeight!=lfCustom.lfWeight) {
                    lfCustom.lfWeight=0;
                }
                if (custom_lf.lfItalic!=lfCustom.lfItalic) {
                    lfCustom.lfItalic=0;
                }
                if (custom_lf.lfUnderline!=lfCustom.lfUnderline) {
                    lfCustom.lfUnderline=0;
                }
                if (custom_lf.lfStrikeOut!=lfCustom.lfStrikeOut) {
                    lfCustom.lfStrikeOut=0;
                }
                if (custom_lf.lfCharSet!=lfCustom.lfCharSet) {
                    lfCustom.lfCharSet=0;
                }
                if (custom_lf.lfOutPrecision!=lfCustom.lfOutPrecision) {
                    lfCustom.lfOutPrecision=0;
                }
                if (custom_lf.lfClipPrecision!=lfCustom.lfClipPrecision) {
                    lfCustom.lfClipPrecision=0;
                }
                if (custom_lf.lfQuality!=lfCustom.lfQuality) {
                    lfCustom.lfQuality=0;
                }
                if (custom_lf.lfPitchAndFamily!=lfCustom.lfPitchAndFamily) {
                    lfCustom.lfPitchAndFamily=0;
                }
                if (_tcscmp(custom_lf.lfFaceName,lfCustom.lfFaceName)!=0) {
                    _tcscpy(lfCustom.lfFaceName,_T(""));
                }
            }
        }
    }

    const TCHAR* const ColumnHeading    = _T("Column heading");
    const TCHAR* const RowHeading       = _T("Row heading");
    const TCHAR* const HeaderProperties = _T("Header properties");
    const TCHAR* const StubProperties   = _T("Stub properties");

    bool bEditingStubs = false;
    // determine dlg title, other text messages...
    if (m_pRoster->GetOrientation() == RosterOrientation::Horizontal) {
        if (m_aiSelCol.GetSize()>0) {
            // columns + horizontal --> headers
            dlg.m_csMsg = ColumnHeading;
            dlg.m_csDlgTitle = HeaderProperties;
        }
        else {
            // rows + horizontal --> stubs
            dlg.m_csMsg = RowHeading;
            dlg.m_csDlgTitle = StubProperties;
            bEditingStubs = true;
        }
    }
    else {
        if (m_aiSelRow.GetSize()>0) {
            // columns + vertical --> headers
            dlg.m_csMsg = RowHeading;
            dlg.m_csDlgTitle = StubProperties;
        }
        else {
            // rows + vertical --> stubs
            dlg.m_csMsg = ColumnHeading;
            dlg.m_csDlgTitle = HeaderProperties;
            bEditingStubs = true;
        }
    }

    // determine dlg settings ...
    int h_int = h.has_value() ? static_cast<int>(*h) : -1;
    int v_int = v.has_value() ? static_cast<int>(*v) : -1;
    dlg.m_iHorzAlign = h_int;         // -1 in the dlg drop-down will cause a blank selection, works like Excel!
    dlg.m_iVertAlign = v_int;
    dlg.m_csTxt=csTxt;
    dlg.m_lfDefault = pFF->GetDefaultTextFont();
    dlg.m_iFont=iDefFontFlag;
    if (bCustom) {
        dlg.m_lfCustom = lfCustom;
    }
    else {
        dlg.m_lfCustom = dlg.m_lfDefault;
    }

    // show the dialog ...
    if (dlg.DoModal() != IDOK) {
        return false;
    }

    PortableColor portable_color = PortableColor::FromCOLORREF(dlg.m_color);

    // csc 3/9/03 ... refined following stmt to only set bUpdate if font was changed
    bool bUpdate = ( dlg.m_applytoall ||
                     csTxt != dlg.m_csTxt ||
                     iDefFontFlag != dlg.m_iFont ||
                     h_int != dlg.m_iHorzAlign ||
                     v_int != dlg.m_iVertAlign ||
                     ( dlg.m_iFont == 1 && memcmp(&lfCustom, &dlg.m_lfCustom, sizeof(LOGFONT)) != 0 ) ||
                     apTxt[0]->GetColor() != portable_color );

    if (bUpdate) {
        if (dlg.m_applytoall) {
            FormFileIterator::ForeachCDEText(*pFF, [&](CDEText& text) { text.SetColor(portable_color); });
        }

        for (int i = 0; i<apTxt.GetSize(); ++i) {
            CDEText* pTxt = apTxt[i];

            // csc 9/3/03 ... only reset rect if font or text was changed, not if it was just color or alignment
            if (csTxt!=dlg.m_csTxt || iDefFontFlag!=dlg.m_iFont || (dlg.m_iFont==1 && memcmp((void*)&lfCustom, (void*)&dlg.m_lfCustom, sizeof(LOGFONT))!=0)) {
                pTxt->SetDims(0,0,0,0);
            }

            pTxt->SetColor(portable_color);

            switch(dlg.m_iHorzAlign) {
            case -1:
                // no change...
                break;
            case 2:
                pTxt->SetHorizontalAlignment(std::nullopt);  // right --> default
                break;
            default:
                pTxt->SetHorizontalAlignment(static_cast<HorizontalAlignment>(dlg.m_iHorzAlign));
                break;
            }

            switch(dlg.m_iVertAlign) {
            case -1:
                // no change...
                break;
            case 2:
                pTxt->SetVerticalAlignment(std::nullopt);  // bottom --> default
                break;
            default:
                pTxt->SetVerticalAlignment(static_cast<VerticalAlignment>(dlg.m_iVertAlign));
                break;
            }

            if (!dlg.m_bDisableTxt) {
                if(csTxt!=dlg.m_csTxt && bEditingStubs){//if the stub is modified occ labels will not be used any more and the roster stub texts will be used instead
                    GetRoster()->SetUseOccurrenceLabels(false);
                }
                pTxt->SetText(dlg.m_csTxt);
            }

            if (dlg.m_iFont!=-1) {
                pTxt->SetUseDefaultFont(dlg.m_iFont==0?true:false);
                if (!pTxt->GetUseDefaultFont()) {
                    pTxt->SetFont(dlg.m_lfCustom);
                }
                else {
                    pTxt->SetFont(pFF->GetDefaultTextFont());
                }
            }
            ASSERT(static_cast<LOGFONT>(pTxt->GetFont()).lfHeight!=0);
        }
    }

    return bUpdate;
}


/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnViewLogic()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnViewLogic()
{
    GetParent()->SendMessage(WM_COMMAND, ID_VIEW_LOGIC);
}


/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnQSFEditor()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnQSFEditor()
{
    GetParent()->SendMessage(WM_COMMAND, ID_QSF_EDITOR);
}


/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnShowBoxToolbar()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnShowBoxToolbar()
{
    GetParent()->SendMessage(WM_COMMAND, ID_SHOW_BOXTOOLBAR);
}


/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnAddBox()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnAddBox(int iRow, int iCol, const CRect& rc)
{
    if (m_pRoster->GetRightToLeft()) {
        iCol = GetNumCols() - iCol;
    }

    if (m_pRoster->GetOrientation() == RosterOrientation::Vertical) {
        // col, row need to swapped since CDERoster doesn't change structure when orientation is VERTICAL
        std::swap(iRow, iCol);
    }

    m_pRoster->GetCol(iCol)->GetColumnCell().GetBoxSet().AddBox(std::make_shared<CDEBox>(rc, GetFormView()->GetCurBoxDrawType()));

    GetFormDoc()->SetModifiedFlag();
    CPoint pt = m_ptCorner;
    BuildGrid();
    RecalcLayout();
    ScrollTo(pt.y, pt.x);
}


/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnEditGridProp()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnEditGridProp()
{
    GetFormView()->m_pRightClickItem = m_pRoster;
    GetParent()->SendMessage(WM_COMMAND, ID_EDIT_GRIDPROP);
}

/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnEditGridAutoFit()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnEditGridAutoFit()
{
    GetFormView()->m_pRightClickItem = m_pRoster;
    GetParent()->SendMessage(WM_COMMAND, ID_EDIT_GRID_AUTOFIT);
}




/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnEditJoin()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnEditJoin()
{
    // JH 12/7/07 swap col index for right to left
    bool bSwapColsForRightToLeft = ( m_pRoster->GetRightToLeft() && m_pRoster->GetOrientation() == RosterOrientation::Horizontal );

    // sort list of selected items
    std::set<int> selected_indices;
    CArray<int,int>& aiUnsorted = ( m_pRoster->GetOrientation() == RosterOrientation::Horizontal ) ? m_aiSelCol : m_aiSelRow;

    for( int i = 0; i < aiUnsorted.GetSize(); ++i )
    {
        int col = aiUnsorted[i];

        // JH 12/7/07 swap col index for right to left
        if (bSwapColsForRightToLeft) {
            col = m_pRoster->GetNumCols() - col - 1;
        }

        selected_indices.insert(col);
    }

    ASSERT(selected_indices.size() >= 2);

    // compose a suggested column/row header
    CString suggested_label;

    for( int selected_index : selected_indices )
    {
        // Index could be a row or a column, depending on orientation
        const CGridCell& cell = ( m_pRoster->GetOrientation() == RosterOrientation::Horizontal ) ? GetHeaderCell(selected_index) : GetStubCell(selected_index);

        if( !suggested_label.IsEmpty() )
            suggested_label.Append(_T(" + "));

        suggested_label.Append(cell.GetText(0).GetText());
    }

    if( suggested_label.GetLength() > MAX_LABEL_LEN )
        suggested_label.Truncate(MAX_LABEL_LEN);

    CJoinDlg dlg(m_pRoster->GetOrientation(), suggested_label);

    if( dlg.DoModal() != IDOK )
        return;


    DeselectTrackers();

    int columns_removed = (int)selected_indices.size() - 1;

    // the left-most column will be added to
    int iJoinIndex = *selected_indices.begin();
    CDECol* pJoinCol = m_pRoster->GetCol(iJoinIndex);    

    CDEText& text = pJoinCol->GetHeaderText();
    text.SetDims(0,0,0,0);  // will force the text position to be recalculated
    text.SetText(dlg.GetLabel());

    // adjust free-standing text and boxes for other cols
    for( size_t i = 0; i < m_pRoster->GetNumFreeCells(); ++i )
    {
        CDEFreeCell& free_cell = m_pRoster->GetFreeCell(i);

        if( free_cell.GetColumn() <= iJoinIndex )
            continue;

        // if this is being joined, move the contents
        if( std::find(selected_indices.cbegin(), selected_indices.cend(), free_cell.GetColumn()) != selected_indices.cend() )
        {
            m_pRoster->GetOrCreateFreeCell(free_cell.GetRow(), iJoinIndex).MoveContent(free_cell);
            m_pRoster->RemoveFreeCell(i);
            --i;
        }

        // otherwise modify the column index
        else
        {
            free_cell.SetColumn(free_cell.GetColumn() - columns_removed);
        }
    }

    // Get list of selected of column ptrs so we can remove them
    std::vector<CDECol*> aSelectedCols;
    std::transform(selected_indices.begin(),
                   selected_indices.end(),
                   std::back_inserter(aSelectedCols), [&](int i) { return m_pRoster->GetCol(i); });

    // Remove the columns from the roster and add to temp list
    for (CDECol* pCol : aSelectedCols) {

        // pull fields from group so that we can add them back in correct order
        for (int iColFld = 0; iColFld < pCol->GetNumFields(); ++iColFld) {
            CDEField* pColFld = pCol->GetField(iColFld);
            CDEBlock* pBlock = m_pRoster->GetBlock(pColFld);
            if (pBlock)
                pBlock->RemoveField();
            m_pRoster->RemoveItemAt(m_pRoster->GetItemIndex(pColFld));
        }

        if (pCol != pJoinCol) {
            m_pRoster->RemoveCol(pCol);
        }
    }

    std::vector<CDEField*> aJoinColFields;
    while (pJoinCol->GetNumFields() > 0) {
        aJoinColFields.emplace_back(pJoinCol->GetField(0));
        pJoinCol->RemoveFieldAt(0);
    }

    // Find start index in group in which to insert fields so that they come in same
    // order as in cols post join
    int groupInsertIndex = m_pRoster->GetGroupInsertIndexForCol(m_pRoster->GetColIndex(pJoinCol));

    // add texts and boxes from each of the joining cols
    for (CDECol* pCol : aSelectedCols) {

        if (pCol == pJoinCol) {
            for (CDEField* pFld : aJoinColFields) {
                if (pFld->UseUnicodeTextBox()) {
                    pFld->SetUnicodeTextBoxSize(pFld->GetDims().Size());
                }
                pFld->SetDims(0, 0, 0, 0);
                pJoinCol->AddField(pFld);
                m_pRoster->InsertItemAt(pFld, groupInsertIndex);
                CDEBlock* pBlock = m_pRoster->GetBlockForFieldAt(groupInsertIndex);
                if (pBlock)
                    pBlock->AddField();
                ++groupInsertIndex;
            }
        }
        else {
            pJoinCol->GetColumnCell().MoveContent(pCol->GetColumnCell());

            while (pCol->GetNumFields() > 0) {
                CDEField* pFld = pCol->GetField(0);
                if (pFld->UseUnicodeTextBox()) {
                    pFld->SetUnicodeTextBoxSize(pFld->GetDims().Size());
                }
                pFld->SetDims(0, 0, 0, 0);
                pJoinCol->AddField(pFld);

                m_pRoster->InsertItemAt(pFld, groupInsertIndex);
                CDEBlock* pBlock = m_pRoster->GetBlockForFieldAt(groupInsertIndex);
                if (pBlock)
                    pBlock->AddField();
                ++groupInsertIndex;
                pCol->RemoveFieldAt(0);
            }
        }
    }

    // Delete the removed columns
    for (CDECol* pCol : aSelectedCols) {
        if (pCol != pJoinCol)
            delete pCol;
    }
    ASSERT(m_pRoster->GetNumCols()>1); // if not, we'd have to process possibility of nuking entire roster

    int iSelIndex = m_pRoster->GetColIndex(pJoinCol);

    BuildGrid();
    RecalcLayout();

    // see if we need to shrink the grid to get rid of extra space, and select joined row/col
    CRect rcNewGrid(m_pRoster->GetDims());
    if (m_pRoster->GetOrientation() == RosterOrientation::Horizontal) {
        ASSERT(iSelIndex>=0 && iSelIndex<GetNumCols());
        if (GetTotalRect().Width()<rcNewGrid.Width()) {
            Resize(NONE, GetNumCols());
        }
        SelectColumn(iSelIndex, false, false, true, GetSysColor(COLOR_3DFACE));   // BMD 17 Apr 2001//RGB(128,255,200) chk 12/23/02
    }
    else {
        ASSERT(iSelIndex>=0 && iSelIndex<GetNumRows());
        if (GetTotalRect().Height()<rcNewGrid.Height()) {
            Resize(GetNumRows(), NONE);
        }
        SelectRow(iSelIndex, false, false, true, GetSysColor(COLOR_3DFACE));  // BMD 17 Apr 2001 RGB(128,255,200) chk 12/23/02
    }

    CFormScrollView* pView = GetFormView();
    CFormDoc* pDoc = GetFormDoc();
    pDoc->GetFormTreeCtrl()->ReBuildTree(pView->GetFormIndex(), m_pRoster);
    pView->MarkDictTree();      // updates the icons on the dict tree to reflect the deletion
    pDoc->SetModifiedFlag(true);
}



/////////////////////////////////////////////////////////////////////////////////
//
//              void CFormGrid::OnEditSplit()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormGrid::OnEditSplit()
{
    ASSERT(m_aiSelCol.GetSize()+m_aiSelRow.GetSize()==1);
    ASSERT(m_aptSelCell.GetSize()==0);
    // JH 12/7/07 swap col index for right to left
    bool bSwapColsForRightToLeft = ( m_pRoster->GetRightToLeft() && m_pRoster->GetOrientation() == RosterOrientation::Horizontal );
    // JH 12/7/05 fix split for r to left rosters
    int iColIndex = ( m_pRoster->GetOrientation() == RosterOrientation::Horizontal ) ? m_iActiveCol : m_iActiveRow;
    if (bSwapColsForRightToLeft) {
        iColIndex = m_pRoster->GetNumCols() - iColIndex;
    }
    CDECol* pCol = m_pRoster->GetCol(iColIndex);

    int columns_to_add = pCol->GetNumFields() - 1;
    ASSERT(columns_to_add >= 1);

    CDEField* pFld = pCol->GetField(0);
    if(pFld->UseUnicodeTextBox()){
        pFld->SetUnicodeTextBoxSize(pFld->GetDims().Size());
    }
    pFld->SetDims(0,0,0,0);
    pCol->GetHeaderText().SetText(pFld->GetLabel());

    // create new columns, assign their fields, add them to the roster
    int iSplitCol = m_hitOb.GetCell().x;
    // JH 12/7/05 fix split for r to left rosters
    if (bSwapColsForRightToLeft) {
        iSplitCol = m_pRoster->GetNumCols() - iSplitCol;
    }

    ASSERT(iSplitCol>0 && iSplitCol<m_pRoster->GetNumCols());
    for (int i = 1 ; i<pCol->GetNumFields(); ++i) {
        CDECol* pNewCol = new CDECol;
        pNewCol->SetHeaderText(pCol->GetHeaderText());
        pNewCol->GetHeaderText().SetDims(0,0,0,0);  // force a recalc
        pFld = pCol->GetField(i);
        if(pFld->UseUnicodeTextBox()){
            pFld->SetUnicodeTextBoxSize(pFld->GetDims().Size());
        }
        pFld->SetDims(0,0,0,0);
        pNewCol->AddField(pFld);
        pNewCol->GetHeaderText().SetText(pFld->GetLabel());
//        m_pRoster->InsertColAt(pNewCol,iSplitCol+1);
        // JH 12/7/05 fix split for r to left rosters
        int iNextCol = iSplitCol+i;
        if (bSwapColsForRightToLeft) {
            iNextCol = iSplitCol;
        }
        m_pRoster->InsertColAt(pNewCol,iNextCol);    // changed to restore original order; csc 1/3/01
    }

    // remove extra fields from the initial column
    while (pCol->GetNumFields()>1) {
        pCol->RemoveFieldAt(1);     // csc 1/29/01
    }

    // adjust free-standing text and boxes for other cols
    for( CDEFreeCell& free_cell : m_pRoster->GetFreeCells() )
    {
        if( free_cell.GetColumn() > iColIndex )
            free_cell.SetColumn(free_cell.GetColumn() + columns_to_add);
    }

    // rebuild and redisplay
    BuildGrid();
    RecalcLayout();
    GetFormDoc()->GetFormTreeCtrl()->ReBuildTree( m_pRoster->GetFormNum(), m_pRoster);
}


void CFormGrid::OnLayoutAlign(UINT nID)
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

    // figure out where we'll align
    LONG modification = 0;

    if( nID == ID_LAYOUT_ALIGN_LEFT )
    {
        ASSERT(CanAlign(HorizontalAlignment::Left));

        for( int i = 0; i < GetNumTrackers(); ++i )
        {
            LONG left = GetTracker(i).m_rect.left;
            modification = ( i == 0 ) ? left : std::min(modification, left);
        }
    }

    else if( nID == ID_LAYOUT_ALIGN_RIGHT )
    {
        ASSERT(CanAlign(HorizontalAlignment::Right));

        for( int i = 0; i < GetNumTrackers(); ++i )
        {
            LONG right = GetTracker(i).m_rect.right;
            modification = ( i == 0 ) ? right : std::max(modification, right);
        }
    }


    else if( nID == ID_LAYOUT_ALIGN_TOP )
    {
        ASSERT(CanAlign(VerticalAlignment::Top));

        for( int i = 0; i < GetNumTrackers(); ++i )
        {
            LONG top = GetTracker(i).m_rect.top;
            modification = ( i == 0 ) ? top : std::min(modification, top);
        }
    }

    else 
    {
        ASSERT(nID == ID_LAYOUT_ALIGN_BOTTOM);
        ASSERT(CanAlign(VerticalAlignment::Bottom));

        for( int i = 0; i < GetNumTrackers(); ++i )
        {
            LONG bottom = GetTracker(i).m_rect.bottom;
            modification = ( i == 0 ) ? bottom : std::max(modification, bottom);
        }
    }

    // align the objects
    for( int i = 0; i < GetNumTrackers(); ++i )
    {
        CGridRectTracker& t = GetTracker(i);
        const CHitOb& h = t.GetHitOb();

        CSize szMove = ( nID == ID_LAYOUT_ALIGN_LEFT )    ? CSize(modification - t.m_rect.left, 0) :
                       ( nID == ID_LAYOUT_ALIGN_RIGHT )   ? CSize(modification - t.m_rect.right, 0) :
                       ( nID == ID_LAYOUT_ALIGN_TOP )     ? CSize(0, modification - t.m_rect.top) :
                     /*( nID == ID_LAYOUT_ALIGN_BOTTOM )*/  CSize(0, modification - t.m_rect.bottom);            

        switch( t.GetTrackObject() )
        {
            case CGridRectTracker::trackField:
            {
                CCellField& fld = cell.GetField(h.GetField());
                CDEField* pFld = fld.GetDEField();
                CRect rcTmp = pFld->GetDims() + szMove;
                pFld->SetDims(rcTmp);
                t.m_rect += szMove;
                OnFieldMoved(iRow, iCol, h.GetField());
                break;
            }  

            case CGridRectTracker::trackText:
            {
                CDEText& text = cell.GetText(h.GetText());
                CRect rcTmp = text.GetDims() + szMove;
                text.SetDims(rcTmp);
                t.m_rect += szMove;
                OnTextMoved(iRow, iCol, h.GetText());
                break;
            }  

            case CGridRectTracker::trackBox:
            {
                CDEBox& box = cell.GetBox(h.GetBox());
                box.SetDims(box.GetDims() + szMove);
                t.m_rect += szMove;
                OnBoxMoved(iRow, iCol, h.GetBox());
                break;
            }

            default:
                ASSERT(FALSE);
        }
    }

    // refresh view
    if( m_pRoster->GetOrientation() == RosterOrientation::Horizontal )
    {
        for( int i = 1; i < GetNumRows(); ++i )
            RedrawCell(i, iCol);
    }

    else
    {
        for( int i = 1 ; i < GetNumCols(); ++i )
            RedrawCell(iRow,i);
    }

    UpdateWindow();
}


/////////////////////////////////////////////////////////////////////////////////
//
//              bool CFormGrid::CanAlign()
//
/////////////////////////////////////////////////////////////////////////////////
bool CFormGrid::CanAlign(const std::variant<HorizontalAlignment, VerticalAlignment>& alignment) const
{
    ASSERT(( std::holds_alternative<HorizontalAlignment>(alignment) && ( std::get<HorizontalAlignment>(alignment) == HorizontalAlignment::Left ||
                                                                         std::get<HorizontalAlignment>(alignment) == HorizontalAlignment::Right ) ) ||
           ( std::holds_alternative<VerticalAlignment>(alignment) && ( std::get<VerticalAlignment>(alignment) == VerticalAlignment::Top ||
                                                                       std::get<VerticalAlignment>(alignment) == VerticalAlignment::Bottom )) );


    if (!IsTrackerActive()) {
        // can't align if nothing is selected
        return false;
    }

    // on left/right align, field tops and bottoms cannot overlap with each other
    // on top/bottom align, field lefts and rights cannot overlap with each other
    int iRow = GetTracker(0).GetHitOb().GetCell().y;
    int iCol = GetTracker(0).GetHitOb().GetCell().x;
    ASSERT(iCol>=0 && iCol<GetNumCols());
    ASSERT(iRow>=0 && iRow<GetNumRows());
    const CGridCell& cell = GetCell(iRow,iCol);

    CRect rcTmp;
    CArray<CRect, CRect&> arcFld;  // aligned CRects for fields in this cell

    for (int i = 0; i < GetNumTrackers(); ++i) {
        const CGridRectTracker& t = GetTracker(i);
        const CHitOb& h = t.GetHitOb();
        if (t.GetTrackObject() == CGridRectTracker::trackField) {
            CCellField fld = cell.GetField(h.GetField());
            CDEField* pFld = fld.GetDEField();
            CRect rcFld = pFld->GetDims();
            if (std::holds_alternative<HorizontalAlignment>(alignment)) {
                rcTmp = CRect(0, rcFld.top, 10, rcFld.bottom);
                arcFld.Add(rcTmp);
            }
            else {
                rcTmp = CRect(rcFld.left, 0, rcFld.right, 10);
                arcFld.Add(rcTmp);
            }
        }
    }

    // make sure that none of the fld rects overlaps with any of the others
    for (int i = 0; i<arcFld.GetSize(); ++i) {
        for (int j=i+1 ; j<arcFld.GetSize() ; j++) {
            CRect& rcA = arcFld[i];
            CRect& rcB = arcFld[j];
            if ((rcA & rcB) != CRect(0,0,0,0)) {
                return false;
            }
        }
    }
    return true;
}

void CFormGrid::OnEditPaste()
{
    // Paste  to clipboard
    CFormDoc* pDoc = GetFormDoc();
    UINT uFormat = pDoc->GetClipBoardFormat(FD_TEXT_FORMAT);
    pDoc->ClipToFile(uFormat);

    CSpecFile clipFile;
    CString csClipFile = pDoc->GetClipFile();
    clipFile.Open(csClipFile, CFile::modeRead);

    CFormScrollView* pView = GetFormView();
    CDEGroup* pGroup = pView->GetCurGroup();
    CIMSAString csCmd;    // the string command  (left side of =)
    CIMSAString csArg;    // the string argument (right side of =)
    CDEFormFile* pFF = pView->GetFormFile();

        while (clipFile.GetLine(csCmd, csArg) == SF_OK)         // csc 5/5/97
        {
            if (csCmd == _T("[Column]"))
            {
                CDECol* pCol = new CDECol();
                pCol->Build(clipFile,pGroup);
                CDEField* pField = pCol->GetField(0);
                const CDataDict* pDict = pDoc->GetSharedDictionary().get();
                const CDictItem* pItem = pDict->LookupName<CDictItem>(pField->GetItemName());
                pField->SetDictItem(pItem);
                //GetCurForm()->FindItem(csFName);
                pField->SetParent(pView->GetCurGroup());
                m_pRoster->AddCol (pCol);
                m_pRoster->AddItem (pCol->GetField(0));   // CDEGroup nds to see it too (for Serpro)
            }
        }
        clipFile.Close();
//      pFF->CreateRosterField (pCol, pRoster);         // this adds it to the roster

    m_pRoster->UpdateFlagsNFonts(*pFF);
    BuildGrid();  // rebuilds underlying structure from CDERoster
    RecalcLayout (CSize(NONE,NONE), false); // recalcs where objs go
    CRect origRect = m_pRoster->GetDims();
    Resize (origRect);    // change viewport of grid
}

void CFormGrid::OnEditCopy()
{
    // Copy levels to clipboard
    CSpecFile clipFile;
    CFormDoc* pDoc = GetFormDoc();
    CString csClipFile = pDoc->GetClipFile();
    clipFile.Open(csClipFile, CFile::modeWrite);

    CDEBlock* pCurrentBlock = NULL;
    CFormTracker        tracker;
    CFormScrollView*  pView = GetFormView();
    CDEForm*                pForm = pView->GetCurForm();
    int i, index, max = pView->GetNumTrackers();
    for (i = 0; i < max; ++i)
    {
        tracker = pView->GetTracker(i);
        index = tracker.GetIndex();
        CDEItemBase*    pItem;
        // is the user is trying to delete the text portion of a data entry box?
        if (!tracker.IsBox())
        {
            pItem = pForm->GetItem(index);
            //if ( pItem->IsKindOf(RUNTIME_CLASS (CDERoster)))
            if (pItem == m_pRoster)
            {
                m_pRoster->Save(clipFile);
            }
        }
    }


    if (m_pRoster->GetOrientation() == RosterOrientation::Horizontal)
    {
        int numcols = m_pRoster->GetNumCols();
        for (int col = 0; col < numcols; col++)
        {
            if (IsColSelected(col))
            {

                CDECol* pCol = m_pRoster->GetCol(col);

                //if there is a block change write out the block first
                CDEField* pColField = pCol->GetField(0);
                CDEBlock* pBlock = m_pRoster->GetBlock(pColField);
                if (pCurrentBlock != pBlock && pBlock != NULL) {
                    pCurrentBlock = pBlock;
                    pCurrentBlock->Save(clipFile);
                }

                pCol->Save(clipFile);

            }
        }
    }
    else
    {
        int numcols = m_pRoster->GetNumCols();
        for (int col = 0; col < numcols; col++)
        {
            if (IsRowSelected(col))
            {
                CDECol* pCol = m_pRoster->GetCol(col);
                //if there is a block change write out the block first
                CDEField* pColField = pCol->GetField(0);
                CDEBlock* pBlock = m_pRoster->GetBlock(pColField);
                if (pCurrentBlock != pBlock && pBlock != NULL) {
                    pCurrentBlock = pBlock;
                    pCurrentBlock->Save(clipFile);
                }
                pCol->Save(clipFile);
            }
        }
    }

    //  Gives wether the column is selected or not IsColSelected(col)
    clipFile.Close();
    UINT uFormat = pDoc->GetClipBoardFormat(FD_COLUMN_FORMAT);
    pDoc->FileToClip(uFormat);

}


void CFormGrid::AlignCell(int row, int column, const std::variant<HorizontalAlignment, VerticalAlignment>& alignment)
{
    CGridCell& cell = GetCell(row, column);
    CRect rcCell = cell.GetRect();
    rcCell.DeflateRect(GRIDCELL_BORDER, GRIDCELL_BORDER);
    rcCell.OffsetRect(-rcCell.TopLeft());

    // only allow the alignment if it will end up with nothing overlapping
    if( cell.GetNumFields() > 1 )
    {
        bool overlapHorz = false;
        bool overlapVert = false;
        CRect totalBounds = cell.GetField(0).GetDEField()->GetDims();

        for( int i = 1; i < cell.GetNumFields(); i++ )
        {
            CRect thisBounds = cell.GetField(i).GetDEField()->GetDims();

            if( ( thisBounds.left >= totalBounds.left && thisBounds.left <= totalBounds.right ) ||
                ( thisBounds.right >= totalBounds.left && thisBounds.right <= totalBounds.right ) ||
                ( totalBounds.left >= thisBounds.left && totalBounds.left <= thisBounds.right ) ||
                ( totalBounds.right >= thisBounds.left && totalBounds.right <= thisBounds.right ) )
            {
                overlapHorz = true;
            }

            if( ( thisBounds.top >= totalBounds.top && thisBounds.top <= totalBounds.bottom ) ||
                ( thisBounds.bottom >= totalBounds.top && thisBounds.bottom <= totalBounds.bottom) ||
                ( totalBounds.top >= thisBounds.top && totalBounds.top <= thisBounds.bottom ) ||
                ( totalBounds.bottom >= thisBounds.top && totalBounds.bottom <= thisBounds.bottom) )
            {
                overlapVert = true;
            }

            // no possible way to align the fields
            if( ( overlapHorz && overlapVert ) ||
                ( std::holds_alternative<HorizontalAlignment>(alignment) && overlapVert ) ||
                ( std::holds_alternative<VerticalAlignment>(alignment) && overlapHorz ) )
            {
                return;
            }

            totalBounds |= thisBounds;
        }
    }

    GetFormDoc()->SetModifiedFlag();

    for( int j = 0; j < cell.GetNumFields(); j++ )
    {
        CCellField & fld = cell.GetField(j);
        CDEField* pFld = fld.GetDEField();
        CRect rcOld = pFld->GetDims();
        CSize szMove;

        if( std::holds_alternative<HorizontalAlignment>(alignment) )
        {
            HorizontalAlignment ha = std::get<HorizontalAlignment>(alignment);
            szMove.cx = ( ha == HorizontalAlignment::Left )  ? ( rcCell.left - rcOld.left ) :
                        ( ha == HorizontalAlignment::Right ) ? ( rcCell.right - rcOld.right ) :
                                                               ( ( ( rcCell.right + rcCell.left ) - ( rcOld.right + rcOld.left ) ) / 2 );
        }

        else
        {
            VerticalAlignment va = std::get<VerticalAlignment>(alignment);
            szMove.cy = ( va == VerticalAlignment::Top )    ? ( rcCell.top - rcOld.top ) :
                        ( va == VerticalAlignment::Bottom ) ? ( rcCell.bottom - rcOld.bottom ) :
                                                              ( ( ( rcCell.bottom + rcCell.top ) - ( rcOld.top + rcOld.bottom ) ) / 2 );
        }

        CRect rcNew = rcOld + szMove;
        pFld->SetDims(rcNew);
        OnFieldMoved(row, column, j);
    }
}


void CFormGrid::AlignFields(const std::variant<HorizontalAlignment, VerticalAlignment>& alignment)
{
    // if a column is selected
    if( GetNumRows() != 0 )
    {
        for( int selected_column = 0; selected_column < m_aiSelCol.GetSize(); ++selected_column )
        {
            int column = m_aiSelCol[selected_column];

            AlignCell(1, column, alignment);

            for( int row = 1; row < GetNumRows(); ++row )
                RedrawCell(row, column);
        }
    }

    // if a row is selected
    if( GetNumCols() != 0 )
    {
        for( int selected_row = 0; selected_row < m_aiSelRow.GetSize(); ++selected_row )
        {
            int row = m_aiSelRow[selected_row];

            AlignCell(row, 1, alignment);

            for( int column = 1; column < GetNumCols(); ++column )
                RedrawCell(row, column);
        }
    }
}

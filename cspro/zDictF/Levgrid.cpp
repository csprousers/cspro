//***************************************************************************
//  File name: LevGrid.cpp
//
//  Description:
//       Data Dictionary level properties grid implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#include "StdAfx.h"
#include "Levgrid.h"

namespace
{
    constexpr int LEVEL_NOTE_COL  = 0;
    constexpr int LEVEL_LABEL_COL = 1;
    constexpr int LEVEL_NAME_COL  = 2;
    constexpr int LEVEL_TYPE_COL  = 3;
    constexpr int LEVEL_REQ_COL   = 4;
    constexpr int LEVEL_MAX_COL   = 5;

    constexpr int LEVEL_NUM_COLS  = LEVEL_MAX_COL + 1;
}


BEGIN_MESSAGE_MAP(CLevelGrid, CDDGrid)
    ON_COMMAND(ID_EDIT_ADD, OnEditAdd)
    ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
    ON_COMMAND(ID_EDIT_INSERT, OnEditInsert)
    ON_COMMAND(ID_EDIT_CUT, OnEditCut)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
    ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
    ON_COMMAND(ID_EDIT_MODIFY, OnEditModify)
    ON_COMMAND(ID_EDIT_NOTES, OnEditNotes)
    ON_COMMAND(ID_EDIT_OCCURRENCELABELS, OnEditOccLabels)
    ON_COMMAND(ID_SHIFT_F10, OnShiftF10)
END_MESSAGE_MAP()


CLevelGrid::CLevelGrid()
    :   m_iLevel(0),
        m_iFirstRow(0),
        m_pLabelEdit(nullptr),
        m_pNameEdit(nullptr),
        m_pTypeEdit(nullptr),
        m_pReqEdit(nullptr),
        m_pMaxEdit(nullptr)
{
}


std::vector<size_t> CLevelGrid::GetSelectedRecords()
{
    std::vector<size_t> selected_records;

    for( long row : GetSelectedRows() )
        selected_records.emplace_back(row - m_iFirstRow);

    return selected_records;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnSetup
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::OnSetup()
{
    EnableExcelBorders(FALSE);
    VScrollAlwaysPresent(TRUE);
    SetNumberCols(LEVEL_NUM_COLS, FALSE);
    SetNumberRows(1, FALSE);
    SetSH_Width(0);
    SetCurrentCellMode(2);
    SetHighlightRow(TRUE);
    SetMultiSelectMode(TRUE);

    m_iButton = AddCellType(&m_button);

    auto set_header = [&](int column, const TCHAR* text, int alignment = 0)
    {
        QuickSetText(column, HEADER_ROW, text);

        if( alignment != 0 )
            QuickSetAlignment(column, HEADER_ROW, alignment);
    };

    set_header(LEVEL_NOTE_COL,  _T("N"));
    set_header(LEVEL_LABEL_COL, _T("Record Label"), UG_ALIGNLEFT);
    set_header(LEVEL_NAME_COL,  _T("Record Name"),  UG_ALIGNLEFT);
    set_header(LEVEL_TYPE_COL,  _T("Type Value"),   UG_ALIGNLEFT);
    set_header(LEVEL_REQ_COL,   _T("Required"),     UG_ALIGNLEFT);
    set_header(LEVEL_MAX_COL,   _T("Max"),          UG_ALIGNRIGHT);

    m_bAdding = false;
    m_bInserting = false;
    m_bEditing = false;
    GotoCell(LEVEL_LABEL_COL, 0);                         // 15 Jan 2002
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::Size
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::Size(CRect rect)
{
    CIMSAString csWidths = AfxGetApp()->GetProfileString(_T("Data Dictionary"), _T("LevelGridWidths"), _T("-1"));
    if (csWidths == _T("-1")) {
        CUGCell cell;
        CUGCellType* pCellType;
        CSize size;

        int iUsed = static_cast<int>(NOTE_WIDTH * (GetDesignerFontZoomLevel() / 100.0));
        SetColWidth(LEVEL_NOTE_COL, iUsed);
        for (int col = LEVEL_MAX_COL ; col > LEVEL_LABEL_COL ; col--) {
            GetCell(col, HEADER_ROW, &cell);
            pCellType = GetCellType(HEADER_ROW, col);
            pCellType->GetBestSize(GetDC(), &size, &cell);
            SetColWidth(col, size.cx + BORDER_WIDTH);
            iUsed += size.cx + BORDER_WIDTH;
        }
        GetCell(LEVEL_LABEL_COL, HEADER_ROW, &cell);
        pCellType = GetCellType(HEADER_ROW, LEVEL_LABEL_COL);
        pCellType->GetBestSize(GetDC(), &size, &cell);
        if (size.cx > rect.Width() - m_GI->m_vScrollWidth - iUsed - 1) {
            SetColWidth(LEVEL_LABEL_COL, size.cx);
        }
        else {
            SetColWidth(LEVEL_LABEL_COL, rect.Width() - m_GI->m_vScrollWidth - iUsed - 1);
        }
        csWidths.Format(_T("%d,%d,%d,%d,%d,%d"), GetColWidth(LEVEL_NOTE_COL),
                                                 GetColWidth(LEVEL_LABEL_COL),
                                                 GetColWidth(LEVEL_NAME_COL),
                                                 GetColWidth(LEVEL_TYPE_COL),
                                                 GetColWidth(LEVEL_REQ_COL),
                                                 GetColWidth(LEVEL_MAX_COL));
        AfxGetApp()->WriteProfileString(_T("Data Dictionary"), _T("LevelGridWidths"), csWidths);
    }
    else {
        CIMSAString csWidth = csWidths.GetToken();
        SetColWidth(LEVEL_NOTE_COL, (int) csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(LEVEL_LABEL_COL, (int) csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(LEVEL_NAME_COL, (int) csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(LEVEL_TYPE_COL, (int) csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(LEVEL_REQ_COL, (int) csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(LEVEL_MAX_COL, (int) csWidth.Val());
    }
    Resize(rect);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::Resize
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::Resize(CRect rect)
{
    MoveWindow(&rect);
    if (m_aEditControl.GetSize() > 0) {
        for (int i = m_iMinCol ; i <= m_iMaxCol ; i++) {
            m_aEditControl[i]->PostMessage(WM_SIZE);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnColSized
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::OnColSized(int col, int* /*width*/)
{
    if (m_aEditControl.GetSize() > 0) {
        for (int i = m_iMinCol ; i <= m_iMaxCol ; i++) {
            if (col == LEVEL_REQ_COL && col == i) {
                CIMSAString csText;
                m_pReqEdit->GetWindowText(csText);
                long row = GetCurrentRow();
                delete m_pReqEdit;
                // Create required edit
                m_pReqEdit = new CDDComboBox();
                CRect rect;
                GetCellRect(LEVEL_REQ_COL, row, &rect);
                rect.top--;
                rect.left--;
                rect.bottom += 3*m_plf->lfHeight;
                rect.right += 2;
                m_pReqEdit->SetRowCol(row, LEVEL_REQ_COL);
                m_pReqEdit->Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, rect, this, 104);
                m_pReqEdit->SetFont(&m_font);
                m_pReqEdit->SetItemHeight (-1, m_plf->lfHeight);   // sets height for static control and button
                m_pReqEdit->SetItemHeight ( 0, m_plf->lfHeight);   // sets height for list box entries
                m_pReqEdit->AddString(_T("Yes"));
                m_pReqEdit->AddString(_T("No"));
                if (csText == _T("Yes")) {
                    m_pReqEdit->SetCurSel(0);
                }
                else {
                    m_pReqEdit->SetCurSel(1);
                }
                m_aEditControl.SetAt(LEVEL_REQ_COL, (CWnd*)m_pReqEdit);
            }
            else {
                m_aEditControl[i]->SendMessage(WM_SIZE);
            }
            m_aEditControl[i]->SendMessage(WM_PAINT);
        }
    }
    CString csWidths;
    csWidths.Format(_T("%d,%d,%d,%d,%d,%d"), GetColWidth(LEVEL_NOTE_COL),
                                             GetColWidth(LEVEL_LABEL_COL),
                                             GetColWidth(LEVEL_NAME_COL),
                                             GetColWidth(LEVEL_TYPE_COL),
                                             GetColWidth(LEVEL_REQ_COL),
                                             GetColWidth(LEVEL_MAX_COL));
    AfxGetApp()->WriteProfileString(_T("Data Dictionary"), _T("LevelGridWidths"), csWidths);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::Update
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::Update(CDataDict* pDict, int level)
{
    ASSERT(level >= 0 && level < (int)m_pDict->GetNumLevels());
    m_pDict = pDict;
    m_iLevel = level;
    if (m_iLevel >= (int)m_pDict->GetNumLevels()) {          // BMD 30 Jul 2003
        m_iLevel = (int)m_pDict->GetNumLevels() - 1;
    }
    Update();
}

void CLevelGrid::Update()
{
    int rows = m_pDict->GetNumRecords();
    SetNumberRows(rows, FALSE);

    COLORREF rgb;
    int ir = 0;
    for( size_t level_number = 0; level_number < m_pDict->GetNumLevels(); ++level_number ) {
	    const DictLevel& dict_level = m_pDict->GetLevel(level_number);
        if ((int)level_number == m_iLevel) {
            rgb = GetSysColor(COLOR_WINDOWTEXT);
        }
        else {
            rgb = GetSysColor(COLOR_GRAYTEXT);
        }
        for (int r = 0 ; r < dict_level.GetNumRecords() ; r++) {
            const CDictRecord* pRec = dict_level.GetRecord(r);
            QuickSetCellType  (LEVEL_NOTE_COL,  ir, m_iButton);
            QuickSetCellTypeEx(LEVEL_NOTE_COL,  ir, UGCT_BUTTONNOFOCUS);
            QuickSetBackColor (LEVEL_NOTE_COL,  ir, GetSysColor(COLOR_BTNFACE));
            QuickSetHBackColor(LEVEL_NOTE_COL,  ir, GetSysColor(COLOR_BTNFACE));
            if (pRec->GetNote().GetLength() > 0) {
                if (rgb == GetSysColor(COLOR_WINDOWTEXT)) {
                    QuickSetBitmap(LEVEL_NOTE_COL,  ir, m_pNoteYes);
                }
                else {
                    QuickSetBitmap(LEVEL_NOTE_COL,  ir, m_pNoteYesGrayed);
                }
            }
            else {
                if (rgb == GetSysColor(COLOR_WINDOWTEXT)) {
                    QuickSetBitmap(LEVEL_NOTE_COL,  ir, m_pNoteNo);
                }
                else {
                    QuickSetBitmap(LEVEL_NOTE_COL,  ir, m_pNoteNoGrayed);
                }
            }

            QuickSetText     (LEVEL_LABEL_COL, ir, pRec->GetLabel());
            QuickSetTextColor(LEVEL_LABEL_COL, ir, rgb);

            QuickSetText     (LEVEL_NAME_COL, ir, pRec->GetName());
            QuickSetTextColor(LEVEL_NAME_COL, ir, rgb);

            QuickSetText     (LEVEL_TYPE_COL, ir, pRec->GetRecTypeVal());
            QuickSetTextColor(LEVEL_TYPE_COL, ir, rgb);

            QuickSetText      (LEVEL_REQ_COL, ir, BOOL_TO_TEXT(pRec->GetRequired()));
            QuickSetTextColor (LEVEL_REQ_COL, ir, rgb);

            CIMSAString csTemp;
            csTemp.Str(pRec->GetMaxRecs());
            QuickSetText     (LEVEL_MAX_COL, ir, csTemp);
            QuickSetAlignment(LEVEL_MAX_COL, ir, UG_ALIGNRIGHT);
            QuickSetTextColor(LEVEL_MAX_COL, ir, rgb);
            ir++;
        }
    }
    m_iFirstRow = 9999;   // BMD 03 Mar 2004
    for (ir = 0 ; ir < GetNumberRows() ; ir++) {
        CUGCell cell;
        GetCell(LEVEL_LABEL_COL, ir, &cell);
        if (cell.GetTextColor() == GetSysColor(COLOR_WINDOWTEXT)) {
            m_iFirstRow = ir;
            GotoRow(ir);
            for (int col = LEVEL_LABEL_COL ; col < GetNumberCols() ; col++) {
                GetCell(col, ir, &cell);
                cell.SetHTextColor(GetSysColor(COLOR_INACTIVECAPTIONTEXT));
                cell.SetHBackColor(GetSysColor(COLOR_INACTIVECAPTION));
                SetCell(col, ir, &cell);
            }
            break;
        }
    }
    RedrawWindow();
    //force on row change call to fix the property grid refresh when grids change
    OnRowChange(0, 0);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnCanMove
//
/////////////////////////////////////////////////////////////////////////////

int CLevelGrid::OnCanMove(int /*oldcol*/, long /*oldrow*/, int /*newcol*/, long newrow)
{
    CUGCell cell;
    GetCell(LEVEL_LABEL_COL, newrow, &cell);
    if (cell.GetTextColor() == GetSysColor(COLOR_GRAYTEXT)) {
        return FALSE;
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnLClicked
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::OnLClicked(int col, long row, int updn, RECT* /*rect*/, POINT* /*point*/, int /*processed*/)
{
    if (updn) {
        int r = GetCurrentRow();
        if (row != r) {
            m_bCanEdit = false;
        }
    }
    else {
        if (row < 0 || col < 0) {
            if (m_bEditing) {
                EditChange(VK_CANCEL);
            }
            return;
        }
        CView* pView = assert_cast<CView*>(GetParent());
        CDDDoc* pDoc = assert_cast<CDDDoc*>(pView->GetDocument());
        CUGCell cell;
        GetCell(LEVEL_LABEL_COL, row, &cell);
        if (cell.GetTextColor() == GetSysColor(COLOR_GRAYTEXT)) {
            m_bCanEdit = true;
            if (m_bEditing) {
                EditEnd();
                m_bAdding = false;
                m_bInserting = false;
            }
            int iLevel = 0;
            int iRec = -1;
            for( size_t level_number = 0; level_number < m_pDict->GetNumLevels(); ++level_number ) {
	            const DictLevel& dict_level = m_pDict->GetLevel(level_number);
                for (int r = 0 ; r < dict_level.GetNumRecords() ; r++) {
                    iRec++;
                    if (iRec == row) {
                        iLevel = level_number;
                        break;
                    }
                }
            }
            CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
            DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
            pTreeCtrl->SelectNode(*dictionary_dict_tree_node, false, iLevel);
            Update();
            GotoRow(row);
            SetFocus();
            return;
        }
        if (m_bCanEdit) {
            // Don't start an edit with button up on notes col,
            // it will crash with null m_aEditControl
            if (col != LEVEL_NOTE_COL) {
                ClearSelections();
                GotoRow(row);
                EditBegin(col, row, 0);
            }
        }
        else {
            if (m_bEditing) {
                EditEnd();
                m_bAdding = false;
                m_bInserting = false;
            }
            else {
                m_bCanEdit = true;
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnTH_RClicked
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::OnTH_RClicked(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed/* = 0*/)
{
    OnRClicked(col, row, updn, rect, point, processed);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnCB_RClicked
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::OnCB_RClicked(int updn, RECT* rect, POINT* point, int processed)
{
    OnRClicked(-1, -1, updn, rect, point, processed);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnRClicked
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::OnRClicked(int /*col*/, long row, int updn, RECT* /*rect*/, POINT* point, int /*processed*/)
{
    if (updn) {
        if (IsEditing()) {
            m_bAdding = false;
            m_bInserting = false;
            EditChange(VK_CANCEL);
        }
        return;
    }
    CUGCell cell;
    GetCell(LEVEL_LABEL_COL, GetCurrentRow(), &cell);

    BCMenu popMenu;   // BMD 29 Sep 2003
    popMenu.CreatePopupMenu();

    bool modification_allowed = ( cell.GetTextColor() != GetSysColor(COLOR_GRAYTEXT) );
    popMenu.AppendMenuItems(modification_allowed, { { ID_EDIT_CUT, _T("Cu&t\tCtrl+X") },
                                                    { ID_EDIT_COPY, _T("&Copy\tCtrl+C") } });

    CView* pView = assert_cast<CView*>(GetParent());
    CDDDoc* pDoc = assert_cast<CDDDoc*>(pView->GetDocument());

    bool can_paste = pDoc->GetDictClipboard().IsAvailable<CDictRecord>();
    popMenu.AppendMenuItems(can_paste, { { ID_EDIT_PASTE, _T("&Paste\tCtrl+V") } });

    popMenu.AppendMenu(MF_SEPARATOR);

    popMenu.AppendMenuItems(modification_allowed, { { ID_EDIT_MODIFY, _T("&Modify Record\tCtrl+M") } });

    popMenu.AppendMenu(MF_STRING, ID_EDIT_ADD, _T("&Add Record\tCtrl+A"));

    popMenu.AppendMenuItems(modification_allowed, { { ID_EDIT_INSERT, _T("&Insert Record\tIns") },
                                                    { ID_EDIT_DELETE, _T("&Delete Record\tDel") } });

    popMenu.AppendMenu(MF_SEPARATOR);

    popMenu.AppendMenu(MF_STRING, ID_EDIT_NOTES, _T("&Notes...\tCtrl+D"));

    GetCell(LEVEL_MAX_COL, GetCurrentRow(), &cell);
    CIMSAString csMax = cell.GetText();
    popMenu.AppendMenuItems(( csMax.Val() > 1 ), { { ID_EDIT_OCCURRENCELABELS, _T("Occurrence &Labels...") } });

    popMenu.LoadToolbar(IDR_DICT_FRAME);   // BMD 29 Sep 2003

    CRect rectWin;
    GetWindowRect(rectWin);
    if (point->x == 0) {
        CRect rectCell;
        GetCellRect(1, GetCurrentRow(), &rectCell);
        point->x = (rectCell.left + rectCell.right) / 2;
        point->y = (rectCell.top + rectCell.bottom) / 2;
    }
    popMenu.TrackPopupMenu(TPM_RIGHTBUTTON, rectWin.left + point->x, rectWin.top + point->y + GetRowHeight(row), this);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnDClicked
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::OnDClicked(int /*col*/, long row, RECT* /*rect*/, POINT* /*point*/, BOOL /*processed*/)
{
    CUGCell cell;
    GetCell(LEVEL_LABEL_COL, row, &cell);
    if (cell.GetTextColor() == GetSysColor(COLOR_GRAYTEXT)) {
        return;
    }
    CView* pView = assert_cast<CView*>(GetParent());
    CDDDoc* pDoc = assert_cast<CDDDoc*>(pView->GetDocument());
    CTreeCtrl* pTreeCtrl = (CTreeCtrl*)pDoc->GetTreeCtrl();
    HTREEITEM hItem = pTreeCtrl->GetSelectedItem();
    hItem = pTreeCtrl->GetChildItem(hItem);
    for (long i = m_iFirstRow - 1 ; i < row ; i++) {
        hItem = pTreeCtrl->GetNextSiblingItem(hItem);
    }
    pTreeCtrl->Select(hItem, TVGN_CARET);
    ((CDDGView*)pView)->m_gridRecord.PostMessage(WM_SETFOCUS);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnKeyDown
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::OnKeyDown(UINT* vcKey, BOOL /*processed*/)
{
    if (*vcKey==VK_PRIOR) {
        *vcKey = 0;
        if(GetKeyState(VK_CONTROL) < 0) {
            long newrow;
            for (newrow = 0 ; newrow < GetNumberRows() ; newrow++) {
                CUGCell cell;
                GetCell(LEVEL_LABEL_COL, newrow, &cell);
                if (cell.GetTextColor() != GetSysColor(COLOR_GRAYTEXT)) {
                    break;
                }
            }
            ClearSelections();
            GotoRow(newrow);
        }
        else {
            int height = m_GI->m_gridHeight;
            long newrow = GetCurrentRow();
            while (GetRowHeight(newrow) < height) {
                height -= GetRowHeight(newrow);
                newrow--;
                if (newrow < 1) {
                    break;
                }
            }
            for ( ; newrow > 0 ; newrow--) {
                CUGCell cell;
                GetCell(LEVEL_LABEL_COL, newrow, &cell);
                if (cell.GetTextColor() != GetSysColor(COLOR_GRAYTEXT)) {
                    break;
                }
            }
            for ( ; newrow < GetNumberRows() ; newrow++) {
                CUGCell cell;
                GetCell(LEVEL_LABEL_COL, newrow, &cell);
                if (cell.GetTextColor() != GetSysColor(COLOR_GRAYTEXT)) {
                    break;
                }
            }
            ClearSelections();
            GotoRow(newrow);
        }
    }
    else if (*vcKey==VK_NEXT) {
        *vcKey = 0;
        if(GetKeyState(VK_CONTROL) < 0) {
            long newrow;
            for (newrow = GetNumberRows() - 1 ; newrow > 0 ; newrow--) {
                CUGCell cell;
                GetCell(LEVEL_LABEL_COL, newrow, &cell);
                if (cell.GetTextColor() != GetSysColor(COLOR_GRAYTEXT)) {
                    break;
                }
            }
            ClearSelections();
            GotoRow(newrow);
        }
        else {
            int height = m_GI->m_gridHeight;
            long newrow = GetCurrentRow();
            while (GetRowHeight(newrow) < height) {
                height -= GetRowHeight(newrow);
                newrow++;
                if (newrow >= GetNumberRows() - 1) {
                    break;
                }
            }
            for ( ; newrow < GetNumberRows() ; newrow++) {
                CUGCell cell;
                GetCell(LEVEL_LABEL_COL, newrow, &cell);
                if (cell.GetTextColor() != GetSysColor(COLOR_GRAYTEXT)) {
                    break;
                }
            }
            for ( ; newrow > 0 ; newrow--) {
                CUGCell cell;
                GetCell(LEVEL_LABEL_COL, newrow, &cell);
                if (cell.GetTextColor() != GetSysColor(COLOR_GRAYTEXT)) {
                    break;
                }
            }
            ClearSelections();
            GotoRow(newrow);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnCharDown
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::OnCharDown(UINT* vcKey, BOOL /*processed*/)
{
    if (*vcKey == VK_TAB) {
        AfxGetMainWnd()->SendMessage(WM_IMSA_SETFOCUS);
    }
    else if (*vcKey == VK_ESCAPE) {
        CView* pView = assert_cast<CView*>(GetParent());
        CDDDoc* pDoc = assert_cast<CDDDoc*>(pView->GetDocument());
        CTreeCtrl* pTreeCtrl = (CTreeCtrl*)pDoc->GetTreeCtrl();
        HTREEITEM hItem = pTreeCtrl->GetSelectedItem();
        hItem = pTreeCtrl->GetParentItem(hItem);
        pTreeCtrl->Select(hItem, TVGN_CARET);
        ((CDDGView*)pView)->m_gridDict.PostMessage(WM_SETFOCUS);
    }
    else if (*vcKey >= 32) {
        EditBegin(LEVEL_LABEL_COL, GetCurrentRow(), *vcKey);
    }
    else if (*vcKey == VK_RETURN) {
        EditBegin(LEVEL_LABEL_COL, GetCurrentRow(), 0);
    }
    else if (*vcKey == 10) {
        CDDGView* pView = (CDDGView*)GetParent();
        CDDDoc* pDoc = assert_cast<CDDDoc*>(pView->GetDocument());
        CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
        DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
        int col;
        long row;
        EnumFirstSelected(&col, &row);
        int iRec = row - m_iFirstRow;
        pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, m_iLevel, iRec);

        pView->m_iGrid = DictionaryGrid::Record;
        pView->m_gridRecord.SetRedraw(FALSE);
        pView->m_gridRecord.Update(pDoc->GetDict(), m_iLevel, iRec);
        pView->ResizeGrid();
        pView->m_gridRecord.ClearSelections();
        pView->m_gridRecord.GotoRow(0);
        pView->m_gridRecord.SetRedraw(TRUE);
        pView->m_gridRecord.InvalidateRect(NULL);
        pView->m_gridRecord.SetFocus();
        if (pDoc->GetDict()->GetLevel(m_iLevel).GetRecord(iRec)->GetNumItems() == 0) {
            pView->m_gridRecord.SendMessage(WM_COMMAND, ID_EDIT_ADD);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnCellTypeNotify
//
/////////////////////////////////////////////////////////////////////////////

int CLevelGrid::OnCellTypeNotify(long /*ID*/, int col, long row, long msg, long /*param*/)
{
    CUGCell cell;
    GetCell(LEVEL_LABEL_COL, row, &cell);
    if (cell.GetTextColor() == GetSysColor(COLOR_GRAYTEXT)) {
        return FALSE;
    }
    if(msg == UGCT_BUTTONCLICK){
        if(col == LEVEL_NOTE_COL) {
            if (m_bEditing) {
                if (!EditEnd()) {
                    return TRUE;
                }
                m_bAdding = false;
                m_bInserting = false;
            }
            ASSERT(row == GetCurrentRow());
            OnEditNotes();
        }
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::EditBegin
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::EditBegin(int col, long row, UINT vcKey)
{
    ASSERT(row >= 0);
    ASSERT(col >= 0);
    GotoCol(col);
    VScrollEnable(ESB_DISABLE_BOTH);
    if (!(m_bAdding || m_bInserting)) {
        CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
        pDoc->PushUndo(*m_pDict->GetLevel(m_iLevel).GetRecord(row - m_iFirstRow), m_iLevel, row - m_iFirstRow);
    }
    CUGCell cell;
    CRect rect;
    CIMSAString cs;
    m_aEditControl.SetSize(LEVEL_MAX_COL + 1);
    m_bEditing = true;
    m_iEditRow = row;
    ClearSelections();

    // Create label edit
    m_pLabelEdit = new CLabelEdit();
    m_pLabelEdit->SetRowCol(row, LEVEL_LABEL_COL);
    GetCellRect(LEVEL_LABEL_COL, row, &rect);
    m_pLabelEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 100);
    m_pLabelEdit->SetFont(&m_font);
    GetCell(LEVEL_LABEL_COL, row, &cell);
    cell.GetText(&cs);
    m_pLabelEdit->SetWindowText(cs);
    m_aEditControl.SetAt(LEVEL_LABEL_COL, (CWnd*)m_pLabelEdit);

    // Create name edit
    m_pNameEdit = new CNameEdit();
    GetCellRect(LEVEL_NAME_COL, row, &rect);
    m_pNameEdit->SetRowCol(row, LEVEL_NAME_COL);
    m_pNameEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 101);
    m_pNameEdit->SetFont(&m_font);
    GetCell(LEVEL_NAME_COL, row, &cell);
    cell.GetText(&cs);
    m_pNameEdit->SetWindowText(cs);
    m_aEditControl.SetAt(LEVEL_NAME_COL, (CWnd*)m_pNameEdit);
    if (!m_bAdding && !m_bInserting) {
        m_pDict->SetOldName(cs);
        m_pDict->SetChangedObject(m_pDict->GetLevel(m_iLevel).GetRecord(row - m_iFirstRow));
    }

    // Create type edit
    m_pTypeEdit = new CLabelEdit();
    GetCellRect(LEVEL_TYPE_COL, row, &rect);
    m_pTypeEdit->SetRowCol(row, LEVEL_TYPE_COL);
    m_pTypeEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 102);
    m_pTypeEdit->LimitText(m_pDict->GetRecTypeLen());
    m_pTypeEdit->SetFont(&m_font);
    GetCell(LEVEL_TYPE_COL, row, &cell);
    cell.GetText(&cs);
    m_pTypeEdit->SetWindowText(cs);
    m_aEditControl.SetAt(LEVEL_TYPE_COL, (CWnd*)m_pTypeEdit);

    // Create required edit
    m_pReqEdit = new CDDComboBox();
    GetCellRect(LEVEL_REQ_COL, row, &rect);
    rect.top--;
    rect.left--;
    rect.bottom += 3*m_plf->lfHeight;
    rect.right += 2;
    m_pReqEdit->SetRowCol(row, LEVEL_REQ_COL);
    m_pReqEdit->Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, rect, this, 104);
    m_pReqEdit->SetFont(&m_font);
    m_pReqEdit->SetItemHeight (-1, m_plf->lfHeight);   // sets height for static control and button
    m_pReqEdit->SetItemHeight ( 0, m_plf->lfHeight);   // sets height for list box entries
    m_pReqEdit->AddString(_T("Yes"));
    m_pReqEdit->AddString(_T("No"));
    GetCell(LEVEL_REQ_COL, row, &cell);
    cell.GetText(&cs);
    if (cs == _T("Yes")) {
        m_pReqEdit->SetCurSel(0);
    }
    else {
        m_pReqEdit->SetCurSel(1);
    }
    m_aEditControl.SetAt(LEVEL_REQ_COL, (CWnd*)m_pReqEdit);

    // Create max edit
    m_pMaxEdit = new CNumEdit();
    GetCellRect(LEVEL_MAX_COL, row, &rect);
    m_pMaxEdit->SetRowCol(row, LEVEL_MAX_COL);
    m_pMaxEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 104);
    m_pMaxEdit->SetFont(&m_font);
    GetCell(LEVEL_MAX_COL, row, &cell);
    cell.GetText(&cs);
    m_pMaxEdit->SetWindowText(cs);
    m_aEditControl.SetAt(LEVEL_MAX_COL, (CWnd*)m_pMaxEdit);

    // Set focus to field
    m_iEditCol = col;
    m_iMinCol = LEVEL_LABEL_COL;
    m_iMaxCol = LEVEL_MAX_COL;
    if (col == LEVEL_LABEL_COL) {
        m_pLabelEdit->SetFocus();
        if (vcKey > 0) {
            m_pLabelEdit->SendMessage(WM_CHAR, vcKey, 1);
        }
    }
    else {
        m_aEditControl[col]->SetFocus();
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::EditEnd
//
/////////////////////////////////////////////////////////////////////////////

bool CLevelGrid::EditEnd(bool bSilent)
{
    bool bChanged = false;
    bool bChangedName = false;
    bool bValid = true;
    bool bUndo = false;
    long row = m_iEditRow;
    int iRec = row - m_iFirstRow;

    CDictRecord* pRec = m_pDict->GetLevel(m_iLevel).GetRecord(iRec);
    CString csNewLabel, csOldLabel;
    m_aEditControl[LEVEL_LABEL_COL]->GetWindowText(csNewLabel);
    csOldLabel = pRec->GetLabel();
    if (csNewLabel.Compare(csOldLabel) != 0) {
        bChanged = true;
        pRec->SetLabel(csNewLabel);
    }
    CString csNewName, csOldName;
    m_aEditControl[LEVEL_NAME_COL]->GetWindowText(csNewName);
    csOldName = pRec->GetName();
    if (csNewName.Compare(csOldName) != 0) {
        bChanged = true;
        bChangedName = true;
        pRec->SetName(csNewName);
    }
    CString csNewValue, csOldValue;
    m_aEditControl[LEVEL_TYPE_COL]->GetWindowText(csNewValue);
    csOldValue = pRec->GetRecTypeVal();
    if (csNewValue.Compare(csOldValue) != 0) {
        bChanged = true;
        pRec->SetRecTypeVal(csNewValue);
    }
    CString csNewReq;
    m_aEditControl[LEVEL_REQ_COL]->GetWindowText(csNewReq);
    bool bNewReq = false;
    if (csNewReq == _T("Yes")) {
        bNewReq = true;
    }
    bool bOldReq = pRec->GetRequired();
    if (bNewReq != bOldReq) {
        bChanged = true;
        pRec->SetRequired(bNewReq);
    }
    CIMSAString csNewMax;
    m_aEditControl[LEVEL_MAX_COL]->GetWindowText(csNewMax);
    UINT uNewMax = (UINT) csNewMax.Val();
    UINT uOldMax = pRec->GetMaxRecs();
    if (uNewMax != uOldMax) {
        bChanged = true;
        pRec->SetMaxRecs(uNewMax);
    }
    if (m_bAdding || m_bInserting) {
        if (pRec->GetLabel().IsEmpty() && pRec->GetName().IsEmpty()) {
            bUndo = true;
            m_bAdding = false;
            m_bInserting = false;
            bChanged = false;
            bValid = true;
        }
    }
    else if( bChanged == false && pRec->GetLabel().IsEmpty() && pRec->GetName().IsEmpty() ) // 20130412 this should happen when called from OnRClicked (via EditChange), this will prevent a blank row from being accepted
    {
        bUndo = true;
        bChanged = false;
        bValid = true;
    }

    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    DictionaryValidator* dictionary_validator = pDoc->GetDictionaryValidator();
    if (bChanged) {
        if (m_pDict->GetNumRecords() <= 1 && csNewValue.IsEmpty()) {
            m_pDict->SetRecTypeStart(0);
            m_pDict->SetRecTypeLen(0);
        }
        if (m_pDict->IsPosRelative()) {
            dictionary_validator->AdjustStartPositions();
        }
        bValid = dictionary_validator->IsValid(pRec, m_iLevel, iRec, bSilent);
        if (bValid) {
            pDoc->SetModified();
            QuickSetText(LEVEL_LABEL_COL, row, csNewLabel);
            QuickSetText(LEVEL_NAME_COL, row, csNewName);
            QuickSetText(LEVEL_TYPE_COL, row, csNewValue);
            QuickSetText(LEVEL_REQ_COL, row, csNewReq);
            QuickSetText(LEVEL_MAX_COL, row, csNewMax);
        }
        else {
            pRec->SetLabel(csOldLabel);
            pRec->SetName(csOldName);
            pRec->SetRecTypeVal(csOldValue);
            pRec->SetRequired(bOldReq);
            pRec->SetMaxRecs(uOldMax);
        }
    }
    if (bValid) {
        EditQuit();
        if (m_bAdding || m_bInserting) {
            m_pDict->BuildNameList();
        }
        else {
            m_pDict->UpdateNameList(*pRec, m_iLevel, iRec);
        }
        CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
        pTreeCtrl->InvalidateRect(NULL);
    }
    else {
        m_iEditCol = dictionary_validator->GetInvalidEdit();
        if (m_iEditCol < m_iMinCol || m_iEditCol > m_iMaxCol) {  // BMD 10 Mar 2003
            m_iEditCol = m_iMaxCol;
        }
        GotoRow(m_iEditRow);
        m_aEditControl[m_iEditCol]->SetFocus();
    }
    if (bUndo) {
        pDoc->UndoChange(FALSE);
    }
    if (bChangedName && bValid && !m_bAdding && !m_bInserting) {
        AfxGetMainWnd()->PostMessage(UWM::Dictionary::NameChange, (WPARAM)pDoc);
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::EditQuit
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::EditQuit()
{
    SetRedraw(FALSE);
    delete m_pLabelEdit;
    delete m_pNameEdit;
    delete m_pTypeEdit;
    delete m_pReqEdit;
    delete m_pMaxEdit;
    m_aEditControl.RemoveAll();
    SetRedraw(TRUE);
    m_bEditing = false;
    m_bCanEdit = true;
    m_iEditCol = NONE;
    ((CMDIFrameWnd*)AfxGetMainWnd())->MDIGetActive()->SendMessage(WM_INITMENU, IDR_DICT_FRAME);
    SetFocus();
    VScrollEnable(ESB_ENABLE_BOTH);
    VScrollRedraw();
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnSetFocus
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::OnSetFocus(int /*section*/)
{
    CDDGView* pView = (CDDGView*)GetParent();
    CSplitterWnd* pSplitWnd = (CSplitterWnd*)pView->GetParent();
    pSplitWnd->SetActivePane(0,0);
    for (long row = 0 ; row < GetNumberRows() ; row++) {
        for (int col = LEVEL_LABEL_COL ; col < GetNumberCols() ; col++) {
            QuickSetHTextColor(col, row, GetSysColor(COLOR_HIGHLIGHTTEXT));
            QuickSetHBackColor(col, row, GetSysColor(COLOR_HIGHLIGHT));
        }
        RedrawRow(row);
    }
    if (m_aEditControl.GetSize() > 0) {
        m_aEditControl[m_iEditCol]->SetFocus();
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnKillFocus
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::OnKillFocus(int /*section*/)
{
    m_bCanEdit = false;

    int col;
    long row;
    if (EnumFirstSelected(&col, &row) != UG_SUCCESS) {
        return;
    }

    for (col = LEVEL_LABEL_COL ; col < GetNumberCols() ; col++) {
        QuickSetHTextColor(col, row, GetSysColor(COLOR_WINDOWTEXT));
        QuickSetHBackColor(col, row, GetSysColor(COLOR_BTNFACE));
    }
    RedrawRow(row);
    while (EnumNextSelected(&col, &row) == UG_SUCCESS) {
        for (col = LEVEL_LABEL_COL ; col < GetNumberCols() ; col++) {
            QuickSetHTextColor(col, row, GetSysColor(COLOR_WINDOWTEXT));
            QuickSetHBackColor(col, row, GetSysColor(COLOR_BTNFACE));
        }
        RedrawRow(row);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnEditUndo
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::OnEditUndo()
{
    ASSERT(m_bEditing);
    ASSERT(m_iEditCol == LEVEL_LABEL_COL || m_iEditCol == LEVEL_NAME_COL ||
            m_iEditCol == LEVEL_TYPE_COL || m_iEditCol == LEVEL_MAX_COL);

    if (assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->CanUndo()) {
        assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->Undo();
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnEditCut
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::OnEditCut()
{
    if (m_bEditing) {
        ASSERT(m_iEditCol == LEVEL_LABEL_COL || m_iEditCol == LEVEL_NAME_COL ||
                m_iEditCol == LEVEL_TYPE_COL || m_iEditCol == LEVEL_MAX_COL);
        assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->Cut();
        return;
    }
    EditCopy(true);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnEditCopy
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::OnEditCopy()
{
    if (m_bEditing) {
        ASSERT(m_iEditCol == LEVEL_LABEL_COL || m_iEditCol == LEVEL_NAME_COL ||
                m_iEditCol == LEVEL_TYPE_COL || m_iEditCol == LEVEL_MAX_COL);
        assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->Copy();
        return;
    }
    EditCopy(false);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnEditPaste
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::OnEditPaste()
{
    // If editing, paste to the control
    if( m_bEditing )
    {
        ASSERT(m_iEditCol == LEVEL_LABEL_COL || m_iEditCol == LEVEL_NAME_COL ||
               m_iEditCol == LEVEL_TYPE_COL || m_iEditCol == LEVEL_MAX_COL);
        assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->Paste();
        return;
    }

    // Can paste only records
    DictLevel& dict_level = m_pDict->GetLevel(m_iLevel);

    long current_row = GetCurrentRow();
    long record_number = current_row - m_iFirstRow;

    if( record_number < 0 )
    {
        record_number = 0;
    }

    else if( record_number == ( (long)dict_level.GetNumRecords() - 1 ) )
    {
        BeforeAfterDlg dlg;

        if( dlg.DoModal() != IDOK )
            return;

        if( dlg.SelectedAfter() )
            ++record_number;
    }

    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    DictPastedValues<CDictRecord> pasted_records = pDoc->GetDictClipboard().GetNamedElementsFromClipboard<CDictRecord>(this);

    if( pasted_records.values.empty() )
        return;

    // Push old dictionary on stack
    pDoc->PushUndo(*m_pDict, m_iLevel, dict_level.GetNumRecords() - 1);

    // insert the records
    for( CDictRecord& dict_record : pasted_records.values )
    {
        dict_level.InsertRecordAt(record_number, &dict_record);
        ++record_number;
    }

    // update the current language indices
    m_pDict->SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());

    // sync any linked value sets
    m_pDict->SyncLinkedValueSets(CDataDict::SyncLinkedValueSetsAction::OnPaste, &pasted_records.value_set_names_added);

    // Finish up modification
    pDoc->SetModified();

    if( m_pDict->IsPosRelative() )
        pDoc->GetDictionaryValidator()->AdjustStartPositions();

    // Check is dictionary still OK
    m_pDict->BuildNameList();
    pDoc->GetDictionaryValidator()->IsValid(m_pDict, true, true);

    // Update tree
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
    pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, m_iLevel);

    // Update grid
    Update();
    GotoRow(current_row);
    SetFocus();
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnEditAdd
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::OnEditAdd()
{
    // If tree control has focus, position to item on level grid and post add message
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    if (GetFocus() == pTreeCtrl) {
        POSITION pos = pDoc->GetFirstViewPosition();
        ASSERT(pos != NULL);
        CDDGView* pView = (CDDGView*)pDoc->GetNextView(pos);
        DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
        int iLevel = pDoc->GetLevel();
        pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node);
        pView->m_iGrid = DictionaryGrid::Dictionary;
        pView->m_gridDict.SetRedraw(FALSE);
        pView->m_gridDict.Update(pDoc->GetDict());
        pView->ResizeGrid();
        pView->m_gridDict.ClearSelections();
        pView->m_gridDict.GotoRow(iLevel + CDictGrid::GetFirstLevelRow());
        pView->m_gridDict.SetRedraw(TRUE);
        pView->m_gridDict.InvalidateRect(NULL);
        pView->m_gridDict.SetFocus();
        pView->m_gridDict.PostMessage(WM_COMMAND, ID_EDIT_ADD);
        return;
    }

    m_bAdding = true;

    // Push old dictionary on stack
    DictLevel& dict_level = m_pDict->GetLevel(m_iLevel);
    pDoc->PushUndo(*m_pDict, m_iLevel, dict_level.GetNumRecords() - 1);

    // Check if record type needs changing
    size_t num_records = m_pDict->GetNumRecords();
    if (num_records == 1 && m_pDict->GetRecTypeLen() == 0) {
        m_pDict->SetRecTypeStart(1);
        m_pDict->SetRecTypeLen(1);
    }

    // Insert record
    CDictRecord rec;
    rec.GetLabelSet().SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());
    if (num_records > 0) {
        rec.SetRecTypeVal(pDoc->GetDictionaryValidator()->GetDefaultRecTypeVal());
    }

    int iRec = dict_level.GetNumRecords();
    dict_level.AddRecord(&rec);

//    pTreeCtrl->SetUpdateAllViews(false);
    DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
    pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, m_iLevel);

    Update();
    GotoRow(m_iFirstRow + iRec);
//    InvalidateRect(NULL);

    EditBegin(LEVEL_LABEL_COL, m_iFirstRow + iRec, 0);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnEditInsert
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::OnEditInsert()
{
    // If tree control has focus, position to item on dict grid and post delete message
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    if (GetFocus() == pTreeCtrl) {
        POSITION pos = pDoc->GetFirstViewPosition();
        ASSERT(pos != NULL);
        CDDGView* pView = (CDDGView*)pDoc->GetNextView(pos);
        DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
        int iLevel = pDoc->GetLevel();
        pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node);
        pView->m_iGrid = DictionaryGrid::Dictionary;
        pView->m_gridDict.SetRedraw(FALSE);
        pView->m_gridDict.Update(pDoc->GetDict());
        pView->ResizeGrid();
        pView->m_gridDict.ClearSelections();
        pView->m_gridDict.GotoRow(iLevel + CDictGrid::GetFirstLevelRow());
        pView->m_gridDict.SetRedraw(TRUE);
        pView->m_gridDict.InvalidateRect(NULL);
        pView->m_gridDict.SetFocus();
        pView->m_gridDict.PostMessage(WM_COMMAND, ID_EDIT_INSERT);
        return;
    }

    // Find place to insert
    int iRec = GetCurrentRow() - m_iFirstRow;
    ASSERT(iRec >= 0);
    m_bInserting = true;

    // Push old level on stack
    DictLevel& dict_level = m_pDict->GetLevel(m_iLevel);
    pDoc->PushUndo(dict_level, m_iLevel, iRec);

    // Check if record type needs changing
    size_t num_records = m_pDict->GetNumRecords();
    if (num_records == 1 && m_pDict->GetRecTypeLen() == 0) {
        m_pDict->SetRecTypeStart(1);
        m_pDict->SetRecTypeLen(1);
    }

    // Insert record
    CDictRecord rec;
    rec.GetLabelSet().SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());
    if (num_records > 0) {
        rec.SetRecTypeVal(pDoc->GetDictionaryValidator()->GetDefaultRecTypeVal());
    }
    dict_level.InsertRecordAt(iRec, &rec);
    m_pDict->BuildNameList();              // BMD 25 May 2005

    // Update tree
//    pTreeCtrl->SetUpdateAllViews(false);
    DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
    pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, m_iLevel);

    // Update grid
    Update();
    GotoRow(m_iFirstRow + iRec);
//    InvalidateRect(NULL);

    // Begin editing new record
    EditBegin(LEVEL_LABEL_COL, m_iFirstRow + iRec, 0);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnEditDelete
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::OnEditDelete()
{
    // If editing, clear edit control
    if (m_bEditing) {
        ASSERT(m_iEditCol == LEVEL_LABEL_COL || m_iEditCol == LEVEL_NAME_COL ||
                m_iEditCol == LEVEL_TYPE_COL || m_iEditCol == LEVEL_MAX_COL);
        assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->Clear();
        return;
    }
    // If tree control has focus, position to item on dict grid and post delete message
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    if (GetFocus() == pTreeCtrl) {
        POSITION pos = pDoc->GetFirstViewPosition();
        ASSERT(pos != NULL);
        CDDGView* pView = (CDDGView*)pDoc->GetNextView(pos);
        DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
        int iLevel = pDoc->GetLevel();
        pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node);
        pView->m_iGrid = DictionaryGrid::Dictionary;
        pView->m_gridDict.SetRedraw(FALSE);
        pView->m_gridDict.Update(pDoc->GetDict());
        pView->ResizeGrid();
        pView->m_gridDict.ClearSelections();
        pView->m_gridDict.GotoRow(iLevel + CDictGrid::GetFirstLevelRow());
        pView->m_gridDict.SetRedraw(TRUE);
        pView->m_gridDict.InvalidateRect(NULL);
        pView->m_gridDict.SetFocus();
        pView->m_gridDict.PostMessage(WM_COMMAND, ID_EDIT_DELETE);
        return;
    }

    EditDelete(GetSelectedRecords());
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::EditCopy
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::EditCopy(bool bCut)
{
    std::vector<size_t> selected_records = GetSelectedRecords();

    // Copy records to clipboard
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    const DictLevel& dict_level = m_pDict->GetLevel(m_iLevel);
    std::vector<const CDictRecord*> dict_records;

    for( long selected_record : selected_records )
        dict_records.emplace_back(dict_level.GetRecord(selected_record));

    pDoc->GetDictClipboard().PutOnClipboard(this, dict_records);

    // If cut, delete records
    if( bCut )
        EditDelete(selected_records);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::EditDelete
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::EditDelete(const std::vector<size_t>& selected_records)
{
    ASSERT(!selected_records.empty());

    DictLevel& dict_level = m_pDict->GetLevel(m_iLevel);

    // Push old level on stack
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    pDoc->PushUndo(dict_level, m_iLevel, selected_records.front());

    // Delete records
    for( auto record_itr = selected_records.crbegin(); record_itr != selected_records.crend(); ++record_itr )
        dict_level.RemoveRecordAt(*record_itr);

    pDoc->SetModified();

    if( m_pDict->IsPosRelative() )
        pDoc->GetDictionaryValidator()->AdjustStartPositions();

    // Check if dictionary still OK
    m_pDict->BuildNameList();
    pDoc->GetDictionaryValidator()->IsValid(m_pDict, true, true);

    // Set position
    int row_to_select = std::min((int)selected_records.front(), (int)dict_level.GetNumRecords() - 1);

    // Update tree
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
    pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, m_iLevel);

    // Update grid
    Update();
    ClearSelections();
    GotoRow(row_to_select);
    InvalidateRect(NULL);
    SetFocus();
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnEditModify
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::OnEditModify()
{
    UINT vcKey = VK_RETURN;
    BOOL bProcessed = FALSE;
    // If tree control has focus, position to item on record grid and post delete message
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    if (GetFocus() == pTreeCtrl) {
        POSITION pos = pDoc->GetFirstViewPosition();
        ASSERT(pos != NULL);
        CDDGView* pView = (CDDGView*)pDoc->GetNextView(pos);
        DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
        int iLevel = pDoc->GetLevel();
        pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node);
        pView->m_iGrid = DictionaryGrid::Dictionary;
        pView->m_gridDict.SetRedraw(FALSE);
        pView->m_gridDict.Update(pDoc->GetDict());
        pView->ResizeGrid();
        pView->m_gridDict.ClearSelections();
        pView->m_gridDict.GotoRow(iLevel + CDictGrid::GetFirstLevelRow());
        pView->m_gridDict.SetRedraw(TRUE);
        pView->m_gridDict.InvalidateRect(NULL);
        pView->m_gridDict.SetFocus();
        pView->m_gridDict.OnCharDown(&vcKey, bProcessed);
        return;
    }
    OnCharDown(&vcKey, bProcessed);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnEditNotes
//
/////////////////////////////////////////////////////////////////////////////

void CLevelGrid::OnEditNotes()
{
    long row = GetCurrentRow();
    int iLevel = m_iLevel;
    int iRec = row - m_iFirstRow;
    CDictRecord* pRec = m_pDict->GetLevel(iLevel).GetRecord(iRec);
    CString csTitle, csLabel, csNote;
    csTitle.LoadString(IDS_NOTE_TITLE);
    csLabel = pRec->GetLabel().Left(32);
    if (pRec->GetLabel().GetLength() > 32)  {
        csLabel += _T("...");
    }
    csTitle = _T("Record: ") + csLabel + csTitle;
    csNote = pRec->GetNote();
    CNoteDlg dlgNote;
    dlgNote.SetTitle(csTitle);
    dlgNote.SetNote(csNote);
    if (dlgNote.DoModal() == IDOK)  {
        if (csNote != dlgNote.GetNote()) {
            CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
            pDoc->PushUndo(*m_pDict->GetLevel(m_iLevel).GetRecord(row - m_iFirstRow), m_iLevel, row - m_iFirstRow);
            pRec->SetNote(dlgNote.GetNote());
            pDoc->SetModified();
        }
    }
    Update();
    GotoCell(LEVEL_LABEL_COL, row);
    SetFocus();
}

void CLevelGrid::OnEditOccLabels()
{
    GetParent()->SendMessage(WM_COMMAND, ID_EDIT_OCCURRENCELABELS);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLevelGrid::OnShiftF10
//
/////////////////////////////////////////////////////////////////////////////
void CLevelGrid::OnShiftF10()
{
    int col = 0;
    long row = 0;
    RECT rect = RECT();
    POINT point = POINT();
    int processed = 0;

    // 20130412 now simulates the actions of starting and ending a right-click
    OnRClicked(col, row, 1, &rect, &point, processed);

    if( !IsEditing() )
        OnRClicked(col, row, 0, &rect, &point, processed);
}

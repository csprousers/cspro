//***************************************************************************
//  File name: DictGrid.cpp
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
#include "Dictgrid.h"


namespace
{
    constexpr long HEADER_ROW_LEVEL = 2;
    constexpr long LEVEL_ROW_OFFSET = HEADER_ROW_LEVEL + 1;
    static_assert(LEVEL_ROW_OFFSET == CDictGrid::GetFirstLevelRow());
                             
    constexpr int DICT_NOTE_COL     = 0;
    constexpr int DICT_LABEL_COL    = 1;
    constexpr int DICT_NAME_COL     = 2;

    constexpr int DICT_NUM_COLS     = DICT_NAME_COL + 1;
}


BEGIN_MESSAGE_MAP(CDictGrid, CDDGrid)
    ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
    ON_COMMAND(ID_EDIT_ADD, OnEditAdd)
    ON_COMMAND(ID_EDIT_INSERT, OnEditInsert)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_COMMAND(ID_EDIT_CUT, OnEditCut)
    ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
    ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
    ON_COMMAND(ID_EDIT_MODIFY, OnEditModify)
    ON_COMMAND(ID_EDIT_NOTES, OnEditNotes)
    ON_COMMAND(ID_SHIFT_F10, OnShiftF10)
END_MESSAGE_MAP()


CDictGrid::CDictGrid()
    :   m_pLabelEdit(nullptr),
        m_pNameEdit(nullptr)
{
}


std::vector<size_t> CDictGrid::GetSelectedLevels()
{
    std::vector<size_t> selected_levels;

    for( long row : GetSelectedRows() )
        selected_levels.emplace_back(row - LEVEL_ROW_OFFSET);

    return selected_levels;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::OnSetup
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::OnSetup()
{
    EnableExcelBorders(FALSE);
    VScrollAlwaysPresent(TRUE);
    SetNumberCols(DICT_NUM_COLS, FALSE);
    SetNumberRows(LEVEL_ROW_OFFSET + 1, FALSE);
    SetSH_Width(0);
    SetCurrentCellMode(2);
    SetHighlightRow(TRUE);
    SetMultiSelectMode(TRUE);

    m_iButton = AddCellType(&m_button);

    auto set_header = [&](int column, const TCHAR* dict_text, const TCHAR* level_text, int alignment = 0)
    {
        QuickSetText(column, HEADER_ROW, dict_text);
        QuickSetText(column, HEADER_ROW_LEVEL, level_text);

        if( alignment != 0 )
        {
            QuickSetAlignment(column, HEADER_ROW, alignment);
            QuickSetAlignment(column, HEADER_ROW_LEVEL, alignment);
        }

        QuickSetBackColor(column, HEADER_ROW_LEVEL, GetSysColor(COLOR_BTNFACE));
        QuickSetBorder(column, HEADER_ROW_LEVEL, UG_BDR_RAISED);
    };

    set_header(DICT_NOTE_COL,  _T("N"),                _T("N"));
    set_header(DICT_LABEL_COL, _T("Dictionary Label"), _T("Level Label"), UG_ALIGNLEFT);
    set_header(DICT_NAME_COL,  _T("Dictionary Name"),  _T("Level Name"),  UG_ALIGNLEFT);

    m_bAdding = false;
    m_bInserting = false;
    m_bEditing = false;
    GotoCell(DICT_LABEL_COL, 0);                         // 15 Jan 2002
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::Size
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::Size(CRect rect)
{
    CIMSAString csWidths = AfxGetApp()->GetProfileString(_T("Data Dictionary"), _T("DictGridWidths"), _T("-1"));
    if (csWidths == _T("-1")) {
        CUGCell cell;
        CUGCellType* pCellType;
        CSize size;

        int iUsed = static_cast<int>(NOTE_WIDTH * (GetDesignerFontZoomLevel() / 100.0));
        SetColWidth(DICT_NOTE_COL, iUsed);
        GetCell(DICT_NAME_COL, HEADER_ROW, &cell);
        pCellType = GetCellType(HEADER_ROW, DICT_NAME_COL);
        pCellType->GetBestSize(GetDC(), &size, &cell);
        SetColWidth(DICT_NAME_COL, size.cx + BORDER_WIDTH);
        iUsed += size.cx + BORDER_WIDTH;
        GetCell(DICT_LABEL_COL, HEADER_ROW, &cell);
        pCellType = GetCellType(HEADER_ROW, DICT_LABEL_COL);
        pCellType->GetBestSize(GetDC(), &size, &cell);
        if (size.cx > rect.Width() - m_GI->m_vScrollWidth - iUsed - 1) {
            SetColWidth(DICT_LABEL_COL, size.cx);
        }
        else {
            SetColWidth(DICT_LABEL_COL, rect.Width() - m_GI->m_vScrollWidth - iUsed - 1);
        }
        csWidths.Format(_T("%d,%d,%d"), GetColWidth(DICT_NOTE_COL),
                                        GetColWidth(DICT_LABEL_COL),
                                        GetColWidth(DICT_NAME_COL));
        AfxGetApp()->WriteProfileString(_T("Data Dictionary"), _T("DictGridWidths"), csWidths);
    }
    else {
        CIMSAString csWidth = csWidths.GetToken();
        SetColWidth(DICT_NOTE_COL, (int) csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(DICT_LABEL_COL, (int) csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(DICT_NAME_COL, (int) csWidth.Val());
    }
    Resize(rect);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::Resize
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::Resize(CRect rect)
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
//                        CDictGrid::OnColSized
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::OnColSized(int /*col*/, int* /*width*/)
{
    if (m_aEditControl.GetSize() > 0) {
        for (int i = m_iMinCol ; i <= m_iMaxCol ; i++) {
            m_aEditControl[i]->PostMessage(WM_SIZE);
            m_aEditControl[i]->PostMessage(WM_PAINT);
        }
    }
    CString csWidths;
    csWidths.Format(_T("%d,%d,%d"), GetColWidth(DICT_NOTE_COL),
                                    GetColWidth(DICT_LABEL_COL),
                                    GetColWidth(DICT_NAME_COL));
    AfxGetApp()->WriteProfileString(_T("Data Dictionary"), _T("DictGridWidths"), csWidths);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::Update
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::Update(CDataDict* pDict)
{
    m_pDict = pDict;
    Update();
}

void CDictGrid::Update()
{
    QuickSetCellType  (DICT_NOTE_COL,  0, m_iButton);
    QuickSetCellTypeEx(DICT_NOTE_COL,  0, UGCT_BUTTONNOFOCUS);
    QuickSetBackColor (DICT_NOTE_COL,  0, GetSysColor(COLOR_BTNFACE));
    QuickSetHBackColor(DICT_NOTE_COL,  0, GetSysColor(COLOR_BTNFACE));
    if (m_pDict->GetNote().GetLength() > 0) {
        QuickSetBitmap(DICT_NOTE_COL,  0, m_pNoteYes);
    }
    else {
        QuickSetBitmap(DICT_NOTE_COL,  0, m_pNoteNo);
    }
    QuickSetText      (DICT_LABEL_COL, 0, m_pDict->GetLabel());
    QuickSetText      (DICT_NAME_COL,  0, m_pDict->GetName());

    QuickSetBackColor (DICT_NOTE_COL,  1, GetSysColor(COLOR_WINDOW));
    QuickSetHBackColor(DICT_NOTE_COL,  1, GetSysColor(COLOR_WINDOW));
    QuickSetBackColor (DICT_NOTE_COL,  2, GetSysColor(COLOR_BTNFACE));
    QuickSetHTextColor(DICT_NOTE_COL,  2, GetSysColor(COLOR_WINDOWTEXT));
    QuickSetHBackColor(DICT_NOTE_COL,  2, GetSysColor(COLOR_BTNFACE));

    SetNumberRows(m_pDict->GetNumLevels() + LEVEL_ROW_OFFSET, FALSE);
    for( size_t level_number = 0; level_number < m_pDict->GetNumLevels(); ++level_number ) {
        const DictLevel& dict_level = m_pDict->GetLevel(level_number);
        long row = level_number + LEVEL_ROW_OFFSET;
        QuickSetCellType  (DICT_NOTE_COL,  row, m_iButton);
        QuickSetCellTypeEx(DICT_NOTE_COL,  row, UGCT_BUTTONNOFOCUS);
        QuickSetBackColor (DICT_NOTE_COL,  row, GetSysColor(COLOR_BTNFACE));
        QuickSetHBackColor(DICT_NOTE_COL,  row, GetSysColor(COLOR_BTNFACE));
        if (dict_level.GetNote().GetLength() > 0) {
            QuickSetBitmap(DICT_NOTE_COL,  row, m_pNoteYes);
        }
        else {
            QuickSetBitmap(DICT_NOTE_COL,  row, m_pNoteNo);
        }
        QuickSetText      (DICT_LABEL_COL, row, dict_level.GetLabel());
        QuickSetText      (DICT_NAME_COL,  row, dict_level.GetName());
    }
    RedrawWindow();
    //force on row change call to fix the property grid refresh when grids change
    OnRowChange(0, 0);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::OnCanMove
//
/////////////////////////////////////////////////////////////////////////////

int CDictGrid::OnCanMove(int /*oldcol*/, long /*oldrow*/, int /*newcol*/, long newrow)
{
    if (newrow == 0) {
        SetMultiSelectMode(FALSE);
    }
    if (newrow == 1) {
        GotoRow(LEVEL_ROW_OFFSET);
        return FALSE;
    }
    if (newrow == 2) {
        GotoRow(0);
        return FALSE;
    }
    if (newrow >= LEVEL_ROW_OFFSET && GetCurrentRow() == 0) {
        SetMultiSelectMode(TRUE);
    }
    return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::OnLClicked
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::OnLClicked(int col, long row, int updn, RECT* /*rect*/, POINT* /*point*/, int /*processed*/)
{
    if (updn) {
        int r = GetCurrentRow();
        if (row != r) {
            m_bCanEdit = false;
        }
    }
    else {
        if (row < 0 || row == 1 || row == 2 || col < 0) {
            if (m_bEditing) {
                EditChange(VK_CANCEL);
            }
            return;
        }
        if (m_bCanEdit) {
            // Don't start an edit with button up on notes col,
            // it will crash with null m_aEditControl
            if (col != DICT_NOTE_COL) {
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
//                        CDictGrid::OnTH_RClicked
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::OnTH_RClicked(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed/* = 0*/)
{
    OnRClicked(col, row, updn, rect, point, processed);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::OnCB_RClicked
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::OnCB_RClicked(int updn, RECT* rect, POINT* point, int processed)
{
    OnRClicked(-1, -1, updn, rect, point, processed);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::OnRClicked
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::OnRClicked(int /*col*/, long row, int updn, RECT* /*rect*/, POINT* point, int /*processed*/)
{
    if (updn) {
        if (IsEditing()) {
            m_bAdding = false;
            m_bInserting = false;
            EditChange(VK_CANCEL);
        }
        return;
    }
    BCMenu popMenu;   // BMD 29 Sep 2003
    popMenu.CreatePopupMenu();

    bool level_selected = ( GetCurrentRow() >= LEVEL_ROW_OFFSET );
    popMenu.AppendMenuItems(level_selected, { { ID_EDIT_CUT, _T("Cu&t\tCtrl+X") },
                                              { ID_EDIT_COPY, _T("&Copy\tCtrl+C") } });

    CView* pView = assert_cast<CView*>(GetParent());
    CDDDoc* pDoc = assert_cast<CDDDoc*>(pView->GetDocument());

    bool can_paste = pDoc->GetDictClipboard().IsAvailable<DictLevel>();
    popMenu.AppendMenuItems(can_paste, { { ID_EDIT_PASTE, _T("&Paste\tCtrl+V") } });

    popMenu.AppendMenu(MF_SEPARATOR);

    popMenu.AppendMenuItems(level_selected, { { ID_EDIT_MODIFY, _T("&Modify Level\tCtrl+M") } });

    popMenu.AppendMenu(MF_STRING, ID_EDIT_ADD, _T("&Add Level\tCtrl+A"));

    popMenu.AppendMenuItems(level_selected, { { ID_EDIT_INSERT, _T("&Insert Level\tIns") },
                                              { ID_EDIT_DELETE, _T("&Delete Level\tDel") } });

    popMenu.AppendMenu(MF_SEPARATOR);

    popMenu.AppendMenu(MF_STRING, ID_EDIT_NOTES, _T("&Notes...\tCtrl+D"));

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
//                        CDictGrid::OnDClicked
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::OnDClicked(int /*col*/, long row, RECT* /*rect*/, POINT* /*point*/, BOOL /*processed*/)
{
    if (row < LEVEL_ROW_OFFSET) {
        return;
    }
    CView* pView = assert_cast<CView*>(GetParent());
    CDDDoc* pDoc = assert_cast<CDDDoc*>(pView->GetDocument());
    CTreeCtrl* pTreeCtrl = (CTreeCtrl*)pDoc->GetTreeCtrl();
    HTREEITEM hItem = pTreeCtrl->GetSelectedItem();
    hItem = pTreeCtrl->GetChildItem(hItem);
    for (long i = LEVEL_ROW_OFFSET; i < row; i++) {
        hItem = pTreeCtrl->GetNextSiblingItem(hItem);
    }
    pTreeCtrl->Select(hItem, TVGN_CARET);
    ((CDDGView*) pView)->m_gridLevel.PostMessage(WM_SETFOCUS);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::OnKeyDown
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::OnKeyDown(UINT* vcKey, BOOL /*processed*/)
{
    if (*vcKey==VK_PRIOR) {
        *vcKey = 0;
        if(GetKeyState(VK_CONTROL) < 0) {
            ClearSelections();
            GotoRow(0);
        }
        else {
            int height = m_GI->m_gridHeight;
            long newrow = GetCurrentRow();
            while (GetRowHeight(newrow) < height) {
                height -= GetRowHeight(newrow);
                newrow--;
                if (newrow < 4) {
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
            ClearSelections();
            GotoRow(GetNumberRows() - 1);
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
            ClearSelections();
            GotoRow(newrow);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::OnCharDown
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::OnCharDown(UINT* vcKey, BOOL /*processed*/)
{
    if (*vcKey==VK_TAB) {
        AfxGetMainWnd()->SendMessage(WM_IMSA_SETFOCUS);
    }
    else if (*vcKey >= 32) {
        EditBegin(DICT_LABEL_COL, GetCurrentRow(), *vcKey);
    }
    else if (*vcKey == VK_RETURN) {
        EditBegin(DICT_LABEL_COL, GetCurrentRow(), 0);
    }
    else if (*vcKey == 10) {
        int col;
        long row;
        EnumFirstSelected(&col, &row);
        if (row > 0) {
            int iLevel = row - LEVEL_ROW_OFFSET;
            CDDGView* pView = (CDDGView*) GetParent();
            CDDDoc* pDoc = assert_cast<CDDDoc*>(pView->GetDocument());
            CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
            DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
            pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, iLevel);

            pView->m_iGrid = DictionaryGrid::Level;
            pView->m_gridLevel.SetRedraw(FALSE);
            pView->m_gridLevel.Update(pDoc->GetDict(), iLevel);
            pView->ResizeGrid();
            pView->m_gridLevel.ClearSelections();
            pView->m_gridLevel.GotoCell(1, pView->m_gridLevel.GetFirstRow());
            pView->m_gridLevel.SetRedraw(TRUE);
            pView->m_gridLevel.InvalidateRect(NULL);
            pView->m_gridLevel.SetFocus();
            if (pDoc->GetDict()->GetLevel(iLevel).GetNumRecords() == 0) {
                pView->SendMessage(WM_COMMAND, ID_EDIT_ADD);
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::CellTypeNotify
//
/////////////////////////////////////////////////////////////////////////////

int CDictGrid::OnCellTypeNotify(long /*ID*/, int col, long row, long msg, long /*param*/)
{
    if(msg == UGCT_BUTTONCLICK){
        if(col == DICT_NOTE_COL) {
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
//                        CDictGrid::EditBegin
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::EditBegin(int col, long row, UINT vcKey)
{
    ASSERT(row >= 0);
    ASSERT(col >= 0);
    GotoCol(col);
    VScrollEnable(ESB_DISABLE_BOTH);
    if (!(m_bAdding || m_bInserting) && row > 0) {
        CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
        pDoc->PushUndo(m_pDict->GetLevel(row - LEVEL_ROW_OFFSET), row - LEVEL_ROW_OFFSET);
    }

    CUGCell cell;
    CRect rect;
    CIMSAString cs;
    m_aEditControl.SetSize(DICT_NAME_COL + 1);
    m_bEditing = true;
    m_iEditRow = row;
    ClearSelections();

    // Create label edit
    m_pLabelEdit = new CLabelEdit();
    m_pLabelEdit->SetRowCol(row, DICT_LABEL_COL);
    GetCellRect(DICT_LABEL_COL, row, &rect);
    m_pLabelEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 100);
    m_pLabelEdit->SetFont(&m_font);
    GetCell(DICT_LABEL_COL, row, &cell);
    cell.GetText(&cs);
    m_pLabelEdit->SetWindowText(cs);
    m_aEditControl.SetAt(DICT_LABEL_COL, (CWnd*) m_pLabelEdit);

    // Create name edit
    if (m_bAdding || m_bInserting) {
        m_pDict->GetLevel(row - LEVEL_ROW_OFFSET).SetName(_T(" X"));
    }
    m_pNameEdit = new CNameEdit();
    GetCellRect(DICT_NAME_COL, row, &rect);
    m_pNameEdit->SetRowCol(row, DICT_NAME_COL);
    m_pNameEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 101);
    m_pNameEdit->SetFont(&m_font);
    GetCell(DICT_NAME_COL, row, &cell);
    cell.GetText(&cs);
    m_pNameEdit->SetWindowText(cs);
    m_aEditControl.SetAt(DICT_NAME_COL, (CWnd*) m_pNameEdit);
    if (!m_bAdding && !m_bInserting) {
        m_pDict->SetOldName(cs);
        if (row == 0) {
            m_pDict->SetChangedObject(m_pDict);
        }
        else {
            m_pDict->SetChangedObject(&m_pDict->GetLevel(row - LEVEL_ROW_OFFSET));
        }
    }

    // Set focus to field
    m_iEditCol = col;
    m_iMinCol = DICT_LABEL_COL;
    m_iMaxCol = DICT_NAME_COL;
    if (col == DICT_LABEL_COL) {
        m_pLabelEdit->SetFocus();
        if (vcKey > 0) {
            m_pLabelEdit->SendMessage(WM_CHAR, vcKey, 1);
        }
    }
    else {
        m_pNameEdit->SetFocus();
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::EditEnd
//
/////////////////////////////////////////////////////////////////////////////

bool CDictGrid::EditEnd(bool bSilent)
{
    bool bValid = true;
    bool bChanged = false;
    bool bChangedName = false;
    bool bUndo = false;
    CIMSAString csNewLabel, csOldLabel;
    CIMSAString csNewName, csOldName;
    long row = m_iEditRow;
    int iLevel = row - LEVEL_ROW_OFFSET;
    DictLevel* dict_level = nullptr;
    if (row == 0) {
        m_aEditControl[DICT_LABEL_COL]->GetWindowText(csNewLabel);
        csOldLabel = m_pDict->GetLabel();
        if (csNewLabel.Compare(csOldLabel) != 0) {
            bChanged = true;
            m_pDict->SetLabel(csNewLabel);
        }
        m_aEditControl[DICT_NAME_COL]->GetWindowText(csNewName);
        csOldName = m_pDict->GetName();
        if (csNewName.Compare(csOldName) != 0) {
            bChanged = true;
            bChangedName = true;
            m_pDict->SetName(csNewName);
        }
        if (bChanged) {
            CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
            DictionaryValidator* dictionary_validator = pDoc->GetDictionaryValidator();
            bValid = dictionary_validator->IsValid(m_pDict, bSilent);
            if (bValid) {
                QuickSetText(DICT_LABEL_COL, row, csNewLabel);
                QuickSetText(DICT_NAME_COL, row, csNewName);
                pDoc->SetModified();
            }
            else {
                m_pDict->SetLabel(csOldLabel);
                m_pDict->SetName(csOldName);
            }
        }
    }
    else {
        dict_level = &m_pDict->GetLevel(iLevel);
        m_aEditControl[DICT_LABEL_COL]->GetWindowText(csNewLabel);
        csOldLabel = dict_level->GetLabel();
        if (csNewLabel.Compare(csOldLabel) != 0) {
            bChanged = true;
            dict_level->SetLabel(csNewLabel);
        }
        m_aEditControl[DICT_NAME_COL]->GetWindowText(csNewName);
        csOldName = dict_level->GetName();
        if (csNewName.Compare(csOldName) != 0) {
            bChanged = true;
            bChangedName = true;
            dict_level->SetName(csNewName);
        }
        if (m_bAdding || m_bInserting) {
            if (dict_level->GetLabel().IsEmpty() && dict_level->GetName().IsEmpty()) {
                bUndo = true;
                m_bAdding = false;
                m_bInserting = false;
                bChanged = false;
                bValid = true;
            }
        }
        if (bChanged) {
            CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
            DictionaryValidator* dictionary_validator = pDoc->GetDictionaryValidator();
            bValid = dictionary_validator->IsValid(*dict_level, iLevel, bSilent);
            if (bValid) {
                QuickSetText(DICT_LABEL_COL, row, csNewLabel);
                QuickSetText(DICT_NAME_COL, row, csNewName);
            }
            else {
                dict_level->SetLabel(csOldLabel);
                dict_level->SetName(csOldName);
            }
        }
    }
    if (bValid) {
        EditQuit();
        if (m_bAdding || m_bInserting) {
            m_pDict->BuildNameList();
        }
        else if (iLevel >= 0) {
            m_pDict->UpdateNameList(*dict_level, iLevel);
        }
        CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
        CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
        pTreeCtrl->InvalidateRect(NULL);
        if (bChanged) {                            // BMD 17 Jul 2003
            pDoc->SetModified();
        }
    }
    else {
        DictionaryValidator* dictionary_validator = ((CDDDoc*) ((CView*) GetParent())->GetDocument())->GetDictionaryValidator();
        m_iEditCol = dictionary_validator->GetInvalidEdit();
        if (m_iEditCol < m_iMinCol || m_iEditCol > m_iMaxCol) {  // BMD 10 Mar 2003
            m_iEditCol = m_iMaxCol;
        }
        GotoRow(m_iEditRow);
        m_aEditControl[m_iEditCol]->SetFocus();
    }
    if (bUndo) {
        CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
        pDoc->UndoChange(FALSE);
    }
    if (bChangedName && bValid && !m_bAdding && !m_bInserting) {
        CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
        AfxGetMainWnd()->PostMessage(UWM::Dictionary::NameChange, (WPARAM)pDoc);
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::EditQuit
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::EditQuit()
{
    SetRedraw(FALSE);
    delete m_pLabelEdit;
    delete m_pNameEdit;
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


bool CDictGrid::IsEditingNameOrLabelAndCanUndo() const
{
    return ( ( m_iEditCol == DICT_LABEL_COL && assert_cast<CLabelEdit*>(m_aEditControl[DICT_LABEL_COL])->CanUndo() ) ||
             ( m_iEditCol == DICT_NAME_COL && assert_cast<CNameEdit*>(m_aEditControl[DICT_NAME_COL])->CanUndo() ) );
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::OnSetFocus
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::OnSetFocus(int /*section*/)
{
    CDDGView* pView = (CDDGView*) GetParent();
    CSplitterWnd* pSplitWnd = (CSplitterWnd*) pView->GetParent();
    pSplitWnd->SetActivePane(0,0);
    for (long row = 0 ; row < GetNumberRows() ; row++) {
        for (int col = DICT_LABEL_COL ; col < GetNumberCols() ; col++) {
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
//                        CDictGrid::OnKillFocus
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::OnKillFocus(int /*section*/)
{
    m_bCanEdit = false;

    int col;
    long row;
    if (EnumFirstSelected(&col, &row) != UG_SUCCESS) {
        return;
    }

    for (col = DICT_LABEL_COL ; col < GetNumberCols() ; col++) {
        QuickSetHTextColor(col, row, GetSysColor(COLOR_WINDOWTEXT));
        QuickSetHBackColor(col, row, GetSysColor(COLOR_BTNFACE));
    }
    RedrawRow(row);
    while (EnumNextSelected(&col, &row) == UG_SUCCESS) {
        for (col = DICT_LABEL_COL ; col < GetNumberCols() ; col++) {
            QuickSetHTextColor(col, row, GetSysColor(COLOR_WINDOWTEXT));
            QuickSetHBackColor(col, row, GetSysColor(COLOR_BTNFACE));
        }
        RedrawRow(row);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::OnEditUndo
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::OnEditUndo()
{
    ASSERT(m_bEditing);
    ASSERT(m_iEditCol == DICT_LABEL_COL || m_iEditCol == DICT_NAME_COL);
    ASSERT(assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->CanUndo());
    assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->Undo();
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::OnEditCut
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::OnEditCut()
{
    if (m_bEditing) {
        ASSERT(m_iEditCol == DICT_LABEL_COL || m_iEditCol == DICT_NAME_COL);
        assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->Cut();
        return;
    }
    EditCopy(true);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::OnEditCopy
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::OnEditCopy()
{
    if (m_bEditing) {
        ASSERT(m_iEditCol == DICT_LABEL_COL || m_iEditCol == DICT_NAME_COL);
        assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->Copy();
        return;
    }
    EditCopy(false);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::OnEditPaste
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::OnEditPaste()
{
    // If editing, paste to the control
    if( m_bEditing )
    {
        ASSERT(m_iEditCol == DICT_LABEL_COL || m_iEditCol == DICT_NAME_COL);
        assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->Paste();
        return;
    }

    // Can paste only levels
    long current_row = GetCurrentRow();
    long level_number;

    if( current_row == 0 )
    {
        if( m_pDict->GetNumLevels() > 0 )
            return;

        level_number = 0;
    }

    else
    {
        level_number = current_row - LEVEL_ROW_OFFSET;

        if( level_number == ( (long)m_pDict->GetNumLevels() - 1 ) )
        {
            BeforeAfterDlg dlg;

            if( dlg.DoModal() != IDOK )
                return;

            if( dlg.SelectedAfter() )
                ++level_number;
        }
    }

    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    DictPastedValues<DictLevel> pasted_levels = pDoc->GetDictClipboard().GetNamedElementsFromClipboard<DictLevel>(this);

    if( pasted_levels.values.empty() )
        return;

    // Push old dictionary on stack
    pDoc->PushUndo(*m_pDict, (int)m_pDict->GetNumLevels() - 1);

    // insert the levels
    for( DictLevel& dict_level : pasted_levels.values )
    {
        m_pDict->InsertLevel(level_number, std::move(dict_level));
        ++level_number;
    }

    // update the current language indices
    m_pDict->SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());

    // sync any linked value sets
    m_pDict->SyncLinkedValueSets(CDataDict::SyncLinkedValueSetsAction::OnPaste, &pasted_levels.value_set_names_added);

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
    pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node);

    // Update grid
    Update();
    GotoRow(current_row);
    SetFocus();
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::OnEditAdd
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::OnEditAdd()
{
    // If tree control has focus, return
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    if (GetFocus() == pTreeCtrl) {
        return;
    }

    m_bAdding = true;

    // Push old dictionary on stack
    pDoc->PushUndo(*m_pDict, (int)m_pDict->GetNumLevels() - 1);

    // Check if record type needs changing
    if (m_pDict->GetNumRecords() == 1 && m_pDict->GetRecTypeLen() == 0) {
        m_pDict->SetRecTypeStart(1);
        m_pDict->SetRecTypeLen(1);
    }

    // Add new level
    DictLevel dict_level;
    dict_level.GetLabelSet().SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());
    CDictRecord rec;
    rec.GetLabelSet().SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());
    rec.SetLabel(_T("New Record"));
    dict_level.AddRecord(&rec);
    int iLevel = (int)m_pDict->GetNumLevels();
    m_pDict->AddLevel(std::move(dict_level));

    // Check is dictionary still OK
    m_pDict->BuildNameList();
    pDoc->GetDictionaryValidator()->IsValid(m_pDict->GetLevel(iLevel).GetRecord(0), iLevel, 0, true, true);

    // Update tree
//    pTreeCtrl->SetUpdateAllViews(false);
    DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
    pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node);

    // Update grid
    Update();
    GotoRow(iLevel + LEVEL_ROW_OFFSET);
//    InvalidateRect(NULL);

    // Begin editing new level
    EditBegin(DICT_LABEL_COL, iLevel + LEVEL_ROW_OFFSET, 0);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::OnEditInsert
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::OnEditInsert()
{
    // If tree control has focus, return
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    if (GetFocus() == pTreeCtrl) {
        return;
    }

    int row = GetCurrentRow();
    if (row == 0) {
        return;
    }
    int iLevel = row - LEVEL_ROW_OFFSET;
    ASSERT(iLevel >= 0);
    m_bInserting = true;

    // Push old dictionary on stack
    pDoc->PushUndo(*m_pDict, iLevel);

    // Insert new level
    DictLevel dict_level;
    dict_level.GetLabelSet().SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());
    CDictRecord rec;
    rec.GetLabelSet().SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());
    rec.SetLabel(_T("New Record"));
    dict_level.AddRecord(&rec);
    m_pDict->InsertLevel(iLevel, std::move(dict_level));

    // Check is dictionary still OK
    m_pDict->BuildNameList();
    pDoc->GetDictionaryValidator()->IsValid(m_pDict->GetLevel(iLevel).GetRecord(0), iLevel, 0, true, true);

    // Update tree
//    pTreeCtrl->SetUpdateAllViews(false);
    DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
    pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node);

    // Update grid
    Update();
    GotoRow(row);
//    InvalidateRect(NULL);

    // Begin editing new level
    EditBegin(DICT_LABEL_COL, row, 0);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::OnEditDelete
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::OnEditDelete()
{
    // If editing, clear edit control
    if (m_bEditing) {
        ASSERT(m_iEditCol == DICT_LABEL_COL || m_iEditCol == DICT_NAME_COL);
        assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->Clear();
        return;
    }
    // If tree control has focus, return
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    if (GetFocus() == pTreeCtrl) {
        return;
    }
    EditDelete(GetSelectedLevels());
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::EditCopy
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::EditCopy(bool bCut)
{
    std::vector<size_t> selected_levels = GetSelectedLevels();

    // Copy levels to clipboard
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    std::vector<const DictLevel*> dict_levels;

    for( long selected_level : selected_levels )
        dict_levels.emplace_back(&m_pDict->GetLevel(selected_level));

    pDoc->GetDictClipboard().PutOnClipboard(this, dict_levels);

    // If cut, delete levels
    if( bCut )
        EditDelete(selected_levels);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::EditDelete
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::EditDelete(const std::vector<size_t>& selected_levels)
{
    ASSERT(!selected_levels.empty());

    // Push old dictionary on stack
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    pDoc->PushUndo(*m_pDict, selected_levels.front());

    // Delete levels
    for( auto level_itr = selected_levels.crbegin(); level_itr != selected_levels.crend(); ++level_itr )
        m_pDict->RemoveLevel(*level_itr);

    pDoc->SetModified();

    if( m_pDict->IsPosRelative() )
        pDoc->GetDictionaryValidator()->AdjustStartPositions();

    // Check is dictionary still OK
    m_pDict->BuildNameList();
    pDoc->GetDictionaryValidator()->IsValid(m_pDict, true, true);

    // Set position
    int level_to_select = std::min((int)selected_levels.front(), (int)m_pDict->GetNumLevels() - 1);
    int row_to_select = ( level_to_select < 0 ) ? 0 : ( level_to_select + LEVEL_ROW_OFFSET );

    // Update tree
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
    pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node);

    // Update grid
    Update();
    ClearSelections();
    GotoCell(1, row_to_select);
    InvalidateRect(NULL);
    SetFocus();
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::OnEditModify
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::OnEditModify()
{
    UINT vcKey = VK_RETURN;
    BOOL bProcessed = FALSE;
    // If tree control has focus, position to item on record grid and post delete message
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    if (GetFocus() == pTreeCtrl) {
        return;
    }
    OnCharDown(&vcKey, bProcessed);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::OnEditNotes
//
/////////////////////////////////////////////////////////////////////////////

void CDictGrid::OnEditNotes()
{
    long row = GetCurrentRow();
    CIMSAString csTitle, csLabel, csNote;
    csTitle.LoadString(IDS_NOTE_TITLE);
    CNoteDlg dlgNote;
    if (row == 0) {
        csLabel = m_pDict->GetLabel().Left(32);
        if (m_pDict->GetLabel().GetLength() > 32)  {
            csLabel += _T("...");
        }
        csTitle = _T("Dictionary: ") + csLabel + csTitle;
        csNote = m_pDict->GetNote();
        dlgNote.SetTitle(csTitle);
        dlgNote.SetNote(csNote);
        if (dlgNote.DoModal() == IDOK)  {
            if (csNote != dlgNote.GetNote()) {
                CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
                pDoc->PushUndo(*m_pDict);
                m_pDict->SetNote(dlgNote.GetNote());
                pDoc->SetModified();
            }
        }
    }
    else {
        int iLevel = row - LEVEL_ROW_OFFSET;
        DictLevel& dict_level = m_pDict->GetLevel(iLevel);
        csLabel = dict_level.GetLabel().Left(32);
        if (dict_level.GetLabel().GetLength() > 32)  {
            csLabel += _T("...");
        }
        csTitle = _T("Level: ") + csLabel + csTitle;
        csNote = dict_level.GetNote();
        dlgNote.SetTitle(csTitle);
        dlgNote.SetNote(csNote);
        if (dlgNote.DoModal() == IDOK)  {
            if (csNote != dlgNote.GetNote()) {
                CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
                pDoc->PushUndo(m_pDict->GetLevel(row - LEVEL_ROW_OFFSET), row - LEVEL_ROW_OFFSET);
                dict_level.SetNote(dlgNote.GetNote());
                pDoc->SetModified();
            }
        }
    }
    Update();
    GotoCell(DICT_LABEL_COL, row);
    SetFocus();
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDictGrid::OnShiftF10
//
/////////////////////////////////////////////////////////////////////////////
void CDictGrid::OnShiftF10()
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

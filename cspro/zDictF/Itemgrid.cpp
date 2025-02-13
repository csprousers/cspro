//***************************************************************************
//  File name: ItemGrid.cpp
//
//  Description:
//       Data Dictionary item properties grid implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#include "StdAfx.h"
#include "Itemgrid.h"
#include "GenerateVSDlg.h"
#include "VSLabelRenamerDlg.h"
#include <zUtilO/ArrUtil.h>
#include <zDictO/ValueSetFixer.h>
#include <sstream>


namespace
{
    constexpr int ITEM_NOTE_COL     = 0;
    constexpr int ITEM_SETLABEL_COL = 1;
    constexpr int ITEM_SETNAME_COL  = 2;
    constexpr int ITEM_LABEL_COL    = 3;
    constexpr int ITEM_FROM_COL     = 4;
    constexpr int ITEM_TO_COL       = 5;
    constexpr int ITEM_SPECIAL_COL  = 6;

    constexpr int ITEM_NUM_COLS     = ITEM_SPECIAL_COL + 1;

    constexpr TCHAR SHOWBLANK       = '\x7F';
}


BEGIN_MESSAGE_MAP(CItemGrid, CDDGrid)
    ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
    ON_COMMAND(ID_EDIT_CUT, OnEditCut)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
    ON_COMMAND(ID_EDIT_ADD, OnEditAdd)
    ON_COMMAND(ID_EDIT_INSERT, OnEditInsert)
    ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
    ON_COMMAND(ID_EDIT_MODIFY, OnEditModify)
    ON_COMMAND(ID_EDIT_NOTES, OnEditNotes)
    ON_COMMAND(ID_SHIFT_F10, OnShiftF10)
    ON_COMMAND(ID_EDIT_GEN_VALUE_SET, OnEditGenValueSet)
    ON_COMMAND(ID_PASTE_VS_LINK, OnPasteValueSetLink)
    ON_COMMAND_RANGE(ID_REMOVE_VS_LINK, ID_REMOVE_VS_ALL_LINKS, OnRemoveValueSetLink)
    ON_COMMAND(ID_MERGE_VS_VALUES, OnMergeValues)
    ON_COMMAND_RANGE(ID_VS_CASE_FORMAT, ID_VS_CASE_MIXED_ALL_WORDS, OnFormatValueLabels)
    ON_COMMAND(ID_VS_REPLACE_VALUE_LABELS, OnValuesReplaceValueLabels)
    ON_COMMAND(ID_VS_MAKE_FIRST_VS, OnMakeFirstValueSet)
END_MESSAGE_MAP()


CItemGrid::CItemGrid()
    :   m_iLevel(0),
        m_iRec(0),
        m_iItem(0),
        m_iVSet(0),
        m_pSetLabelEdit(nullptr),
        m_pSetNameEdit(nullptr),
        m_pLabelEdit(nullptr),
        m_pFromEdit(nullptr),
        m_pToEdit(nullptr),
        m_pSpecialEdit(nullptr)
{
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::OnSetup
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::OnSetup()
{
    EnableExcelBorders(FALSE);
    VScrollAlwaysPresent(TRUE);
    SetNumberCols(ITEM_NUM_COLS, FALSE);
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

    set_header(ITEM_NOTE_COL,     _T("N"));
    set_header(ITEM_SETLABEL_COL, _T("Value Set Label"), UG_ALIGNLEFT);
    set_header(ITEM_SETNAME_COL,  _T("Value Set Name"),  UG_ALIGNLEFT);
    set_header(ITEM_LABEL_COL,    _T("Value Label"),     UG_ALIGNLEFT);
    set_header(ITEM_FROM_COL,     _T("From"),            UG_ALIGNRIGHT);
    set_header(ITEM_TO_COL,       _T("To"),              UG_ALIGNRIGHT);
    set_header(ITEM_SPECIAL_COL,  _T("Special     "),    UG_ALIGNLEFT);

    m_bAdding = false;
    m_bInserting = false;
    m_bEditing = false;
    GotoCell(ITEM_SETLABEL_COL, 0);                         // 15 Jan 2002
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::Size
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::Size(CRect rect)
{
    CIMSAString csWidths = AfxGetApp()->GetProfileString(_T("Data Dictionary"), _T("ItemGridWidths"), _T("-1"));
    std::vector<std::wstring> aWidths = SO::SplitString(csWidths, ',');
        
    if (aWidths.size() < ITEM_NUM_COLS) {
        CUGCell cell;
        CUGCellType* pCellType;
        CSize size;

        SetColWidth(ITEM_NOTE_COL, (int) (NOTE_WIDTH * (GetDesignerFontZoomLevel() / 100.0)));
        int iUsed = (int) (NOTE_WIDTH * (GetDesignerFontZoomLevel() / 100.0));

        for (int col = ITEM_SPECIAL_COL; col > ITEM_SETLABEL_COL ; col--) {
            if (col == ITEM_LABEL_COL) {
                continue;
            }

            if (col == ITEM_FROM_COL || col == ITEM_TO_COL) {
                GetCell(ITEM_SETNAME_COL, HEADER_ROW, &cell);
                pCellType = GetCellType(HEADER_ROW, ITEM_SETNAME_COL);
                pCellType->GetBestSize(GetDC(), &size, &cell);
            }
            else {
                GetCell(col, HEADER_ROW, &cell);
                pCellType = GetCellType(HEADER_ROW, col);
                pCellType->GetBestSize(GetDC(), &size, &cell);
            }
            SetColWidth(col, size.cx + BORDER_WIDTH);
            iUsed += size.cx + BORDER_WIDTH;
        }
        GetCell(ITEM_SETLABEL_COL, HEADER_ROW, &cell);
        pCellType = GetCellType(HEADER_ROW, ITEM_SETLABEL_COL);
        pCellType->GetBestSize(GetDC(), &size, &cell);
        if (2 * size.cx > rect.Width() - m_GI->m_vScrollWidth - iUsed - 1) {
            SetColWidth(ITEM_SETLABEL_COL, size.cx + BORDER_WIDTH);
            SetColWidth(ITEM_LABEL_COL,    size.cx + BORDER_WIDTH);
        }
        else {
            SetColWidth(ITEM_SETLABEL_COL, (rect.Width() - m_GI->m_vScrollWidth - iUsed)/2);
            SetColWidth(ITEM_LABEL_COL,    (rect.Width() - m_GI->m_vScrollWidth - iUsed)/2);
        }

        csWidths.Format(_T("%d,%d,%d,%d,%d,%d,%d"), GetColWidth(ITEM_NOTE_COL),
                                                    GetColWidth(ITEM_SETLABEL_COL),
                                                    GetColWidth(ITEM_SETNAME_COL),
                                                    GetColWidth(ITEM_LABEL_COL),
                                                    GetColWidth(ITEM_FROM_COL),
                                                    GetColWidth(ITEM_TO_COL),
                                                    GetColWidth(ITEM_SPECIAL_COL));
        AfxGetApp()->WriteProfileString(_T("Data Dictionary"), _T("ItemGridWidths"), csWidths);
    }
    else {
        CIMSAString csWidth = csWidths.GetToken();
        SetColWidth(ITEM_NOTE_COL, (int)csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(ITEM_SETLABEL_COL, (int)csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(ITEM_SETNAME_COL, (int)csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(ITEM_LABEL_COL, (int)csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(ITEM_FROM_COL, (int)csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(ITEM_TO_COL, (int)csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(ITEM_SPECIAL_COL, (int)csWidth.Val());
    }

    Resize(rect);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::Resize
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::Resize(CRect rect)
{
    MoveWindow(&rect, FALSE);
    if (m_aEditControl.GetSize() > 0) {
        for (int i = m_iMinCol ; i <= m_iMaxCol ; i++) {
            m_aEditControl[i]->PostMessage(WM_SIZE);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::OnColSized
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::OnColSized(int col,int* /*width*/)
{
    if (m_aEditControl.GetSize() > 0) {
        for (int i = m_iMinCol ; i <= m_iMaxCol ; i++) {
            if (col == ITEM_SPECIAL_COL && col == i) {
                CIMSAString csText;
                m_pSpecialEdit->GetWindowText(csText);
                long row = GetCurrentRow();
                delete m_pSpecialEdit;
                m_pSpecialEdit = new CDDComboBox();
                CRect rect;
                GetCellRect(ITEM_SPECIAL_COL, row, &rect);
                rect.top--;
                rect.left--;
                rect.bottom += 6*m_plf->lfHeight;
                rect.right += 2;
                m_pSpecialEdit->SetRowCol(row, ITEM_SPECIAL_COL);
                m_pSpecialEdit->Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, rect, this, 104);
                m_pSpecialEdit->SetFont(&m_font);
                m_pSpecialEdit->SetItemHeight (-1, m_plf->lfHeight);   // sets height for static control and button
                m_pSpecialEdit->SetItemHeight ( 0, m_plf->lfHeight);   // sets height for list box entries
                m_pSpecialEdit->AddString(_T(""));
                m_pSpecialEdit->AddString(_T("Missing"));
                m_pSpecialEdit->AddString(_T("Refused"));
                m_pSpecialEdit->AddString(_T("NotAppl"));
                m_pSpecialEdit->AddString(_T("Default"));
                if (csText == _T("")) {
                    m_pSpecialEdit->SetCurSel(0);
                }
                else if (csText == _T("Missing")) {
                    m_pSpecialEdit->SetCurSel(1);
                }
                else if (csText == _T("Refused")) {
                    m_pSpecialEdit->SetCurSel(2);
                }
                else if (csText == _T("NotAppl")) {
                    m_pSpecialEdit->SetCurSel(3);
                }
                else {
                    m_pSpecialEdit->SetCurSel(4);
                }
                m_aEditControl.SetAt(ITEM_SPECIAL_COL, (CWnd*) m_pSpecialEdit);
            }
            else {
                m_aEditControl[i]->SendMessage(WM_SIZE);
            }
            m_aEditControl[i]->SendMessage(WM_PAINT);
        }
    }
    CString csWidths;
    csWidths.Format(_T("%d,%d,%d,%d,%d,%d,%d"), GetColWidth(ITEM_NOTE_COL),
                                                GetColWidth(ITEM_SETLABEL_COL),
                                                GetColWidth(ITEM_SETNAME_COL),
                                                GetColWidth(ITEM_LABEL_COL),
                                                GetColWidth(ITEM_FROM_COL),
                                                GetColWidth(ITEM_TO_COL),
                                                GetColWidth(ITEM_SPECIAL_COL));
    AfxGetApp()->WriteProfileString(_T("Data Dictionary"), _T("ItemGridWidths"), csWidths);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::Update
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::Update(CDataDict* pDict, int level, int rec, int item, int vset)
{
    ASSERT(level >= 0 && item >= 0);
    m_pDict = pDict;
    m_iLevel = level;

    SetNumberCols(ITEM_NUM_COLS, FALSE);

    if (m_iLevel >= (int)m_pDict->GetNumLevels()) {          // BMD 30 Jul 2003
        m_iLevel = (int)m_pDict->GetNumLevels() - 1;
    }
    const DictLevel& dict_level = m_pDict->GetLevel(m_iLevel);
    m_iRec = rec;
    if (m_iRec >= dict_level.GetNumRecords()) {
        m_iRec = dict_level.GetNumRecords() - 1;
    }
    const CDictRecord* pRec = dict_level.GetRecord(m_iRec);
    m_iItem = item;
    if (m_iItem >= pRec->GetNumItems()) {
        m_iItem = pRec->GetNumItems() - 1;
    }
    const CDictItem* pItem = pRec->GetItem(m_iItem);
    m_iVSet = vset;
    if (m_iVSet >= (int)pItem->GetNumValueSets()) {
        m_iVSet = pItem->GetNumValueSets() - 1;
    }
    Update();
}


void CItemGrid::Update()
{
    int iGotoRow = 0;
    m_aValue.RemoveAll();
    VALUEKEY key;
    CDictItem* pItem = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec)->GetItem(m_iItem);
    if (!DictionaryRules::CanHaveValueSet(*pItem)) {
        // No vsets for binary items
        EnableWindow(FALSE);
        SetNumberRows(0, FALSE);
        return;
    }
    else {
        EnableWindow(TRUE);

        if (IsString(pItem->GetContentType())) {
            QuickSetAlignment(ITEM_FROM_COL,     HEADER_ROW, UG_ALIGNLEFT);
            QuickSetAlignment(ITEM_TO_COL  ,     HEADER_ROW, UG_ALIGNLEFT);
        }
        else if (IsNumeric(pItem->GetContentType())) {
            QuickSetAlignment(ITEM_FROM_COL,     HEADER_ROW, UG_ALIGNRIGHT);
            QuickSetAlignment(ITEM_TO_COL  ,     HEADER_ROW, UG_ALIGNRIGHT);
        }

        // Calculate number of rows
        int rows = 0;
        for( const DictValueSet& dict_value_set : pItem->GetValueSets() ) {
            rows++;
            for( const DictValue& dict_value : dict_value_set.GetValues() ) {
                rows += dict_value.GetNumValuePairs();
            }
        }
        SetNumberRows(rows, FALSE);

        int ir = 0;
        for (int vs = 0 ; vs < (int)pItem->GetNumValueSets() ; vs++) {
            // Set value set row
            if (vs == m_iVSet) {
                iGotoRow = ir;
            }
            key.vset = vs;
            key.value = NONE;
            key.vpair = NONE;
            DictValueSet& dict_value_set = pItem->GetValueSet(vs);
            QuickSetCellType  (ITEM_NOTE_COL,     ir, m_iButton);
            QuickSetCellTypeEx(ITEM_NOTE_COL,     ir, UGCT_BUTTONNOFOCUS);
            QuickSetBackColor (ITEM_NOTE_COL,     ir, GetSysColor(COLOR_BTNFACE));
            QuickSetHBackColor(ITEM_NOTE_COL,     ir, GetSysColor(COLOR_BTNFACE));
            if (dict_value_set.GetNote().GetLength() > 0) {
                QuickSetBitmap(ITEM_NOTE_COL,     ir, m_pNoteYes);
            }
            else {
                QuickSetBitmap(ITEM_NOTE_COL,     ir, m_pNoteNo);
            }
            QuickSetText      (ITEM_SETLABEL_COL, ir, dict_value_set.GetLabel());
            QuickSetText      (ITEM_SETNAME_COL,  ir, dict_value_set.GetName());

            size_t value_set_links = m_pDict->CountValueSetLinks(dict_value_set);
            bool is_real_linked_value_set = ( value_set_links >= 2 );
            COLORREF linkedVSColor = is_real_linked_value_set ? RGB(255,200,200) : GetSysColor(COLOR_WINDOW);

            QuickSetText(ITEM_LABEL_COL, ir, is_real_linked_value_set ? FormatText(_T("<Linked Value Set: %d Links>"), (int)value_set_links) : _T(""));

            // 20110120 for linked value sets
            QuickSetBackColor(ITEM_SETLABEL_COL, ir, linkedVSColor);
            QuickSetBackColor(ITEM_SETNAME_COL, ir, linkedVSColor);
            QuickSetBackColor(ITEM_LABEL_COL, ir, linkedVSColor);
            QuickSetBackColor(ITEM_FROM_COL, ir, linkedVSColor);
            QuickSetBackColor(ITEM_TO_COL, ir, linkedVSColor);
            QuickSetBackColor(ITEM_SPECIAL_COL, ir, linkedVSColor);

            QuickSetText      (ITEM_FROM_COL,     ir, _T(""));
            QuickSetText      (ITEM_TO_COL,       ir, _T(""));
            QuickSetText      (ITEM_SPECIAL_COL,  ir, _T(""));
            m_aValue.Add(key);
            ir++;
            int v = 0;
            for( const DictValue& dict_value : dict_value_set.GetValues() ) {
                // Set value row
                key.value = v;

                QuickSetCellType  (ITEM_NOTE_COL,     ir, m_iButton);
                QuickSetCellTypeEx(ITEM_NOTE_COL,     ir, UGCT_BUTTONNOFOCUS);
                QuickSetBackColor (ITEM_NOTE_COL,     ir, GetSysColor(COLOR_BTNFACE));
                QuickSetHTextColor(ITEM_NOTE_COL,     ir, GetSysColor(COLOR_WINDOWTEXT));
                QuickSetHBackColor(ITEM_NOTE_COL,     ir, GetSysColor(COLOR_BTNFACE));
                if (dict_value.GetNote().GetLength() > 0) {
                    QuickSetBitmap(ITEM_NOTE_COL,     ir, m_pNoteYes);
                }
                else {
                    QuickSetBitmap(ITEM_NOTE_COL,     ir, m_pNoteNo);
                }
                QuickSetText      (ITEM_SETLABEL_COL, ir, _T(""));
                QuickSetText      (ITEM_SETNAME_COL,  ir, _T(""));
                QuickSetText      (ITEM_LABEL_COL,    ir, dict_value.GetLabel());
                QuickSetTextColor (ITEM_LABEL_COL,    ir, dict_value.GetTextColor().ToCOLORREF());

                QuickSetText(ITEM_SPECIAL_COL, ir, dict_value.IsSpecial() ? SpecialValues::ValueToString(dict_value.GetSpecialValue(), false) : _T(""));

                int p = 0;
                for( const DictValuePair& dict_value_pair : dict_value.GetValuePairs() ) {
                    // Set value pair row
                    key.vpair = p;
                    if (p == 0) {
                        QuickSetHTextColor(ITEM_NOTE_COL,     ir, GetSysColor(COLOR_WINDOWTEXT));
                        QuickSetHBackColor(ITEM_NOTE_COL,     ir, GetSysColor(COLOR_BTNFACE));
                    }
                    else {
                        QuickSetCellType  (ITEM_NOTE_COL,  ir, UGCT_NORMAL);
                        QuickSetBitmap    (ITEM_NOTE_COL,  ir, NULL);
                        QuickSetBackColor (ITEM_NOTE_COL,  ir, GetSysColor(COLOR_WINDOW));
                        QuickSetHBackColor(ITEM_NOTE_COL,  ir, GetSysColor(COLOR_HIGHLIGHT));
                        QuickSetText  (ITEM_SETLABEL_COL,  ir, _T(""));
                        QuickSetText  (ITEM_SETNAME_COL,   ir, _T(""));
                        QuickSetText  (ITEM_LABEL_COL,     ir, _T(" "));
                    }
                    if (pItem->GetContentType() == ContentType::Alpha) {
                        QuickSetAlignment (ITEM_FROM_COL,     ir, UG_ALIGNLEFT);
                    }
                    else if (pItem->GetContentType() == ContentType::Numeric) {
                        QuickSetAlignment (ITEM_FROM_COL,     ir, UG_ALIGNRIGHT);
                    }
                    else {
                        ASSERT(FALSE); // should not have value sets for binary items
                    }

                    // 20110120 for linked value sets
                    QuickSetBackColor(ITEM_SETLABEL_COL, ir, linkedVSColor);
                    QuickSetBackColor(ITEM_SETNAME_COL, ir, linkedVSColor);
                    QuickSetBackColor(ITEM_LABEL_COL, ir, linkedVSColor);
                    QuickSetBackColor(ITEM_FROM_COL, ir, linkedVSColor);
                    QuickSetBackColor(ITEM_TO_COL, ir, linkedVSColor);
                    QuickSetBackColor(ITEM_SPECIAL_COL, ir, linkedVSColor);

                    CString csTemp = dict_value_pair.GetFrom();
                    csTemp.Replace(SPACE, SHOWBLANK);
                    QuickSetText      (ITEM_FROM_COL,     ir, csTemp);
                    if (pItem->GetContentType() == ContentType::Alpha) {
                        QuickSetAlignment (ITEM_TO_COL,     ir, UG_ALIGNLEFT);
                    }
                    else if (pItem->GetContentType() == ContentType::Numeric) {
                        QuickSetAlignment (ITEM_TO_COL,     ir, UG_ALIGNRIGHT);
                    }
                    else {
                        ASSERT(FALSE); // no value sets for binary items
                    }
                    csTemp = dict_value_pair.GetTo();
                    csTemp.Replace(SPACE, SHOWBLANK);
                    QuickSetText      (ITEM_TO_COL,       ir, csTemp);
                    if (p > 0) {
                        QuickSetText  (ITEM_SPECIAL_COL,  ir, _T(""));
                    }
                    m_aValue.Add(key);
                    ir++;
                    p++;
                }

                v++;
            }
        }

        // Set colors
        for (ir = 0 ; ir < GetNumberRows() ; ir++) {
            CUGCell cell;
            for (int col = ITEM_SETLABEL_COL ; col < GetNumberCols() ; col++) {
                GetCell(col, ir, &cell);
                if (ir == 0) {
                    cell.SetHTextColor(GetSysColor(COLOR_WINDOWTEXT));
                    cell.SetHBackColor(GetSysColor(COLOR_BTNFACE));
                }
                else {
                    cell.SetHTextColor(GetSysColor(COLOR_HIGHLIGHTTEXT));
                    cell.SetHBackColor(GetSysColor(COLOR_HIGHLIGHT));
                }
                SetCell(col, ir, &cell);
            }
        }
        GotoRow(iGotoRow);
        if (iGotoRow != GetCurrentRow()) {
            GotoCell(ITEM_SETLABEL_COL,iGotoRow);       // Klug because of GotoRow problem
        }
    }
    CView* pView = assert_cast<CView*>(GetParent());
    CDDDoc* pDoc = assert_cast<CDDDoc*>(pView->GetDocument());
    pDoc->SetLevel(m_iLevel);
    pDoc->SetRec(m_iRec);
    pDoc->SetItem(m_iItem);
    RedrawWindow();
    //force on row change call to fix the property grid refresh when grids change
    OnRowChange(0, 0);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::OnCanMove
//
/////////////////////////////////////////////////////////////////////////////

int CItemGrid::OnCanMove(int /*oldcol*/, long /*oldrow*/, int /*newcol*/, long /*newrow*/)
{
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::OnLClicked
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::OnLClicked(int col, long row, int updn, RECT* /*rect*/, POINT* /*point*/, int /*processed*/)
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
        if (m_bCanEdit) {
            if (m_aValue[row].value == NONE) {
                if (col <= ITEM_SETNAME_COL) {
                    m_iMinCol = ITEM_SETLABEL_COL;
                    m_iMaxCol = ITEM_SETNAME_COL;
                    ClearSelections();
                    GotoRow(row);
                    EditBegin(col, row, 0);
                }
            }
            else {
                if (col > ITEM_SETNAME_COL) {
                    m_iMinCol = ITEM_LABEL_COL;
                    m_iMaxCol = ITEM_SPECIAL_COL;
                    ClearSelections();
                    GotoRow(row);
                    EditBegin(col, row, 0);
                }
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
//                        CItemGrid::OnTH_RClicked
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::OnTH_RClicked(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed/* = 0*/)
{
    OnRClicked(col, row, updn, rect, point, processed);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::OnCB_RClicked
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::OnCB_RClicked(int updn, RECT* rect, POINT* point, int processed)
{
    OnRClicked(-1, -1, updn, rect, point, processed);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::OnRClicked
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::OnRClicked(int /*col*/, long row, int updn, RECT* /*rect*/, POINT* point, int /*processed*/)
{
    if (updn) {
        if (IsEditing()) {
            m_bAdding = false;
            m_bInserting = false;
            EditChange(VK_CANCEL);
        }
        return;
    }

    if( GetNumberRows() == 0 ) // 20110811
        row = -1; // a crash would occur if a value set didn't already exist

    long current_row = std::min(GetCurrentRow(), GetNumberRows() - 1);

    std::vector<long> selected_rows = GetSelectedRows();

    // 20110901 see if only values from the same value set are selected
    std::optional<int> selected_vset;
    std::optional<bool> only_values_selected;

    for( long selected_row : selected_rows )
    {
        if( GetValue(selected_row) == NONE )
        {
            only_values_selected = false;
            break;
        }

        else
        {
            int this_vset = m_aValue[selected_row].vset;

            if( !selected_vset.has_value() ) // we haven't yet said which value set we're using
            {
                selected_vset = this_vset;
            }

            else if( *selected_vset != this_vset ) // this only works on values from the same value set
            {
                only_values_selected = false;
                break;
            }
        }
    }

    if( !only_values_selected.has_value() )
    {
        // the options related to this require at least two values
        only_values_selected = ( selected_rows.size() >= 2 );
    }


    BCMenu popMenu;    // BMD 29 Sep 2003
    popMenu.CreatePopupMenu();

    bool has_rows_selected = ( GetNumberRows() > 0 );
    popMenu.AppendMenuItems(has_rows_selected, { { ID_EDIT_CUT, _T("Cu&t\tCtrl+X") },
                                                 { ID_EDIT_COPY, _T("&Copy\tCtrl+C") } });

    CView* pView = assert_cast<CView*>(GetParent());
    CDDDoc* pDoc = assert_cast<CDDDoc*>(pView->GetDocument());

    bool can_paste = IsClipboardValidForValueSetPaste(*pDoc);
    popMenu.AppendMenuItems(can_paste, { { ID_EDIT_PASTE, _T("&Paste\tCtrl+V") } });

    popMenu.AppendMenu(MF_SEPARATOR);

    CDictItem* pItem = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec)->GetItem(m_iItem);

    if( can_paste && pDoc->GetDictClipboard().IsAvailable<DictValueSet>() ) // 20110118
        popMenu.AppendMenu(MF_STRING, ID_PASTE_VS_LINK, _T("Paste Value Set Link\tCtrl+Alt+V"));

    bool value_set_header_selected = ( !selected_rows.empty() && GetValue(selected_rows.front()) == NONE );

    if( value_set_header_selected )
    {
        const DictValueSet& dict_value_set = pItem->GetValueSet(m_aValue[selected_rows.front()].vset);
        size_t value_set_links = m_pDict->CountValueSetLinks(dict_value_set);

        if( value_set_links >= 2 ) // 20110120
        {
            popMenu.AppendMenu(MF_STRING, ID_REMOVE_VS_LINK, _T("Remove Value Set Link"));

            if( value_set_links > 2 ) // 20110121
                popMenu.AppendMenu(MF_STRING, ID_REMOVE_VS_ALL_LINKS, FormatText(_T("Remove All Value Set's %d Links"), (int)value_set_links));
        }

        // i'm not sure when formatTextMenu gets deleted, but it had to be created with new or else the program would crash
        CMenu* formatTextMenu = new CMenu;
        formatTextMenu->CreatePopupMenu();

        BCMenu::AppendMenuItems(*formatTextMenu, { { ID_VS_CASE_FORMAT, _T("Formatted String") },
                                                   { ID_VS_CASE_UPPER, _T("All Upper Case") },
                                                   { ID_VS_CASE_LOWER, _T("All Lower Case") },
                                                   { ID_VS_CASE_MIXED_FIRST_WORD, _T("First Letter Upper Case") },
                                                   { ID_VS_CASE_MIXED_ALL_WORDS, _T("First Letter of Each Word Upper Case") } });

        popMenu.AppendMenu(MF_POPUP, (UINT)formatTextMenu->GetSafeHmenu(), _T("Format Value Pair Labels"));

        // for replacing only value labels
        popMenu.AppendMenuItems(IsClipboardFormatAvailable(_tCF_TEXT), { { ID_VS_REPLACE_VALUE_LABELS, _T("Replace Value Labels (Paste from Clipboard)") } });

        popMenu.AppendMenu(MF_SEPARATOR);

        bool first_value_set_selected = ( &dict_value_set == &pItem->GetValueSet(0) );
        popMenu.AppendMenuItems(!first_value_set_selected, { { ID_VS_MAKE_FIRST_VS, _T("Make Primary Value Set") } });
    }

    if( *only_values_selected ) // 20110901
        popMenu.AppendMenu(MF_STRING, ID_MERGE_VS_VALUES, _T("M&erge Value Pairs"));

    popMenu.AppendMenuItems(IsNumeric(*pItem), { { ID_EDIT_GEN_VALUE_SET, _T("&Generate Value Set\tCtrl+G") } });

    popMenu.AppendMenu(MF_SEPARATOR);

    if( !has_rows_selected || !value_set_header_selected )
    {
        popMenu.AppendMenuItems(has_rows_selected, { { ID_EDIT_MODIFY, _T("&Modify Value\tCtrl+M") } });
    }

    else
    {
        popMenu.AppendMenuItems({ { ID_EDIT_MODIFY, _T("&Modify Value Set\tCtrl+M") } });
    }

    if( !has_rows_selected )
    {
        popMenu.AppendMenuItems({ { ID_EDIT_ADD, _T("&Add Value Set\tCtrl+A") } });
        popMenu.AppendMenuItems(false, { { ID_EDIT_INSERT, _T("&Insert Value\tIns") },
                                         { ID_EDIT_DELETE, _T("&Delete Value\tDel") } });
    }

    else
    {
        bool add_value_set = ( value_set_header_selected && ( ( current_row + 1 ) != GetNumberRows() ) &&
                                                            ( GetValue(current_row + 1) != NONE ) );
        popMenu.AppendMenuItems({ { ID_EDIT_ADD, add_value_set ? _T("&Add Value Set\tCtrl+A") :
                                                                 _T("&Add Value\tCtrl+A") } });

        popMenu.AppendMenuItems({ { ID_EDIT_INSERT, ( current_row == 0 || value_set_header_selected ) ? _T("&Insert Value Set\tIns") :
                                                                                                        _T("&Insert Value\tIns") } });

        popMenu.AppendMenuItems({ { ID_EDIT_DELETE, value_set_header_selected ? _T("&Delete Value Set\tDel") :
                                                                                _T("&Delete Value\tDel") } });
    }

    popMenu.AppendMenu(MF_SEPARATOR);

    long currow = GetCurrentRow();
    bool enable_notes = ( row >= 0 && GetVPair(currow) == 0 );
    popMenu.AppendMenuItems(enable_notes, { { ID_EDIT_NOTES, _T("&Notes...\tCtrl+D") } });

    popMenu.LoadToolbar(IDR_DICT_FRAME);   // BMD 29 Sep 2003

    CRect rectWin;
    GetWindowRect(rectWin);
    if (point->x == 0) {
        CRect rectCell;
        GetCellRect(1, current_row, &rectCell);
        point->x = (rectCell.left + rectCell.right) / 2;
        point->y = (rectCell.top + rectCell.bottom) / 2;
    }
    popMenu.TrackPopupMenu(TPM_RIGHTBUTTON, rectWin.left + point->x, rectWin.top + point->y + GetRowHeight(row), this);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::OnKeyDown
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::OnKeyDown(UINT* vcKey, BOOL /*processed*/)
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
                if (newrow < 1) {
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
//                        CItemGrid::OnCharDown
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::OnCharDown(UINT* vcKey, BOOL /*processed*/)
{
    if (*vcKey==VK_TAB) {
        AfxGetMainWnd()->SendMessage(WM_IMSA_SETFOCUS);
        return;
    }
    if (*vcKey == VK_ESCAPE) {
        CDDGView* pView = (CDDGView*) GetParent();
        CDDDoc* pDoc = assert_cast<CDDDoc*>(pView->GetDocument());
        CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
        DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
        pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, m_iLevel, m_iRec);

        pView->m_iGrid = DictionaryGrid::Record;
        pView->m_gridRecord.SetRedraw(FALSE);
        pView->m_gridRecord.Update(pDoc->GetDict(), m_iLevel, m_iRec);
        pView->ResizeGrid();
        pView->m_gridRecord.ClearSelections();
        pView->m_gridRecord.GotoRow(pView->m_gridRecord.GetRow(m_iItem));
        pView->m_gridRecord.SetRedraw(TRUE);
        pView->m_gridRecord.InvalidateRect(NULL);
        pView->m_gridRecord.SetFocus();
        return;
    }
    if (GetNumberRows() == 0) {
        return;
    }
    CUGCell cell;
    GetCell(ITEM_SETNAME_COL, GetCurrentRow(), &cell);
    CIMSAString text;
    cell.GetText(&text);
    if (text.GetLength() != 0) {
        m_iMinCol = ITEM_SETLABEL_COL;
        m_iMaxCol = ITEM_SETNAME_COL;
        if (*vcKey >= 32) {
            EditBegin(ITEM_SETLABEL_COL, GetCurrentRow(), *vcKey);
        }
        else if (*vcKey == VK_RETURN) {
            EditBegin(ITEM_SETLABEL_COL, GetCurrentRow(), 0);
        }
    }
    else {
        m_iMinCol = ITEM_LABEL_COL;
        m_iMaxCol = ITEM_SPECIAL_COL;
        GetCell(ITEM_LABEL_COL, GetCurrentRow(), &cell);
        cell.GetText(&text);
        if (*vcKey >= 32) {
            if (text.GetLength() == 1 && SO::IsBlank(text)) {
                EditBegin(ITEM_FROM_COL, GetCurrentRow(), *vcKey);
            }
            else {
                EditBegin(ITEM_LABEL_COL, GetCurrentRow(), *vcKey);
            }
        }
        else if (*vcKey == VK_RETURN) {
            if (text.GetLength() == 1 && SO::IsBlank(text)) {
                EditBegin(ITEM_FROM_COL, GetCurrentRow(), 0);
            }
            else {
                EditBegin(ITEM_LABEL_COL, GetCurrentRow(), 0);
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::OnCellTypeNotify
//
/////////////////////////////////////////////////////////////////////////////

int CItemGrid::OnCellTypeNotify(long /*ID*/, int col, long row, long msg, long /*param*/)
{
    if(msg == UGCT_BUTTONCLICK){
        if(col == ITEM_NOTE_COL) {
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
//                        CItemGrid::EditBegin
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::EditBegin(int col, long row, UINT vcKey)
{
    ASSERT(row >= 0);
    ASSERT(col >= 0);
    GotoCol(col);
    VScrollEnable(ESB_DISABLE_BOTH);
    if (!(m_bAdding || m_bInserting)) {
        CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
        pDoc->PushUndo(*m_pDict, m_iLevel, m_iRec, m_iItem, m_aValue[row].vset, row);
        CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
        CDictItem* pItem = ((CDictItem*) m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec)->GetItem(GetItem()));
        DictValueSet& dict_value_set = pItem->GetValueSet(m_aValue[row].vset);
        m_pDict->SetChangedObject(&dict_value_set);
        m_pDict->SetOldName(dict_value_set.GetName());
        pTreeCtrl->SetUpdateAllViews(false);
    }
    CUGCell cell;
    CRect rect;
    CIMSAString cs;
    m_aEditControl.SetSize(ITEM_NUM_COLS);
    m_bEditing = true;
    m_iEditRow = row;
    ClearSelections();

    if (m_iMinCol == ITEM_SETLABEL_COL) {
        // Create value set label edit
        m_pSetLabelEdit = new CLabelEdit();
        m_pSetLabelEdit->SetRowCol(row, ITEM_SETLABEL_COL);
        GetCellRect(ITEM_SETLABEL_COL, row, &rect);
        m_pSetLabelEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 100);
        m_pSetLabelEdit->SetFont(&m_font);
        GetCell(ITEM_SETLABEL_COL, row, &cell);
        cell.GetText(&cs);
        m_pSetLabelEdit->SetWindowText(cs);
        m_aEditControl.SetAt(ITEM_SETLABEL_COL, (CWnd*) m_pSetLabelEdit);

        // Create value set name edit
        m_pSetNameEdit = new CNameEdit();
        GetCellRect(ITEM_SETNAME_COL, row, &rect);
        m_pSetNameEdit->SetRowCol(row, ITEM_SETNAME_COL);
        m_pSetNameEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 101);
        m_pSetNameEdit->SetFont(&m_font);
        GetCell(ITEM_SETNAME_COL, row, &cell);
        cell.GetText(&cs);
        m_pSetNameEdit->SetWindowText(cs);
        m_aEditControl.SetAt(ITEM_SETNAME_COL, (CWnd*) m_pSetNameEdit);
    }
    else {
        // Create value label edit
        m_pLabelEdit = new CLabelEdit();
        m_pLabelEdit->SetRowCol(row, ITEM_LABEL_COL);
        GetCellRect(ITEM_LABEL_COL, row, &rect);
        m_pLabelEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 100);
        m_pLabelEdit->SetFont(&m_font);
        GetCell(ITEM_LABEL_COL, row, &cell);
        cell.GetText(&cs);
        cs.Replace(SHOWBLANK, SPACE);
        m_pLabelEdit->SetWindowText(cs);
        m_aEditControl.SetAt(ITEM_LABEL_COL, (CWnd*) m_pLabelEdit);

        // Create value from edit
        CDictItem* pItem = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec)->GetItem(m_iItem);
        int iLen = pItem->GetCompleteLen();

        m_pFromEdit = new CFromToEdit();
        m_pFromEdit->SetRowCol(row, ITEM_FROM_COL);
        GetCellRect(ITEM_FROM_COL, row, &rect);
        m_pFromEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 100);
        m_pFromEdit->SetFont(&m_font);
        m_pFromEdit->SetLimitText(iLen);
        GetCell(ITEM_FROM_COL, row, &cell);
        cell.GetText(&cs);
        cs.Replace(SHOWBLANK, SPACE);
        m_pFromEdit->SetWindowText(cs);
        m_aEditControl.SetAt(ITEM_FROM_COL, (CWnd*) m_pFromEdit);

        // Create value to edit
        m_pToEdit = new CFromToEdit();
        m_pToEdit->SetRowCol(row, ITEM_TO_COL);
        GetCellRect(ITEM_TO_COL, row, &rect);
        m_pToEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 100);
        m_pToEdit->SetFont(&m_font);
        m_pToEdit->SetLimitText(iLen);
        GetCell(ITEM_TO_COL, row, &cell);
        cell.GetText(&cs);
        m_pToEdit->SetWindowText(cs);
        m_aEditControl.SetAt(ITEM_TO_COL, (CWnd*) m_pToEdit);

        // Create value special edit
        m_pSpecialEdit = new CDDComboBox();
        GetCellRect(ITEM_SPECIAL_COL, row, &rect);
        rect.top--;
        rect.left--;
        rect.bottom += 6*m_plf->lfHeight;
        rect.right += 2;
        m_pSpecialEdit->SetRowCol(row, ITEM_SPECIAL_COL);
        m_pSpecialEdit->Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, rect, this, 104);
        m_pSpecialEdit->SetFont(&m_font);
        m_pSpecialEdit->SetItemHeight (-1, m_plf->lfHeight);   // sets height for static control and button
        m_pSpecialEdit->SetItemHeight ( 0, m_plf->lfHeight);   // sets height for list box entries
        m_pSpecialEdit->AddString(_T(""));
        m_pSpecialEdit->AddString(SpecialValues::ValueToString(MISSING, false));
        m_pSpecialEdit->AddString(SpecialValues::ValueToString(REFUSED, false));
        m_pSpecialEdit->AddString(SpecialValues::ValueToString(NOTAPPL, false));
        m_pSpecialEdit->AddString(SpecialValues::ValueToString(DEFAULT, false));
        GetCell(ITEM_SPECIAL_COL, row, &cell);
        cell.GetText(&cs);
        if (cs == _T("")) {
            m_pSpecialEdit->SetCurSel(0);
        }
        else if (cs == _T("Missing")) {
            m_pSpecialEdit->SetCurSel(1);
        }
        else if (cs == _T("Refused")) {
            m_pSpecialEdit->SetCurSel(2);
        }
        else if (cs == _T("NotAppl")) {
            m_pSpecialEdit->SetCurSel(3);
        }
        else {
            m_pSpecialEdit->SetCurSel(4);
        }
        m_aEditControl.SetAt(ITEM_SPECIAL_COL, (CWnd*) m_pSpecialEdit);

    }

    // Set focus to field
    m_iEditCol = col;
    m_aEditControl[col]->SetFocus();
    if (vcKey > 0) {
        m_aEditControl[col]->SendMessage(WM_CHAR, vcKey, 1);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::EditEnd
//
/////////////////////////////////////////////////////////////////////////////

bool CItemGrid::EditEnd(bool bSilent)
{
    bool bChanged = false;
    bool bValid = true;
    bool bUndo = false;
    bool bVSet1NameChange = false;
    int row = m_iEditRow;

    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    int iVSet = m_aValue[row].vset;
    DictValueSet& dict_value_set = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec)->GetItem(m_iItem)->GetValueSet(iVSet);
    if (m_aValue[row].value == NONE) {
        CIMSAString csNewSetLabel, csOldSetLabel;
        m_aEditControl[ITEM_SETLABEL_COL]->GetWindowText(csNewSetLabel);
        csOldSetLabel = dict_value_set.GetLabel();
        if (csNewSetLabel.Compare(csOldSetLabel) != 0) {
            bChanged = true;
            bVSet1NameChange = true;

            if( m_bAdding )
                dict_value_set.GetLabelSet().SetLabels(LabelSet(csNewSetLabel));

            else
                dict_value_set.SetLabel(csNewSetLabel);
        }
        CIMSAString csNewSetName, csOldSetName;
        m_aEditControl[ITEM_SETNAME_COL]->GetWindowText(csNewSetName);
        csOldSetName = dict_value_set.GetName();
        if (csNewSetName.Compare(csOldSetName) != 0) {
            bChanged = true;
            dict_value_set.SetName(csNewSetName);
        }
        if (m_bAdding || m_bInserting) {
            if (dict_value_set.GetLabel().IsEmpty() && dict_value_set.GetName().IsEmpty()) {
                bUndo = true;
                m_bAdding = false;
                m_bInserting = false;
                bChanged = false;
                bValid = true;
            }
        }
        if (bChanged || !csNewSetLabel.IsEmpty() || !csNewSetName.IsEmpty()) {
            bValid = pDoc->GetDictionaryValidator()->IsValid(dict_value_set, m_iLevel, m_iRec, m_iItem, iVSet, bSilent);
            if (bValid) {
                if (bChanged) {                       // BMD 17 Jul 2003
                    pDoc->SetModified();
                }
                QuickSetText(ITEM_SETLABEL_COL, row, csNewSetLabel);
                QuickSetText(ITEM_SETNAME_COL, row, csNewSetName);
            }
            else {
                dict_value_set.SetLabel(csOldSetLabel);
                dict_value_set.SetName(csOldSetName);
            }
        }
    }
    else {
        int iValue = m_aValue[row].value;
        int iVPair = m_aValue[row].vpair;
        DictValue& dict_value = dict_value_set.GetValue(iValue);
        DictValuePair& dict_value_pair = dict_value.GetValuePair(iVPair);
        CIMSAString csNewLabel, csOldLabel;

        CIMSAString csNewSpecType;
        std::optional<double> old_special_value = dict_value.GetSpecialValue<std::optional<double>>();

        m_aEditControl[ITEM_LABEL_COL]->GetWindowText(csNewLabel);
        if (iVPair == 0) {
            csOldLabel = dict_value.GetLabel();
            if (csNewLabel.Compare(csOldLabel) != 0) {
                bChanged = true;
                dict_value.SetLabel(csNewLabel);
            }

            m_aEditControl[ITEM_SPECIAL_COL]->GetWindowText(csNewSpecType);
            std::optional<double> new_special_value = SpecialValues::StringIsSpecial<std::optional<double>>(csNewSpecType);

            if (new_special_value != old_special_value) {
                bChanged = true;
                dict_value.SetSpecialValue(new_special_value);
            }
        }

        CString csOldFrom = dict_value_pair.GetFrom();
        CString csOldTo = dict_value_pair.GetTo();

        CString csNewFrom;
        m_aEditControl[ITEM_FROM_COL]->GetWindowText(csNewFrom);
        bool bNewFromWasEmpty = csNewFrom.IsEmpty(); // 20130412 (i don't know why it is being set to a space below)
        if (bNewFromWasEmpty) {
            csNewFrom = _T(" ");
        }

        CString csNewTo;
        m_aEditControl[ITEM_TO_COL]->GetWindowText(csNewTo);

        dict_value_pair.SetFrom(csNewFrom);
        dict_value_pair.SetTo(csNewTo);

        // fix numeric value pairs
        CDictChildWnd* pChildWnd = assert_cast<CDictChildWnd*>(assert_cast<CMDIFrameWnd*>(AfxGetMainWnd())->MDIGetActive());
        const CDictItem* dict_item = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec)->GetItem(m_iItem);
        ValueSetFixer value_set_fixer(*dict_item, pChildWnd->GetDecimal());
        value_set_fixer.Fix(dict_value_pair);

        csNewFrom = dict_value_pair.GetFrom();
        csNewTo = dict_value_pair.GetTo();
        
        if (csNewFrom.Compare(csOldFrom) != 0) {
            bChanged = true;
            m_aEditControl[ITEM_FROM_COL]->SetWindowText(dict_value_pair.GetFrom());
        }

        if (csNewTo.Compare(csOldTo) != 0) {
            bChanged = true;
            m_aEditControl[ITEM_TO_COL]->SetWindowText(dict_value_pair.GetTo());
        }

        if (bNewFromWasEmpty && dict_value_pair.GetTo().IsEmpty() && (m_bAdding || m_bInserting)) {
            if (csNewLabel.IsEmpty()) {
                bUndo = true;
                m_bAdding = false;
                m_bInserting = false;
                bChanged = false;
                bValid = true;
            }
        }
        if (bChanged) {
            if (iVPair == 0) {
                bValid = pDoc->GetDictionaryValidator()->IsValid(dict_value, m_iLevel, m_iRec, m_iItem, iVSet, iValue, bSilent);
            }
            if (bValid) {
                bValid &= pDoc->GetDictionaryValidator()->IsValid(dict_value_pair, m_iLevel, m_iRec, m_iItem, iVSet, iValue, bSilent);
            }
            if (bValid) {
                pDoc->SetModified();
                if (iVPair == 0) {
                    QuickSetText(ITEM_LABEL_COL, row, csNewLabel);
                    QuickSetText(ITEM_SPECIAL_COL, row, csNewSpecType);
                }
                CIMSAString cs;
                cs = dict_value_pair.GetFrom();
                cs.Replace(SPACE, SHOWBLANK);
                QuickSetText(ITEM_FROM_COL, row, cs);
                cs = dict_value_pair.GetTo();
                cs.Replace(SPACE, SHOWBLANK);
                QuickSetText(ITEM_TO_COL, row, cs);
            }
            else {
                // DD_STD_REFACTOR_TODO perhaps fix...this can be tested by adding a label, leaving from/to blank, and setting special to not-notappl,
                // then hit ctrl+enter ... a new row will show up before the message box is displayed [ctrl+enter must be pressed while on the special column]
                // for some reason the AfxMessageBox displaying errors in IsValid above will cause OnEditAdd
                // to be called, at which point dict_value/dict_value_pair may not be valid, so don't use the reference on error
                if (iVPair == 0) {
                    dict_value_set.GetValue(iValue).SetLabel(csOldLabel);
                    dict_value_set.GetValue(iValue).SetSpecialValue(old_special_value);
                }
                dict_value_set.GetValue(iValue).GetValuePair(iVPair).SetFrom(csOldFrom);
                dict_value_set.GetValue(iValue).GetValuePair(iVPair).SetTo(csOldTo);
            }
        }
    }
    if (bValid) {
        // Decide whether this is a value & first vpair or a continuing vpair
        if (m_aValue[row].value != NONE) {
            if ((m_bAdding || m_bInserting) && m_aValue[row].vpair == 0 && m_aValue[row].value > 0) {
                const DictValue& dict_value = dict_value_set.GetValue(m_aValue[row].value);
                CIMSAString csTemp = dict_value.GetLabel();
                csTemp.TrimLeft();
                if (!dict_value.IsSpecial() && dict_value.GetLabel().GetLength() > 0 && csTemp.GetLength() == 0) {
                    DictValue& prev_dict_value = dict_value_set.GetValue(m_aValue[row - 1].value);
                    prev_dict_value.AddValuePair(dict_value.GetValuePair(0));
                    dict_value_set.RemoveValue(m_aValue[row].value);
                    SetRedraw(FALSE);
                    Update();
                    GotoRow(row);
                    SetRedraw(TRUE);
                }
            }
            else if (!(m_bAdding || m_bInserting)) {
                DictValue& dict_value = dict_value_set.GetValue(m_aValue[row].value);
                CIMSAString csLabel, csValueType;
                m_aEditControl[ITEM_LABEL_COL]->GetWindowText(csLabel);
                m_aEditControl[ITEM_SPECIAL_COL]->GetWindowText(csValueType);
                CIMSAString csTemp = csLabel;
                csTemp.TrimLeft();
                if (csValueType.GetLength() == 0 && csLabel.GetLength() > 0 && csTemp.GetLength() == 0) {
                    if (m_aValue[row].value > 0 && m_aValue[row].vpair == 0) {
                        DictValue& prev_dict_value = dict_value_set.GetValue(m_aValue[row - 1].value);
                        prev_dict_value.AddValuePair(dict_value.GetValuePair(0));
                        dict_value_set.RemoveValue(m_aValue[row].value);
                        SetRedraw(FALSE);
                        Update();
                        GotoRow(row);
                        SetRedraw(TRUE);
                    }
                }
                else {
                    if (m_aValue[row].vpair > 0) {
                        DictValue new_dict_value;
                        new_dict_value.GetLabelSet().SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());
                        new_dict_value.SetLabel(csLabel);

                        std::optional<double> new_special_value = SpecialValues::StringIsSpecial<std::optional<double>>(csValueType);
                        new_dict_value.SetSpecialValue(new_special_value);

                        size_t p = m_aValue[row].vpair;
                        while (p < dict_value.GetNumValuePairs()) {
                            new_dict_value.AddValuePair(dict_value.GetValuePair(p));
                            dict_value.RemoveValuePair(m_aValue[row].vpair);
                        }

                        int i = m_aValue[row].value + 1;
                        dict_value_set.InsertValue(i, std::move(new_dict_value));

                        SetRedraw(FALSE);
                        Update();
                        GotoRow(row);
                        SetRedraw(TRUE);
                    }
                }
            }
        }
        if (pDoc->GetDictionaryValidator()->AdjustValues(dict_value_set)) {
            SetRedraw(FALSE);
            Update();
            GotoRow(row);
            SetRedraw(TRUE);
        }

        // Quit editing
        EditQuit();
        if (m_bAdding || m_bInserting) {
            m_pDict->BuildNameList();
        }
        else {
            m_pDict->UpdateNameList(dict_value_set, m_iLevel, m_iRec, m_iItem, iVSet);
        }
        if (bVSet1NameChange) {
            CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
            DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
            pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, m_iLevel, m_iRec, m_iItem);
            SetFocus();
        }
    }
    else {
        m_iEditCol = pDoc->GetDictionaryValidator()->GetInvalidEdit();
        if (m_iEditCol < m_iMinCol || m_iEditCol > m_iMaxCol) {  // BMD 10 Mar 2003
            m_iEditCol = m_iMaxCol;
        }
        GotoRow(m_iEditRow);
        m_aEditControl[m_iEditCol]->SetFocus();
    }
    if (bUndo) {
        pDoc->UndoChange(FALSE);
    }
    //SAVY for vset name change reconcile (shld I do this just for tables then???
    if (bChanged && bValid && !m_bAdding && !m_bInserting) {
        if (m_iMinCol == ITEM_SETLABEL_COL) {
            AfxGetMainWnd()->PostMessage(UWM::Dictionary::NameChange, (WPARAM)pDoc);
        }
        else {
            AfxGetMainWnd()->PostMessage(UWM::Dictionary::ValueLabelChange, (WPARAM)pDoc);
        }
    }

    // sync any linked value sets
    if( dict_value_set.IsLinkedValueSet() )
        m_pDict->SyncLinkedValueSets(&dict_value_set);

    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::EditQuit
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::EditQuit()
{
    SetRedraw(FALSE);
    if (m_iMinCol == ITEM_SETLABEL_COL) {
        delete m_pSetLabelEdit;
        delete m_pSetNameEdit;
    }
    else {
        delete m_pLabelEdit;
        delete m_pFromEdit;
        delete m_pToEdit;
        delete m_pSpecialEdit;
    }
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
//                        CItemGrid::OnSetFocus
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::OnSetFocus(int /*section*/)
{
    CDDGView* pView = (CDDGView*) GetParent();
    CSplitterWnd* pSplitWnd = (CSplitterWnd*) pView->GetParent();
    pSplitWnd->SetActivePane(0,0);
    for (long row = 0 ; row < GetNumberRows() ; row++) {
        for (int col = ITEM_SETLABEL_COL ; col < GetNumberCols() ; col++) {
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
//                        CItemGrid::OnKillFocus
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::OnKillFocus(int /*section*/)
{
    m_bCanEdit = false;

    int col;
    long row;
    if (EnumFirstSelected(&col, &row) != UG_SUCCESS) {
        return;
    }

    for (col = ITEM_SETLABEL_COL ; col < GetNumberCols() ; col++) {
        QuickSetHTextColor(col, row, GetSysColor(COLOR_WINDOWTEXT));
        QuickSetHBackColor(col, row, GetSysColor(COLOR_BTNFACE));
    }
    RedrawRow(row);
    while (EnumNextSelected(&col, &row) == UG_SUCCESS) {
        for (col = ITEM_SETLABEL_COL ; col < GetNumberCols() ; col++) {
            QuickSetHTextColor(col, row, GetSysColor(COLOR_WINDOWTEXT));
            QuickSetHBackColor(col, row, GetSysColor(COLOR_BTNFACE));
        }
        RedrawRow(row);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::OnEditUndo
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::OnEditUndo()
{
    ASSERT(m_bEditing);

    ASSERT(m_iEditCol == ITEM_SETLABEL_COL || m_iEditCol == ITEM_SETNAME_COL ||
            m_iEditCol == ITEM_LABEL_COL || m_iEditCol == ITEM_FROM_COL ||
            m_iEditCol == ITEM_TO_COL);

    assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->Undo();
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::OnEditCut
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::OnEditCut()
{
    if (m_bEditing) {

        ASSERT(m_iEditCol == ITEM_SETLABEL_COL || m_iEditCol == ITEM_SETNAME_COL ||
                m_iEditCol == ITEM_LABEL_COL || m_iEditCol == ITEM_FROM_COL ||
                m_iEditCol == ITEM_TO_COL);

        assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->Cut();
        return;
    }
    EditCopy(true);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::OnEditCopy
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::OnEditCopy()
{
    if (m_bEditing) {
        ASSERT(m_iEditCol == ITEM_SETLABEL_COL || m_iEditCol == ITEM_SETNAME_COL ||
                m_iEditCol == ITEM_LABEL_COL || m_iEditCol == ITEM_FROM_COL ||
                m_iEditCol == ITEM_TO_COL);

        assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->Copy();
        return;
    }
    EditCopy(false);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::OnEditAdd
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::OnEditAdd()
{
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();

    // If tree control has focus, position to item on record grid and post add message
    DictTreeNode* dict_tree_node = pTreeCtrl->GetTreeNode(pTreeCtrl->GetSelectedItem());
    if (GetFocus() == pTreeCtrl && dict_tree_node->GetValueSetIndex() == NONE) {
        pTreeCtrl->m_bFromTree = false;
        POSITION pos = pDoc->GetFirstViewPosition();
        ASSERT(pos != NULL);
        CDDGView* pView = (CDDGView*) pDoc->GetNextView(pos);
        DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
        int iLevel = pDoc->GetLevel();
        int iRec = pDoc->GetRec();
        int iItem = pDoc->GetItem();
        pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, iLevel, iRec);
        pView->m_iGrid = DictionaryGrid::Record;
        pView->m_gridRecord.SetRedraw(FALSE);
        pView->m_gridRecord.Update(pDoc->GetDict(), iLevel, iRec);
        pView->ResizeGrid();
        pView->m_gridRecord.ClearSelections();
        pView->m_gridRecord.GotoRow(pView->m_gridRecord.GetRow(iItem));
        pView->m_gridRecord.SetRedraw(TRUE);
        pView->m_gridRecord.InvalidateRect(NULL);
        pView->m_gridRecord.SetFocus();
        pView->m_gridRecord.PostMessage(WM_COMMAND, ID_EDIT_ADD);
        return;
    }

    int row = 0;
    CDictItem* pItem = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec)->GetItem(m_iItem);

    bool bValueSet = true;
    if (GetFocus() == pTreeCtrl || pTreeCtrl->m_bFromTree) {
        pTreeCtrl->m_bFromTree = false;
        row = GetNumberRows();
    }
    else {
        if (pItem->HasValueSets()) {
            row = GetNumberRows();
            if (m_aValue[GetCurrentRow()].value == NONE) {
                const DictValueSet& dict_value_set = pItem->GetValueSet(m_aValue[GetCurrentRow()].vset);
                if (!dict_value_set.HasValues()) {
                    bValueSet = false;
                }
            }
            else {
                bValueSet = false;
            }
        }
    }
    m_bAdding = true;
    int iVSet = 0;
    if (pItem->HasValueSets()) {
        iVSet = m_aValue[GetCurrentRow()].vset;
    }
    if (!bValueSet) {
        if (m_aValue[GetCurrentRow()].value == NONE) {
            row = GetCurrentRow() + 1;
        }
        else {
            for (row = GetCurrentRow() ; row < GetNumberRows() ; row++) {
                if (m_aValue[row].value == NONE) {
                    break;
                }
            }
        }
    }

    // Push old values on stack
    if (!pItem->HasValueSets()) {
        pDoc->PushUndo(*m_pDict, m_iLevel, m_iRec, m_iItem, 0, NONE);
    }
    else {
        pDoc->PushUndo(*m_pDict, m_iLevel, m_iRec, m_iItem, iVSet, row - 1);
    }

    // Insert value set or value
    if (bValueSet) {
        DictValueSet dict_value_set;
        dict_value_set.GetLabelSet().SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());
        CIMSAString csName = pItem->GetName();
        if (!pItem->HasValueSets()) {
            dict_value_set.GetLabelSet().SetLabels(pItem->GetLabelSet());
            csName += _T("_VS1");
        }
        else {
            CIMSAString csNum;
            csNum.Str((int)pItem->GetNumValueSets() + 1);
            csName += _T("_VS") + csNum;
        }
        csName = pDoc->GetDict()->GetUniqueName(csName);
        dict_value_set.SetName(csName);
        pItem->AddValueSet(std::move(dict_value_set));
        m_iMinCol = ITEM_SETLABEL_COL;
        m_iMaxCol = ITEM_SETNAME_COL;
    }
    else {
        DictValueSet& dict_value_set = pItem->GetValueSet(m_aValue[GetCurrentRow()].vset);
        DictValue dict_value;
        dict_value.GetLabelSet().SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());
        dict_value.AddValuePair(DictValuePair());
        dict_value_set.AddValue(std::move(dict_value));
        m_iMinCol = ITEM_LABEL_COL;
        m_iMaxCol = ITEM_SPECIAL_COL;
    }

    // Update tree
    pTreeCtrl->SetUpdateAllViews(false);
    DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
    pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, m_iLevel, m_iRec, m_iItem);

    // Update grid
    SetRedraw(FALSE);
    Update();
    GotoRow(row);
    SetRedraw(TRUE);
    InvalidateRect(NULL);

    // Begin editing new item
    if (bValueSet) {
        EditBegin(ITEM_SETLABEL_COL, row, 0);
    }
    else {
        EditBegin(ITEM_LABEL_COL, row, 0);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::OnEditInsert
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::OnEditInsert()
{
    // If tree control has focus, position to item on record grid and post delete message
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    DictTreeNode* dict_tree_node = pTreeCtrl->GetTreeNode(pTreeCtrl->GetSelectedItem());
    if (GetFocus() == pTreeCtrl && dict_tree_node->GetValueSetIndex() == NONE) {
        POSITION pos = pDoc->GetFirstViewPosition();
        ASSERT(pos != NULL);
        CDDGView* pView = (CDDGView*) pDoc->GetNextView(pos);
        DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
        int iLevel = pDoc->GetLevel();
        int iRec = pDoc->GetRec();
        int iItem = pDoc->GetItem();
        pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, iLevel, iRec);
        pView->m_iGrid = DictionaryGrid::Record;
        pView->m_gridRecord.SetRedraw(FALSE);
        pView->m_gridRecord.Update(pDoc->GetDict(), iLevel, iRec);
        pView->ResizeGrid();
        pView->m_gridRecord.ClearSelections();
        pView->m_gridRecord.GotoRow(pView->m_gridRecord.GetRow(iItem));
        pView->m_gridRecord.SetRedraw(TRUE);
        pView->m_gridRecord.InvalidateRect(NULL);
        pView->m_gridRecord.SetFocus();
        pView->m_gridRecord.PostMessage(WM_COMMAND, ID_EDIT_INSERT);
        return;
    }

    int row = GetCurrentRow();
    CDictItem* pItem = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec)->GetItem(m_iItem);

    m_bValueSet = true;
    if (row != 0) {
        if (m_aValue[row].value != NONE) {
            m_bValueSet = false;
        }
    }
    m_bInserting = true;

    // Push old values on stack
    pDoc->PushUndo(*m_pDict, m_iLevel, m_iRec, m_iItem, (int)pItem->GetNumValueSets() - 1, row);

    // Insert value set or value
    if (m_bValueSet) {
        int iVSet = m_aValue[row].vset;
        DictValueSet dict_value_set;
        dict_value_set.GetLabelSet().SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());
        CIMSAString csName = pItem->GetName();
        if (!pItem->HasValueSets()) {
            dict_value_set.SetLabel(pItem->GetLabel());
            csName += _T("_VS1");
        }
        else {
            CIMSAString csNum;
            csNum.Str((int)pItem->GetNumValueSets() + 1);
            csName += _T("_VS") + csNum;
        }
        dict_value_set.SetName(csName);
        pItem->InsertValueSet(iVSet, std::move(dict_value_set));
        m_iMinCol = ITEM_SETLABEL_COL;
        m_iMaxCol = ITEM_SETNAME_COL;
    }
    else {
        DictValueSet& dict_value_set = pItem->GetValueSet(m_aValue[row - 1].vset);
        if (m_aValue[row].vpair == NONE || m_aValue[row].vpair == 0) {
            DictValue dict_value;
            dict_value.GetLabelSet().SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());
            dict_value.AddValuePair(DictValuePair());
            int iValue = m_aValue[row - 1].value + 1;
            dict_value_set.InsertValue(iValue, std::move(dict_value));
        }
        else {
            DictValue& dict_value = dict_value_set.GetValue(m_aValue[row].value);
            dict_value.InsertValuePair(m_aValue[row].vpair, DictValuePair());
        }
        m_iMinCol = ITEM_LABEL_COL;
        m_iMaxCol = ITEM_SPECIAL_COL;
    }

    // Update tree
    pTreeCtrl->SetUpdateAllViews(false);

    // Update grid
    SetRedraw(FALSE);
    Update();
    GotoRow(row);
    SetRedraw(TRUE);
    InvalidateRect(NULL);


    // Begin editing new item
    if (m_bValueSet) {
        EditBegin(ITEM_SETLABEL_COL, row, 0);
    }
    else {
        EditBegin(ITEM_LABEL_COL, row, 0);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::OnEditDelete
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::OnEditDelete()
{
    // If editing, clear edit control
    if (m_bEditing) {
        ASSERT(m_iEditCol == ITEM_SETLABEL_COL || m_iEditCol == ITEM_SETNAME_COL ||
                m_iEditCol == ITEM_LABEL_COL || m_iEditCol == ITEM_FROM_COL ||
                m_iEditCol == ITEM_TO_COL);

        assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->Clear();
        return;
    }
    // If tree control has focus, position to item on record grid and post delete message
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    DictTreeNode* dict_tree_node = pTreeCtrl->GetTreeNode(pTreeCtrl->GetSelectedItem());
    if (GetFocus() == pTreeCtrl && dict_tree_node->GetValueSetIndex() == NONE) {
        POSITION pos = pDoc->GetFirstViewPosition();
        ASSERT(pos != NULL);
        CDDGView* pView = (CDDGView*) pDoc->GetNextView(pos);
        DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
        int iLevel = pDoc->GetLevel();
        int iRec = pDoc->GetRec();
        int iItem = pDoc->GetItem();
        pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, iLevel, iRec);
        pView->m_iGrid = DictionaryGrid::Record;
        pView->m_gridRecord.SetRedraw(FALSE);
        pView->m_gridRecord.Update(pDoc->GetDict(), iLevel, iRec);
        pView->ResizeGrid();
        pView->m_gridRecord.ClearSelections();
        pView->m_gridRecord.GotoRow(pView->m_gridRecord.GetRow(iItem));
        pView->m_gridRecord.SetRedraw(TRUE);
        pView->m_gridRecord.InvalidateRect(NULL);
        pView->m_gridRecord.SetFocus();
        pView->m_gridRecord.PostMessage(WM_COMMAND, ID_EDIT_DELETE);
        return;
    }
    EditDelete();
}


/////////////////////////////////////////////////////////////////////////////
//
//                        Copy + Paste Helpers
//
/////////////////////////////////////////////////////////////////////////////

namespace
{
    // returns an empty vector if the line is not valid
    std::vector<std::wstring> GetTabDelimitedComponents(wstring_view line)
    {
        // look for lines of the form: "<label>\t<from>\t<to>\t<image_filename>\n"
        // where all but the from value can be blank
        std::vector<std::wstring> values = SO::SplitString(line, '\t');

        if( values.size() < 2 || values.size() > 4 || values[1].empty() )
            values.clear();

        return values;
    }

    DictValueSet CreateValueSetFromTabDelimitedText(const std::wstring& text, ContentType content_type, const CString& dictionary_filename)
    {
        // the value set will be given a name and label in OnEditPaste
        DictValueSet dict_value_set;

        SO::ForeachLine(text, false,
            [&](wstring_view line)
            {
                std::vector<CString> values = WS2CS_Vector(GetTabDelimitedComponents(line));

                if( values.empty() )
                    return true;

                // 20140904 left-trim any zeros so they don't show up in the values
                if( IsNumeric(content_type) )
                {
                    auto fix = [&](CString& val)
                    {
                        val = clearString(val, true);

                        if( !val.IsEmpty() && val[0] == _T('.') )
                            val.Insert(0, _T('0'));
                    };

                    fix(values[1]);

                    if( values.size() > 2 )
                        fix(values[2]);
                }

                DictValue dict_value;
                dict_value.SetLabel(values[0]);

                // get the full path of an image filename
                if( values.size() > 3 )
                    dict_value.SetImageFilename(WS2CS(MakeFullPath(PortableFunctions::PathGetDirectory(dictionary_filename), CS2WS(values[3]))));

                size_t value_index = dict_value_set.GetNumValues();

                // if the label and image filename are blank, associate this with the last value when possible
                if( value_index > 0 && dict_value.GetLabel().IsEmpty() && dict_value.GetImageFilename().IsEmpty() )
                {
                    --value_index;
                }

                // otherwise add a new value entry
                else
                {
                    dict_value_set.AddValue(std::move(dict_value));
                }

                // add the value pair
                dict_value_set.GetValue(value_index).AddValuePair(DictValuePair(values[1], ( values.size() > 2 ) ? values[2] : CString()));

                return true;
            });

        ASSERT(dict_value_set.GetNumValues() != 0);

        return dict_value_set;
    }


    void OutputDictValue(std::wostringstream& stream, const DictValuePair& dict_value_pair)
    {
        stream << _T("\t") << (LPCTSTR)dict_value_pair.GetFrom()
               << _T("\t") << (LPCTSTR)dict_value_pair.GetTo();
    }

    std::wostringstream& operator<<(std::wostringstream& stream, const DictValuePair& dict_value_pair)
    {
        OutputDictValue(stream, dict_value_pair);
        stream << std::endl;
        return stream;
    }

    std::wostringstream& operator<<(std::wostringstream& stream, const DictValue& dict_value)
    {
        stream << (LPCTSTR)dict_value.GetLabel();

        bool first_pair = true;

        for( const DictValuePair& dict_value_pair : dict_value.GetValuePairs() )
        {
            OutputDictValue(stream, dict_value_pair);

            if( first_pair && !dict_value.GetImageFilename().IsEmpty() )
                stream << _T("\t") << (LPCTSTR)dict_value.GetImageFilename();

            stream << std::endl;

            first_pair = false;
        }

        return stream;
    }

    std::wostringstream& operator<<(std::wostringstream& stream, const DictValueSet& dict_value_set)
    {
        stream << (LPCTSTR)dict_value_set.GetLabel() << std::endl;

        for( const DictValue& dict_value : dict_value_set.GetValues() )
            stream << dict_value;

        return stream;
    }
}


bool CItemGrid::IsClipboardValidForValueSetPaste(const CDDDoc& dictionary_doc)
{
    if( dictionary_doc.GetDictClipboard().IsAvailable<DictValueSet>() ||
        dictionary_doc.GetDictClipboard().IsAvailable<DictValue>() ||
        dictionary_doc.GetDictClipboard().IsAvailable<DictValuePair>() )
    {
        return true;
    }

    else if( IsClipboardFormatAvailable(_tCF_TEXT) )
    {
        bool valid_line_found = false;

        SO::ForeachLine(WinClipboard::GetText(), false,
            [&](wstring_view line)
            {
                valid_line_found = !GetTabDelimitedComponents(line).empty();
                return !valid_line_found;
            });

        return valid_line_found;
    }

    return false;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::EditCopy
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::EditCopy(bool bCut)
{
    std::vector<long> selected_rows = GetSelectedRows();
    ASSERT(!selected_rows.empty());

    // only entities of one type will be copied/cut, so if, for example,
    // VS1 and some values of VS2 are selected, then value sets will be copied,
    // with only the values of VS2 that are selected included as part of the VS2 copy
    const CDictItem* dict_item = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec)->GetItem(m_iItem);

    // on the first pass, determine what kinds of things are being copied
    std::vector<const DictValueSet*> dict_value_sets_to_copy;
    std::vector<long> dict_value_sets_rows;

    std::vector<const DictValue*> dict_values_to_copy;
    std::vector<long> dict_values_rows;

    std::vector<const DictValuePair*> dict_value_pairs_to_copy;
    std::vector<long> dict_value_pair_rows;

    for( long selected_row : selected_rows )
    {
        const DictValueSet& dict_value_set = dict_item->GetValueSet(m_aValue[selected_row].vset);

        // copying an entire value set
        if( m_aValue[selected_row].value == NONE )
        {
            dict_value_sets_to_copy.emplace_back(&dict_value_set);
            dict_value_sets_rows.emplace_back(selected_row);
            continue;
        }

        // if the value set / value pair is part of an entire value set that has been copied, there is no need to copy it individually
        if( std::find(dict_value_sets_to_copy.cbegin(), dict_value_sets_to_copy.cend(), &dict_value_set) != dict_value_sets_to_copy.cend() )
            continue;

        const DictValue& dict_value = dict_value_set.GetValue(m_aValue[selected_row].value);

        // copying an entire value
        if( m_aValue[selected_row].vpair < 1 )
        {
            dict_values_to_copy.emplace_back(&dict_value);
            dict_values_rows.emplace_back(selected_row);
            continue;
        }

        // if the value pair is part of an entire value that has been copied, there is no need to copy it individually
        if( std::find(dict_values_to_copy.cbegin(), dict_values_to_copy.cend(), &dict_value) != dict_values_to_copy.cend() )
            continue;

        const DictValuePair& dict_value_pair = dict_value.GetValuePair(m_aValue[selected_row].vpair);
        dict_value_pairs_to_copy.emplace_back(&dict_value_pair);
        dict_value_pair_rows.emplace_back(selected_row);
    }


    // potentially create fake values from value pairs
    std::map<std::tuple<int, int>, std::unique_ptr<DictValue>> fake_dict_values;

    if( !dict_value_pairs_to_copy.empty() && ( !dict_values_to_copy.empty() || !dict_value_sets_to_copy.empty() ) )
    {
        for( size_t i = 0; i < dict_value_pair_rows.size(); ++i )
        {
            long selected_row = dict_value_pair_rows[i];
            std::tuple<int, int> index(m_aValue[selected_row].vset, m_aValue[selected_row].value);

            // the parent value should not have been copied
            ASSERT(dict_values_to_copy.cend() == std::find(dict_values_to_copy.cbegin(), dict_values_to_copy.cend(),
                                                           &dict_item->GetValueSet(std::get<0>(index)).GetValue(std::get<1>(index))));

            // create a fake value, or add this to a previously created fake value
            std::unique_ptr<DictValue>& fake_dict_value = fake_dict_values[index];

            if( fake_dict_value == nullptr )
            {
                fake_dict_value = std::make_unique<DictValue>();

                // insert this fake value in the correct spot (sorted by row)
                size_t insertion_index = std::distance(dict_values_rows.cbegin(),
                                                       std::upper_bound(dict_values_rows.cbegin(), dict_values_rows.cend(), selected_row));
                dict_values_to_copy.insert(dict_values_to_copy.begin() + insertion_index, fake_dict_value.get());
                dict_values_rows.insert(dict_values_rows.begin() + insertion_index, selected_row);
            }

            fake_dict_value->AddValuePair(*dict_value_pairs_to_copy[i]);
        }

        dict_value_pairs_to_copy.clear();
    }


    // potentially create fake value sets from values
    std::map<long, std::unique_ptr<DictValueSet>> fake_dict_value_sets;

    if( !dict_values_to_copy.empty() && !dict_value_sets_to_copy.empty() )
    {
        for( size_t i = 0; i < dict_values_rows.size(); ++i )
        {
            long selected_row = dict_values_rows[i];
            int index = m_aValue[selected_row].vset;

            // the parent value set should not have been copied
            ASSERT(dict_value_sets_to_copy.cend() == std::find(dict_value_sets_to_copy.cbegin(), dict_value_sets_to_copy.cend(),
                                                               &dict_item->GetValueSet(index)));

            // create a fake value set, or add this to a previously created fake value set
            std::unique_ptr<DictValueSet>& fake_dict_value_set = fake_dict_value_sets[index];

            if( fake_dict_value_set == nullptr )
            {
                fake_dict_value_set = std::make_unique<DictValueSet>();

                const DictValueSet& source_dict_value_set = dict_item->GetValueSet(index);
                fake_dict_value_set->SetName(source_dict_value_set.GetName());
                fake_dict_value_set->GetLabelSet().SetLabels(source_dict_value_set.GetLabelSet());

                // insert this fake value set in the correct spot (sorted by row)
                size_t insertion_index = std::distance(dict_value_sets_rows.cbegin(),
                                                       std::upper_bound(dict_value_sets_rows.cbegin(), dict_value_sets_rows.cend(), selected_row));
                dict_value_sets_to_copy.insert(dict_value_sets_to_copy.begin() + insertion_index, fake_dict_value_set.get());
                dict_value_sets_rows.insert(dict_value_sets_rows.begin() + insertion_index, selected_row);
            }

            fake_dict_value_set->AddValue(*dict_values_to_copy[i]);
        }

        dict_values_to_copy.clear();
    }

    ASSERT(1 == ( dict_value_sets_to_copy.empty() ? 0 : 1 ) +
                ( dict_values_to_copy.empty() ? 0 : 1 ) +
                ( dict_value_pairs_to_copy.empty() ? 0 : 1 ));

    // copy the entities to the clipboard (in JSON and tab-delimited text formats)
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    std::wostringstream stream;

    auto copy = [&](const auto& entities_to_copy)
    {
        pDoc->GetDictClipboard().PutOnClipboard(this, entities_to_copy, dict_item);

        for( const auto& entity : entities_to_copy )
            stream << *entity;
    };

    if( !dict_value_sets_to_copy.empty() )
    {
        copy(dict_value_sets_to_copy);
    }

    else if( !dict_values_to_copy.empty() )
    {
        copy(dict_values_to_copy);
    }

    else
    {
        copy(dict_value_pairs_to_copy);
    }

    WinClipboard::PutText(this, stream.str().c_str(), false);

    // If cut, delete values
    if( bCut )
        EditDelete();
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::OnEditPaste
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::OnEditPaste()
{
    // If editing, paste to the control
    if( m_bEditing )
    {
        ASSERT(m_iEditCol == ITEM_SETLABEL_COL || m_iEditCol == ITEM_SETNAME_COL ||
               m_iEditCol == ITEM_LABEL_COL || m_iEditCol == ITEM_FROM_COL ||
               m_iEditCol == ITEM_TO_COL);
        assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->Paste();
        return;
    }

    // get the entities on the clipboard
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    CDictItem* dict_item = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec)->GetItem(m_iItem);

    std::vector<DictValueSet> pasted_value_sets;
    CString parent_item_name_of_pasted_value_sets;
    std::vector<DictValue> pasted_values;
    std::vector<DictValuePair> pasted_value_pairs;

    if( pDoc->GetDictClipboard().IsAvailable<DictValueSet>() )
    {
        DictPastedValues<DictValueSet> full_pasted_value_sets = pDoc->GetDictClipboard().GetNamedElementsFromClipboard<DictValueSet>(this);
        pasted_value_sets = std::move(full_pasted_value_sets.values);
        parent_item_name_of_pasted_value_sets = full_pasted_value_sets.parent_dict_element_name;
    }

    else if( pDoc->GetDictClipboard().IsAvailable<DictValue>() )
    {
        pasted_values = pDoc->GetDictClipboard().GetFromClipboard<DictValue>(this);
    }

    else if( pDoc->GetDictClipboard().IsAvailable<DictValuePair>() )
    {
        pasted_value_pairs = pDoc->GetDictClipboard().GetFromClipboard<DictValuePair>(this);
    }

    else
    {
        ASSERT(IsClipboardFormatAvailable(_tCF_TEXT));
        pasted_value_sets.emplace_back(CreateValueSetFromTabDelimitedText(WinClipboard::GetText(this), dict_item->GetContentType(), pDoc->GetPathName()));
    }

    if( pasted_value_sets.empty() && pasted_values.empty() && pasted_value_pairs.empty() )
        return;


    // if there are no value sets and values or value pairs have been pasted, convert them to value sets
    if( dict_item->GetNumValueSets() == 0 && pasted_value_sets.empty() )
    {
        if( !pasted_value_pairs.empty() )
        {
            DictValue& dict_value = pasted_values.emplace_back();

            for( DictValuePair& dict_value_pair : pasted_value_pairs )
                dict_value.AddValuePair(std::move(dict_value_pair));

            pasted_value_pairs.clear();
        }

        ASSERT(!pasted_values.empty());

        DictValueSet& dict_value_set = pasted_value_sets.emplace_back();
        dict_value_set.SetValues(std::move(pasted_values));
    }

    ASSERT(1 == ( pasted_value_sets.empty() ? 0 : 1 ) +
                ( pasted_values.empty() ? 0 : 1 ) +
                ( pasted_value_pairs.empty() ? 0 : 1 ));


    // Find insertion point
    long current_row = std::min(GetCurrentRow(), GetNumberRows() - 1);
    size_t value_set_index = 0;
    size_t value_index = 0;
    size_t value_pair_index = 0;

    if( current_row >= 0 )
    {
        bool on_value_set_row = ( m_aValue[current_row].value == NONE );
        bool on_last_row_of_editor = ( ( current_row + 1 ) == GetNumberRows() );

        value_set_index = m_aValue[current_row].vset;

        if( !on_value_set_row )
        {
            value_index = m_aValue[current_row].value;
            value_pair_index = m_aValue[current_row].vpair;
        }

        // show the before/after dialog if...
        bool dialog_canceled = false;

        auto show_dialog = [&](size_t& index)
        {
            BeforeAfterDlg dlg;

            dialog_canceled = ( dlg.DoModal() != IDOK );

            if( !dialog_canceled && dlg.SelectedAfter() )
                ++index;
        };

        if( !pasted_value_sets.empty() )
        {
            // ... pasting a value set on the last value set or on the last row of the editor
            if( on_last_row_of_editor || ( on_value_set_row && ( value_set_index + 1 ) == dict_item->GetNumValueSets() ) )
                show_dialog(value_set_index);
        }

        else if( !on_value_set_row )
        {
            const DictValueSet& dict_value_set = dict_item->GetValueSet(value_set_index);

            if( !pasted_values.empty() )
            {
                // ... pasting a value on the last value or on the last row of the editor
                if( on_last_row_of_editor || ( value_index + 1 ) == dict_value_set.GetNumValues() )
                    show_dialog(value_index);
            }

            else if( !pasted_value_pairs.empty() )
            {
                const DictValue& dict_value = dict_value_set.GetValue(value_index);

                // ... pasting a value pair on the last value pair
                if( value_pair_index > 0 && ( value_pair_index + 1 ) == dict_value.GetNumValuePairs() )
                    show_dialog(value_pair_index);
            }
        }
    }


    // Push old dictionary on stack
    if( current_row < 0 )
    {
        pDoc->PushUndo(*m_pDict, m_iLevel, m_iRec, m_iItem);
    }

    else
    {
        pDoc->PushUndo(*m_pDict, m_iLevel, m_iRec, m_iItem, m_aValue[current_row].vset, current_row);
    }


    // this class will fix numeric value pairs
    CDictChildWnd* pChildWnd = assert_cast<CDictChildWnd*>(assert_cast<CMDIFrameWnd*>(AfxGetMainWnd())->MDIGetActive());
    ValueSetFixer value_set_fixer(*dict_item, pChildWnd->GetDecimal());


    // paste value pairs
    // -----------------
    DictValueSet* dict_value_set_for_linked_syncing = nullptr;

    if( !pasted_value_pairs.empty() )
    {
        ASSERT(value_set_index < dict_item->GetNumValueSets());
        DictValueSet& dict_value_set = dict_item->GetValueSet(value_set_index);
        dict_value_set_for_linked_syncing = &dict_value_set;

        // if the pair cannot be attached to a value, create a value for this
        if( dict_value_set.GetNumValues() == 0 )
        {
            ASSERT(value_index == 0 && value_pair_index == 0);
            dict_value_set.AddValue(DictValue());
        }

        ASSERT(value_index < dict_value_set.GetNumValues());
        DictValue& dict_value = dict_value_set.GetValue(value_index);

        // the pair should never be added as the first pair (unless this is a newly added value)
        if( dict_value.HasValuePairs() && value_pair_index == 0 )
            value_pair_index = 1;

        ASSERT(value_pair_index <= dict_value.GetNumValuePairs());

        for( DictValuePair& dict_value_pair : pasted_value_pairs )
        {
            // fix the numeric value pairs
            value_set_fixer.Fix(dict_value_pair);

            // insert the value pair
            dict_value.InsertValuePair(value_pair_index, std::move(dict_value_pair));
            ++value_pair_index;
        }
    }


    // paste values
    // ------------
    else if( !pasted_values.empty() )
    {
        ASSERT(value_set_index < dict_item->GetNumValueSets());
        DictValueSet& dict_value_set = dict_item->GetValueSet(value_set_index);
        dict_value_set_for_linked_syncing = &dict_value_set;

        ASSERT(value_index <= dict_value_set.GetNumValues());

        for( DictValue& dict_value : pasted_values )
        {
            // fix the numeric value pairs
            value_set_fixer.Fix(dict_value);

            // insert the value
            dict_value_set.InsertValue(value_index, std::move(dict_value));
            ++value_index;
        }
    }


    // paste value sets
    // ----------------
    else
    {
        ASSERT(!pasted_value_sets.empty());
        ASSERT(value_set_index <= dict_item->GetNumValueSets());

        const CDataDict& dictionary = pDoc->GetDictionary();
        std::set<CString> additional_names_in_use;

        for( DictValueSet& dict_value_set : pasted_value_sets )
        {
            // don't paste as a linked value set
            dict_value_set.UnlinkValueSet();

            // ensure that the value set name is unique
            auto set_unique_name_and_add_to_set = [&](const CString& name_candidate)
            {
                dict_value_set.SetName(dictionary.GetUniqueName(name_candidate, NONE, NONE, NONE, NONE, &additional_names_in_use));
                additional_names_in_use.insert(dict_value_set.GetName());
            };

            // if a value set was cut from this item and then pasted on the same item,
            // most likely to reorder the value sets, use the name and label from the original value set
            if( !dict_value_set.GetName().IsEmpty() && dict_item->GetName() == parent_item_name_of_pasted_value_sets )
            {
                set_unique_name_and_add_to_set(dict_value_set.GetName());
            }

            // otherwise default to a name based on the number of value sets and use the item label as the label
            else
            {
                set_unique_name_and_add_to_set(FormatText(_T("%s_VS%d"), (LPCTSTR)dict_item->GetName(), (int)dict_item->GetNumValueSets() + 1));
                dict_value_set.SetLabel(dict_item->GetLabel());
            }

            // fix the numeric value pairs
            value_set_fixer.Fix(dict_value_set);

            // insert the value set
            dict_item->InsertValueSet(value_set_index, std::move(dict_value_set));
            ++value_set_index;
        }
    }


    // update the current language indices
    m_pDict->SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());

    // sync any linked value sets
    if( dict_value_set_for_linked_syncing != nullptr && dict_value_set_for_linked_syncing->IsLinkedValueSet() )
        m_pDict->SyncLinkedValueSets(dict_value_set_for_linked_syncing);

    // Finish up modification
    pDoc->SetModified();

    // Check is dictionary still OK
    m_pDict->BuildNameList();
    pDoc->GetDictionaryValidator()->IsValid(m_pDict, true, true);

    // Update tree
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    pTreeCtrl->SetUpdateAllViews(false);
    DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
    pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, m_iLevel, m_iRec, m_iItem);

    // Update grid
    Update();
    ClearSelections();
    GotoRow(current_row);
    InvalidateRect(NULL);
    SetFocus();
}


void CItemGrid::OnPasteValueSetLink() // 20110118
{
    if( m_bEditing )
        return;

    // code modified from the regular paste function
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    std::vector<DictValueSet> pasted_value_sets = pDoc->GetDictClipboard().GetFromClipboard<DictValueSet>(this);

    if( pasted_value_sets.empty() )
    {
        ASSERT(false);
        return;
    }

    CDictItem* source_dict_item;
    DictValueSet* source_dict_value_set;

    // search for the source value set
    if( !m_pDict->LookupName<DictValueSet>(pasted_value_sets.front().GetName(), nullptr, nullptr, &source_dict_item, &source_dict_value_set) )
    {
        // if the source value set could not be found, paste the value set as as an unlinked value set
        OnEditPaste();
        return;
    }

    // make sure the item's attributes are the same
    CDictItem* dest_dict_item = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec)->GetItem(m_iItem);

    if( dest_dict_item->GetContentType() != source_dict_item->GetContentType() ||
        dest_dict_item->GetLen() != source_dict_item->GetLen() ||
        dest_dict_item->GetDecimal() != source_dict_item->GetDecimal() ||
        dest_dict_item->GetDecChar() != source_dict_item->GetDecChar() )
    {
        AfxMessageBox(_T("You can only paste a value set link if the item's attributes are the same."));
        return;
    }

    if( dest_dict_item == source_dict_item )
    {
        AfxMessageBox(_T("A value set must link to a value set in a different item."));
        return;
    }

    if( source_dict_value_set->IsLinkedValueSet() )
    {
        for( const DictValueSet& this_dict_value_set : dest_dict_item->GetValueSets() )
        {
            if( this_dict_value_set.IsLinkedValueSet() && this_dict_value_set.GetLinkedValueSetCode() == source_dict_value_set->GetLinkedValueSetCode() )
            {
                AfxMessageBox(_T("For any given item you can only have one linkage to a given value set."));
                return;
            }
        }
    }

    // Push old dictionary on stack
    pDoc->PushUndo(*m_pDict, m_iLevel, m_iRec, m_iItem, 0);
    pDoc->SetModified();

    // create the linked value set
    DictValueSet new_dict_value_set;
    new_dict_value_set.GetLabelSet().SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());
    new_dict_value_set.SetLabel(dest_dict_item->GetLabel());

    // use the default name and label and then make it unique
    CString new_name = FormatText(_T("%s_VS%d"), (LPCTSTR)dest_dict_item->GetName(), (int)dest_dict_item->GetNumValueSets() + 1);
    new_name = pDoc->GetDict()->GetUniqueName(new_name);
    new_dict_value_set.SetName(new_name);

    new_dict_value_set.LinkValueSet(*source_dict_value_set);

    dest_dict_item->AddValueSet(std::move(new_dict_value_set));

    // sync any linked value sets
    std::vector<CString> value_set_names_added = { new_name };
    m_pDict->SyncLinkedValueSets(CDataDict::SyncLinkedValueSetsAction::OnPaste, &value_set_names_added);

    // Check is dictionary still OK
    m_pDict->BuildNameList();
    pDoc->GetDictionaryValidator()->IsValid(m_pDict,true,true);

    // Update tree
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    pTreeCtrl->SetUpdateAllViews(false);
    DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
    pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node,m_iLevel,m_iRec,m_iItem);

    // Update grid
    Update();
    ClearSelections();
    GotoRow(-1);
    InvalidateRect(NULL);
    SetFocus();
}


void CItemGrid::OnRemoveValueSetLink(UINT nID) // 20110120
{
    CDictItem* pItem = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec)->GetItem(m_iItem);
    long selrow = GetCurrentRow();
    DictValueSet& dict_value_set = pItem->GetValueSet(m_aValue[selrow].vset);

    // Push old dictionary on stack
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    pDoc->PushUndo(*m_pDict, m_iLevel, m_iRec, m_iItem, selrow);
    pDoc->SetModified();

    if( nID == ID_REMOVE_VS_LINK )
    {
        dict_value_set.UnlinkValueSet();
    }

    else
    {
        ASSERT(nID == ID_REMOVE_VS_ALL_LINKS);

        std::wstring linked_value_set_code = dict_value_set.GetLinkedValueSetCode();
        ASSERT(!linked_value_set_code.empty());

        DictionaryIterator::Foreach<DictValueSet>(*m_pDict,
            [&](DictValueSet& dict_value_set)
            {
                if( dict_value_set.GetLinkedValueSetCode() == linked_value_set_code )
                    dict_value_set.UnlinkValueSet();
            });
    }

    // remove the coloration that signified that it was a linked value set
    Update();
    ClearSelections();
    GotoRow(selrow);
    InvalidateRect(NULL);
    SetFocus();
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::EditDelete
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::EditDelete()
{
    std::vector<long> selected_rows = GetSelectedRows();
    ASSERT(!selected_rows.empty());

    // Push old values on stack
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    pDoc->PushUndo(*m_pDict, m_iLevel, m_iRec, m_iItem, m_aValue[selected_rows.front()].vset, selected_rows.front());

    CDictItem* dict_item = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec)->GetItem(m_iItem);
    std::set<DictValueSet*> updated_dict_value_sets;

    // delete in reverse because somes selected values or value pairs may be part of an entity that is also being deleted
    for( auto row_itr = selected_rows.crbegin(); row_itr != selected_rows.crend(); ++row_itr )
    {
        long selected_row = *row_itr;
        DictValueSet& dict_value_set = dict_item->GetValueSet(m_aValue[selected_row].vset);

        // delete an entire value set
        if( m_aValue[selected_row].value == NONE )
        {
            // remove this as an updated value set for linking, even though values may have been removed,
            // with the assumption that modifications should not impact other linked value sets when
            // the value set is being removed for this item
            updated_dict_value_sets.erase(&dict_value_set);

            dict_item->RemoveValueSet(m_aValue[selected_row].vset);
        }

        else
        {
            updated_dict_value_sets.insert(&dict_value_set);

            // delete an entire value
            if( m_aValue[selected_row].vpair < 1 )
            {
                dict_value_set.RemoveValue(m_aValue[selected_row].value);
            }

            // delete a value pair
            else
            {
                dict_value_set.GetValue(m_aValue[selected_row].value).RemoveValuePair(m_aValue[selected_row].vpair);
            }
        }
    }

    pDoc->SetModified();

    // sync any linked value sets
    for( DictValueSet* updated_dict_value_set : updated_dict_value_sets )
    {
        if( updated_dict_value_set->IsLinkedValueSet() )
            m_pDict->SyncLinkedValueSets(updated_dict_value_set);
    }

    // Check if dictionary still OK
    m_pDict->BuildNameList();
    pDoc->GetDictionaryValidator()->IsValid(m_pDict, true, true);

    // Update tree
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    pTreeCtrl->SetUpdateAllViews(false);
    DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
    pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, m_iLevel, m_iRec, m_iItem);

    // Update grid
    SetRedraw(FALSE);
    Update();
    ClearSelections();

    // Set position
    int row_to_select = std::min((int)selected_rows.front(), (int)GetNumberRows() - 1);
    GotoRow(row_to_select);

    SetRedraw(TRUE);
    InvalidateRect(NULL);
    SetFocus();
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::OnEditModify
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::OnEditModify()
{
    UINT vcKey = VK_RETURN;
    BOOL bProcessed = FALSE;
    // If tree control has focus, position to item on record grid and post delete message
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    POSITION pos = pDoc->GetFirstViewPosition();
    ASSERT(pos != NULL);
    CDDGView* pView = (CDDGView*) pDoc->GetNextView(pos);
    DictTreeNode* dict_tree_node = pTreeCtrl->GetTreeNode(pTreeCtrl->GetSelectedItem());
    if (GetFocus() == pTreeCtrl && dict_tree_node->GetValueSetIndex() == NONE) {
        DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
        int iLevel = pDoc->GetLevel();
        int iRec = pDoc->GetRec();
        int iItem = pDoc->GetItem();
        pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, iLevel, iRec);
        pView->m_iGrid = DictionaryGrid::Record;
        pView->m_gridRecord.SetRedraw(FALSE);
        pView->m_gridRecord.Update(pDoc->GetDict(), iLevel, iRec);
        pView->ResizeGrid();
        pView->m_gridRecord.ClearSelections();
        pView->m_gridRecord.GotoRow(pView->m_gridRecord.GetRow(iItem));
        pView->m_gridRecord.SetRedraw(TRUE);
        pView->m_gridRecord.InvalidateRect(NULL);
        pView->m_gridRecord.SetFocus();
        pView->m_gridRecord.OnCharDown(&vcKey, bProcessed);
        return;
    }
    OnCharDown(&vcKey, bProcessed);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::OnEditNotes
//
/////////////////////////////////////////////////////////////////////////////

void CItemGrid::OnEditNotes()
{
    long row = GetCurrentRow();
    int iLevel = m_iLevel;
    int iRec = m_iRec;
    int iItem = m_iItem;
    CDictItem* dict_item = m_pDict->GetLevel(iLevel).GetRecord(iRec)->GetItem(iItem);
    CIMSAString csTitle, csLabel, csNote;
    csTitle.LoadString(IDS_NOTE_TITLE);
    CNoteDlg dlgNote;

    int iVSet = m_aValue[row].vset;
    DictValueSet& dict_value_set = dict_item->GetValueSet(iVSet);

    if (m_aValue[row].value == NONE) {
        csLabel = dict_value_set.GetLabel().Left(32);
        if (dict_value_set.GetLabel().GetLength() > 32)  {
            csLabel += _T("...");
        }
        csTitle = _T("Value Set: ") + csLabel + csTitle;
        csNote = dict_value_set.GetNote();
        dlgNote.SetTitle(csTitle);
        dlgNote.SetNote(csNote);
        if (dlgNote.DoModal() == IDOK)  {
            if (csNote != dlgNote.GetNote()) {
                CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
                pDoc->PushUndo(*dict_item, m_iLevel, m_iRec, m_iItem, m_aValue[row].vset, row);
                dict_value_set.SetNote(dlgNote.GetNote());
                pDoc->SetModified();
            }
        }
        Update();
        GotoCell(ITEM_SETLABEL_COL, row);
    }
    else {
        int iValue = m_aValue[row].value;
        DictValue& dict_value = dict_value_set.GetValue(iValue);
        csLabel = dict_value.GetLabel().Left(32);
        if (dict_value.GetLabel().GetLength() > 32)  {
            csLabel += _T("...");
        }
        csTitle = _T("Value: ") +csLabel + csTitle;
        csNote = dict_value.GetNote();
        dlgNote.SetTitle(csTitle);
        dlgNote.SetNote(csNote);
        if (dlgNote.DoModal() == IDOK)  {
            if (csNote != dlgNote.GetNote()) {
                CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
                pDoc->PushUndo(*dict_item, m_iLevel, m_iRec, m_iItem, m_aValue[row].vset, row);
                dict_value.SetNote(dlgNote.GetNote());
                pDoc->SetModified();

                // sync any linked value sets
                if( dict_value_set.IsLinkedValueSet() )
                    m_pDict->SyncLinkedValueSets(&dict_value_set);
            }
        }
        Update();
        GotoCell(ITEM_LABEL_COL, row);
    }
    SetFocus();
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::GetRow
//
/////////////////////////////////////////////////////////////////////////////

int CItemGrid::GetRow(int iVSet, int iValue)
{
    for (int r = 0 ; r < m_aValue.GetSize() ; r++) {
        if (m_aValue[r].vset == iVSet && m_aValue[r].value == iValue) {
            return r;
        }
    }
    ASSERT(false);
    return NONE;
}


void CItemGrid::OnEditGenValueSet()
{
    GenerateVSDlg dlg;
    CDDGView* pView = (CDDGView*) GetParent();
    dlg.m_pDoc =  (CDDDoc*) pView->GetDocument();
    dlg.m_pItem = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec)->GetItem(m_iItem);
    if (dlg.DoModal() != IDOK) {
        InvalidateRect(NULL);
        return;
    }
    bool bZeroDec = false;
    if (GetPrivateProfileInt(_T("intl"), _T("iLZero"), 0, _T("WIN.INI")) == 1) {
        bZeroDec = true;
    }

    DictValueSet dict_value_set;
    dict_value_set.SetName(dlg.m_sName);
    dict_value_set.GetLabelSet().SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());
    dict_value_set.SetLabel(dlg.m_sLabel);

    TCHAR pszTemp[30];
    GetPrivateProfileString(_T("intl"), _T("sDecimal"), _T("."), pszTemp, 30, _T("WIN.INI"));
    TCHAR cDec = pszTemp[0];
    GetPrivateProfileString(_T("intl"), _T("sGrouping"), _T(""), pszTemp, 30, _T("WIN.INI"));
    CIMSAString sGrouping = pszTemp;
    int iGroup = 0;
    if (sGrouping == _T("3;0")) {
        iGroup = 3;
    }
    else if (sGrouping == _T("3;2;0")) {
        iGroup = 2;
    }
    GetPrivateProfileString(_T("intl"), _T("sThousand"), _T(""), pszTemp, 30, _T("WIN.INI"));
    CIMSAString sThousand = dlg.m_bUseThousandsSeparator ? pszTemp : CString();
    for (double lower = dlg.m_dFrom ; lower <= dlg.m_dTo ; lower += dlg.m_dInterval) {
        DictValue dict_value;
        dict_value.GetLabelSet().SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());
        double upper = lower + dlg.m_dInterval - dlg.m_dMinInterval;
        if (upper > dlg.m_dTo) {
            upper = dlg.m_dTo;
        }
        CIMSAString sLabel;
        UINT uDec = dlg.m_pItem->GetDecimal();
        UINT uLen = dlg.m_pItem->GetLen();
        if (uDec > 0 && !dlg.m_pItem->GetDecChar()) {
            uLen++;
        }
        CIMSAString sLower = dtoa(lower, pszTemp, dlg.m_pItem->GetDecimal(), cDec, bZeroDec);
        CIMSAString sUpper = dtoa(upper, pszTemp, dlg.m_pItem->GetDecimal(), cDec, bZeroDec);

        // Grouping for lower
        CIMSAString sTemp;
        int iLowLen = sLower.GetLength();
        int iLoc = sLower.Find(cDec);
        if (iLoc >= 0) {
            iLowLen = iLoc;
        }
        if (sLower[0] == '-') {
            sLower = sLower.Mid(1);
            iLowLen--;
            sTemp = _T("-");
        }
        else {
            sTemp = _T("");
        }
        if (iGroup > 0 && iLowLen > 3) {
            if (iGroup == 3) {
                int m = iLowLen % 3;
                if (m > 0) {
                    sTemp += sLower.Left(m) + sThousand;
                    iLowLen -=m;
                    sLower = sLower.Mid(m);
                }
                while (iLowLen > 3) {
                    sTemp += sLower.Left(3) + sThousand;
                    iLowLen -= 3;
                    sLower = sLower.Mid(3);
                }
                sTemp += sLower;
                sLower = sTemp;
            }
            else {
                int m = (iLowLen - 3) % 2;
                if (m > 0) {
                    sTemp += sLower.Left(m) + sThousand;
                    iLowLen -=m;
                    sLower = sLower.Mid(m);
                }
                while (iLowLen > 3) {
                    sTemp += sLower.Left(2) + sThousand;
                    iLowLen -= 2;
                    sLower = sLower.Mid(2);
                }
                sTemp += sLower;
                sLower = sTemp;
            }
        }
        else {
            sLower = sTemp + sLower;
        }
        // Grouping for upper
        int iUpLen = sUpper.GetLength();
        iLoc = sUpper.Find(cDec);
        if (iLoc >= 0) {
            iUpLen = iLoc;
        }
        if (sUpper[0] == '-') {
            sUpper = sUpper.Mid(1);
            iUpLen--;
            sTemp = _T("-");
        }
        else {
            sTemp = _T("");
        }
        if (iGroup > 0 && iUpLen > 3) {
            if (iGroup == 3) {
                int m = iUpLen % 3;
                if (m > 0) {
                    sTemp += sUpper.Left(m) + sThousand;
                    iUpLen -=m;
                    sUpper = sUpper.Mid(m);
                }
                while (iUpLen > 3) {
                    sTemp += sUpper.Left(3) + sThousand;
                    iUpLen -= 3;
                    sUpper = sUpper.Mid(3);
                }
                sTemp += sUpper;
                sUpper = sTemp;
            }
            else {
                int m = (iUpLen - 3) % 2;
                if (m > 0) {
                    sTemp += sUpper.Left(m) + sThousand;
                    iUpLen -=m;
                    sUpper = sUpper.Mid(m);
                }
                while (iUpLen > 3) {
                    sTemp += sUpper.Left(2) + sThousand;
                    iUpLen -= 2;
                    sUpper = sUpper.Mid(2);
                }
                sTemp += sUpper;
                sUpper = sTemp;
            }
        }
        else {
            sUpper = sTemp + sUpper;
        }

        if (dlg.m_dInterval == dlg.m_dMinInterval) {
            sLabel.Format(dlg.m_sTemplate, (LPCTSTR)sLower);
        }
        else {
            sLabel.Format(dlg.m_sTemplate, (LPCTSTR)sLower, (LPCTSTR)sUpper);
        }
        dict_value.SetLabel(sLabel);

        CString from = dtoa(lower, pszTemp, dlg.m_pItem->GetDecimal(), cDec, bZeroDec);
        CString to;

        if( dlg.m_dInterval > dlg.m_dMinInterval )
            to = dtoa(upper, pszTemp, dlg.m_pItem->GetDecimal(), cDec, bZeroDec);

        dict_value.AddValuePair(DictValuePair(from, to));

        dict_value_set.AddValue(std::move(dict_value));
    }

    dlg.m_pDoc->PushUndo(*m_pDict, m_iLevel, m_iRec, m_iItem, 0, NONE);
    dlg.m_pDoc->SetModified();

    dlg.m_pItem->AddValueSet(std::move(dict_value_set));
    m_pDict->BuildNameList();

    // Update tree
    CDDTreeCtrl* pTreeCtrl = dlg.m_pDoc->GetDictTreeCtrl();
    pTreeCtrl->SetUpdateAllViews(false);
    DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*dlg.m_pDoc);
    pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, m_iLevel, m_iRec, m_iItem);

    // Update grid
    SetRedraw(FALSE);
    Update();
//    GotoRow(row);
    SetRedraw(TRUE);
    InvalidateRect(NULL);
}


void CItemGrid::OnMakeFirstValueSet()
{
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    CDictItem* dict_item = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec)->GetItem(m_iItem);
    int cursel = GetCurrentRow();

    // push old dictionary on stack
    pDoc->PushUndo(*m_pDict, m_iLevel, m_iRec, m_iItem, cursel);
    pDoc->SetModified();

    DictValueSet new_first_dict_value_set = std::move(dict_item->GetValueSet(m_aValue[cursel].vset));
    dict_item->RemoveValueSet(m_aValue[cursel].vset);
    dict_item->InsertValueSet(0, std::move(new_first_dict_value_set));

    // Update tree
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    pTreeCtrl->SetUpdateAllViews(false);
    DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
    pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node,m_iLevel,m_iRec,m_iItem);

    // refresh the grid
    Update();
    ClearSelections();
    GotoRow(0);
    InvalidateRect(NULL);
    SetFocus();
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CItemGrid::OnShiftF10
//
/////////////////////////////////////////////////////////////////////////////
void CItemGrid::OnShiftF10()
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


void CItemGrid::OnMergeValues() // 20110901
{
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());

    // push old dictionary on stack
    pDoc->PushUndo(*m_pDict, m_iLevel, m_iRec, m_iItem, GetCurrentRow());
    pDoc->SetModified();

    std::vector<long> selected_rows = GetSelectedRows();
    ASSERT(!selected_rows.empty());

    CDictItem* dict_item = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec)->GetItem(m_iItem);
    DictValueSet& dict_value_set = dict_item->GetValueSet(m_aValue[selected_rows.front()].vset);
    DictValue& main_dict_value = dict_value_set.GetValue(m_aValue[selected_rows.front()].value);

    size_t num_initial_pairs = main_dict_value.GetNumValuePairs();

    for( size_t i = selected_rows.size() - 1; i >= 1; --i )
    {
        DictValue& this_dict_value = dict_value_set.GetValue(m_aValue[selected_rows[i]].value);
        const DictValuePair& this_dict_value_pair = this_dict_value.GetValuePair(m_aValue[selected_rows[i]].vpair);

        main_dict_value.InsertValuePair(num_initial_pairs, this_dict_value_pair);

        if( this_dict_value.GetNumValuePairs() == 1 )
            dict_value_set.RemoveValue(m_aValue[selected_rows[i]].value);

        else
            this_dict_value.RemoveValuePair(m_aValue[selected_rows[i]].vpair);
    }

    // sync any linked value sets
    if( dict_value_set.IsLinkedValueSet() )
        m_pDict->SyncLinkedValueSets(&dict_value_set);

    // refresh the grid
    Update();
    ClearSelections();
    GotoRow(selected_rows.front());
    InvalidateRect(NULL);
    SetFocus();
}


void CItemGrid::OnFormatValueLabels(UINT nID)
{
    std::optional<CString> format_template;

    if( nID == ID_VS_CASE_FORMAT )
    {
        VSLabelRenamerDlg dlg;

        if( dlg.DoModal() != IDOK )
            return;

        format_template = dlg.GetTemplate();
    }

    // format the labels
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    CDictItem* dict_item = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec)->GetItem(m_iItem);
    int cursel = GetCurrentRow();

    // push old dictionary on stack
    pDoc->PushUndo(*m_pDict, m_iLevel, m_iRec, m_iItem, cursel);
    pDoc->SetModified();

    // get the rows selected
    DictValueSet& dict_value_set = dict_item->GetValueSet(m_aValue[GetCurrentRow()].vset);

    for( DictValue& dict_value : dict_value_set.GetValues() )
    {
        CString label = dict_value.GetLabel();

        if( format_template.has_value() )
        {
            label = *format_template;
            label.Replace(_T("%s"), dict_value.GetLabel());

            if( dict_value.HasValuePairs() )
            {
                const DictValuePair& dict_value_pair = dict_value.GetValuePair(0);

                CString str = dict_value_pair.GetFrom();
                label.Replace(_T("%f"), str.Trim());

                str = dict_value_pair.GetTo();
                label.Replace(_T("%t"), str.Trim());
            }
        }

        else if( nID == ID_VS_CASE_UPPER )
        {
            label.MakeUpper();
        }

        else if( nID == ID_VS_CASE_LOWER )
        {
            label.MakeLower();
        }

        else
        {
            int j = 0;
            bool keepProcessing = true;

            while( keepProcessing )
            {
                // find the next first character of a word
                bool foundWord = false;

                while( keepProcessing && !foundWord )
                {
                    if( j == label.GetLength() )
                        keepProcessing = false;

                    else
                    {
                        TCHAR thisChar = label.GetAt(j);

                        if( ( thisChar >= _T('a') && thisChar <= 'z' ) || ( thisChar >= _T('A') && thisChar <= 'Z' ) )
                            foundWord = true;

                        else
                            j++;
                    }
                }

                if( foundWord )
                {
                    if( label.GetAt(j) > 'Z' )
                        label.SetAt(j,label.GetAt(j) - ( _T('a') - 'A' ));

                    if( nID == ID_VS_CASE_MIXED_FIRST_WORD )
                    {
                        keepProcessing = false;
                    }

                    else // go until the end of the word
                    {
                        j++;

                        while( j < label.GetLength() )
                        {
                            TCHAR thisChar = label.GetAt(j);

                            // 20130201 added the / and \ characters
                            if( thisChar == _T(' ') || thisChar == _T('/') || thisChar == _T('\\') )
                                break;

                            j++;
                        }
                    }
                }
            }
        }

        dict_value.SetLabel(label);
    }

    // sync any linked value sets
    if( dict_value_set.IsLinkedValueSet() )
        m_pDict->SyncLinkedValueSets(&dict_value_set);

    // refresh the grid
    Update();
    ClearSelections();
    GotoRow(cursel);
    InvalidateRect(NULL);
    SetFocus();
}


void CItemGrid::OnValuesReplaceValueLabels()
{
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    CDictItem* dict_item = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec)->GetItem(m_iItem);
    int cursel = GetCurrentRow();

    DictValueSet& dict_value_set = dict_item->GetValueSet(m_aValue[GetCurrentRow()].vset);

    if( !dict_value_set.HasValues() )
        return;

    // push old dictionary on stack
    pDoc->PushUndo(*m_pDict, m_iLevel, m_iRec, m_iItem, cursel);
    pDoc->SetModified();

    auto dict_values_itr = dict_value_set.GetValues().begin();
    const auto& dict_values_end = dict_value_set.GetValues().end();

    SO::ForeachLine(WinClipboard::GetText(), true,
        [&](CString line)
        {
            // skip past anything in another column (specified by a tab)
            int tab_pos = line.Find('\t');

            if( tab_pos >= 0 )
                line = line.Left(tab_pos);

            line = line.Trim();

            // only process non-blank lines
            if( !line.IsEmpty() )
            {
                dict_values_itr->SetLabel(line);
                ++dict_values_itr;
            }

            return ( dict_values_itr != dict_values_end );
        });

    // sync any linked value sets
    if( dict_value_set.IsLinkedValueSet() )
        m_pDict->SyncLinkedValueSets(&dict_value_set);

    // refresh the grid
    Update();
    ClearSelections();
    GotoRow(cursel);
    InvalidateRect(NULL);
    SetFocus();
}

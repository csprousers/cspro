//***************************************************************************
//  File name: RecGrid.cpp
//
//  Description:
//       Data Dictionary record properties grid implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#include "StdAfx.h"
#include "Recgrid.h"
#include "StartPositionDlg.h"
#include <zDictO/Definitions.h>
#include <zDictO/ValueSetFixer.h>


namespace
{
    constexpr int REC_NOTE_COL     =  0;
    constexpr int REC_LABEL_COL    =  1;
    constexpr int REC_NAME_COL     =  2;
    constexpr int REC_DATATYPE_COL =  3;
    constexpr int REC_START_COL    =  4;
    constexpr int REC_LEN_COL      =  5;
    constexpr int REC_ITEMTYPE_COL =  6;
    constexpr int REC_OCC_COL      =  7;
    constexpr int REC_DEC_COL      =  8;
    constexpr int REC_DECCHAR_COL  =  9;
    constexpr int REC_ZEROFILL_COL = 10;

    constexpr int REC_NUM_COLS     = REC_ZEROFILL_COL + 1;
}


BEGIN_MESSAGE_MAP(CRecordGrid, CDDGrid)
    ON_COMMAND(ID_EDIT_ADD, OnEditAdd)
    ON_COMMAND(ID_EDIT_INSERT, OnEditInsert)
    ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
    ON_COMMAND(ID_EDIT_CUT, OnEditCut)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
    ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
    ON_COMMAND(ID_EDIT_MAKE_SUBITEMS, OnEditMakeSubitems)
    ON_COMMAND(ID_EDIT_FLATTEN_OCCURRENCES, OnEditFlattenOccurrences)
    ON_COMMAND(ID_EDIT_MODIFY, OnEditModify)
    ON_COMMAND(ID_EDIT_NOTES, OnEditNotes)
    ON_COMMAND(ID_EDIT_OCCURRENCELABELS, OnEditOccLabels)
    ON_COMMAND(ID_SHIFT_F10, OnShiftF10)
END_MESSAGE_MAP()


CRecordGrid::CRecordGrid()
    :   m_iLevel(0),
        m_iRec(0),
        m_iOldStart(0),
        m_iOldLen(0),
        m_pLabelEdit(nullptr),
        m_pNameEdit(nullptr),
        m_pStartEdit(nullptr),
        m_pLenEdit(nullptr),
        m_pDataTypeEdit(nullptr),
        m_pItemTypeEdit(nullptr),
        m_pOccEdit(nullptr),
        m_pDecEdit(nullptr),
        m_pDecCharEdit(nullptr),
        m_pZeroFillEdit(nullptr)
{
}


int CRecordGrid::GetStartColumn()
{
    return REC_START_COL;
}

int CRecordGrid::GetLabelColumn()
{
    return REC_LABEL_COL;
}


std::vector<size_t> CRecordGrid::GetSelectedItems()
{
    std::vector<size_t> selected_items;

    for( long row : GetSelectedRows() )
    {
        if( m_aItem[row].level != NONE )
            selected_items.emplace_back(m_aItem[row].item);
    }

    return selected_items;
}


bool CRecordGrid::GetSelectedItemsAndQueryForSubitems(std::vector<size_t>& selected_items, bool& subitems_too, const TCHAR* action_text)
{
    selected_items = GetSelectedItems();
    ASSERT(!selected_items.empty() && !subitems_too);

    if( selected_items.size() == 1 )
    {
        const CDictRecord* dict_record = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec);
        const CDictItem* dict_item = dict_record->GetItem(selected_items.front());

        if( dict_item->GetItemType() == ItemType::Item && ( selected_items.front() + 1 ) < (size_t)dict_record->GetNumItems() )
        {
            const CDictItem* potential_subitem = dict_record->GetItem(selected_items.front() + 1);

            if( potential_subitem->IsSubitem() )
            {
                int action = AfxMessageBox(FormatText(_T("%s subitems too?"), action_text), MB_YESNOCANCEL);

                if( action == IDCANCEL )
                {
                    return false;
                }

                else if( action == IDYES )
                {
                    subitems_too = true;
                    selected_items.emplace_back(selected_items.front() + 1);

                    for( size_t i = selected_items.front() + 2;
                         i < (size_t)dict_record->GetNumItems() && dict_record->GetItem(i)->IsSubitem();
                         ++i )
                    {
                        selected_items.emplace_back(i);
                    }
                }
            }
        }
    }

    return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::OnSetup
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnSetup()
{
    EnableExcelBorders(FALSE);
    VScrollAlwaysPresent(TRUE);
    SetNumberCols(REC_NUM_COLS, FALSE);
    SetNumberRows(2, FALSE);
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

    set_header(REC_NOTE_COL,     _T("N"));
    set_header(REC_LABEL_COL,    _T("Item Label                "), UG_ALIGNLEFT);
    set_header(REC_NAME_COL,     _T("Item Name  "),                UG_ALIGNLEFT);
    set_header(REC_START_COL,    _T("  Start"),                    UG_ALIGNRIGHT);
    set_header(REC_LEN_COL,      _T("  Len"),                      UG_ALIGNRIGHT);
    set_header(REC_DATATYPE_COL, _T("Data Type    "),              UG_ALIGNLEFT);
    set_header(REC_ITEMTYPE_COL, _T("Item Type  "),                UG_ALIGNLEFT);                                        
    set_header(REC_OCC_COL,      _T("Occ"),                        UG_ALIGNRIGHT);
    set_header(REC_DEC_COL,      _T("Dec"),                        UG_ALIGNRIGHT);
    set_header(REC_DECCHAR_COL,  _T("Dec Char"),                   UG_ALIGNLEFT);
    set_header(REC_ZEROFILL_COL, _T("Zero Fill"),                  UG_ALIGNLEFT);

    m_bAdding = false;
    m_bInserting = false;
    m_bEditing = false;
    GotoCell(REC_LABEL_COL, 0);                         // 15 Jan 2002
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::Size
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::Size(CRect rect)
{
    CIMSAString csWidths = AfxGetApp()->GetProfileString(_T("Data Dictionary"), _T("RecGridWidths"), _T("-1"));
    if (csWidths == _T("-1")) {
        CUGCell cell;
        CUGCellType* pCellType;
        CSize size;

        int iUsed = static_cast<int>(NOTE_WIDTH * (GetDesignerFontZoomLevel() / 100.0));
        SetColWidth(REC_NOTE_COL, iUsed);
        for (int col = REC_ZEROFILL_COL ; col > REC_LABEL_COL ; col--) {
            GetCell(col, HEADER_ROW, &cell);
            pCellType = GetCellType(HEADER_ROW, col);
            pCellType->GetBestSize(GetDC(), &size, &cell);
            SetColWidth(col, size.cx + BORDER_WIDTH);
            iUsed += size.cx + BORDER_WIDTH;
        }
        GetCell(REC_LABEL_COL, HEADER_ROW, &cell);
        pCellType = GetCellType(HEADER_ROW, REC_LABEL_COL);
        pCellType->GetBestSize(GetDC(), &size, &cell);
        if (size.cx > rect.Width() - m_GI->m_vScrollWidth - iUsed - 1) {
            SetColWidth(REC_LABEL_COL, size.cx);
        }
        else {
            SetColWidth(REC_LABEL_COL, rect.Width() - m_GI->m_vScrollWidth - iUsed - 1);
        }
        csWidths.Format(_T("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d"), GetColWidth(REC_NOTE_COL),
                                                                GetColWidth(REC_LABEL_COL),
                                                                GetColWidth(REC_NAME_COL),
                                                                GetColWidth(REC_START_COL),
                                                                GetColWidth(REC_LEN_COL),
                                                                GetColWidth(REC_DATATYPE_COL),
                                                                GetColWidth(REC_ITEMTYPE_COL),
                                                                GetColWidth(REC_OCC_COL),
                                                                GetColWidth(REC_DEC_COL),
                                                                GetColWidth(REC_DECCHAR_COL),
                                                                GetColWidth(REC_ZEROFILL_COL));
        AfxGetApp()->WriteProfileString(_T("Data Dictionary"), _T("RecGridWidths"), csWidths);
    }
    else {
        CIMSAString csWidth = csWidths.GetToken();
        SetColWidth(REC_NOTE_COL, (int) csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(REC_LABEL_COL, (int) csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(REC_NAME_COL, (int) csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(REC_START_COL, (int) csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(REC_LEN_COL, (int) csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(REC_DATATYPE_COL, (int) csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(REC_ITEMTYPE_COL, (int) csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(REC_OCC_COL, (int) csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(REC_DEC_COL, (int) csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(REC_DECCHAR_COL, (int) csWidth.Val());
        csWidth = csWidths.GetToken();
        SetColWidth(REC_ZEROFILL_COL, (int) csWidth.Val());
    }
    Resize(rect);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::Resize
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::Resize(CRect rect)
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
//                        CRecordGrid::OnColSized
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnColSized(int col, int* /*width*/)
{
    if (m_aEditControl.GetSize() > 0) {
        long row = GetCurrentRow();
        CRect rect;
        for (int i = m_iMinCol ; i <= m_iMaxCol ; i++) {
            if (col == REC_DATATYPE_COL && col == i) {
                CString csText;
                m_pDataTypeEdit->GetWindowText(csText);
                delete m_pDataTypeEdit;
                // Create data type edit
                CreateDataTypesCombo(row, csText);
            }
            else if (col == REC_ITEMTYPE_COL && col == i) {
                CString csText;
                m_pItemTypeEdit->GetWindowText(csText);
                delete m_pItemTypeEdit;
                // Create item type edit
                CreateItemTypesCombo(row, csText);
            }
            if (col == REC_DECCHAR_COL && col == i) {
                CString csText;
                m_pDecCharEdit->GetWindowText(csText);
                delete m_pDecCharEdit;
                CreateDecCharCombo(row, csText);
            }
            if (col == REC_ZEROFILL_COL && col == i) {
                CString csText;
                m_pZeroFillEdit->GetWindowText(csText);
                delete m_pZeroFillEdit;
                CreateZeroFillCombo(row, csText);
            }
            else {
                m_aEditControl[i]->SendMessage(WM_SIZE);
            }
            m_aEditControl[i]->SendMessage(WM_PAINT);
        }
        UpdateEnabledEditControls();
    }
    CString csWidths;
    csWidths.Format(_T("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d"), GetColWidth(REC_NOTE_COL),
                                                            GetColWidth(REC_LABEL_COL),
                                                            GetColWidth(REC_NAME_COL),
                                                            GetColWidth(REC_START_COL),
                                                            GetColWidth(REC_LEN_COL),
                                                            GetColWidth(REC_DATATYPE_COL),
                                                            GetColWidth(REC_ITEMTYPE_COL),
                                                            GetColWidth(REC_OCC_COL),
                                                            GetColWidth(REC_DEC_COL),
                                                            GetColWidth(REC_DECCHAR_COL),
                                                            GetColWidth(REC_ZEROFILL_COL));
    AfxGetApp()->WriteProfileString(_T("Data Dictionary"), _T("RecGridWidths"), csWidths);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::Update
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::Update(CDataDict* pDict, int level, int rec)
{
    m_pDict = pDict;
    m_iLevel = level;
    if (m_iLevel >= (int)m_pDict->GetNumLevels()) {          // BMD 30 Jul 2003
        m_iLevel = (int)m_pDict->GetNumLevels() - 1;
    }
    const DictLevel& dict_level = m_pDict->GetLevel(m_iLevel);
    m_iRec = rec;
    if (m_iRec >= dict_level.GetNumRecords()) {
        m_iRec = dict_level.GetNumRecords() - 1;
    }
    Update();
}

void CRecordGrid::Update()
{
    CDictRecord* pRec;
    COLORREF rgb;

    CView* pView = assert_cast<CView*>(GetParent());
    CDDDoc* pDoc = assert_cast<CDDDoc*>(pView->GetDocument());
    // new stuff
    m_aItem.RemoveAll();
    ClearSelections();  // must clear otherwise result of GetCurrentRow can
                        // point to row that no longer exists JH 6/4/07
    ITEMSORT item;
    // Add record type
    item.level = NONE;
    item.rec   = NONE;
    item.item  = RECTYPE;
    item.start =  m_pDict->GetRecTypeStart();
    m_aItem.Add(item);
    // Add id items for each level
    for( size_t level_number = 0; level_number < m_pDict->GetNumLevels(); ++level_number ) {
	    DictLevel& dict_level = m_pDict->GetLevel(level_number);
        item.level = level_number;
        item.rec = COMMON;
        pRec = dict_level.GetIdItemsRec();
        for (int i = 0 ; i < pRec->GetNumItems() ; i++) {
            item.item = i;
            item.start = pRec->GetItem(i)->GetStart();
            m_aItem.Add(item);
        }
    }
    // Add record items for specified record (if not id)
    if (m_iRec != COMMON) {
        item.level = m_iLevel;
        item.rec = m_iRec;
        pRec = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec);
        for (int i = 0 ; i < pRec->GetNumItems() ; i++) {
            item.item = i;
            item.start = pRec->GetItem(i)->GetStart();
            m_aItem.Add(item);
        }
    }
    // Sort items
    SortItems();
    // Display items
    SetNumberRows(m_aItem.GetSize(), FALSE);
    int iRow = 0;
    for (int ir = 0 ; ir < m_aItem.GetSize() ; ir++) {
        int l = m_aItem[ir].level;
        if (l == NONE) {
            UpdateRecTypeItem(iRow);
            iRow++;
            continue;
        }
        int r = m_aItem[ir].rec;
        pRec = m_pDict->GetLevel(l).GetRecord(r);
        int i = m_aItem[ir].item;
        if (pRec->GetItem(i)->GetItemType() == ItemType::Subitem) {
            continue;
        }
        if (m_iRec == COMMON) {
            if (m_iLevel == l) {
                rgb = GetSysColor(COLOR_WINDOWTEXT);
            }
            else {
                rgb = GetSysColor(COLOR_GRAYTEXT);
            }
        }
        else {
            if (r != COMMON) {
                rgb = GetSysColor(COLOR_WINDOWTEXT);
            }
            else {
                rgb = GetSysColor(COLOR_GRAYTEXT);
            }
        }
        UpdateRecItem(*pRec->GetItem(i), iRow, rgb);
        iRow++;
        i++;
        while (i < pRec->GetNumItems()) {
            if (pRec->GetItem(i)->GetItemType() == ItemType::Item) {
                break;
            }
            UpdateRecItem(*pRec->GetItem(i), iRow, rgb);
            iRow++;
            i++;
        }
    }
    // Highlight the first available row
    for (int ir = 0 ; ir < GetNumberRows() ; ir++) {
        CUGCell cell;
        GetCell(REC_LABEL_COL, ir, &cell);
        rgb = cell.GetTextColor();
        if (rgb == GetSysColor(COLOR_WINDOWTEXT)) {
            GotoRow(ir);
            for (int col = REC_LABEL_COL ; col < GetNumberCols() ; col++) {
                GetCell(col, ir, &cell);
                cell.SetHTextColor(GetSysColor(COLOR_WINDOWTEXT));
                cell.SetHBackColor(GetSysColor(COLOR_BTNFACE));
                SetCell(col, ir, &cell);
            }
            break;
        }
    }                                       // BMD  06 Jan 2002

    if (GetFocus() == this) {                               // 27 Oct 2000 -- BMD -- Glenn bug
        GotoCell(REC_LABEL_COL, 0);                         // 02 Dec 2000 -- BMD -- Serpro bug
    }
    else {
        int iLevel = pDoc->GetLevel();
        int iRec = pDoc->GetRec();
        GotoCell(REC_LABEL_COL, 0);                         // GotoCell resets level and record
        pDoc->SetLevel(iLevel);
        pDoc->SetRec(iRec);
    }

    pView = (CView*) GetParent();
    pDoc = (CDDDoc*) pView->GetDocument();
    pDoc->SetItem(0);
    RedrawWindow();
    //force on row change call to fix the property grid refresh when grids change
    OnRowChange(0, 0);
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CRecordGrid::SortItems
//
/////////////////////////////////////////////////////////////////////////////

static inline int compare(const void* elem1, const void* elem2)
{
    if (((ITEMSORT*) elem1)->start == ((ITEMSORT*) elem2)->start) {
        return (((ITEMSORT*) elem1)->item - ((ITEMSORT*) elem2)->item);
    }
    else {
        return (((ITEMSORT*) elem1)->start - ((ITEMSORT*) elem2)->start);
    }
}


void CRecordGrid::SortItems()
{
    qsort(m_aItem.GetData(), m_aItem.GetSize(), sizeof(ITEMSORT), compare);
}

/////////////////////////////////////////////////////////////////////////////
//
//                            CRecordGrid::UpdateRecTypeItem
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::UpdateRecTypeItem(int row)
{
    CIMSAString csTemp;

    QuickSetHTextColor(REC_NOTE_COL,     row, GetSysColor(COLOR_WINDOWTEXT));
    QuickSetHBackColor(REC_NOTE_COL,     row, GetSysColor(COLOR_BTNFACE));
    QuickSetText      (REC_LABEL_COL,    row, _T("(record type)"));
    QuickSetTextColor (REC_LABEL_COL,    row, GetSysColor(COLOR_GRAYTEXT));
    QuickSetText      (REC_NAME_COL,     row, _T(""));
    QuickSetTextColor (REC_NAME_COL,     row, GetSysColor(COLOR_GRAYTEXT));
    csTemp.Str(m_pDict->GetRecTypeStart());
    QuickSetText      (REC_START_COL,    row, csTemp);
    QuickSetAlignment (REC_START_COL,    row, UG_ALIGNRIGHT);
    QuickSetTextColor (REC_START_COL,    row, GetSysColor(COLOR_WINDOWTEXT));
    csTemp.Str(m_pDict->GetRecTypeLen());
    QuickSetText      (REC_LEN_COL,      row, csTemp);
    QuickSetAlignment (REC_LEN_COL,      row, UG_ALIGNRIGHT);
    QuickSetTextColor (REC_LEN_COL,      row, RGB(0,0,0));
    QuickSetText      (REC_DATATYPE_COL, row, _T("Alpha"));
    QuickSetTextColor (REC_DATATYPE_COL, row, GetSysColor(COLOR_GRAYTEXT));
    QuickSetText      (REC_ITEMTYPE_COL, row, _T(""));
    QuickSetTextColor (REC_ITEMTYPE_COL, row, GetSysColor(COLOR_GRAYTEXT));
    QuickSetText      (REC_OCC_COL,      row, _T(""));
    QuickSetTextColor (REC_OCC_COL,      row, GetSysColor(COLOR_GRAYTEXT));
    QuickSetText      (REC_DEC_COL,      row, _T(""));
    QuickSetTextColor (REC_DEC_COL,      row, GetSysColor(COLOR_GRAYTEXT));
    QuickSetText      (REC_DECCHAR_COL,  row, _T(""));
    QuickSetTextColor (REC_DECCHAR_COL,  row, GetSysColor(COLOR_GRAYTEXT));
    QuickSetText      (REC_ZEROFILL_COL, row, _T(""));
    QuickSetTextColor (REC_ZEROFILL_COL, row, GetSysColor(COLOR_GRAYTEXT));
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CRecordGrid::UpdateRecItem
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::UpdateRecItem(const CDictItem& dict_item, int row, COLORREF rgb)
{
    QuickSetCellType  (REC_NOTE_COL, row, m_iButton);
    QuickSetCellTypeEx(REC_NOTE_COL, row, UGCT_BUTTONNOFOCUS);
    QuickSetBackColor (REC_NOTE_COL, row, GetSysColor(COLOR_BTNFACE));
    QuickSetHBackColor(REC_NOTE_COL, row, GetSysColor(COLOR_BTNFACE));

    bool editable = ( rgb == GetSysColor(COLOR_WINDOWTEXT) );
    bool note_defined = !dict_item.GetNote().IsEmpty();

    QuickSetBitmap(REC_NOTE_COL, row, ( editable && note_defined ) ? m_pNoteYes :
                                      ( editable )                 ? m_pNoteNo :
                                      ( note_defined )             ? m_pNoteYesGrayed :
                                                                     m_pNoteNoGrayed);

    auto set_cell = [&](int column, const TCHAR* text, int alignment = 0)
    {
		QuickSetText(column, row, text);
        QuickSetTextColor(column, row, rgb);

		if( alignment != 0 )
			QuickSetAlignment(column, row, alignment);
    };

    set_cell(REC_LABEL_COL, dict_item.GetLabel());

    set_cell(REC_NAME_COL, dict_item.GetName());

    set_cell(REC_START_COL, IntToString(dict_item.GetStart()), UG_ALIGNRIGHT);

    set_cell(REC_LEN_COL, IntToString(dict_item.GetLen()), UG_ALIGNRIGHT);

    set_cell(REC_DATATYPE_COL, ToString(dict_item.GetContentType()));

    set_cell(REC_ITEMTYPE_COL, ( dict_item.GetItemType() == ItemType::Item ) ? _T("Item") : _T("Subitem"));

    set_cell(REC_OCC_COL, IntToString(dict_item.GetOccurs()), UG_ALIGNRIGHT);

    set_cell(REC_DEC_COL, IsNumeric(dict_item) ? IntToString(dict_item.GetDecimal()) : CString(), UG_ALIGNRIGHT);
    set_cell(REC_DECCHAR_COL, ( dict_item.GetDecimal() > 0 ) ? BOOL_TO_TEXT(dict_item.GetDecChar()) : CString());

    set_cell(REC_ZEROFILL_COL, DictionaryRules::CanHaveZeroFill(dict_item.GetContentType()) ? BOOL_TO_TEXT(dict_item.GetZeroFill()) : CString());
}


void CRecordGrid::CreateDataTypesCombo(long row, const CString& currentDataType)
{
    const CDictRecord* dict_record = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec);
    std::vector<ContentType> valid_content_types;

    for( ContentType content_type : GetContentTypesSupportedByDictionary() )
    {
        if( !dict_record->IsIdRecord() || DictionaryRules::CanBeIdItem(content_type) )
        {
            if( m_pDict->EnableBinaryItems() || !IsBinary(content_type) )
                valid_content_types.emplace_back(content_type);
        }
    }

    m_pDataTypeEdit = new CDDComboBox();
    CRect rect;
    GetCellRect(REC_DATATYPE_COL, row, &rect);
    rect.top--;
    rect.left--;
    rect.bottom += (valid_content_types.size() + 1) * m_plf->lfHeight;
    rect.right += 2;
    m_pDataTypeEdit->SetRowCol(row, REC_DATATYPE_COL);
    m_pDataTypeEdit->Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, rect, this, 104);
    m_pDataTypeEdit->SetFont(&m_font);
    m_pDataTypeEdit->SetItemHeight(-1, m_plf->lfHeight);   // sets height for static control and button
    m_pDataTypeEdit->SetItemHeight(0, m_plf->lfHeight);   // sets height for list box entries

    for( ContentType content_type : valid_content_types )
        m_pDataTypeEdit->AddString(ToString(content_type));

    m_pDataTypeEdit->SetCurSel(m_pDataTypeEdit->FindStringExact(0, currentDataType));
    m_aEditControl.SetAt(REC_DATATYPE_COL, (CWnd*)m_pDataTypeEdit);
}


void CRecordGrid::CreateItemTypesCombo(long row, const CString& currentItemType)
{
    m_pItemTypeEdit = new CDDComboBox();
    CRect rect;
    GetCellRect(REC_ITEMTYPE_COL, row, &rect);
    rect.top--;
    rect.left--;
    rect.bottom += 3 * m_plf->lfHeight;
    rect.right += 2;
    m_pItemTypeEdit->SetRowCol(row, REC_ITEMTYPE_COL);
    m_pItemTypeEdit->Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, rect, this, 104);
    m_pItemTypeEdit->SetFont(&m_font);
    m_pItemTypeEdit->SetItemHeight(-1, m_plf->lfHeight);   // sets height for static control and button
    m_pItemTypeEdit->SetItemHeight(0, m_plf->lfHeight);   // sets height for list box entries
    m_pItemTypeEdit->AddString(_T("Item"));
    m_pItemTypeEdit->AddString(_T("Subitem"));
    if (currentItemType == _T("Item")) {
        m_pItemTypeEdit->SetCurSel(0);
    }
    else {
        m_pItemTypeEdit->SetCurSel(1);
    }
    m_aEditControl.SetAt(REC_ITEMTYPE_COL, (CWnd*)m_pItemTypeEdit);
}


void CRecordGrid::CreateDecEdit(long row, const CString& current_dec)
{
    m_pDecEdit = new CNumEdit();
    CRect rect;
    GetCellRect(REC_DEC_COL, row, &rect);
    m_pDecEdit->SetRowCol(row, REC_DEC_COL);
    m_pDecEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 104);
    m_pDecEdit->SetFont(&m_font);
    m_pDecEdit->SetLimitText(1);
    m_pDecEdit->SetWindowText(current_dec);
    m_aEditControl.SetAt(REC_DEC_COL, (CWnd*)m_pDecEdit);
}


void CRecordGrid::CreateDecCharCombo(long row, const CString& current_dec_char)
{
    m_pDecCharEdit = new CDDComboBox();
    CRect rect;
    GetCellRect(REC_DECCHAR_COL, row, &rect);
    rect.top--;
    rect.left--;
    rect.bottom += 3 * m_plf->lfHeight;
    rect.right += 2;
    m_pDecCharEdit->SetRowCol(row, REC_DECCHAR_COL);
    m_pDecCharEdit->Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, rect, this, 104);
    m_pDecCharEdit->SetFont(&m_font);
    m_pDecCharEdit->SetItemHeight(-1, m_plf->lfHeight);   // sets height for static control and button
    m_pDecCharEdit->SetItemHeight(0, m_plf->lfHeight);   // sets height for list box entries
    m_pDecCharEdit->AddString(_T("Yes"));
    m_pDecCharEdit->AddString(_T("No"));
    if (current_dec_char == _T("Yes")) {
        m_pDecCharEdit->SetCurSel(0);
    }
    else if (current_dec_char == _T("No")) {
        m_pDecCharEdit->SetCurSel(1);
    }
    m_aEditControl.SetAt(REC_DECCHAR_COL, (CWnd*)m_pDecCharEdit);
}


void CRecordGrid::CreateZeroFillCombo(long row, const CString& current_zero_fill)
{
    m_pZeroFillEdit = new CDDComboBox();
    CRect rect;
    GetCellRect(REC_ZEROFILL_COL, row, &rect);
    rect.top--;
    rect.left--;
    rect.bottom += 3 * m_plf->lfHeight;
    rect.right += 2;
    m_pZeroFillEdit->SetRowCol(row, REC_ZEROFILL_COL);
    m_pZeroFillEdit->Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, rect, this, 104);
    m_pZeroFillEdit->SetFont(&m_font);
    m_pZeroFillEdit->SetItemHeight(-1, m_plf->lfHeight);   // sets height for static control and button
    m_pZeroFillEdit->SetItemHeight(0, m_plf->lfHeight);   // sets height for list box entries
    m_pZeroFillEdit->AddString(_T("Yes"));
    m_pZeroFillEdit->AddString(_T("No"));
    if (current_zero_fill == _T("Yes")) {
        m_pZeroFillEdit->SetCurSel(0);
    }
    else if (current_zero_fill == _T("No")) {
        m_pZeroFillEdit->SetCurSel(1);
    }
    m_aEditControl.SetAt(REC_ZEROFILL_COL, (CWnd*)m_pZeroFillEdit);
}


void CRecordGrid::CreateOccEdit(long row, const CString& current_occ)
{
    m_pOccEdit = new CNumEdit();
    CRect rect;
    GetCellRect(REC_OCC_COL, row, &rect);
    m_pOccEdit->SetRowCol(row, REC_OCC_COL);
    m_pOccEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 104);
    m_pOccEdit->SetFont(&m_font);
    m_pOccEdit->SetWindowText(current_occ);
    m_aEditControl.SetAt(REC_OCC_COL, (CWnd*)m_pOccEdit);
}


void CRecordGrid::CreateLenEdit(long row, const CString& current_len)
{
    m_pLenEdit = new CNumEdit();
    CRect rect;
    GetCellRect(REC_LEN_COL, row, &rect);
    m_pLenEdit->SetRowCol(row, REC_LEN_COL);
    m_pLenEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 104);
    m_pLenEdit->SetFont(&m_font);
    if (m_aItem[row].level == NONE) {
        m_pLenEdit->SetLimitText(1);
    }
    else {
        m_pLenEdit->SetLimitText(3);
    }
    m_pLenEdit->SetWindowText(current_len);
    m_aEditControl.SetAt(REC_LEN_COL, (CWnd*)m_pLenEdit);
}


void CRecordGrid::CreateStartPositionEdit(long row, const CString& current_start)
{
    m_pStartEdit = new CNumEdit();
    CRect rect;
    GetCellRect(REC_START_COL, row, &rect);
    m_pStartEdit->SetRowCol(row, REC_START_COL);
    m_pStartEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 104);
    m_pStartEdit->SetFont(&m_font);
    m_pStartEdit->SetWindowText(current_start);
    m_aEditControl.SetAt(REC_START_COL, (CWnd*)m_pStartEdit);
}


void CRecordGrid::CreateLabelEdit(long row, const CString& current_label)
{
    m_pLabelEdit = new CLabelEdit();
    m_pLabelEdit->SetRowCol(row, REC_LABEL_COL);
    CRect rect;
    GetCellRect(REC_LABEL_COL, row, &rect);
    m_pLabelEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 100);
    m_pLabelEdit->SetFont(&m_font);
    m_pLabelEdit->SetWindowText(current_label);
    m_aEditControl.SetAt(REC_LABEL_COL, (CWnd*)m_pLabelEdit);
}


void CRecordGrid::CreateNameEdit(long row, const CString& current_name)
{
    m_pNameEdit = new CNameEdit();
    CRect rect;
    GetCellRect(REC_NAME_COL, row, &rect);
    m_pNameEdit->SetRowCol(row, REC_NAME_COL);
    m_pNameEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 101);
    m_pNameEdit->SetFont(&m_font);
    m_pNameEdit->SetWindowText(current_name);
    m_aEditControl.SetAt(REC_NAME_COL, (CWnd*)m_pNameEdit);
}


void CRecordGrid::UpdateEnabledEditControls()
{
    const CDictRecord* dict_record = m_pDict->GetLevel(m_aItem[m_iEditRow].level).GetRecord(m_aItem[m_iEditRow].rec);
    const CDictItem* dict_item = dict_record->GetItem(m_aItem[m_iEditRow].item);

    CString content_type_text;
    m_pDataTypeEdit->GetWindowText(content_type_text);
    ContentType content_type = *FromString<ContentType>(content_type_text);

    if (DictionaryRules::CanModifyLength(content_type)) {
        m_pLenEdit->EnableWindow(TRUE);
    }
    else {
        m_pLenEdit->EnableWindow(FALSE);
        m_pLenEdit->SetWindowText(IntToString(DictionaryDefaults::ItemLen));
    }

    if (DictionaryRules::CanBeSubitem(*dict_record, content_type)) {
        m_pItemTypeEdit->EnableWindow(TRUE);
    }
    else {
        m_pItemTypeEdit->SetCurSel(0);
        m_pItemTypeEdit->EnableWindow(FALSE);
    }

    if (DictionaryRules::CanItemHaveMultipleOccurrences(*dict_record)) {
        m_pOccEdit->EnableWindow(TRUE);
    }
    else {
        ASSERT(dict_item->GetOccurs() == 1);
        m_pOccEdit->EnableWindow(FALSE);
    }

    if (DictionaryRules::CanHaveDecimals(*dict_record, content_type)) {
        m_pDecEdit->EnableWindow(TRUE);
        m_pDecCharEdit->EnableWindow(TRUE);
        m_pDecEdit->SetWindowText(IntToString(dict_item->GetDecimal()));
        m_pDecCharEdit->SetCurSel(dict_item->GetDecChar() ? 0 : 1);
    }
    else {
        m_pDecEdit->EnableWindow(FALSE);
        m_pDecCharEdit->EnableWindow(FALSE);
        m_pDecEdit->SetWindowText(CString());
        m_pDecCharEdit->SetCurSel(-1);
    }

    if (DictionaryRules::CanHaveZeroFill(content_type)) {
        m_pZeroFillEdit->EnableWindow(TRUE);
        m_pZeroFillEdit->SetCurSel(dict_item->GetZeroFill() ? 0 : 1);
    }
    else {
        m_pZeroFillEdit->EnableWindow(FALSE);
        m_pZeroFillEdit->SetCurSel(-1);
    }
}




/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::OnCanMove
//
/////////////////////////////////////////////////////////////////////////////

int CRecordGrid::OnCanMove(int /*oldcol*/, long oldrow, int newcol, long newrow)
{
    CUGCell cell;
    if (newcol == REC_NOTE_COL) {
        GetCell(REC_LABEL_COL, newrow, &cell);
    }
    else {
        GetCell(newcol, newrow, &cell);
    }
    CView* pView = assert_cast<CView*>(GetParent());
    CDDDoc* pDoc = assert_cast<CDDDoc*>(pView->GetDocument());
    if (newrow >= 0 && m_aItem.GetSize() > newrow) {
        if (m_aItem[newrow].item == RECTYPE) {
            pDoc->SetLevel(m_iLevel);
            if (m_iRec >= 0) {
                pDoc->SetRec(m_iRec);
            }
            else {
                pDoc->SetRec(0);
            }
            pDoc->SetItem(NONE);
        }
        else {
            pDoc->SetLevel(m_aItem[newrow].level);
            pDoc->SetRec(m_aItem[newrow].rec);
            pDoc->SetItem(m_aItem[newrow].item);
        }
    }
    COLORREF rgb = cell.GetTextColor();
    if (rgb == GetSysColor(COLOR_GRAYTEXT)) {
        return FALSE;
    }
    if (oldrow != newrow) {                             // BMD 06 Jan 2002
        pDoc->UpdateAllViews((CDDGView*) GetParent());
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::OnLClicked
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnLClicked(int col, long row, int updn, RECT* /*rect*/, POINT* /*point*/, int /*processed*/)
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
        if (col == REC_NOTE_COL) {
            GetCell(REC_LABEL_COL, row, &cell);
        }
        else {
            GetCell(col, row, &cell);
        }
        if (cell.GetTextColor() == GetSysColor(COLOR_GRAYTEXT)) {
            m_bCanEdit = true;
            if (m_bEditing) {
                if (EditEnd()) {
                    m_bAdding = false;
                    m_bInserting = false;
                }
                else {
                    return;
                }
            }
            int iLevel = m_aItem[row].level;
            if (iLevel == NONE) {
                return;
            }
            int iRec = m_aItem[row].rec;
            CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
            DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
            pTreeCtrl->SelectNode(*dictionary_dict_tree_node, false, iLevel, iRec);
            Update();
            ClearSelections();          // BMD 18 Jul 2003
            GotoRow(row);
            SetFocus();
            return;
        }
        if (m_bCanEdit) {
            if (col != REC_NOTE_COL) {
                // Don't start an edit with button up on notes col,
                // it will crash with null m_aEditControl
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
//                        CRecordGrid::OnTH_RClicked
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnTH_RClicked(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed/* = 0*/)
{
    OnRClicked(col, row, updn, rect, point, processed);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::OnCB_RClicked
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnCB_RClicked(int updn, RECT* rect, POINT* point, int processed)
{
    OnRClicked(-1, -1, updn, rect, point, processed);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::OnRClicked
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnRClicked(int /*col*/, long row, int updn, RECT* /*rect*/, POINT* point, int /*processed*/)
{
    if (updn) {
        if (IsEditing()) {
            m_bAdding = false; // 20130411 added these statements so that no new rows would be created in EditChange
            m_bInserting = false;
            EditChange(VK_CANCEL);
        }
        return;
    }
    // Determine whether record type or subitems are included
    bool bIncludesRT = false;
    bool bIncludesSubItem = false;
    CUGCell cell;
    GetCell(REC_LABEL_COL, GetCurrentRow(), &cell);
    const CDictRecord* pRec = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec);
    int selCol;
    long selRow;

    int rtCode = EnumFirstSelected(&selCol, &selRow);
    if (selRow == 0 && selCol == 0) {
        bIncludesRT = true;
    }

    while (rtCode == UG_SUCCESS) {
        if (selCol != REC_LABEL_COL) {
            rtCode = EnumNextSelected(&selCol, &selRow);
            continue;
        }
        long iRows = GetNumberRows();
        if (selRow >= iRows) {
            selRow = iRows - 1;
        }
        GetCell(selCol, selRow, &cell);
        if (GetItem(selRow) == RECTYPE || cell.GetTextColor() == GetSysColor(COLOR_GRAYTEXT)) {
            bIncludesRT = true;
            break;
        }
        if (pRec->GetItem(GetItem(selRow))->GetItemType() == ItemType::Subitem) {
            bIncludesSubItem = true;
        }
        rtCode = EnumNextSelected(&selCol, &selRow);
    }

    BCMenu popMenu;   // BMD 29 Sep 2003
    popMenu.CreatePopupMenu();

    popMenu.AppendMenuItems(!bIncludesRT, { { ID_EDIT_CUT, _T("Cu&t\tCtrl+X") },
                                            { ID_EDIT_COPY, _T("&Copy\tCtrl+C") } });

    CView* pView = assert_cast<CView*>(GetParent());
    CDDDoc* pDoc = assert_cast<CDDDoc*>(pView->GetDocument());

    bool can_paste = pDoc->GetDictClipboard().IsAvailable<CDictItem>();
    popMenu.AppendMenuItems(can_paste, { { ID_EDIT_PASTE, _T("&Paste\tCtrl+V") } });

    popMenu.AppendMenu(MF_SEPARATOR);

    popMenu.AppendMenuItems(!bIncludesRT, { { ID_EDIT_MODIFY, _T("&Modify Item\tCtrl+M") } });

    popMenu.AppendMenu(MF_STRING, ID_EDIT_ADD, _T("&Add Item\tCtrl+A"));

    popMenu.AppendMenuItems(!bIncludesRT, { { ID_EDIT_INSERT, _T("&Insert Item\tIns") },
                                            { ID_EDIT_DELETE, _T("&Delete Item\tDel") } });

    popMenu.AppendMenu(MF_SEPARATOR);

    long currow = GetCurrentRow();
    bool can_edit_notes = ( currow >= 0 && GetItem(currow) != RECTYPE);
    popMenu.AppendMenuItems(can_edit_notes, { { ID_EDIT_NOTES, _T("&Notes...\tCtrl+D") } });

    popMenu.AppendMenu(MF_SEPARATOR);

    bool can_convert_to_subitems = ( !bIncludesRT && !bIncludesSubItem && !pRec->IsIdRecord() );
    popMenu.AppendMenuItems(can_convert_to_subitems, { { ID_EDIT_MAKE_SUBITEMS, _T("Convert to &Subitems") } });

    GetCell(REC_OCC_COL, GetCurrentRow(), &cell);
    CIMSAString csOcc = cell.GetText();

    bool is_single_item_with_occurrences = ( csOcc.Val() > 1 && GetNumberRowsSelected() == 1 );
    popMenu.AppendMenuItems(is_single_item_with_occurrences, { { ID_EDIT_FLATTEN_OCCURRENCES, _T("Flatten &Occurrences") },
                                                               { ID_EDIT_OCCURRENCELABELS, _T("Occurrence &Labels...") } });

    popMenu.LoadToolbar(IDR_DICT_FRAME);   // BMD 29 Sep 2003

    CRect rectWin;
    GetWindowRect(rectWin);
    if (point->x == 0) {
        CRect rectCell;
        GetCellRect(1, selRow, &rectCell);
        point->x = (rectCell.left + rectCell.right) / 2;
        point->y = (rectCell.top + rectCell.bottom) / 2;
    }
    popMenu.TrackPopupMenu(TPM_RIGHTBUTTON, rectWin.left + point->x, rectWin.top + point->y + GetRowHeight(row), this);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::OnDClicked
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnDClicked(int /*col*/, long row, RECT* /*rect*/, POINT* /*point*/, BOOL /*processed*/)
{
    if (row < 0) {
        return;
    }
    if (m_aItem[row].rec < 0) {
        return;
    }
    CView* pView = assert_cast<CView*>(GetParent());
    CDDDoc* pDoc = assert_cast<CDDDoc*>(pView->GetDocument());
    CTreeCtrl* pTreeCtrl = (CTreeCtrl*)pDoc->GetTreeCtrl();
    HTREEITEM hItem = pTreeCtrl->GetSelectedItem();
    hItem = pTreeCtrl->GetChildItem(hItem);
    for (int i = 0 ; i < m_aItem[row].item ; i++) {
        hItem = pTreeCtrl->GetNextSiblingItem(hItem);
    }
    pTreeCtrl->Select(hItem, TVGN_CARET);
    ((CDDGView*) pView)->m_gridItem.PostMessage(WM_SETFOCUS);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::OnKeyDown
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnKeyDown(UINT* vcKey, BOOL /*processed*/)
{
    if (*vcKey==VK_PRIOR) {
        *vcKey = 0;
        if(GetKeyState(VK_CONTROL) < 0) {
            long newrow;
            for (newrow = 0 ; newrow < GetNumberRows() ; newrow++) {
                CUGCell cell;
                GetCell(REC_LABEL_COL, newrow, &cell);
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
                GetCell(REC_LABEL_COL, newrow, &cell);
                if (cell.GetTextColor() != GetSysColor(COLOR_GRAYTEXT)) {
                    break;
                }
            }
            for ( ; newrow < GetNumberRows() ; newrow++) {
                CUGCell cell;
                GetCell(REC_LABEL_COL, newrow, &cell);
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
                GetCell(REC_LABEL_COL, newrow, &cell);
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
                GetCell(REC_LABEL_COL, newrow, &cell);
                if (cell.GetTextColor() != GetSysColor(COLOR_GRAYTEXT)) {
                    break;
                }
            }
            for ( ; newrow > 0 ; newrow--) {
                CUGCell cell;
                GetCell(REC_LABEL_COL, newrow, &cell);
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
//                        CRecordGrid::OnCharDown
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnCharDown(UINT* vcKey, BOOL /*processed*/)
{
    if (*vcKey==VK_TAB) {
        AfxGetMainWnd()->SendMessage(WM_IMSA_SETFOCUS);
    }
    else if (*vcKey == VK_ESCAPE) {
        CView* pView = assert_cast<CView*>(GetParent());
        CDDDoc* pDoc = assert_cast<CDDDoc*>(pView->GetDocument());
        CTreeCtrl* pTreeCtrl = (CTreeCtrl*)pDoc->GetTreeCtrl();
        HTREEITEM hItem = pTreeCtrl->GetSelectedItem();
        hItem = pTreeCtrl->GetParentItem(hItem);
        pTreeCtrl->Select(hItem, TVGN_CARET);
        ((CDDGView*) pView)->m_gridLevel.PostMessage(WM_SETFOCUS);
    }
    else if (*vcKey >= 32) {
        EditBegin(REC_LABEL_COL, GetCurrentRow(), *vcKey);
    }
    else if (*vcKey == VK_RETURN) {
        if (m_aItem[GetCurrentRow()].level != NONE) {
            EditBegin(REC_LABEL_COL, GetCurrentRow(), 0);
        }
        else {
            EditBegin(REC_START_COL, GetCurrentRow(), 0);
        }
    }
    else if (*vcKey == 10) {
        int col;
        long row;
        EnumFirstSelected(&col, &row);
        int iItem = m_aItem[row].item;
        if (iItem >= 0) {
            CDDGView* pView = (CDDGView*) GetParent();
            CDDDoc* pDoc = assert_cast<CDDDoc*>(pView->GetDocument());
            CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
            DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
            pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, m_iLevel, m_iRec, iItem);

            pView->m_iGrid = DictionaryGrid::Item;
            pView->m_gridItem.SetRedraw(FALSE);
            pView->m_gridItem.Update(pDoc->GetDict(), m_iLevel, m_iRec, iItem, 0);
            pView->ResizeGrid();
            pView->m_gridItem.ClearSelections();
            pView->m_gridItem.GotoRow(0);
            pView->m_gridItem.SetRedraw(TRUE);
            pView->m_gridItem.InvalidateRect(NULL);
            pView->m_gridItem.SetFocus();
            if (pView->m_gridItem.GetNumberRows() == 0) {
                pView->m_gridItem.SendMessage(WM_COMMAND, ID_EDIT_ADD);
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::OnCellTypeNotify
//
/////////////////////////////////////////////////////////////////////////////

int CRecordGrid::OnCellTypeNotify(long /*ID*/, int col, long row, long msg, long /*param*/)
{
    CUGCell cell;
    GetCell(REC_LABEL_COL, row, &cell);
    if (cell.GetTextColor() == GetSysColor(COLOR_GRAYTEXT)) {
        return FALSE;
    }
    if(msg == UGCT_BUTTONCLICK){
        if(col == REC_NOTE_COL) {
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
//                        CRecordGrid::EditBegin
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::EditBegin(int col, long row, UINT vcKey)
{
    ASSERT(row >= 0);
    ASSERT(col >= 0);
    GotoCol(col);
    VScrollEnable(ESB_DISABLE_BOTH);
    if (!(m_bAdding || m_bInserting) && (vcKey == 0 || vcKey >= 32)) {
        int iItem = m_aItem[row].item;
        CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
        pDoc->PushUndo(*m_pDict, m_iLevel, m_iRec, iItem);
    }
    CUGCell cell;
    CRect rect;
    CIMSAString cs;
    m_aEditControl.SetSize(REC_ZEROFILL_COL + 1);
    m_bEditing = true;
    m_iEditRow = row;
    ClearSelections();

    if (m_aItem[row].level == NONE) {
        if (!(col == REC_START_COL || col == REC_LEN_COL)) {
            col = REC_START_COL;
        }
    }
    else {
        // Create label edit
        GetCell(REC_LABEL_COL, row, &cell);
        cell.GetText(&cs);
        CreateLabelEdit(row, cs);

        // Create name edit
        GetCell(REC_NAME_COL, row, &cell);
        cell.GetText(&cs);
        CreateNameEdit(row, cs);
        if (!m_bAdding && !m_bInserting) {   // BMD 20 Sep 2005
            m_pDict->SetOldName(cs);
        }
    }

    // Create start edit
    GetCell(REC_START_COL, row, &cell);
    cell.GetText(&cs);
    CreateStartPositionEdit(row, cs);
    m_iOldStart = (int) cs.Val();

    // Create len edit
    GetCell(REC_LEN_COL, row, &cell);
    cell.GetText(&cs);
    CreateLenEdit(row, cs);

    if (m_aItem[row].level != NONE) {
        // Create data type edit
        GetCell(REC_DATATYPE_COL, row, &cell);
        cell.GetText(&cs);
        CreateDataTypesCombo(row, cs);

        // Create item type edit
        GetCell(REC_ITEMTYPE_COL, row, &cell);
        cell.GetText(&cs);
        CreateItemTypesCombo(row, cs);

        // Create occ edit
        GetCell(REC_OCC_COL, row, &cell);
        cell.GetText(&cs);
        CreateOccEdit(row, cs);

        // Create dec edit
        GetCell(REC_DEC_COL, row, &cell);
        cell.GetText(&cs);
        CreateDecEdit(row, cs);

        // Create dec char combo
        GetCell(REC_DECCHAR_COL, row, &cell);
        cell.GetText(&cs);
        CreateDecCharCombo(row, cs);

        // Create zero fill combo
        GetCell(REC_ZEROFILL_COL, row, &cell);
        cell.GetText(&cs);
        CreateZeroFillCombo(row, cs);

        UpdateEnabledEditControls();
    }


    // Set focus to field
    if (m_aItem[row].level != NONE) {
        m_iMinCol = REC_LABEL_COL;
        m_iMaxCol = REC_ZEROFILL_COL;
    }
    else {
        m_iMinCol = REC_START_COL;
        m_iMaxCol = REC_LEN_COL;
    }
    if (!m_aEditControl[col]->IsWindowEnabled()) {
        col = REC_LABEL_COL;
    }
    m_iEditCol = col;
    m_aEditControl[col]->SetFocus();
    if (vcKey >= 32) {
        if (m_aItem[row].level != NONE) {
            m_pLabelEdit->SendMessage(WM_CHAR, vcKey, 1);
        }
        else {
            m_pStartEdit->SendMessage(WM_CHAR, vcKey, 1);       // BMD 23 Feb 2005
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::EditEnd
//
/////////////////////////////////////////////////////////////////////////////

bool CRecordGrid::EditEnd(bool bSilent)
{
    bool bChanged = false;
    bool bChangedName = false;
    bool bValid = true;
    bool bUpdateTree = false;
    bool bUndo = false;
    bool changes_will_break_linked_value_sets = false; // 20110121
    std::map<CString, int> broken_linked_value_sets;
    int iLevel = m_aItem[m_iEditRow].level;
    int iRec = m_aItem[m_iEditRow].rec;
    int iItem = m_aItem[m_iEditRow].item;
    CString csNewName, csOldName;   // BMD 20 Sep 2005
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    DictionaryValidator* dictionary_validator = pDoc->GetDictionaryValidator();

    if (iLevel == NONE) {
        CIMSAString csNewRTStart;
        m_aEditControl[REC_START_COL]->GetWindowText(csNewRTStart);
        UINT uNewRTStart = (UINT) csNewRTStart.Val();
        UINT uOldRTStart = m_pDict->GetRecTypeStart();
        if (uNewRTStart != uOldRTStart) {
            bChanged = true;
            m_pDict->SetRecTypeStart(uNewRTStart);
        }
        CIMSAString csNewRTLen;
        m_aEditControl[REC_LEN_COL]->GetWindowText(csNewRTLen);
        UINT uNewRTLen = (UINT) csNewRTLen.Val();
        UINT uOldRTLen = m_pDict->GetRecTypeLen();
        if (uNewRTLen != uOldRTLen) {
            bChanged = true;
            m_pDict->SetRecTypeLen(uNewRTLen);
        }
        if (bChanged) {
            if (m_pDict->IsPosRelative()) {
                dictionary_validator->AdjustStartPositions();
            }
            bValid = dictionary_validator->IsValid(m_pDict, bSilent);
            if (bValid) {
                QuickSetText(REC_START_COL, m_iEditRow, csNewRTStart);
                QuickSetText(REC_LEN_COL, m_iEditRow, csNewRTLen);
                pDoc->SetModified();
                for( size_t level_number = 0; level_number < m_pDict->GetNumLevels(); ++level_number ) {
	                DictLevel& dict_level = m_pDict->GetLevel(level_number);
                    for (int r = 0 ; r < dict_level.GetNumRecords() ; r++) {
                        CDictRecord* pRec = dict_level.GetRecord(r);
                        CIMSAString csRecTypeVal = pRec->GetRecTypeVal();
                        pRec->SetRecTypeVal(csRecTypeVal.AdjustLenLeft(m_pDict->GetRecTypeLen(), ZERO));
                        pRec->SetRecLen(dictionary_validator->GetRecordLength(level_number, r));
                    }
                }
            }
            else {
                m_pDict->SetRecTypeStart(uOldRTStart);
                m_pDict->SetRecTypeLen(uOldRTLen);
            }
        }
    }
    else {
        CDictItem* pItem = m_pDict->GetLevel(iLevel).GetRecord(iRec)->GetItem(iItem);
        CString csNewLabel, csOldLabel;
        m_aEditControl[REC_LABEL_COL]->GetWindowText(csNewLabel);
        csOldLabel = pItem->GetLabel();
        if (csNewLabel.Compare(csOldLabel) != 0) {
            bChanged = true;
            pItem->SetLabel(csNewLabel);
        }

        m_aEditControl[REC_NAME_COL]->GetWindowText(csNewName);
        csOldName = pItem->GetName();
        if (csNewName.Compare(csOldName) != 0) {
            bChanged = true;
            bChangedName = true;
            pItem->SetName(csNewName);
        }

        if (csNewLabel.IsEmpty() && csNewName.IsEmpty()) {
            bChanged = true;
        }

        CIMSAString csNewStart;
        m_aEditControl[REC_START_COL]->GetWindowText(csNewStart);
        UINT uNewStart = (UINT) csNewStart.Val();
        UINT uOldStart = pItem->GetStart();
        if (uNewStart != uOldStart) {
            bChanged = true;
            pItem->SetStart(uNewStart);
        }

        CIMSAString csNewLen;
        m_aEditControl[REC_LEN_COL]->GetWindowText(csNewLen);
        UINT uNewLen = (UINT) csNewLen.Val();
        UINT uOldLen = pItem->GetLen();
        if (uNewLen != uOldLen) {
            bChanged = true;
            changes_will_break_linked_value_sets = true;
            pItem->SetLen(uNewLen);
        }

        CString csNewDataType;
        m_aEditControl[REC_DATATYPE_COL]->GetWindowText(csNewDataType);
        ContentType contentTypeNew = *FromString<ContentType>(csNewDataType);

        ContentType contentTypeOld = pItem->GetContentType();
        if (contentTypeNew != contentTypeOld) {
            bChanged = true;
            changes_will_break_linked_value_sets = true;
            pItem->SetContentType(contentTypeNew);
        }

        CString csNewItemType;
        m_aEditControl[REC_ITEMTYPE_COL]->GetWindowText(csNewItemType);
        ItemType itNew = ItemType::Item;
        if (csNewItemType == _T("Subitem")) {
            itNew = ItemType::Subitem;
        }

        ItemType itOld = pItem->GetItemType();
        if (itNew != itOld) {
            bChanged = true;
            pItem->SetItemType(itNew);
        }

        CIMSAString csNewOcc;
        m_aEditControl[REC_OCC_COL]->GetWindowText(csNewOcc);
        UINT uNewOcc = (UINT) csNewOcc.Val();
        UINT uOldOcc = pItem->GetOccurs();
        if (uNewOcc != uOldOcc) {
            bChanged = true;
            pItem->SetOccurs(uNewOcc);
        }

        UINT uOldDec = pItem->GetDecimal();
        bool bOldDecChar = pItem->GetDecChar();
        bool bOldZeroFill = pItem->GetZeroFill();
        CIMSAString csNewDec;
        CString csNewDecChar;
        CString csNewZeroFill;

        if (IsNumeric(pItem->GetContentType())) {
            m_aEditControl[REC_DEC_COL]->GetWindowText(csNewDec);
            UINT uNewDec = (UINT) csNewDec.Val();
            if (uNewDec != uOldDec) {
                bChanged = true;
                changes_will_break_linked_value_sets = true;
                pItem->SetDecimal(uNewDec);
            }

            m_aEditControl[REC_DECCHAR_COL]->GetWindowText(csNewDecChar);
            bool bNewDecChar = false;
            if (csNewDecChar == _T("Yes")) {
                bNewDecChar = true;
            }
            if (bNewDecChar != bOldDecChar) {
                bChanged = true;
                changes_will_break_linked_value_sets = true;
                pItem->SetDecChar(bNewDecChar);
            }
            if (bNewDecChar) {
                if (pItem->GetLen() < 2 ) {
                    bChanged = true;
                    pItem->SetDecChar(false);
                }
            }

            m_aEditControl[REC_ZEROFILL_COL]->GetWindowText(csNewZeroFill);
            bool bNewZeroFill = false;
            if (csNewZeroFill == _T("Yes")) {
                bNewZeroFill = true;
            }
            if (bNewZeroFill != bOldZeroFill) {
                bChanged = true;
                pItem->SetZeroFill(bNewZeroFill);
            }
        }
        else {
            // Non-numeric items
            pItem->SetDecimal(0);
            pItem->SetDecChar(false);
            pItem->SetZeroFill(false);
        }

        if (m_bAdding || m_bInserting) {
            if (pItem->GetLabel().IsEmpty() && pItem->GetName().IsEmpty()) {
                bUndo = true;
                m_bAdding = false;
                m_bInserting = false;
                bChanged = false;
                bValid = true;
            }
        }

        if (bChanged) {
            // Check for start/len change and subitems      BMD 04 Sep 2002
            bool bChgSubItemStart = false;
            if (itOld == ItemType::Item && itOld == itNew && (uNewStart != (UINT) m_iOldStart || uNewLen != uOldLen || uNewOcc != uOldOcc) ) {  // Chirag 10 Mar 2003
                CDictRecord* pRec = m_pDict->GetLevel(iLevel).GetRecord(iRec);
                for (int iSubItem = iItem + 1 ; iSubItem < pRec->GetNumItems() ; iSubItem++) {
                    CDictItem* pSubItem = pRec->GetItem(iSubItem);
                    if (pSubItem->GetItemType() == ItemType::Item) {
                        break;
                    }
                    if (uNewStart != (UINT) m_iOldStart) {
                        bChgSubItemStart = true;
                    }
                    if (uNewLen >= (UINT) m_iOldLen) {
                        break;
                    }
                    if ((pSubItem->GetStart() + pSubItem->GetLen() * pSubItem->GetOccurs()) > (m_iOldStart + uNewLen)) {  // Chirag 10 Mar 2003

                        CString csMsg;
                        csMsg.Format(_T("Subitem: %s extends beyond end of the item.\nChange subitem length, then change item length."), (LPCTSTR)pSubItem->GetName());
                        AfxMessageBox(csMsg);
                        bValid = false;
                        dictionary_validator->SetInvalidEdit(REC_LEN_COL);  // Chirag 10 Mar 2003
                    }
                }
            }
            // Check for value set validity
            if( bValid && !DictionaryRules::CanHaveValueSet(*pItem) && pItem->HasValueSets() ) {
                AfxMessageBox(FormatText(_T("Items of type %s cannot have value sets. Delete them before changing the data type."), ToString(pItem->GetContentType())));
                bValid = false;
                dictionary_validator->SetInvalidEdit(REC_DATATYPE_COL);
            }
            if (bValid) {
                // if any of the value sets were linked, those linkages must be removed because the value set type is different
                if( changes_will_break_linked_value_sets )
                {
                    for( DictValueSet& dict_value_set : pItem->GetValueSets() )
                    {
                        if( dict_value_set.IsLinkedValueSet() )
                        {
                            int remaining_links = (int)m_pDict->CountValueSetLinks(dict_value_set) - 1;

                            if( remaining_links >= 1 )
                                broken_linked_value_sets.try_emplace(dict_value_set.GetName(), remaining_links);

                            dict_value_set.UnlinkValueSet();
                        }
                    }
                }

                if (m_pDict->IsPosRelative()) {
                    bValid = dictionary_validator->IsValidPre(pItem, iLevel, iRec, iItem, false);
                    if (bValid) {
                        // Reposition item to correct serial position
                        int iItemOld = iItem;
                        CDictRecord* pRec = m_pDict->GetLevel(iLevel).GetRecord(iRec);
                        int iStart = pItem->GetStart();
                        if (iStart != m_iOldStart) {
                            int iIndex = dictionary_validator->FindNewPos(iStart, iItem, pRec);
                            CDictItem item = *pRec->GetItem(iItem);
                            if (iIndex < iItem) {
                                pRec->RemoveItemAt(iItem);
                                pRec->InsertItemAt(iIndex, &item);
                                iItem = iIndex;
                            }
                            else if (iIndex > iItem) {
                                pRec->InsertItemAt(iIndex, &item);
                                pRec->RemoveItemAt(iItem);
                                iItem = iIndex - 1;
                            }
                            // Move rest of subitems
                            if (bChgSubItemStart) {
                                pItem = m_pDict->GetLevel(iLevel).GetRecord(iRec)->GetItem(iItem);
                                if (iIndex < iItemOld) {
                                    for (int iSubItem = iItemOld + 1 ; iSubItem < pRec->GetNumItems(); iSubItem++) {
                                        CDictItem subItem = *pRec->GetItem(iSubItem);
                                        if (subItem.GetItemType() == ItemType::Item) {
                                            break;
                                        }
                                        subItem.SetStart(pItem->GetStart() + (subItem.GetStart() - m_iOldStart));
                                        iIndex++;
                                        pRec->RemoveItemAt(iSubItem);
                                        pRec->InsertItemAt(iIndex, &subItem);
                                    }
                                }
                                else if (iIndex > iItemOld) {
                                    for (int iSubItem = iItemOld ; iSubItem < pRec->GetNumItems(); ) {
                                        CDictItem subItem = *pRec->GetItem(iSubItem);
                                        if (subItem.GetItemType() == ItemType::Item) {
                                            break;
                                        }
                                        subItem.SetStart(pItem->GetStart() + (subItem.GetStart() - m_iOldStart));
                                        pRec->InsertItemAt(iIndex, &subItem);
                                        pRec->RemoveItemAt(iSubItem);
                                    }
                                }
                            }
                        }
                        pItem = m_pDict->GetLevel(iLevel).GetRecord(iRec)->GetItem(iItem);
                        m_pDict->BuildNameList();
                        dictionary_validator->AdjustStartPositions();
                        bValid = dictionary_validator->IsValid(pItem, iLevel, iRec, iItem, bSilent, true);  // BMD 05 Mar 2003
                    }
                }
                else {  // Absolute
                    bValid = dictionary_validator->IsValid(pItem, iLevel, iRec, iItem, bSilent);
                    if (bValid && bChgSubItemStart) {
                        CDictRecord* pRec = m_pDict->GetLevel(iLevel).GetRecord(iRec);
                        for (int iSubItem = iItem + 1 ; iSubItem < pRec->GetNumItems(); iSubItem++) {
                            CDictItem* pSubItem = pRec->GetItem(iSubItem);
                            if (pSubItem->GetItemType() == ItemType::Item) {
                                break;
                            }
                            pSubItem->SetStart(uNewStart + (pSubItem->GetStart() - m_iOldStart));
                        }
                    }
                }
            }

            if (bValid) {
                // if name changed, rename value sets
                pItem = m_pDict->GetLevel(iLevel).GetRecord(iRec)->GetItem(iItem);
                m_pDict->BuildNameList();             // Mike problem with names 19 Oct 2000

                if (csNewName.Compare(csOldName) != 0) {
                    int v = 0;
                    for( DictValueSet& dict_value_set : pItem->GetValueSets() ) {
                        CString csName = FormatText(_T("%s_VS%d"), (LPCTSTR)pItem->GetName(), v + 1);
                        dict_value_set.SetName(m_pDict->GetUniqueName(csName));
                        m_pDict->UpdateNameList(dict_value_set, iLevel, iRec, iItem, v);
                        ++v;
                    }
                }
                pDoc->SetModified();
                QuickSetText(REC_LABEL_COL, m_iEditRow, csNewLabel);
                QuickSetText(REC_NAME_COL, m_iEditRow, csNewName);
                QuickSetText(REC_START_COL, m_iEditRow, csNewStart);
                QuickSetText(REC_LEN_COL, m_iEditRow, csNewLen);
                QuickSetText(REC_DATATYPE_COL, m_iEditRow, csNewDataType);
                QuickSetText(REC_ITEMTYPE_COL, m_iEditRow, csNewItemType);
                QuickSetText(REC_OCC_COL, m_iEditRow, csNewOcc);
                QuickSetText(REC_DEC_COL, m_iEditRow, csNewDec);
                QuickSetText(REC_DECCHAR_COL, m_iEditRow, csNewDecChar);
                QuickSetText(REC_ZEROFILL_COL, m_iEditRow, csNewZeroFill);

                // fix numeric value pairs
                CDictChildWnd* pChildWnd = assert_cast<CDictChildWnd*>(assert_cast<CMDIFrameWnd*>(AfxGetMainWnd())->MDIGetActive());
                ValueSetFixer value_set_fixer(*pItem, pChildWnd->GetDecimal());
                value_set_fixer.Fix(pItem->GetValueSets());

                dictionary_validator->IsValid(pItem, iLevel, iRec, iItem, true, true);
            }
            else {
                pItem->SetLabel(csOldLabel);
                pItem->SetName(csOldName);
                pItem->SetStart(uOldStart);
                pItem->SetLen(uOldLen);
                pItem->SetContentType(contentTypeOld);
                pItem->SetItemType(itOld);
                pItem->SetOccurs(uOldOcc);
                pItem->SetDecimal(uOldDec);
                pItem->SetDecChar(bOldDecChar);
                pItem->SetZeroFill(bOldZeroFill);
            //  dictionary_validator->IsValidPre(pItem, iLevel, iRec, iItem, false);
            }
        }
        if (bChanged && bValid) {
            if (csOldLabel != csNewLabel) {
                if (pItem->GetNumValueSets() == 1) {
                    pItem->GetValueSet(0).SetLabel(csNewLabel);
                }
                else if (pItem->GetNumValueSets() > 1) {
                    if (AfxMessageBox(_T("Make first value set label the same as this item label?"), MB_YESNO) == IDYES) {
                        pItem->GetValueSet(0).SetLabel(csNewLabel);
                    }
                }
                bUpdateTree = true;
            }
            if (itOld != itNew) {
                bUpdateTree = true;
            }
            if (uOldOcc != uNewOcc) {
                bUpdateTree = true;
            }
            if (uOldStart != uNewStart) {
                dictionary_validator->Sort(iLevel, iRec);
            }
        }
    }
    if (bValid) {
        EditQuit();
        if (m_bAdding || m_bInserting) {
            m_pDict->BuildNameList();
        }
        else if (iLevel != NONE) {
            m_pDict->UpdateNameList(iLevel, iRec);
        }
        CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
        if (bUpdateTree || m_bInserting) {
            DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
            pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, iLevel, iRec);
        }
        pTreeCtrl->InvalidateRect(NULL);
        int row = GetCurrentRow();
        pDoc->UpdateAllViews(NULL);
        ClearSelections();
        GotoRow(row);
        OnSetFocus(1);
//        InvalidateRect(NULL);
    }
    else {
        m_iEditCol = dictionary_validator->GetInvalidEdit();
        if (m_iEditCol < 0) {   // BMD 04 Sep 2002
            m_iEditCol = 4;
        }
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
        const CDictItem* dict_item = m_pDict->LookupName<CDictItem>(csNewName);
        m_pDict->SetChangedObject(dict_item);
        AfxGetMainWnd()->PostMessage(UWM::Dictionary::NameChange, (WPARAM)pDoc);
    }

    for( const auto& [value_set_name, remaining_links] : broken_linked_value_sets )
    {
        AfxMessageBox(FormatText(_T("The link of value set '%s' to %d other value sets was removed due to item changes"),
                                    (LPCTSTR)value_set_name, remaining_links));
    }

    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::EditQuit
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::EditQuit()
{
    SetRedraw(FALSE);
    if (m_aItem[m_iEditRow].level != NONE) {
        delete m_pLabelEdit;
        delete m_pNameEdit;
    }
    delete m_pStartEdit;
    delete m_pLenEdit;
    if (m_aItem[m_iEditRow].level != NONE) {
        delete m_pDataTypeEdit;
        delete m_pItemTypeEdit;
        delete m_pOccEdit;
        delete m_pDecEdit;
        delete m_pDecCharEdit;
        delete m_pZeroFillEdit;
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

void CRecordGrid::EditChange(UINT uChar)
{
    CDDGrid::EditChange(uChar);
    switch (uChar) {
        case DD_ONSEL:
            if (m_iEditCol == REC_DATATYPE_COL) {
                UpdateEnabledEditControls();
            }
        break;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::OnSetFocus
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnSetFocus(int /*section*/)
{
    CDDGView* pView = (CDDGView*) GetParent();
    CSplitterWnd* pSplitWnd = (CSplitterWnd*) pView->GetParent();
    pSplitWnd->SetActivePane(0,0);
    for (long row = 0 ; row < GetNumberRows() ; row++) {
        for (int col = REC_LABEL_COL ; col < GetNumberCols() ; col++) {
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
//                        CRecordGrid::OnKillFocus
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnKillFocus(int /*section*/)
{
    m_bCanEdit = false;

    int col;
    long row;
    if (EnumFirstSelected(&col, &row) != UG_SUCCESS) {
        return;
    }

    for (col = REC_LABEL_COL ; col < GetNumberCols() ; col++) {
        QuickSetHTextColor(col, row, GetSysColor(COLOR_WINDOWTEXT));
        QuickSetHBackColor(col, row, GetSysColor(COLOR_BTNFACE));
    }
    RedrawRow(row);
    while (EnumNextSelected(&col, &row) == UG_SUCCESS) {
        for (col = REC_LABEL_COL ; col < GetNumberCols() ; col++) {
            QuickSetHTextColor(col, row, GetSysColor(COLOR_WINDOWTEXT));
            QuickSetHBackColor(col, row, GetSysColor(COLOR_BTNFACE));
        }
        RedrawRow(row);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::GetRow
//
/////////////////////////////////////////////////////////////////////////////

int CRecordGrid::GetRow(int iItem)
{
    if (iItem == RECTYPE) {
        for (int r = 0 ; r < m_aItem.GetSize() ; r++) {
            if (m_aItem[r].item == RECTYPE) {
                return r;
            }
        }
    }
    else {
        int i = NONE;
        for (int r = 0 ; r < m_aItem.GetSize() ; r++ ) {
            CUGCell cell;
            GetCell(REC_LABEL_COL, r, &cell);
            if (cell.GetTextColor() != GetSysColor(COLOR_GRAYTEXT)) {
                i++;
            }
            if (i == iItem) {
                return r;
            }
        }
    }
    ASSERT(false);
    return NONE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::OnEditUndo
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnEditUndo()
{
    ASSERT(m_bEditing);
    ASSERT(m_iEditCol == REC_LABEL_COL || m_iEditCol == REC_NAME_COL ||
            m_iEditCol == REC_START_COL || m_iEditCol == REC_LEN_COL ||
            m_iEditCol == REC_OCC_COL || m_iEditCol == REC_DEC_COL);
    assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->Undo();
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::OnEditCut
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnEditCut()
{
    if (m_bEditing) {
        ASSERT(m_iEditCol == REC_LABEL_COL || m_iEditCol == REC_NAME_COL ||
                m_iEditCol == REC_START_COL || m_iEditCol == REC_LEN_COL ||
                m_iEditCol == REC_OCC_COL || m_iEditCol == REC_DEC_COL);
        assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->Cut();
        return;
    }
    EditCopy(true);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::OnEditCopy
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnEditCopy()
{
    if (m_bEditing) {
        ASSERT(m_iEditCol == REC_LABEL_COL || m_iEditCol == REC_NAME_COL ||
                m_iEditCol == REC_START_COL || m_iEditCol == REC_LEN_COL ||
                m_iEditCol == REC_OCC_COL || m_iEditCol == REC_DEC_COL);
        assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->Copy();
        return;
    }
    EditCopy(false);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::OnEditPaste
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnEditPaste()
{
    // If editing, paste to the control
    if( m_bEditing )
    {
        ASSERT(m_iEditCol == REC_LABEL_COL || m_iEditCol == REC_NAME_COL ||
               m_iEditCol == REC_START_COL || m_iEditCol == REC_LEN_COL ||
               m_iEditCol == REC_OCC_COL || m_iEditCol == REC_DEC_COL);
        assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->Paste();
        return;
    }

    // Can paste only items
    CDictRecord* dict_record = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec);

    // Find insertion point
    long current_row = GetCurrentRow();
    long item_number = m_aItem[current_row].item;

    // if on the record type row, insert the item at the first position
    if( m_iLevel != m_aItem[current_row].level || m_iRec != m_aItem[current_row].rec )
        item_number = 0;

    unsigned start_position = 1;

    // If relative, make any adjustment to starting item position
    if( m_pDict->IsPosRelative() )
    {
        if( item_number < (long)dict_record->GetNumItems() )
            start_position = dict_record->GetItem(item_number)->GetStart();

        if( item_number == ( (long)dict_record->GetNumItems() - 1 ) )
        {
            BeforeAfterDlg dlg;

            if( dlg.DoModal() != IDOK )
                return;

            if( dlg.SelectedAfter() )
                ++item_number;
        }
    }

    // If absolute, calculate trial starting position from current row
    else
    {        
        if( dict_record->GetNumItems() > 0 )
        {
            bool offset_by_one = ( item_number != 0 &&
                                   item_number != ( (long)dict_record->GetNumItems() - 1 ) );
            const CDictItem* dict_item = dict_record->GetItem(item_number - ( offset_by_one ? 1 : 0 ));
            start_position = dict_item->GetStart() + dict_item->GetLen() * dict_item->GetOccurs();
        }

        // Ask for starting position
        StartPositionDlg dlg(start_position);

        if( dlg.DoModal() != IDOK )
            return;

        start_position = dlg.GetStartPosition();

        // Find item position of given starting position
        for( item_number = 0;
             item_number < (long)dict_record->GetNumItems() && dict_record->GetItem(item_number)->GetStart() < start_position;
             ++item_number )
        {
        }
    }

    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    DictPastedValues<CDictItem> pasted_items = pDoc->GetDictClipboard().GetNamedElementsFromClipboard<CDictItem>(this);

    if( pasted_items.values.empty() )
        return;

    // Push old dictionary on stack
    int undo_item_pos = ( dict_record->GetNumItems() == 0 ) ? RECTYPE : ( dict_record->GetNumItems() - 1 );
    pDoc->PushUndo(*m_pDict, m_iLevel, m_iRec, undo_item_pos);


    // insert the items, doing an initial adjustment of the start positions;
    // the dictionary validator will fix any start positions after the paste
    bool first_item = true;

    for( CDictItem& dict_item : pasted_items.values )
    {
        if( first_item )
        {
            // adjust the start position based on the first of the pasted items
            start_position -= dict_item.GetStart();
            first_item = false;

            // 22 Feb 2002 ... if the first pasted item is a subitem being pasted on an
            // item, paste it into the item (rather than before the item)
            if( dict_item.IsSubitem() && item_number < (long)dict_record->GetNumItems() && !dict_record->GetItem(item_number)->IsSubitem() )
                ++item_number;
        }

        dict_item.SetStart(start_position + dict_item.GetStart());

        dict_record->InsertItemAt(item_number, &dict_item);
        ++item_number;
    }

    // update the current language indices
    m_pDict->SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());

    // sync any linked value sets
    m_pDict->SyncLinkedValueSets(CDataDict::SyncLinkedValueSetsAction::OnPaste, &pasted_items.value_set_names_added);

    // Finish up modification
    pDoc->SetModified();

    if( m_pDict->IsPosRelative()  )
        pDoc->GetDictionaryValidator()->AdjustStartPositions();

    // Check is dictionary still OK
    m_pDict->BuildNameList();

    // roll back the changes if they are not valid
    if( !pDoc->GetDictionaryValidator()->IsValid(m_pDict, false, true) )
    {
        pDoc->UndoChange(false);
    }

    else
    {
        CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
        DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
        pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, m_iLevel, m_iRec);
    }

    // Update grid
    Update();
    ClearSelections();
    GotoRow(current_row);
    InvalidateRect(NULL);
    SetFocus();
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::OnEditAdd
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnEditAdd()
{
    // If tree control has focus, position to item on level grid and post add message
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    if (GetFocus() == pTreeCtrl) {
        POSITION pos = pDoc->GetFirstViewPosition();
        ASSERT(pos != NULL);
        CDDGView* pView = (CDDGView*) pDoc->GetNextView(pos);
        DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
        int iLevel = pDoc->GetLevel();
        int iRec = pDoc->GetRec();
        pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, iLevel);
        pView->m_iGrid = DictionaryGrid::Level;
        pView->m_gridLevel.SetRedraw(FALSE);
        pView->m_gridLevel.Update(pDoc->GetDict(), iLevel);
        pView->ResizeGrid();
        pView->m_gridLevel.ClearSelections();
        pView->m_gridLevel.GotoRow(pView->m_gridLevel.GetFirstRow() + iRec);
        pView->m_gridLevel.SetRedraw(TRUE);
        pView->m_gridLevel.InvalidateRect(NULL);
        pView->m_gridLevel.SetFocus();
        pView->m_gridLevel.PostMessage(WM_COMMAND, ID_EDIT_ADD);
        return;
    }

    m_bAdding = true;

    // Push old rec on stack
    CDictRecord* pRec = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec);
    if (pRec->GetNumItems() == 0) {
        pDoc->PushUndo(*m_pDict, m_iLevel, m_iRec, RECTYPE);
    }
    else {
        pDoc->PushUndo(*m_pDict, m_iLevel, m_iRec, pRec->GetNumItems() - 1);
    }

    // Calculate next starting position
    int iNewStart = pDoc->GetDictionaryValidator()->GetDefaultItemStart(m_iLevel, m_iRec, pRec->GetNumItems());
//    int iItem = pRec->GetNumItems() - 1;
//    int iNewStart = pRec->GetItem(iItem)->GetStart() + pRec->GetItem(iItem)->GetLen();

    // Insert record
    CDictItem item;
    item.GetLabelSet().SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());
    item.SetStart(iNewStart);
    item.SetZeroFill(m_pDict->IsZeroFill());
    item.SetDecChar(m_pDict->IsDecChar());

    int iItem = pRec->GetNumItems();
    pRec->AddItem(&item);

    // If necessary, remove all gaps
    if (m_pDict->IsPosRelative()) {
        pDoc->GetDictionaryValidator()->AdjustStartPositions();
    }

    // Update tree
//    pTreeCtrl->SetUpdateAllViews(false);
    DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
    pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, m_iLevel, m_iRec);

    // Update grid
    Update();
    int row = GetRow(iItem);
    GotoRow(row);
//    InvalidateRect(NULL);

    // Begin editing new item
    EditBegin(REC_LABEL_COL, row, 0);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::OnEditInsert
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnEditInsert()
{
    // If tree control has focus, position to item on level grid and post delete message
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    if (GetFocus() == pTreeCtrl) {
        POSITION pos = pDoc->GetFirstViewPosition();
        ASSERT(pos != NULL);
        CDDGView* pView = (CDDGView*) pDoc->GetNextView(pos);
        DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
        int iLevel = pDoc->GetLevel();
        int iRec = pDoc->GetRec();
        pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, iLevel);
        pView->m_iGrid = DictionaryGrid::Level;
        pView->m_gridLevel.SetRedraw(FALSE);
        pView->m_gridLevel.Update(pDoc->GetDict(), iLevel);
        pView->ResizeGrid();
        pView->m_gridLevel.ClearSelections();
        pView->m_gridLevel.GotoRow(pView->m_gridLevel.GetFirstRow() + iRec);
        pView->m_gridLevel.SetRedraw(TRUE);
        pView->m_gridLevel.InvalidateRect(NULL);
        pView->m_gridLevel.SetFocus();
        pView->m_gridLevel.PostMessage(WM_COMMAND, ID_EDIT_INSERT);
        return;
    }

    m_bInserting = true;

    // Find the selected item
    int iItem = m_aItem[GetCurrentRow()].item;

    // Push old rec on stack
    CDictRecord* pRec = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec);
    pDoc->PushUndo(*pRec, m_iLevel, m_iRec, iItem);

    // Calculate starting position
    int iNewStart;
    if (m_pDict->IsPosRelative()) {
        iNewStart = pRec->GetItem(iItem)->GetStart();
    }
    else {
        if (iItem > 0) {
            CDictItem* pItem = pRec->GetItem(iItem);
            if (pItem->GetItemType() == ItemType::Item) {
                CDictItem* pPrevItem = pRec->GetItem(iItem - 1);
                iNewStart = pPrevItem->GetStart() + (pPrevItem->GetLen() * pPrevItem->GetOccurs());
            }
            else {
                iNewStart = pRec->GetItem(iItem)->GetStart();
            }
        }
        else {
            iNewStart = pRec->GetItem(iItem)->GetStart();
        }
    }

    // Insert item
    CDictItem item;
    item.GetLabelSet().SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());
    item.SetStart(iNewStart);
    item.SetItemType(pRec->GetItem(iItem)->GetItemType());  // BMD 21 Feb 2002
    item.SetZeroFill(m_pDict->IsZeroFill());
    item.SetDecChar(m_pDict->IsDecChar());
    pRec->InsertItemAt(iItem, &item);

    // Update tree
//    pTreeCtrl->SetUpdateAllViews(false);
    DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
    pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, m_iLevel, m_iRec);

    // Update grid
    Update();
    int row = GetRow(iItem);
    GotoRow(row);
//    InvalidateRect(NULL);

    // Begin editing new item
    EditBegin(REC_LABEL_COL, row, 0);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::OnEditDelete
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnEditDelete()
{
    // If editing, clear edit control
    if (m_bEditing) {
        ASSERT(m_iEditCol == REC_LABEL_COL || m_iEditCol == REC_NAME_COL ||
                m_iEditCol == REC_START_COL || m_iEditCol == REC_LEN_COL ||
                m_iEditCol == REC_OCC_COL || m_iEditCol == REC_DEC_COL);
        assert_cast<CEdit*>(m_aEditControl[m_iEditCol])->Clear();
        return;
    }
    // If tree control has focus, position to item on level grid and post delete message
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    if (GetFocus() == pTreeCtrl) {
        POSITION pos = pDoc->GetFirstViewPosition();
        ASSERT(pos != NULL);
        CDDGView* pView = (CDDGView*) pDoc->GetNextView(pos);
        DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
        int iLevel = pDoc->GetLevel();
        int iRec = pDoc->GetRec();
        pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, iLevel);
        pView->m_iGrid = DictionaryGrid::Level;
        pView->m_gridLevel.SetRedraw(FALSE);
        pView->m_gridLevel.Update(pDoc->GetDict(), iLevel);
        pView->ResizeGrid();
        pView->m_gridLevel.ClearSelections();
        pView->m_gridLevel.GotoRow(pView->m_gridLevel.GetFirstRow() + iRec);
        pView->m_gridLevel.SetRedraw(TRUE);
        pView->m_gridLevel.InvalidateRect(NULL);
        pView->m_gridLevel.SetFocus();
        pView->m_gridLevel.PostMessage(WM_COMMAND, ID_EDIT_DELETE);
        return;
    }

    std::vector<size_t> selected_items;
    bool subitems_too = false;

    if( !GetSelectedItemsAndQueryForSubitems(selected_items, subitems_too, _T("Delete")) )
        return;

    EditDelete(selected_items, subitems_too);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::EditCopy
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::EditCopy(bool bCut)
{
    std::vector<size_t> selected_items;
    bool subitems_too = false;

    if( !GetSelectedItemsAndQueryForSubitems(selected_items, subitems_too, bCut ? _T("Cut") : _T("Copy")) )
        return;

    // Copy items to clipboard
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    const CDictRecord* dict_record = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec);
    std::vector<const CDictItem*> dict_items;

    for( long selected_item : selected_items )
        dict_items.emplace_back(dict_record->GetItem(selected_item));

    pDoc->GetDictClipboard().PutOnClipboard(this, dict_items);

    // If cut, delete items
    if( bCut )
        EditDelete(selected_items, subitems_too);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::EditDelete
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::EditDelete(const std::vector<size_t>& selected_items, bool subitems_too)
{
    ASSERT(!selected_items.empty());

    CDictRecord* dict_record = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec);

    // Push old record on stack
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    pDoc->PushUndo(*m_pDict, m_iLevel, m_iRec, selected_items.front());

    // Delete items
    for( auto item_itr = selected_items.crbegin(); item_itr != selected_items.crend(); ++item_itr )
    {
        bool promote_subitems_to_items = ( !subitems_too && !dict_record->GetItem(*item_itr)->IsSubitem() );

        dict_record->RemoveItemAt(*item_itr);

        // if deleting an item but not the subitems, turn the subitems into items
        if( promote_subitems_to_items )
        {
            for( size_t i = *item_itr; i < (size_t)dict_record->GetNumItems(); ++i )
            {
                CDictItem* potential_subitem = dict_record->GetItem(i);

                if( !potential_subitem->IsSubitem() )
                    break;

                potential_subitem->SetItemType(ItemType::Item);
            }
        }
    }

    pDoc->SetModified();

    // If necessary, remove all gaps
    if( m_pDict->IsPosRelative() )
        pDoc->GetDictionaryValidator()->AdjustStartPositions();

    // Check if dictionary still OK
    m_pDict->BuildNameList();
    pDoc->GetDictionaryValidator()->IsValid(m_pDict, true, true);

    // Set position
    int item_to_select = std::min((int)selected_items.front(), dict_record->GetNumItems() - 1);
    long row_to_select = GetRow(item_to_select);

    // Update tree
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
    pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, m_iLevel, m_iRec);

    // Update grid
    Update();
    ClearSelections();
    GotoRow(row_to_select);
    InvalidateRect(NULL);
    SetFocus();
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::OnEditMakeSubitems
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnEditMakeSubitems()
{
    std::vector<size_t> selected_items = GetSelectedItems();

    CDictRecord* dict_record = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec);

    // Find first item and its starting position
    const CDictItem* first_dict_item = dict_record->GetItem(selected_items.front());
    int iNewStart = first_dict_item->GetStart();

    // Calculate length
    int iLastStart = iNewStart;
    int iLastLen = first_dict_item->GetLen();

    for( auto item_itr = selected_items.cbegin() + 1; item_itr != selected_items.cend(); ++item_itr )
    {
        const CDictItem* dict_item = dict_record->GetItem(*item_itr);
        iLastStart = dict_item->GetStart();
        iLastLen = dict_item->GetLen() * dict_item->GetOccurs();  // Chirag 10 Mar 2003
    }

    for( size_t i = selected_items.back() + 1; i < (size_t)dict_record->GetNumItems(); ++i )
    {
        const CDictItem* dict_item = dict_record->GetItem(i);

        if( dict_item->GetItemType() == ItemType::Item )
            break;

        iLastStart = dict_item->GetStart();
        iLastLen = dict_item->GetLen() * dict_item->GetOccurs();   // Chirag 10 Mar 2003
    }

    int iNewLen = iLastStart - iNewStart + iLastLen;

    // Push old rec on stack
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    pDoc->PushUndo(*dict_record, m_iLevel, m_iRec, selected_items.front());

    // Change items to subitems
    for( size_t selected_item : selected_items )
        dict_record->GetItem(selected_item)->SetItemType(ItemType::Subitem);

    // Insert parent item
    CDictItem parent_dict_item;
    parent_dict_item.GetLabelSet().SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());
    parent_dict_item.SetStart(iNewStart);
    parent_dict_item.SetLen(iNewLen);
    parent_dict_item.SetContentType(ContentType::Alpha);
    dict_record->InsertItemAt(selected_items.front(), &parent_dict_item);

    // Update tree
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
    pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, m_iLevel, m_iRec);

    // Update grid
    Update();
    long row_to_select = GetRow(selected_items.front());
    GotoRow(row_to_select);

    // Begin editing new item
    EditBegin(REC_LABEL_COL, row_to_select, 1);       // Inserting a single line
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::OnEditModify
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnEditModify()
{
    UINT vcKey = VK_RETURN;
    BOOL bProcessed = FALSE;
    // If tree control has focus, position to item on record grid and post delete message
    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    POSITION pos = pDoc->GetFirstViewPosition();
    ASSERT(pos != NULL);
    CDDGView* pView = (CDDGView*) pDoc->GetNextView(pos);
    if (GetFocus() == pTreeCtrl) {
        DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
        int iLevel = pDoc->GetLevel();
        int iRec = pDoc->GetRec();
        pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, iLevel);
        pView->m_iGrid = DictionaryGrid::Level;
        pView->m_gridLevel.SetRedraw(FALSE);
        pView->m_gridLevel.Update(pDoc->GetDict(), iLevel);
        pView->ResizeGrid();
        pView->m_gridLevel.ClearSelections();
        pView->m_gridLevel.GotoRow(pView->m_gridLevel.GetFirstRow() + iRec);
        pView->m_gridLevel.SetRedraw(TRUE);
        pView->m_gridLevel.InvalidateRect(NULL);
        pView->m_gridLevel.SetFocus();
        pView->m_gridLevel.OnCharDown(&vcKey, bProcessed);
        return;
    }
    OnCharDown(&vcKey, bProcessed);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::OnEditNotes
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnEditNotes()
{
    long row = GetCurrentRow();
    int iLevel = m_aItem[row].level;
    int iRec = m_aItem[row].rec;
    int iItem = m_aItem[row].item;
    CDictItem* pItem = m_pDict->GetLevel(iLevel).GetRecord(iRec)->GetItem(iItem);
    CString csTitle, csLabel, csNote;
    csTitle.LoadString(IDS_NOTE_TITLE);
    csLabel = pItem->GetLabel().Left(32);
    if (pItem->GetLabel().GetLength() > 32)  {
        csLabel += _T("...");
    }
    csTitle = _T("Item: ")+ csLabel + csTitle;
    csNote = pItem->GetNote();
    CNoteDlg dlgNote;
    dlgNote.SetTitle(csTitle);
    dlgNote.SetNote(csNote);
    if (dlgNote.DoModal() == IDOK)  {
        if (csNote != dlgNote.GetNote()) {
            CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
            pDoc->PushUndo(*pItem, m_iLevel, m_iRec, iItem);
            pItem->SetNote(dlgNote.GetNote());
            pDoc->SetModified();
        }
    }
    Update();
    GotoCell(REC_LABEL_COL, row);
    SetFocus();
}

void CRecordGrid::OnEditOccLabels()
{
    GetParent()->SendMessage(WM_COMMAND, ID_EDIT_OCCURRENCELABELS);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::OnShiftF10
//
/////////////////////////////////////////////////////////////////////////////
void CRecordGrid::OnShiftF10()
{
    int col = 0;
    long row = 0;
    RECT rect = RECT();
    POINT point = POINT();
    int processed = 0;

    // 20130411 now simulates the actions of starting and ending a right-click
    OnRClicked(col, row, 1, &rect, &point, processed);

    if( !IsEditing() )
        OnRClicked(col, row, 0, &rect, &point, processed);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::OnEditFlattenOccurrences
//
/////////////////////////////////////////////////////////////////////////////

void CRecordGrid::OnEditFlattenOccurrences() // 20130224
{
    int col;
    long row;
    EnumFirstSelected(&col, &row);
    int iItem = m_aItem[row].item;

    CDDDoc* pDoc = assert_cast<CDDDoc*>(assert_cast<CView*>(GetParent())->GetDocument());
    CDictRecord* dict_record = m_pDict->GetLevel(m_iLevel).GetRecord(m_iRec);
    CDictItem* dict_item = dict_record->GetItem(iItem);

    int occurrences = (int)dict_item->GetOccurs();
    ASSERT(occurrences != 1);

    pDoc->PushUndo(*m_pDict, m_iLevel, m_iRec, iItem);
    pDoc->SetModified();


    auto create_unique_name = [&](CString base_name, int zero_based_occurrence)
    {
        base_name.AppendFormat(_T("_%d"), zero_based_occurrence + 1);
        return pDoc->GetDict()->GetUniqueName(base_name);
    };

    CString base_item_name = dict_item->GetName();

    // case 1: a repeating item with subitems
    if( dict_item->GetItemType() == ItemType::Item && ( ( iItem + 1 ) < dict_record->GetNumItems() ) && dict_record->GetItem(iItem + 1)->GetItemType() == ItemType::Subitem )
    {
        int iEndSubitem = iItem + 1;

        while( ( ( iEndSubitem + 1 ) < dict_record->GetNumItems() ) && dict_record->GetItem(iEndSubitem + 1)->GetItemType() == ItemType::Subitem )
            iEndSubitem++;

        int iNumSubitems = iEndSubitem - iItem;
        int total_items_per_occurrence = 1 + iNumSubitems;

        // rename the first parent occurrence and its subitems
        dict_item->SetName(create_unique_name(base_item_name, 0));
        dict_item->SetOccurs(1);

        std::vector<CString> base_subitem_names;

        for( int k = 0; k < iNumSubitems; k++ )
        {
            CDictItem* pSubitem = dict_record->GetItem(iItem + k + 1);
            base_subitem_names.emplace_back(pSubitem->GetName());
            pSubitem->SetName(create_unique_name(pSubitem->GetName(), 0));
        }

        // add items and subitems for occurrences 2+
        for( int occurrence = 1; occurrence < occurrences; ++occurrence )
        {
            // the parent item
            CDictItem new_parent_dict_item(*dict_item);

            new_parent_dict_item.SetName(create_unique_name(base_item_name, occurrence));
            new_parent_dict_item.SetStart(dict_item->GetStart() + occurrence * dict_item->GetLen());

            dict_record->InsertItemAt(iItem + occurrence * total_items_per_occurrence, &new_parent_dict_item);

            // the subitems
            for( int k = 0; k < iNumSubitems; k++ )
            {
                CDictItem* pSubitem = dict_record->GetItem(iItem + k + 1);
                CDictItem new_subitem(*pSubitem);

                new_subitem.SetName(create_unique_name(base_subitem_names[k], occurrence));
                new_subitem.SetStart(pSubitem->GetStart() + occurrence * dict_item->GetLen());

                dict_record->InsertItemAt(iItem + occurrence * total_items_per_occurrence + k + 1, &new_subitem);
            }
        }

        // rename and link the value sets
        std::vector<std::tuple<CString, DictValueSet*>> base_value_sets_and_names;
        
        for( int occurrence = 0; occurrence < occurrences; ++occurrence )
        {
            size_t value_set_counter = 0;

            for( int i = 0; i < total_items_per_occurrence; ++i )
            {
                CDictItem* this_dict_item = dict_record->GetItem(iItem + occurrence * total_items_per_occurrence + i);

                for( size_t v = 0; v < this_dict_item->GetNumValueSets(); ++v )
                {
                    DictValueSet& this_dict_value_set = this_dict_item->GetValueSet(v);

                    if( occurrence == 0 )
                    {
                        ASSERT(value_set_counter == base_value_sets_and_names.size());
                        base_value_sets_and_names.emplace_back(this_dict_value_set.GetName(), &this_dict_value_set);
                    }

                    else
                    {
                        this_dict_value_set.LinkValueSet(*std::get<1>(base_value_sets_and_names[value_set_counter]));
                    }

                    this_dict_value_set.SetName(create_unique_name(std::get<0>(base_value_sets_and_names[value_set_counter]), occurrence));

                    ++value_set_counter;
                }
            }
        }
    }


    // case 2: a repeating item with no subitems, or a repeating subitem
    else
    {
        // rename the first occurrence
        dict_item->SetName(create_unique_name(base_item_name, 0));
        dict_item->SetOccurs(1);

        // add items for occurrences 2+
        for( int occurrence = 1; occurrence < occurrences; ++occurrence )
        {
            CDictItem new_dict_item(*dict_item);

            new_dict_item.SetName(create_unique_name(base_item_name, occurrence));
            new_dict_item.SetStart(dict_item->GetStart() + occurrence * dict_item->GetLen());

            dict_record->InsertItemAt(iItem + occurrence, &new_dict_item);
        }

        // rename and link the value sets
        std::vector<std::tuple<CString, DictValueSet*>> base_value_sets_and_names;

        for( int occurrence = 0; occurrence < occurrences; ++occurrence )
        {
            CDictItem* this_dict_item = dict_record->GetItem(iItem + occurrence);

            for( size_t v = 0; v < this_dict_item->GetNumValueSets(); ++v )
            {
                DictValueSet& this_dict_value_set = this_dict_item->GetValueSet(v);

                if( occurrence == 0 )
                {
                    base_value_sets_and_names.emplace_back(this_dict_value_set.GetName(), &this_dict_value_set);
                }

                else
                {
                    this_dict_value_set.LinkValueSet(*std::get<1>(base_value_sets_and_names[v]));
                }

                this_dict_value_set.SetName(create_unique_name(std::get<0>(base_value_sets_and_names[v]), occurrence));
            }                
        }
    }


    // Update tree
    CDDTreeCtrl* pTreeCtrl = pDoc->GetDictTreeCtrl();
    DictionaryDictTreeNode* dictionary_dict_tree_node = pTreeCtrl->GetDictionaryTreeNode(*pDoc);
    pTreeCtrl->ReBuildTree(*dictionary_dict_tree_node, m_iLevel, m_iRec);

    // Update grid
    Update();
    ClearSelections();
    GotoRow(row);
    InvalidateRect(NULL);
    SetFocus();
}

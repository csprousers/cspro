//***************************************************************************
//  File name: DDGrid.cpp
//
//  Description:
//       Data Dictionary base grid implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              10 Nov 99   bmd     Created for CSPro 2.0
//
//***************************************************************************

#include "StdAfx.h"
#include "DDGrid.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BEGIN_MESSAGE_MAP(CDDGrid,CUGCtrl)
    //{{AFX_MSG_MAP(CDDGrid)
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDGrid::SetBitmaps
//
/////////////////////////////////////////////////////////////////////////////

CDDGrid::CDDGrid()
{
    m_bAdding = FALSE;
    m_bInserting = FALSE;
    m_bEditing = FALSE;
    m_iEditCol = NONE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDGrid::SetBitmaps
//
/////////////////////////////////////////////////////////////////////////////

void CDDGrid::SetBitmaps(CBitmap* pNoteNo, CBitmap* pNoteYes, CBitmap* pNoteNoGrayed, CBitmap* pNoteYesGrayed) {

    m_pNoteNo = pNoteNo;
    m_pNoteYes = pNoteYes;
    m_pNoteNoGrayed = pNoteNoGrayed;
    m_pNoteYesGrayed = pNoteYesGrayed;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDGrid::EditChange
//
/////////////////////////////////////////////////////////////////////////////

void CDDGrid::EditChange(UINT uChar) {

    CDDGView* pView = (CDDGView*) GetParent();
    CDDDoc* pDoc = assert_cast<CDDDoc*>(pView->GetDocument());
    DictionaryValidator* dictionary_validator = pDoc->GetDictionaryValidator();
    bool bAdding;
    bool bInserting;
    long row;
    switch(uChar)  {
    case VK_ESCAPE:
        EditQuit();
        m_bAdding = FALSE;
        m_bInserting = FALSE;
        if (!(GetCurrentRow() == 0 && pView->m_iGrid == DictionaryGrid::Dictionary)) {
            bool bChanged = pDoc->IsModified();                 // BMD 17 Jul 2003
            pDoc->UndoChange(FALSE);
            pDoc->SetModified(bChanged);                        // BMD 17 Jul 2003
        }
        break;
    case VK_RETURN:
        m_iEditCol++;
        if (m_iEditCol == 2) {      // except when modifying
            CIMSAString csName, csLabel;
            m_aEditControl[1]->GetWindowText(csLabel);
            m_aEditControl[2]->GetWindowText(csName);
            if (!csLabel.IsEmpty() && csName.IsEmpty())  {
                csName = csLabel;
                csName.MakeNameRestrictLength();
                csName = pDoc->GetDict()->GetUniqueName(csName);
                m_aEditControl[2]->SetWindowText(csName);
                m_aEditControl[2]->Invalidate();
            }
        }
        if (pView->m_iGrid == DictionaryGrid::Item) {
            if (m_iEditCol == 5) {
                CIMSAString csFrom, csSpecial;
                m_aEditControl[4]->GetWindowText(csFrom);
                m_aEditControl[6]->GetWindowText(csSpecial);
                CDictItem* pItem = pDoc->GetDict()->GetLevel(pDoc->GetLevel()).GetRecord(pDoc->GetRec())->GetItem(pDoc->GetItem());
                if (SO::IsBlank(csFrom) && csSpecial.IsEmpty() && pItem->GetContentType() == ContentType::Numeric) {
                    csSpecial = _T("NotAppl");
                    assert_cast<CDDComboBox*>(m_aEditControl[6])->SetCurSel(3);
                    m_aEditControl[6]->Invalidate();
                }
            }
        }
        if (m_iEditCol > m_iMaxCol) {
            m_iEditCol = m_iMaxCol;
            if (EditEnd()) {
                pDoc->SetModified();
                EditContinue();   // End this row a posibly continue to next
            }
            else {
                m_iEditCol = dictionary_validator->GetInvalidEdit();
                if (m_iEditCol < m_iMinCol || m_iEditCol > m_iMaxCol) {
                    m_iEditCol = m_iMaxCol;                    // BMD 07 Mar 2003
                }
                m_aEditControl[m_iEditCol]->SetFocus();
            }
        }
        else {
            m_aEditControl[m_iEditCol]->SetFocus();
            GotoCell(m_iEditCol, m_iEditRow);
            if (m_aEditControl.GetSize() > 0) {
                for (int i = m_iMinCol ; i <= m_iMaxCol ; i++) {
                    m_aEditControl[i]->PostMessage(WM_SIZE);
                }
            }
        }
        break;
    case VK_CANCEL:
        m_iEditCol = m_iMaxCol;
        if (EditEnd()) {
            pDoc->SetModified();
            EditContinue();   // End this row a posibly continue to next
        }
        else {
            m_iEditCol = dictionary_validator->GetInvalidEdit();
            if (m_iEditCol < m_iMinCol || m_iEditCol > m_iMaxCol) {  // BMD 10 Mar 2003
                m_iEditCol = m_iMaxCol;
            }
            m_aEditControl[m_iEditCol]->SetFocus();
        }
        break;
    case VK_TAB:
        if (GetKeyState(VK_SHIFT) < 0) {
            // Skip read-only cols
            do {
                --m_iEditCol;
            } while (m_iEditCol >= m_iMinCol && !m_aEditControl[m_iEditCol]->IsWindowEnabled());
        }
        else {
            if (m_iEditCol == 1) {
                // autofill name from label
                CIMSAString csName, csLabel;
                m_aEditControl[1]->GetWindowText(csLabel);
                m_aEditControl[2]->GetWindowText(csName);
                if (!csLabel.IsEmpty() && csName.IsEmpty())  {
                    csName = csLabel;
                    csName.MakeNameRestrictLength();

                    // 20111229 if the entered characters are all non-valid characters (like chinese or arabic), leave the name blank rather than fill it in with "name"
                    if( csName.CompareNoCase(_T("NAME")) || !csLabel.CompareNoCase(_T("NAME")) )
                    {
                        csName = pDoc->GetDict()->GetUniqueName(csName);
                        m_aEditControl[2]->SetWindowText(csName);
                        m_aEditControl[2]->Invalidate();
                    }
                }
            }
            // Skip read-only cols
            do {
                ++m_iEditCol;
            }
            while (m_iEditCol <= m_iMaxCol && !m_aEditControl[m_iEditCol]->IsWindowEnabled());
        }

        if (m_iEditCol < m_iMinCol || m_iEditCol > m_iMaxCol) {
            m_iEditCol = m_iMaxCol;
            if (EditEnd()) {
                pDoc->SetModified();
                EditContinue();   // End this row a posibly continue to next
            }
            else {
                m_iEditCol = dictionary_validator->GetInvalidEdit();
                if (m_iEditCol < m_iMinCol || m_iEditCol > m_iMaxCol) {  // BMD 10 Mar 2003
                    m_iEditCol = m_iMaxCol;
                }
                m_aEditControl[m_iEditCol]->SetFocus();
            }
        }
        else {
            m_aEditControl[m_iEditCol]->SetFocus();
            GotoCell(m_iEditCol, m_iEditRow);
            if (m_aEditControl.GetSize() > 0) {
                for (int i = m_iMinCol ; i <= m_iMaxCol ; i++) {
                    m_aEditControl[i]->PostMessage(WM_SIZE);
                }
            }
        }
        break;
    case VK_UP:
        bAdding = m_bAdding;
        bInserting = m_bInserting;
        if (EditEnd()) {
            pDoc->SetModified();
            row = GetCurrentRow();
            if (bAdding || bInserting) {
                GotoRow(row);
            }
            else {
                GotoRow(--row);
            }
            m_bAdding = FALSE;
            m_bInserting = FALSE;
        }
        else {
            m_iEditCol = dictionary_validator->GetInvalidEdit();
            if (m_iEditCol < m_iMinCol || m_iEditCol > m_iMaxCol) {  // BMD 10 Mar 2003
                m_iEditCol = m_iMaxCol;
            }
            m_aEditControl[m_iEditCol]->SetFocus();
        }
        break;
    case VK_DOWN:
        bAdding = m_bAdding;
        bInserting = m_bInserting;
        if (EditEnd()) {
            pDoc->SetModified();
            row = GetCurrentRow();
            if (bAdding || bInserting) {
                GotoRow(row);
            }
            else {
                GotoRow(++row);
            }
            m_bAdding = FALSE;
            m_bInserting = FALSE;
        }
        else {
            m_iEditCol = dictionary_validator->GetInvalidEdit();
            if (m_iEditCol < m_iMinCol || m_iEditCol > m_iMaxCol) {  // BMD 10 Mar 2003
                m_iEditCol = m_iMaxCol;
            }
            m_aEditControl[m_iEditCol]->SetFocus();
        }
        break;
    case DD_ONSEL:
        break;
    default:
        ASSERT(FALSE);
        break;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDGrid::EditContinue
//
/////////////////////////////////////////////////////////////////////////////

void CDDGrid::EditContinue()
{
    CDDGView* pView = (CDDGView*) GetParent();
    if (m_bAdding) {
        PostMessage(WM_COMMAND, ID_EDIT_ADD);
    }
    else if (m_bInserting) {
        if (pView->m_iGrid == DictionaryGrid::Item) {
            if (m_bValueSet) {
                m_bValueSet = false;
                PostMessage(WM_COMMAND, ID_EDIT_ADD);
                return;
            }
        }
        GotoRow(GetCurrentRow() + 1);
        PostMessage(WM_COMMAND, ID_EDIT_INSERT);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDGrid::OnCanSizeTopHdg
//
/////////////////////////////////////////////////////////////////////////////

int CDDGrid::OnCanSizeTopHdg(){

    return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDGrid::OnCanSizeCol
//
/////////////////////////////////////////////////////////////////////////////

int CDDGrid::OnCanSizeCol(int col){

    if (col < 1) {
        return FALSE;
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDGrid::OnColSizing
//
/////////////////////////////////////////////////////////////////////////////

void CDDGrid::OnColSizing(int, int* width) 
{
    if (*width < 30) {
        *width = 30;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDGrid::OnRowChange
//
/////////////////////////////////////////////////////////////////////////////

void CDDGrid::OnRowChange(long /*oldrow*/, long newrow)
{
    m_bCanEdit = FALSE;

    CDDGView* pView = dynamic_cast<CDDGView*>(GetParent());
    if( pView == nullptr )
        return;

    CDDDoc* pDoc = static_cast<CDDDoc*>(pView->GetDocument());
    CDataDict& dictionary = pDoc->GetDictionary();
    DictBase* dict_base = nullptr;

    CDDGrid& current_grid = pView->GetCurrentGrid();
    long current_row = current_grid.GetCurrentRow();

    // this check seems to fix some issues with the current row not returning
    // the correct value as entities (e.g., records) were deleted
    if( GetFocus() != current_grid.m_CUGGrid )
        current_row = newrow;

    if( pView->m_iGrid == DictionaryGrid::Dictionary )
    {
        if( current_row == 0 )
        {
            dict_base = &dictionary;
        }

        else
        {
            size_t level_number = (size_t)( current_row - CDictGrid::GetFirstLevelRow() );

            if( level_number < dictionary.GetNumLevels() )
                dict_base = &dictionary.GetLevel(level_number);
        }
    }

    else if( pView->m_iGrid == DictionaryGrid::Level )
    {
        CLevelGrid& gridLevel = pView->m_gridLevel;
        DictLevel& dict_level = dictionary.GetLevel(gridLevel.GetLevel());
        size_t record_number = (size_t)( current_row - gridLevel.GetFirstRow() );

        if( record_number < (size_t)dict_level.GetNumRecords() )
            dict_base = dict_level.GetRecord(record_number);  
    }

    else if( pView->m_iGrid == DictionaryGrid::Record )
    {
        CRecordGrid& gridRecord = pView->m_gridRecord;
        int item_number_or_code = gridRecord.GetItem(current_row);

        if( item_number_or_code != RECTYPE )
        {
            CDictRecord* dict_record = dictionary.GetLevel(gridRecord.GetLevel()).GetRecord(gridRecord.GetRecord());
            size_t item_number = (size_t)item_number_or_code;

            if( item_number < (size_t)dict_record->GetNumItems() )
                dict_base = dict_record->GetItem(item_number);
        }
    }

    else if( pView->m_iGrid == DictionaryGrid::Item )
    {
        CItemGrid& gridItem = pView->m_gridItem;

        if( current_row >= 0 && current_row < gridItem.GetNumberRows() && gridItem.HasVSets() )
        {
            CDictItem* dict_item = dictionary.GetLevel(gridItem.GetLevel()).GetRecord(gridItem.GetRecord())->GetItem(gridItem.GetItem());
            int value_set_code = gridItem.GetVSet(current_row);

            if( (size_t)value_set_code < dict_item->GetNumValueSets() )
            {
                DictValueSet& dict_value_set = dict_item->GetValueSet(value_set_code);
                int value_code = gridItem.GetValue(current_row);

                if( value_code == NONE )
                {
                    dict_base = &dict_value_set;
                }

                else if( (size_t)value_code < dict_value_set.GetNumValues() )
                {
                    dict_base = &dict_value_set.GetValue(value_code);
                }
            }
        }
    }

    CDictChildWnd* pDictChildWnd = assert_cast<CDictChildWnd*>(pView->GetParentFrame());

    if( pDictChildWnd->GetDictDlgBar()->GetSafeHwnd() != nullptr &&
        pDictChildWnd->GetDictDlgBar()->GetPropCtrl()->GetSafeHwnd() != nullptr )
    {
        pDictChildWnd->GetDictDlgBar()->GetPropCtrl()->Initialize(pDoc, dict_base);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDGrid::OnHScroll
//
/////////////////////////////////////////////////////////////////////////////

void CDDGrid::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar*) {

    m_CUGHScroll->HScroll(nSBCode, nPos);
    if (m_aEditControl.GetSize() > 0) {
        for (int i = m_iMinCol ; i <= m_iMaxCol ; i++) {
            m_aEditControl[i]->PostMessage(WM_SIZE);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDGrid::OnVScroll
//
/////////////////////////////////////////////////////////////////////////////


void CDDGrid::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar*) {

    m_CUGVScroll->VScroll(nSBCode, nPos);
    if (m_aEditControl.GetSize() > 0) {
        for (int i = m_iMinCol ; i <= m_iMaxCol ; i++) {
            m_aEditControl[i]->PostMessage(WM_SIZE);
        }
    }
}

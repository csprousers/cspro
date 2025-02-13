// RelGrid.cpp: implementation of the CRelGrid class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "RelGrid.h"
#include "RelDlg.h"


namespace
{
    constexpr int REL_NAME      = 0;
    constexpr int REL_PRIM      = 1;
    constexpr int REL_PRIM_LINK = 2;
    constexpr int REL_SEC       = 3;
    constexpr int REL_SEC_LINK  = 4;

    constexpr int REL_NUM_COLS  = REL_SEC_LINK + 1;
}


BEGIN_MESSAGE_MAP(CRelGrid, CDDGrid)
    ON_WM_SETFOCUS()
    ON_WM_KEYDOWN()
    ON_COMMAND(ID_EDIT_ADD, OnEditAdd)
    ON_COMMAND(ID_EDIT_INSERT, OnEditInsert)
    ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
    ON_COMMAND(ID_EDIT_MODIFY, OnEditModify)
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRelGrid::CRelGrid()
    :   m_pOccEdit(nullptr),
        m_pNameEdit(nullptr),
        m_iRel(-1),
        m_iRelPart(-1)
{        
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRelGrid::OnSetup
/////////////////////////////////////////////////////////////////////////////

void CRelGrid::OnSetup()
{
    EnableExcelBorders(FALSE);
    VScrollAlwaysPresent(TRUE);
    SetNumberCols(REL_NUM_COLS, FALSE);
    SetSH_Width(0);
    if(GetDesignerFontZoomLevel()  > 100 ){
        HScrollAlwaysPresent(TRUE);
    }
    SetDefRowHeight( (int)( DEF_ROW_HEIGHT * (GetDesignerFontZoomLevel() / 100.0) ));
    SetTH_Height( (int)( DEF_TOPHEADING_HEIGHT * (GetDesignerFontZoomLevel() / 100.0) ));
    SetCurrentCellMode(2);
    SetHighlightRow(TRUE);
    SetMultiSelectMode(FALSE);

    int column_width = (int)( 170 * (GetDesignerFontZoomLevel() / 100.0) );

	auto set_header = [&](int column, const TCHAR* text)
	{
		QuickSetText(column, HEADER_ROW, text);
		QuickSetAlignment(column, HEADER_ROW, UG_ALIGNLEFT);
        SetColWidth(column, column_width);
	};

    set_header(REL_NAME,      _T("Relation Name"));
    set_header(REL_PRIM,      _T("Primary"));
    set_header(REL_PRIM_LINK, _T("Linked By"));
    set_header(REL_SEC,       _T("Secondary"));
    set_header(REL_SEC_LINK,  _T("Linked By"));

    m_bAdding = false;
    m_bInserting = false;
    m_bEditing = false;

    // retrieving data from the Dictionary
    m_dictRelations = m_pDict->GetRelations();

    size_t actrows = 0;

    for( const auto& dict_relation : m_dictRelations )
        actrows += dict_relation.GetRelationParts().size();

    SetNumberRows((long)actrows, FALSE);
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CRelGrid::EditBegin
//
/////////////////////////////////////////////////////////////////////////////

void CRelGrid::EditBegin(int col, long row, UINT vcKey)
{
    CRelDlg* pView = (CRelDlg*) GetParent();
    CDDDoc* pDoc = (CDDDoc*) pView->m_pDoc;

    ASSERT(row >= 0);
    ASSERT(col >= 0);
    GotoCol(col);
    VScrollEnable(ESB_DISABLE_BOTH);

    CUGCell cell, cell1;
    CRect rect;
    CString cs;
    m_aEditControl.SetSize(REL_SEC_LINK + 1);
    m_bEditing = true;
    m_iEditRow = row;

// All the document stuff
    ClearSelections();

    CRelDlg* pdlg = assert_cast<CRelDlg*>(GetParent());
    std::vector<CString> prim_sec_names = pdlg->m_pDoc->GetPrimSecNames();

    // Setup for relation name
    m_pNameEdit = new CNameEdit();
    m_pNameEdit->SetRowCol(row, REL_NAME);
    GetCellRect(REL_NAME, row, &rect);
    m_pNameEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 101);
    m_pNameEdit->SetFont(&m_font);
    GetCell(REL_NAME, row, &cell);
    cell.GetText(&cs);
    m_pNameEdit->SetWindowText(cs);
    m_aEditControl.SetAt(REL_NAME, (CWnd*) m_pNameEdit);

    // Setup for primary record or item
    CDDComboBox* pComboBox = new CDDComboBox();
    GetCellRect(REL_PRIM, row, &rect);
    rect.top--;
    rect.left--;
//    rect.bottom += (aObject1List.GetSize()+1)*m_plf->lfHeight;
    rect.bottom += (20 + 1)*m_plf->lfHeight;
    rect.right += 2;
    pComboBox->SetRowCol(row, REL_PRIM);
    pComboBox->Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, rect, this, 101);
    pComboBox->SetFont(&m_font);
    pComboBox->SetItemHeight (-1, m_plf->lfHeight);   // sets height for static control and button
    pComboBox->SetItemHeight ( 0, m_plf->lfHeight);   // sets height for list box entries
    for( const CString& name : prim_sec_names ) {
        pComboBox->AddString(name);
    }
    if (m_bInserting) {
        GetCell(REL_NAME, row + 1, &cell);
    cell.GetText(&cs);
        if (cs.IsEmpty()) {
            GetCell(REL_PRIM, row + 1, &cell);
            cell.GetText(&cs);
        }
        else {
            cs = _T("");
        }
    }
    else {
        GetCell(REL_PRIM, row, &cell);
        cell.GetText(&cs);
    }
    int pos = pComboBox->FindStringExact(0, cs);
    if (pos >= 0) {
        pComboBox->SetCurSel(pos);
    }
    else {
        pComboBox->SetCurSel(0);
    }
    m_aEditControl.SetAt(REL_PRIM, (CWnd*) pComboBox);
    CString string;
    ((CDDComboBox*)m_aEditControl[REL_PRIM])->GetWindowText(string);


    CStringArray strarray;
    pDoc->GetRelLinksList(string,strarray);

    // Setup for primary relation link
    pComboBox = new CDDComboBox();
    GetCellRect(REL_PRIM_LINK, row, &rect);
    rect.top--;
    rect.left--;
    rect.bottom += (20 + 1) * m_plf->lfHeight;// strarray.GetSize()
    rect.right += 2;
    pComboBox->SetRowCol(row, REL_PRIM_LINK);
    pComboBox->Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL , rect, this, 103);
    pComboBox->SetFont(&m_font);
    pComboBox->SetItemHeight (-1, m_plf->lfHeight);   // sets height for static control and button
    pComboBox->SetItemHeight ( 0, m_plf->lfHeight);   // sets height for list box entries
    for (int index = 0; index <strarray.GetSize(); index++) {
        pComboBox->AddString(strarray[index]);
    }

    GetCell(REL_PRIM_LINK, row, &cell);
    cell.GetText(&cs);
    pos = pComboBox->FindStringExact(0,cs);
    if (pos >= 0) {
        pComboBox->SetCurSel(pos);
    }
    else {
        pComboBox->SetCurSel(0);
    }
    m_aEditControl.SetAt(REL_PRIM_LINK, (CWnd*) pComboBox);

    // Setup for secondary record or item
    pComboBox = new CDDComboBox();
    GetCellRect(REL_SEC, row, &rect);
    rect.top--;
    rect.left--;
//    rect.bottom += (aObject1List.GetSize()+1)*m_plf->lfHeight;
    rect.bottom += (20 + 1)*m_plf->lfHeight;
    rect.right += 2;
    pComboBox->SetRowCol(row, REL_SEC);
    pComboBox->Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, rect, this, 103);
    pComboBox->SetFont(&m_font);
    pComboBox->SetItemHeight (-1, m_plf->lfHeight);   // sets height for static control and button
    pComboBox->SetItemHeight ( 0, m_plf->lfHeight);   // sets height for list box entries
    for( const CString& name : prim_sec_names ) {
        pComboBox->AddString(name);
    }
    GetCell(REL_SEC, row, &cell);
    cell.GetText(&cs);
    pos = pComboBox->FindStringExact(0,cs);
    if (pos >= 0) {
        pComboBox->SetCurSel(pos);
    }
    else {
        pComboBox->SetCurSel(0);
    }
    m_aEditControl.SetAt(REL_SEC, (CWnd*) pComboBox);

    CString string2;
    ((CDDComboBox*)m_aEditControl[REL_SEC])->GetWindowText(string2);
    CStringArray strarray2;
    pDoc->GetRelLinksList(string2,strarray2);

    // Setup for secondary relation link
    pComboBox = new CDDComboBox();
    GetCellRect(REL_SEC_LINK, row, &rect);
    rect.top--;
    rect.left--;
    rect.bottom += (20 + 1)*m_plf->lfHeight;//strarray2.GetSize()
    rect.right += 2;
    pComboBox->SetRowCol(row, REL_SEC_LINK);
    pComboBox->Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, rect, this, 103);
    pComboBox->SetFont(&m_font);
    pComboBox->SetItemHeight (-1, m_plf->lfHeight);   // sets height for static control and button
    pComboBox->SetItemHeight ( 0, m_plf->lfHeight);   // sets height for list box entries

    for (int index = 0; index <strarray2.GetSize(); index++) {
        pComboBox->AddString(strarray2[index]);
    }
    GetCell(REL_SEC_LINK, row, &cell);
    cell.GetText(&cs);
    pos = pComboBox->FindStringExact(0,cs);
    if (pos >= 0) {
        pComboBox->SetCurSel(pos);
    }
    else {
        pComboBox->SetCurSel(0);
    }
    m_aEditControl.SetAt(REL_SEC_LINK, (CWnd*) pComboBox);

    // Set focus to field

    m_iEditCol = col;
    m_editRow = row;
    GetCurrentRelation(row);

    m_aEditControl[col]->SetFocus();

    m_iMinCol = REL_NAME;
    m_iMaxCol = REL_SEC_LINK;

    if (vcKey >= 32) {
        m_aEditControl[col]->SendMessage(WM_CHAR, vcKey, 1);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::EditEnd
//
/////////////////////////////////////////////////////////////////////////////

bool CRelGrid::EditEnd(bool /*bSilent*/)
{
    if (!m_bEditing) {
        return false;
    }
    CString sName;
    CString sObject1;
    CString sObject2;
    CString sLink1;
    CString sLink2;
    m_aEditControl[REL_NAME]->GetWindowText(sName);
    m_aEditControl[REL_PRIM]->GetWindowText(sObject1);
    m_aEditControl[REL_SEC]->GetWindowText(sObject2);
    m_aEditControl[REL_PRIM_LINK]->GetWindowText(sLink1);
    m_aEditControl[REL_SEC_LINK]->GetWindowText(sLink2);

    // Edit rules for the relations.
    if (sName.IsEmpty() && m_iEditRow == 0) {
        AfxMessageBox(_T("A relation name is required."));
        m_iEditCol = 0;
        return false;
    }
    if (!m_bAdding && !m_bInserting) {
        CString sTemp;
        CUGCell cell;
        GetCell(REL_NAME,m_iEditRow,&cell);
        cell.GetText(&sTemp);
        if ((sName.IsEmpty() && !SO::IsBlank(sTemp)) || (!sName.IsEmpty() && SO::IsBlank(sTemp))) {
            AfxMessageBox(_T("Can't add or delete relation name while editing."));
            m_iEditCol = 0;
            return false;
        }
    }
    if (!sName.IsEmpty()) {
        if (!CIMSAString::IsName(sName)) {
            AfxMessageBox(_T("Relation name must contain A-Z, 0-9, or underline and start with a letter."));
            m_iEditCol = 0;
            return false;
        }
        if (m_pDict->LookupName(sName, nullptr)) {
            AfxMessageBox(_T("Relation name not unique in dictionary."));
            m_iEditCol = 0;
            return false;
        }
        else {
            int iRel = -1;
            for( const auto& dict_relation : m_pDict->GetRelations() ) {
                ++iRel;
                if (dict_relation.GetName() == sName) {
                    if (m_bAdding || m_bInserting) {
                        AfxMessageBox(_T("Duplicate relation name."));
                        m_iEditCol = 0;
                        return false;
                    }
                    else {
                        int iCurRel = -1;
                        for (int row = 0 ; row <= m_iEditRow ; row++) {
                            CString sTemp;
                            CUGCell cell;
                            GetCell(REL_NAME,row,&cell);
                            cell.GetText(&sTemp);
                            if (!sTemp.IsEmpty()) {
                                iCurRel++;
                            }
                        }
                        if (iRel != iCurRel) {
                            AfxMessageBox(_T("Duplicate relation name."));
                            m_iEditCol = 0;
                            return false;
                        }
                    }
                }
            }
        }
        if (CIMSAString::IsReservedWord(sName)) {
            AfxMessageBox(_T("Relation name is a reserved word."));
            m_iEditCol = 0;
            return false;
        }

    }
    if (m_bInserting) {
        CString sTemp;
        CUGCell cell;
        GetCell(REL_NAME,m_iEditRow + 1,&cell);
        cell.GetText(&sTemp);
        if (SO::IsBlank(sTemp) && !SO::IsBlank(sName)) {
            AfxMessageBox(_T("Can't insert relation in the middle of another relation."));
            m_iEditCol = 0;
            return false;
        }
    }
    if (sObject1.CompareNoCase(sObject2) == 0) {  // Primary cannot equal secondary
        AfxMessageBox(_T("Secondary rec or item cannot be the same as primary."));
        m_iEditCol = 3;
        return false;
    }
    CString sObj1Name = sObject1.Right(sObject1.GetLength() - sObject1.ReverseFind('.')-1);
    if (sName.IsEmpty()) {
        CUGCell cell;
        GetCell(REL_PRIM,m_iEditRow - 1,&cell);
        CString sPrevPrim;
        cell.GetText(&sPrevPrim);
        if (sPrevPrim.CompareNoCase(sObj1Name) != 0) {
            AfxMessageBox(_T("Only one primary for a relation."));
            m_iEditCol = 1;
            return false;
        }
    }
    int row = m_iEditRow;
    int rmin;
    for (rmin = row ; ; rmin--) {
        CString sTemp;
        if (rmin == row) {
            sTemp = sName;
        }
        else {
            CUGCell cell;
            GetCell(REL_NAME,rmin,&cell);
            cell.GetText(&sTemp);
        }
        if (!sTemp.IsEmpty()) {
            break;
        }
    }
    int rmax;
    for (rmax = row + 1 ; ; rmax++) {
        if (rmax >= GetNumberRows()) {
            rmax--;
            break;
        }
        CString sTemp;
        if (rmax == row) {
            sTemp = sName;
        }
        else {
            CUGCell cell;
            GetCell(REL_NAME,rmax,&cell);
            cell.GetText(&sTemp);
        }
        if (!sTemp.IsEmpty()) {
            rmax--;
            break;
        }
    }
    CString sObj2Name = sObject2.Right(sObject2.GetLength() - sObject2.ReverseFind('.')-1);
    for (int p = rmin; p <= rmax ; p++) {
        if (p != row) {
            CUGCell cell;
            GetCell(REL_SEC,p,&cell);
            CString sSec;
            cell.GetText(&sSec);
            if (sObj2Name == sSec.Right(sSec.GetLength() - sSec.ReverseFind('.')-1)) {
                AfxMessageBox(_T("Secondaries within a relation cannot be duplicated"));
                        m_iEditCol = 3;
                        return false;
                    }
                }
            }

    if (sLink1 != REL_OCC && sLink2 == REL_OCC) {
        const CDictItem* pPrimItem = m_pDict->LookupName<CDictItem>(sLink1);
        ASSERT(pPrimItem != NULL);
        if (pPrimItem->GetContentType() != ContentType::Numeric) {
            AfxMessageBox(_T("Primary link must be numeric."));
            m_iEditCol = 2;
            return false;
        }
    }
    else if  (sLink1 == REL_OCC && sLink2 != REL_OCC) {
        const CDictItem* pSecItem = m_pDict->LookupName<CDictItem>(sLink2);
        ASSERT(pSecItem != NULL);
        if (pSecItem->GetContentType() != ContentType::Numeric) {
            AfxMessageBox(_T("Secondary link must be numeric."));
            m_iEditCol = 4;
            return false;
        }
    }
    else if  (sLink1 != REL_OCC && sLink2 != REL_OCC) {
        const CDictItem* pPrimItem = m_pDict->LookupName<CDictItem>(sLink1);
        ASSERT(pPrimItem != NULL);
        const CDictItem* pSecItem = m_pDict->LookupName<CDictItem>(sLink2);
        ASSERT(pSecItem != NULL);
        if (pPrimItem->GetContentType() != pSecItem->GetContentType()) {
            AfxMessageBox(_T("Primary and Secondary links must have the same data type."));
            m_iEditCol = 2;
            return false;
        }
    }

    if (m_bAdding) {
        if (!sName.IsEmpty()) {
            // New relation and first part
            DictRelation dict_relation;
            dict_relation.SetName(sName);
            dict_relation.SetPrimaryName(CS2WS(sObject1.Right(sObject1.GetLength() - sObject1.ReverseFind('.') - 1)));
            DictRelationPart dict_relation_part;
            if (sLink1.CompareNoCase(REL_OCC) != 0) {
                dict_relation_part.SetPrimaryLink(CS2WS(sLink1));
            }
            dict_relation_part.SetSecondaryName(CS2WS(sObject2.Right(sObject2.GetLength() - sObject2.ReverseFind('.') - 1)));
            if (sLink2.CompareNoCase(REL_OCC) != 0) {
                dict_relation_part.SetSecondaryLink(CS2WS(sLink2));
            }
            dict_relation.AddRelationPart(std::move(dict_relation_part));
            m_dictRelations.emplace_back(std::move(dict_relation));
        }
        else {
            // 2nd, 3rd, etc. part for a relation
            DictRelation& dict_relation = m_dictRelations.back();
            DictRelationPart dict_relation_part;
            if (sLink1.CompareNoCase(REL_OCC) != 0) {
                dict_relation_part.SetPrimaryLink(CS2WS(sLink1));
            }
            dict_relation_part.SetSecondaryName(CS2WS(sObject2.Right(sObject2.GetLength() - sObject2.ReverseFind('.') - 1)));
            if (sLink2.CompareNoCase(REL_OCC) != 0) {
                dict_relation_part.SetSecondaryLink(CS2WS(sLink2));
            }
            dict_relation.AddRelationPart(std::move(dict_relation_part));
        }
    }
    else if (m_bInserting) {
        if (!sName.IsEmpty()) {
            // New relation and first part
            DictRelation dict_relation;
            dict_relation.SetName(sName);
            dict_relation.SetPrimaryName(CS2WS(sObject1.Right(sObject1.GetLength() - sObject1.ReverseFind('.') - 1)));
            DictRelationPart dict_relation_part;
            if (sLink1.CompareNoCase(REL_OCC) != 0) {
                dict_relation_part.SetPrimaryLink(CS2WS(sLink1));
            }
            dict_relation_part.SetSecondaryName(CS2WS(sObject2.Right(sObject2.GetLength() - sObject2.ReverseFind('.') - 1)));
            if (sLink2.CompareNoCase(REL_OCC) != 0) {
                dict_relation_part.SetSecondaryLink(CS2WS(sLink2));
            }
            dict_relation.AddRelationPart(std::move(dict_relation_part));
            GetCurrentRelation(GetCurrentRow() - 1);
            m_dictRelations.insert(m_dictRelations.begin() + m_iRel + 1, std::move(dict_relation));
        }
        else {
            // 2nd, 3rd, etc. part for a relation
            GetCurrentRelation(GetCurrentRow() - 1);
            DictRelation& dict_relation = m_dictRelations[m_iRel];
            DictRelationPart dict_relation_part;
            if (sLink1.CompareNoCase(REL_OCC) != 0) {
                dict_relation_part.SetPrimaryLink(CS2WS(sLink1));
            }
            dict_relation_part.SetSecondaryName(CS2WS(sObject2.Right(sObject2.GetLength() - sObject2.ReverseFind('.') - 1)));
            if (sLink2.CompareNoCase(REL_OCC) != 0) {
                dict_relation_part.SetSecondaryLink(CS2WS(sLink2));
            }
            dict_relation.InsertRelationPart(m_iRelPart + 1, std::move(dict_relation_part));
        }
    }
    else {
        GetCurrentRelation(m_editRow);
        DictRelation& dict_relation = m_dictRelations[m_iRel];
        if (m_iRelPart == 0) {
            dict_relation.SetName(sName);
        }
        dict_relation.SetPrimaryName(CS2WS(sObject1));
        DictRelationPart& dict_relation_part = dict_relation.GetRelationPart(m_iRelPart);
        if (sLink1.CompareNoCase(REL_OCC) == 0) {
            dict_relation_part.SetPrimaryLink(_T(""));
        }
        else {
            dict_relation_part.SetPrimaryLink(CS2WS(sLink1));
        }
        dict_relation_part.SetSecondaryName(CS2WS(sObject2.Right(sObject2.GetLength() - sObject2.ReverseFind('.') - 1)));
        if (sLink2.CompareNoCase(REL_OCC) == 0) {
            dict_relation_part.SetSecondaryLink(_T(""));
        }
        else {
            dict_relation_part.SetSecondaryLink(CS2WS(sLink2));
        }
    }

    // Remove edit controls
    m_bAdding = false;
    m_bInserting = false;
    m_bEditing = false;
    GetParent()->GetDlgItem(IDC_ADD)->EnableWindow(TRUE);
    GetParent()->GetDlgItem(IDC_INSERT)->EnableWindow(TRUE);
    GetParent()->GetDlgItem(IDC_DELETE)->EnableWindow(TRUE);
    for (int i = 0; i < m_aEditControl.GetSize();i++) {
        delete m_aEditControl[i];
    }
    m_aEditControl.RemoveAll();
    Update();

    return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRecordGrid::EditQuit
//
/////////////////////////////////////////////////////////////////////////////

void CRelGrid::EditQuit()
{
    SetRedraw(FALSE);
    for (int i = 0; i < m_aEditControl.GetSize();i++) {
        delete m_aEditControl[i];
    }
    m_aEditControl.RemoveAll();
    SetRedraw(TRUE);
    m_iEditCol = NONE;
    if (m_bAdding || m_bInserting) {
        DeleteRow(m_iEditRow);
    }
    ClearSelections();
    Update();
    SetFocus();
    RedrawAll();
    m_bAdding = false;
    m_bInserting = false;
    m_bEditing = false;
    m_bCanEdit = true;
    GetParent()->GetDlgItem(IDC_ADD)->EnableWindow(TRUE);
    GetParent()->GetDlgItem(IDC_INSERT)->EnableWindow(TRUE);
    GetParent()->GetDlgItem(IDC_DELETE)->EnableWindow(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CRelGrid::OnColSized
//
/////////////////////////////////////////////////////////////////////////////

void CRelGrid::OnColSized(int /*col*/, int* /*width*/)
{
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CRelGrid::OnEditModify
//
/////////////////////////////////////////////////////////////////////////////

void CRelGrid::OnEditModify()
{
    int row = GetCurrentRow();
    if (row < 0) {
        return;
    }
    if (m_bEditing || m_bAdding) {
        if (!EditEnd())
        return;
    }
    EditBegin(0,row,0);
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CRelGrid::OnEditAdd
//
/////////////////////////////////////////////////////////////////////////////

void CRelGrid::OnEditAdd()
{
    if (m_bEditing) {
        return;
    }
    GetParent()->GetDlgItem(IDC_ADD)->EnableWindow(FALSE);
    GetParent()->GetDlgItem(IDC_INSERT)->EnableWindow(FALSE);
    GetParent()->GetDlgItem(IDC_DELETE)->EnableWindow(FALSE);
    m_bAdding = true;
    int row = GetNumberRows();
    InsertRow(row);
    GotoRow(row);
    EditBegin(REL_NAME, row, 0);
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CRelGrid::OnEditInsert
//
/////////////////////////////////////////////////////////////////////////////

void CRelGrid::OnEditInsert()
{
    if (m_bEditing || GetCurrentRow() < 0) {
        return;
    }

    GetParent()->GetDlgItem(IDC_ADD)->EnableWindow(FALSE);
    GetParent()->GetDlgItem(IDC_INSERT)->EnableWindow(FALSE);
    GetParent()->GetDlgItem(IDC_DELETE)->EnableWindow(FALSE);
    m_bInserting = true;
    int row = GetCurrentRow();
    InsertRow(row);
    GotoRow(row);
    EditBegin(REL_NAME, row, 0);
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CRelGrid::Update
//
/////////////////////////////////////////////////////////////////////////////

void CRelGrid::Update()
{
    const CDictRecord* pRec;
    const CDictItem* pItem;
    int row = 0;
    for( const auto& dict_relation : m_dictRelations ) {
        QuickSetText (REL_NAME, row, dict_relation.GetName());
        for( const auto& dict_relation_part : dict_relation.GetRelationParts() ) {
            m_pDict->LookupName(dict_relation.GetPrimaryName(), nullptr, &pRec, &pItem);
            if (pItem == NULL) {
                QuickSetText (REL_PRIM, row, dict_relation.GetPrimaryName().c_str());
            }
            else {
                QuickSetText (REL_PRIM, row, pRec->GetName() + _T(".") + dict_relation.GetPrimaryName().c_str());
            }
            if (dict_relation_part.IsPrimaryLinkedByOccurrence()) {
                QuickSetText (REL_PRIM_LINK, row, REL_OCC);
            }
            else {
                QuickSetText (REL_PRIM_LINK, row, dict_relation_part.GetPrimaryLink().c_str());
            }
            m_pDict->LookupName(dict_relation_part.GetSecondaryName(), nullptr, &pRec, &pItem);
            if (pItem == NULL) {
                QuickSetText (REL_SEC      , row, dict_relation_part.GetSecondaryName().c_str());
            }
            else {
                QuickSetText (REL_SEC      , row, pRec->GetName() + _T(".") + dict_relation_part.GetSecondaryName().c_str());
            }
            if (dict_relation_part.IsSecondaryLinkedByOccurrence()) {
                QuickSetText (REL_SEC_LINK, row, REL_OCC);
            }
            else {
                QuickSetText (REL_SEC_LINK , row, dict_relation_part.GetSecondaryLink().c_str());
            }
            row++;
        }
    }
    RedrawWindow();
}



/////////////////////////////////////////////////////////////////////////////
//
//                        CRelGrid::EditChange
//
/////////////////////////////////////////////////////////////////////////////

void CRelGrid::EditChange(UINT uChar)
{
    CRelDlg* pView = (CRelDlg*) GetParent();
    CDDDoc* pDoc = (CDDDoc*) pView->m_pDoc;
    bool bAdding;
    bool bInserting;
    long row;
    switch(uChar)  {
    case VK_ESCAPE:
        EditQuit();
        break;
    case VK_RETURN:
    case VK_TAB:
        if (m_iEditCol == 0) {
            CString csname;
            m_aEditControl[0]->GetWindowText(csname);
            if (csname.IsEmpty()) {
                CComboBox * pRelObj1Edit = (CComboBox *)m_aEditControl[1];
                if (m_iEditRow > 0) {
                    m_iEditCol = 1;
                    CString cs;
                    CUGCell cell;
                    GetCell(1,m_iEditRow-1,&cell);
                    cs = cell.GetText();
                    int pos = pRelObj1Edit->FindStringExact(0,cs);
                    if (pos >= 0) {
                        pRelObj1Edit->SetCurSel(pos);
                    }
                    else {
                        pRelObj1Edit->SetCurSel(0);
                    }
                    CStringArray strarray2;
                    CString string2;
                    pRelObj1Edit->GetWindowText(string2);

                    pDoc->GetRelLinksList(string2,strarray2);

                    CRect rect;
                    GetCellRect(REL_PRIM_LINK, m_iEditRow, &rect);
                    rect.top--;
                    rect.left--;
                    rect.bottom += (strarray2.GetSize() + 1) * m_plf->lfHeight;
                    rect.right += 2;

                    GetCell(2,m_iEditRow-1,&cell);
                    cs = cell.GetText();
                    pRelObj1Edit = (CComboBox *)m_aEditControl[2];
                    pRelObj1Edit->ResetContent();
                    for (int index = 0; index <strarray2.GetSize(); index++) {
                        pRelObj1Edit->AddString(strarray2[index]);
                    }
                    pRelObj1Edit->MoveWindow(&rect);
                    int pos1 = pRelObj1Edit->FindStringExact(0,cs);
                    if (pos1 >=0 && pos1 < pRelObj1Edit->GetCount()) {
                        pRelObj1Edit->SetCurSel(pos1);
                    }
                    else {
                        pRelObj1Edit->SetCurSel(0);
                    }
                }
            }

        }
        if (uChar == VK_TAB && GetKeyState(VK_SHIFT) < 0) {
            m_iEditCol--;
        }
        else {
            m_iEditCol++;
        }
        if (m_iEditCol < m_iMinCol || m_iEditCol > m_iMaxCol) {
            m_iEditCol = m_iMaxCol;
            if (EditEnd()) {
                EditContinue();   // End this row a posibly continue to next
            }
            else {
                if (m_iEditCol < m_iMinCol || m_iEditCol > m_iMaxCol) {  // BMD 10 Mar 2003
                    m_iEditCol = m_iMaxCol;
                }
                MessageBeep(0);
                m_aEditControl[m_iEditCol]->SetFocus();
            }
        }
        else {
            GotoCell(m_iEditCol, m_iEditRow);
            m_aEditControl[m_iEditCol]->SetFocus();
        }
        break;

    case VK_CANCEL:
        m_iEditCol = m_iMaxCol;
        if (EditEnd()) {
            EditContinue();   // End this row a posibly continue to next
            //  EditQuit();   // End this row a posibly continue to next
        }
        else {
            if (m_iEditCol < m_iMinCol || m_iEditCol > m_iMaxCol) {  // BMD 10 Mar 2003
                m_iEditCol = m_iMaxCol;
            }
            m_aEditControl[m_iEditCol]->SetFocus();
        }
        break;

    case DD_ONSEL:
        if (m_iEditCol == REL_PRIM)
        {
            CString string;
            CDDComboBox* pCombo = (CDDComboBox*)m_aEditControl[REL_PRIM];
            ((CDDComboBox*)m_aEditControl[REL_PRIM])->GetLBText(pCombo->GetCurSel(),string);
            ((CDDComboBox*)m_aEditControl[REL_PRIM_LINK])->ResetContent();
            CStringArray strarray;
            pDoc->GetRelLinksList(string,strarray);
            CRect rect;
            GetCellRect(REL_PRIM_LINK, m_iEditRow, &rect);
//          rect.top--;
            rect.left--;
            rect.bottom += (strarray.GetSize() + 1) * m_plf->lfHeight;
            rect.right += 2;

            for (int index = 0; index <strarray.GetSize(); index++)
            {
                ((CDDComboBox*)m_aEditControl[REL_PRIM_LINK])->AddString(strarray[index]);
            }
            ((CDDComboBox*)m_aEditControl[REL_PRIM_LINK])->SetCurSel(0);
            ((CDDComboBox*)m_aEditControl[REL_PRIM_LINK])->MoveWindow(&rect);
        }
        if (m_iEditCol == REL_SEC)
        {
            CString string;
            ((CDDComboBox*)m_aEditControl[REL_SEC])->GetWindowText(string);
            ((CDDComboBox*)m_aEditControl[REL_SEC_LINK])->ResetContent();
            CStringArray strarray;
            pDoc->GetRelLinksList(string,strarray);
            for (int index = 0; index <strarray.GetSize(); index++)
            {
                ((CDDComboBox*)m_aEditControl[REL_SEC_LINK])->AddString(strarray[index]);
            }
            ((CDDComboBox*)m_aEditControl[REL_SEC_LINK])->SetCurSel(0);
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
            m_bAdding = false;
            m_bInserting = false;
            m_bEditing = false;
            GetParent()->GetDlgItem(IDC_ADD)->EnableWindow(TRUE);
            GetParent()->GetDlgItem(IDC_INSERT)->EnableWindow(TRUE);
            GetParent()->GetDlgItem(IDC_DELETE)->EnableWindow(TRUE);
        }
        else {
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
            m_bAdding = false;
            m_bInserting = false;
            m_bEditing = false;
            GetParent()->GetDlgItem(IDC_ADD)->EnableWindow(TRUE);
            GetParent()->GetDlgItem(IDC_INSERT)->EnableWindow(TRUE);
            GetParent()->GetDlgItem(IDC_DELETE)->EnableWindow(TRUE);
        }
        else {
            if (m_iEditCol < m_iMinCol || m_iEditCol > m_iMaxCol) {  // BMD 10 Mar 2003
                m_iEditCol = m_iMaxCol;
            }
            m_aEditControl[m_iEditCol]->SetFocus();
        }
        break;
    default:
        ASSERT(FALSE);
        break;
    }
}



void CRelGrid::OnSetFocus(CWnd* /*pOldWnd*/)
{
    if (m_aEditControl.GetSize() > 0 && m_iEditCol < m_aEditControl.GetSize())
        m_aEditControl[m_iEditCol]->SetFocus();
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CRelGrid::EditContinue
//
/////////////////////////////////////////////////////////////////////////////

void CRelGrid::EditContinue()
{
    ClearSelections();
    Update();
    GotoRow(m_iEditRow);
    SetFocus();
}



/////////////////////////////////////////////////////////////////////////////
//
//                        CRelGrid::OnRClicked
//
/////////////////////////////////////////////////////////////////////////////

void CRelGrid::OnRClicked(int /*col*/,long row,int updn,RECT */*rect*/,POINT* point,int /*processed*/)
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

    if (!m_bEditing)
        popMenu.AppendMenu(MF_STRING , ID_EDIT_MODIFY, _T("&Modify Relation"));
    else
        popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_EDIT_MODIFY, _T("&Modify Relation"));

    if (!m_bAdding)
        popMenu.AppendMenu(MF_STRING , ID_EDIT_ADD, _T("&Add Relation"));
    else
        popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_EDIT_ADD, _T("&Add Relation"));

    if (m_bAdding || m_bEditing)
    {
        popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_EDIT_INSERT, _T("&Insert Relation"));
        popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_EDIT_DELETE, _T("&Delete Relation"));
    }
    else
    {
        popMenu.AppendMenu(MF_STRING , ID_EDIT_INSERT, _T("&Insert Relation"));
        popMenu.AppendMenu(MF_STRING , ID_EDIT_DELETE, _T("&Delete Relation"));
    }

    popMenu.LoadToolbar(IDR_DICT_FRAME);   // BMD 29 Sep 2003

    CRect rectWin;
    GetWindowRect(rectWin);
    popMenu.TrackPopupMenu(TPM_RIGHTBUTTON, rectWin.left + point->x, rectWin.top + point->y + GetRowHeight(row), this);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CRelGrid::OnLClicked
//
/////////////////////////////////////////////////////////////////////////////

void CRelGrid::OnLClicked(int col,long row,int updn,RECT */*rect*/,POINT */*point*/,int /*processed*/)
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
        CUGCell cell;
        GetCell(col, row, &cell);

        if (m_bEditing) {
            if (EditEnd()) {
                m_bAdding = false;
                m_bInserting = false;
                m_bEditing = false;
                GetParent()->GetDlgItem(IDC_ADD)->EnableWindow(TRUE);
                GetParent()->GetDlgItem(IDC_INSERT)->EnableWindow(TRUE);
                GetParent()->GetDlgItem(IDC_DELETE)->EnableWindow(TRUE);
            }
            else {
                GotoRow(m_iEditRow);
                m_aEditControl[m_iEditCol]->SetFocus();
                return;
            }
        }

        Update();
        GotoRow(row);
        SetFocus();
        return;
    }

}

/////////////////////////////////////////////////////////////////////////////
//
//                        CRelGrid::OnEditDelete
//
/////////////////////////////////////////////////////////////////////////////

void CRelGrid::OnEditDelete()
{
    if (m_bEditing) {
        return;
    }
    int iCurRow = GetCurrentRow();
    if (iCurRow < 0) {
        return;
    }
    GetCurrentRelation(iCurRow);
    if (m_iRel < 0) {
        AfxMessageBox(_T("Either the relation is incomplete or not selected"));
        return;
    }
    int iDelRows;
    if (m_iRelPart > 0) {
        iDelRows = 1;
        m_dictRelations[m_iRel].RemoveRelationPart(m_iRelPart);
    }
    else {
        iDelRows = (int)m_dictRelations[m_iRel].GetRelationParts().size();
        m_dictRelations.erase(m_dictRelations.begin() + m_iRel);
    }
    for (int i = 0 ; i < iDelRows ; i++) {
        DeleteRow(iCurRow);
    }
    Update();
    RedrawWindow();
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CRelGrid::OnDClicked
//
/////////////////////////////////////////////////////////////////////////////

void CRelGrid::OnDClicked(int col, long row, RECT* /*rect*/, POINT* /*point*/, BOOL /*processed*/)
{
    if (row < 0) {
        return;
    }
    if (m_bEditing) {
        if (!EditEnd()) {
            return;
    }
    }
    GetParent()->GetDlgItem(IDC_ADD)->EnableWindow(FALSE);
    GetParent()->GetDlgItem(IDC_INSERT)->EnableWindow(FALSE);
    GetParent()->GetDlgItem(IDC_DELETE)->EnableWindow(FALSE);
    EditBegin(col,row,0);
}

void CRelGrid::OnKeyDown(UINT* /*vcKey*/, BOOL /*processed*/)
{
}

void CRelGrid::OnKeyDown(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
    if (m_bEditing) {
        return;
    }
    CRelDlg* pView = (CRelDlg*) GetParent();
    switch (nChar )
    {
    case VK_RETURN:
        if (GetCurrentRow() == -1)
            OnEditAdd();
        else
            EditBegin(GetCurrentCol(),GetCurrentRow(),0);
        break;
    case VK_UP:
        if (GetCurrentRow()>0)
            GotoRow(GetCurrentRow()-1);
        break;
    case VK_DOWN:
        if (GetCurrentRow() < GetNumberRows()-1)
            GotoRow(GetCurrentRow()+1);
        break;
    case VK_ESCAPE:
        pView->SendMessage(WM_CLOSE);
        break;
    default:
        break;
    }
}

int CRelGrid::GetCurrentRelation(int row)
{
    m_iRel = -1;
    m_iRelPart = -1;
    if (!m_dictRelations.empty()) {
        for (int r = 0 ; r <= row ; r++) {
            CUGCell cell;
            GetCell(REL_NAME, r, &cell);
            CString sName;
            cell.GetText(&sName);
            if (sName.IsEmpty()) {
                m_iRelPart++;
            }
            else {
                m_iRel++;
                m_iRelPart = 0;
            }
        }
    }
    return m_iRel;
}

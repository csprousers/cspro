// AreaDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "AreaDlg.h"
#include "ConDlg.h"


// CAreaDlg dialog

IMPLEMENT_DYNAMIC(CAreaDlg, CDialog)
CAreaDlg::CAreaDlg(CWnd* pParent) :
    CDialog(IDD_AREA, pParent)
    , m_bIsModified(FALSE)
    , m_bShow_Zero_Areas(FALSE)
    , m_iLowestLevel(0)
{
}

CAreaDlg::~CAreaDlg()
{
    RemoveAllConSpecs();
}

/////////////////////////////////////////////////////////////////////////////
//
//                      CAreaDlg::DoDataExchange
//
/////////////////////////////////////////////////////////////////////////////

void CAreaDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_SHOW_ZERO_AREAS, m_bShow_Zero_Areas);
    DDX_CBIndex(pDX, IDC_LOWEST_LEVEL, m_iLowestLevel);
}

/////////////////////////////////////////////////////////////////////////////
//
//                      CAreaDlg::OnInitDialog
//
/////////////////////////////////////////////////////////////////////////////

BOOL CAreaDlg::OnInitDialog()
{
    ASSERT(m_pCurrDict);
    ASSERT_VALID(m_pConsolidate);

    // Execute base class
    BOOL bRetVal = CDialog::OnInitDialog();
    if (!bRetVal) {
        return bRetVal;
    }

    // Build questionnaire item list box
    CListBox* pQuestBox = (CListBox*) GetDlgItem(IDC_QUESTID_LIST);
    ASSERT_VALID(pQuestBox);
    pQuestBox->ResetContent();

    for( const DictLevel& dict_level : m_pCurrDict->GetLevels() ) {
        const CDictRecord* pItemIds = dict_level.GetIdItemsRec();
        for (int iCount = 0 ; iCount < pItemIds->GetNumItems(); iCount++)  {
            const CDictItem* pIDItem = pItemIds->GetItem(iCount);
            if (pIDItem->GetContentType() == ContentType::Numeric && pIDItem->GetDecimal() == 0) {
                int iLastPos = pQuestBox->AddString(pIDItem->GetLabel());
                pQuestBox->SetItemDataPtr(iLastPos, (void*)pIDItem);
            }
        }
    }

    //  Build the area item list box
    CListBox* pAreaBox = (CListBox*) GetDlgItem(IDC_AREAID_LIST);
    ASSERT_VALID(pAreaBox);
    pAreaBox->ResetContent();
    for (int i = 0 ; i < m_pConsolidate->GetNumAreas() ; i++) {
        const CDictItem* pDictItem = m_pCurrDict->LookupName<CDictItem>(m_pConsolidate->GetArea(i));
        int iLastPos = pAreaBox->AddString(pDictItem->GetLabel());
        pAreaBox->SetItemDataPtr(iLastPos, (void*)pDictItem);
   }
    pQuestBox->SetFocus();
    pQuestBox->SetSel(0);

    // Initialize extra record for item selection
    CComboBox* pRecBox = (CComboBox*) GetDlgItem(IDC_QUEST_REC);
    ASSERT_VALID(pRecBox);
    pRecBox->ResetContent();
    pRecBox->AddString(_T("*** None ***"));

    for( const DictLevel& dict_level : m_pCurrDict->GetLevels() ) {
        for (int j = 0 ; j < dict_level.GetNumRecords() ; j++) {
            const CDictRecord* pRecord = dict_level.GetRecord(j);
            if (pRecord->GetMaxRecs() == 1) {
                int iLastPos = pRecBox->AddString(pRecord->GetLabel());
                pRecBox->SetItemDataPtr(iLastPos, (void*)pRecord);
            }
        }
    }
    if (pRecBox->GetCount() > 0) {
        pRecBox->SetCurSel(0);
        GetDlgItem(IDC_QUEST_REC)->EnableWindow(TRUE);
    }
    else {
        GetDlgItem(IDC_QUEST_REC)->EnableWindow(FALSE);
    }

    // Initialize Keep Areas with No Tallies check box
    m_bShow_Zero_Areas = TRUE;
    UpdateData(FALSE);
    GetDlgItem(IDC_SHOW_ZERO_AREAS)->EnableWindow(FALSE);

    // Initialize standard
    CComboBox* pLowest = (CComboBox*) GetDlgItem(IDC_QUEST_REC);
    ASSERT_VALID(pLowest);
    m_bStandard = m_pConsolidate->IsStandard();
    m_iLowestLevel = m_pConsolidate->GetStandardLevel();
    if (m_bStandard) {
        GenerateStandard();
    }
    else {
        RemoveAllConSpecs();
        for (int i = 0 ; i < m_pConsolidate->GetNumConSpecs() ; i++) {
            CConSpec* pConSpec = new CConSpec(m_pConsolidate->GetConSpec(i));
            m_aConSpecs.Add(pConSpec);
        }
    }
    UpdateLowest();
    m_iLowestLevel = m_pConsolidate->GetStandardLevel();
    if (m_bStandard) {
        ((CButton*) GetDlgItem(IDC_STANDARD))->SetCheck(BST_CHECKED);
    }
    else {
        ((CButton*) GetDlgItem(IDC_CUSTOM))->SetCheck(BST_CHECKED);
    }
    EnableDisable();
    UpdateData(FALSE);
    CenterWindow();
    return bRetVal;
}

BEGIN_MESSAGE_MAP(CAreaDlg, CDialog)
    ON_BN_CLICKED(IDC_AID_ADD, OnBnClickedAidAdd)
    ON_BN_CLICKED(IDC_AID_REMOVE, OnBnClickedAidRemove)
    ON_LBN_SELCHANGE(IDC_QUESTID_LIST, OnLbnSelchangeQuestidList)
    ON_LBN_SELCHANGE(IDC_AREAID_LIST, OnLbnSelchangeAreaidList)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
    ON_BN_CLICKED(IDC_MOVE_UP, OnBnClickedMoveUp)
    ON_BN_CLICKED(IDC_MOVE_DOWN, OnBnClickedMoveDown)
    ON_LBN_SETFOCUS(IDC_AREAID_LIST, OnLbnSetfocusAreaidList)
    ON_LBN_SETFOCUS(IDC_QUESTID_LIST, OnLbnSetfocusQuestidList)
    ON_BN_CLICKED(IDC_STANDARD, OnBnClickedStandard)
    ON_BN_CLICKED(IDC_CUSTOM, OnBnClickedCustom)
    ON_CBN_SELCHANGE(IDC_LOWEST_LEVEL, OnCbnSelchangeLowestLevel)
    ON_CBN_SELCHANGE(IDC_QUEST_REC, OnCbnSelchangeQuestRec)
    ON_BN_CLICKED(IDC_EDIT_CUSTOM, OnBnClickedEditCustom)
    ON_LBN_DBLCLK(IDC_QUESTID_LIST, &CAreaDlg::OnLbnDblclkQuestidList)
    ON_LBN_DBLCLK(IDC_AREAID_LIST, &CAreaDlg::OnLbnDblclkAreaidList)
END_MESSAGE_MAP()


// CAreaDlg message handlers

/////////////////////////////////////////////////////////////////////////////
//
//                      CAreaDlg::OnBnClickedAidAdd
//
/////////////////////////////////////////////////////////////////////////////

void CAreaDlg::OnBnClickedAidAdd()
{
    if (!m_bStandard) {
        if (AfxMessageBox(_T("Current Consolidation Specifications will be lost!\n\nSpecification will be made Standard."),
                MB_OKCANCEL | MB_ICONQUESTION) == IDCANCEL) {
            return;
        }
        ((CButton*) GetDlgItem(IDC_STANDARD))->SetCheck(BST_CHECKED);
        ((CButton*) GetDlgItem(IDC_CUSTOM))->SetCheck(BST_UNCHECKED);
        m_bStandard = true;
    }
    CListBox* pQuestBox = (CListBox*) GetDlgItem(IDC_QUESTID_LIST);
    ASSERT_VALID(pQuestBox);
    CListBox* pAreaBox  = (CListBox*) GetDlgItem(IDC_AREAID_LIST);
    ASSERT_VALID(pAreaBox);

    // Get Selected Items
    CIMSAString cs;
    int iSel;
    int iNumSels = pQuestBox->GetSelCount();
    ASSERT(iNumSels > 0);
    int* piIndex = new int [iNumSels];
    pQuestBox->GetSelItems(iNumSels, piIndex);

    // Add Selected Items to Area
    for (iSel = 0 ; iSel < iNumSels ; iSel++)  {
        pQuestBox->GetText(piIndex[iSel], cs);
        if (pAreaBox->FindStringExact(-1, cs) == LB_ERR)  {
            const CDictItem* pIDItem = (const CDictItem*)pQuestBox->GetItemDataPtr(piIndex[iSel]);
            int iLastPos = pAreaBox->AddString(cs);
            pAreaBox->SetItemDataPtr(iLastPos, (void*)pIDItem);
        }
    }

    // Update
    pQuestBox->SetFocus();
    EnableDisable();
    UpdateLowest();
    GenerateStandard();
    m_bIsModified = TRUE;
    delete piIndex;
}

/////////////////////////////////////////////////////////////////////////////
//
//                      CAreaDlg::OnBnClickedAidRemove
//
/////////////////////////////////////////////////////////////////////////////

void CAreaDlg::OnBnClickedAidRemove()
{
    if (!m_bStandard) {
        if (AfxMessageBox(_T("Current Consolidation Specifications will be lost!\n\nSpecification will be made Standard."),
                MB_OKCANCEL | MB_ICONQUESTION) == IDCANCEL) {
            return;
        }
        m_bStandard = true;
        ((CButton*) GetDlgItem(IDC_STANDARD))->SetCheck(BST_CHECKED);
        ((CButton*) GetDlgItem(IDC_CUSTOM))->SetCheck(BST_UNCHECKED);
    }
    CListBox* pQuestBox = (CListBox*) GetDlgItem(IDC_QUESTID_LIST);
    ASSERT_VALID(pQuestBox);
    CListBox* pAreaBox = (CListBox*) GetDlgItem(IDC_AREAID_LIST);
    ASSERT_VALID(pAreaBox);

    // Get Selections
    int iNumSels = pAreaBox->GetSelCount();
    ASSERT(iNumSels > 0);
    int* piIndex = new int [iNumSels];
    pAreaBox->GetSelItems(iNumSels, piIndex);

    // Remove Selected Items
    for (int i = iNumSels-1 ; i >= 0 ; i--)  {
        pAreaBox->DeleteString(piIndex[i]);
    }

    // Update
    pAreaBox->SetSel (-1, FALSE);
    if (pAreaBox->GetCount() > 0) {
        if (piIndex[0] < pAreaBox->GetCount()) {
            pAreaBox->SetSel(piIndex[0]);
        }
        else {
            pAreaBox->SetSel(pAreaBox->GetCount() - 1);
        }
        pAreaBox->SetFocus();
    }
    else {
        pQuestBox->SetSel(0);
        pQuestBox->SetFocus();
    }
    EnableDisable();
    UpdateLowest();
    GenerateStandard();
    m_bIsModified = TRUE;
    delete piIndex;
}

/////////////////////////////////////////////////////////////////////////////
//
//                      CAreaDlg::OnBnClickedMoveUp
//
/////////////////////////////////////////////////////////////////////////////

void CAreaDlg::OnBnClickedMoveUp()
{
    if (!m_bStandard) {
        if (AfxMessageBox(_T("Current Consolidation Specifications will be lost!\n\nSpecification will be made Standard."),
                MB_OKCANCEL | MB_ICONQUESTION) == IDCANCEL) {
            return;
        }
        m_bStandard = true;
        ((CButton*) GetDlgItem(IDC_STANDARD))->SetCheck(BST_CHECKED);
        ((CButton*) GetDlgItem(IDC_CUSTOM))->SetCheck(BST_UNCHECKED);
    }
    CListBox* pAreaBox = (CListBox*) GetDlgItem(IDC_AREAID_LIST);
    ASSERT_VALID(pAreaBox);

    // Get Item
    int iSel = 0;
    pAreaBox->GetSelItems(1,&iSel);
    CIMSAString sLabel = _T("");
    pAreaBox->GetText(iSel,sLabel);
    const CDictItem* pIDItem = (const CDictItem*)pAreaBox->GetItemDataPtr(iSel);

    // Move Item Up
    pAreaBox->DeleteString(iSel);
    int iLastPos = pAreaBox->InsertString(iSel - 1,sLabel);
    pAreaBox->SetItemDataPtr(iLastPos, (void*)pIDItem);
    pAreaBox->SetSel(iLastPos);

    // Update
    EnableDisable();
    UpdateLowest();
    GenerateStandard();
    m_bIsModified = TRUE;
    pAreaBox->SetFocus();
}

/////////////////////////////////////////////////////////////////////////////
//
//                      CAreaDlg::OnBnClickedMoveDown
//
/////////////////////////////////////////////////////////////////////////////

void CAreaDlg::OnBnClickedMoveDown()
{
    if (!m_bStandard) {
        if (AfxMessageBox(_T("Current Consolidation Specifications will be lost!\n\nSpecification will be made Standard."),
                MB_OKCANCEL | MB_ICONQUESTION) == IDCANCEL) {
            return;
        }
        m_bStandard = true;
        ((CButton*) GetDlgItem(IDC_STANDARD))->SetCheck(BST_CHECKED);
        ((CButton*) GetDlgItem(IDC_CUSTOM))->SetCheck(BST_UNCHECKED);
    }
    CListBox* pAreaBox = (CListBox*) GetDlgItem(IDC_AREAID_LIST);
    ASSERT_VALID(pAreaBox);

    // Get Item
    int iSel = 0;
    pAreaBox->GetSelItems(1,&iSel);
    CIMSAString sLabel = _T("");
    pAreaBox->GetText(iSel,sLabel);
    const CDictItem* pIDItem = (const CDictItem*)pAreaBox->GetItemDataPtr(iSel);

    // Move Item Down
    pAreaBox->DeleteString(iSel);
    int iLastPos = pAreaBox->InsertString(iSel + 1,sLabel);
    pAreaBox->SetItemDataPtr(iLastPos, (void*)pIDItem);
    pAreaBox->SetSel(iLastPos);

    // Update
    EnableDisable();
    UpdateLowest();
    GenerateStandard();
    m_bIsModified = TRUE;
    pAreaBox->SetFocus();
}

/////////////////////////////////////////////////////////////////////////////
//
//                      CAreaDlg::OnLbnSelchangeQuestidList
//
/////////////////////////////////////////////////////////////////////////////

void CAreaDlg::OnLbnSelchangeQuestidList()
{
    EnableDisable();
}

/////////////////////////////////////////////////////////////////////////////
//
//                      CAreaDlg::OnLbnSelchangeAreaidList
//
/////////////////////////////////////////////////////////////////////////////

void CAreaDlg::OnLbnSelchangeAreaidList()
{
    EnableDisable();
}

/////////////////////////////////////////////////////////////////////////////
//
//                         CAreaDlg::OnBnClickedOk
//
/////////////////////////////////////////////////////////////////////////////

void CAreaDlg::OnBnClickedOk()
{
    OnOK();
    CListBox* pAreaBox = (CListBox*) GetDlgItem(IDC_AREAID_LIST);
    ASSERT_VALID(pAreaBox);
    m_pConsolidate->Reset();

    for (int i = 0 ; i < pAreaBox->GetCount() ; i++) {
        const CDictItem* pIDItem = (const CDictItem*)pAreaBox->GetItemDataPtr(i);
        m_pConsolidate->SetArea(i, pIDItem->GetName());
    }
    m_pConsolidate->SetStandard(m_bStandard);
    m_pConsolidate->SetStandardLevel(m_iLowestLevel);
    if (m_bStandard) {
        m_pConsolidate->GenerateStandard();
    }
    else {
        m_pConsolidate->RemoveAllConSpecs();
        for (int k = 0 ; k < m_aConSpecs.GetSize() ; k++) {
            CConSpec* pConSpec = new CConSpec(m_aConSpecs[k]);
            m_pConsolidate->SetConSpec(k, pConSpec);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                  CAreaDlg::OnLbnSetfocusAreaidList
//
/////////////////////////////////////////////////////////////////////////////

void CAreaDlg::OnLbnSetfocusAreaidList()
{
    CListBox* pQuestBox = (CListBox*) GetDlgItem(IDC_QUESTID_LIST);
    ASSERT_VALID(pQuestBox);
    if (pQuestBox->GetCount() > 0) {
        pQuestBox->SelItemRange(FALSE, 0, pQuestBox->GetCount() - 1);
    }
    EnableDisable();
}

/////////////////////////////////////////////////////////////////////////////
//
//                  CAreaDlg::OnLbnSetfocusQuestidList
//
/////////////////////////////////////////////////////////////////////////////

void CAreaDlg::OnLbnSetfocusQuestidList()
{
    CListBox* pAreaBox = (CListBox*) GetDlgItem(IDC_AREAID_LIST);
    ASSERT_VALID(pAreaBox);
    if (pAreaBox->GetCount() > 0) {
        pAreaBox->SelItemRange(FALSE, 0, pAreaBox->GetCount() - 1);
    }
    EnableDisable();
}

/////////////////////////////////////////////////////////////////////////////
//
//                         CAreaDlg::OnBnClickedStandard
//
/////////////////////////////////////////////////////////////////////////////

void CAreaDlg::OnBnClickedStandard()
{
    m_bStandard = true;
    ((CButton*) GetDlgItem(IDC_STANDARD))->SetCheck(BST_CHECKED);
    GetDlgItem(IDC_LOWEST_LEVEL)->EnableWindow(TRUE);
    GetDlgItem(IDC_EDIT_CUSTOM)->EnableWindow(FALSE);
    GenerateStandard();
    m_bIsModified = TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
//                         CAreaDlg::OnBnClickedCustom
//
/////////////////////////////////////////////////////////////////////////////

void CAreaDlg::OnBnClickedCustom()
{
    m_bStandard = false;
    ((CButton*) GetDlgItem(IDC_CUSTOM))->SetCheck(BST_CHECKED);
    GetDlgItem(IDC_LOWEST_LEVEL)->EnableWindow(FALSE);
    GetDlgItem(IDC_EDIT_CUSTOM)->EnableWindow(TRUE);
    m_bIsModified = TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
//                 CAreaDlg::OnCbnSelchangeLowestLevel
//
/////////////////////////////////////////////////////////////////////////////

void CAreaDlg::OnCbnSelchangeLowestLevel()
{
    UpdateData(TRUE);
    GenerateStandard();
    m_bIsModified = TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
//                          CAreaDlg::EnableDisable
//
/////////////////////////////////////////////////////////////////////////////

void CAreaDlg::EnableDisable(void)
{
    CListBox* pQuestBox = (CListBox*) GetDlgItem(IDC_QUESTID_LIST);
    ASSERT_VALID(pQuestBox);
    CListBox* pAreaBox = (CListBox*) GetDlgItem(IDC_AREAID_LIST);
    ASSERT_VALID(pAreaBox);
    if (pAreaBox->GetCount() > 0) {
        pAreaBox->EnableWindow(TRUE);
        GetDlgItem(IDC_STANDARD)->EnableWindow(TRUE);
        GetDlgItem(IDC_CUSTOM)->EnableWindow(TRUE);
        if (m_bStandard) {
            GetDlgItem(IDC_LOWEST_LEVEL)->EnableWindow(TRUE);
            GetDlgItem(IDC_EDIT_CUSTOM)->EnableWindow(FALSE);
        }
        else {
            GetDlgItem(IDC_LOWEST_LEVEL)->EnableWindow(FALSE);
            GetDlgItem(IDC_EDIT_CUSTOM)->EnableWindow(TRUE);
        }
        if (GetFocus() == pAreaBox) {
            GetDlgItem(IDC_AID_ADD)->EnableWindow(FALSE);
            if (pAreaBox->GetSelCount() > 0) {
                GetDlgItem(IDC_AID_REMOVE)->EnableWindow(TRUE);
                if (pAreaBox->GetSelCount() == 1) {
                    if (pAreaBox->GetSel(0)) {
                        GetDlgItem(IDC_MOVE_UP)->EnableWindow(FALSE);
                        if (pAreaBox->GetCount() == 1) {
                            GetDlgItem(IDC_MOVE_DOWN)->EnableWindow(FALSE);
                        }
                        else {
                            GetDlgItem(IDC_MOVE_DOWN)->EnableWindow(TRUE);
                        }
                    }
                    else if (pAreaBox->GetSel(pAreaBox->GetCount() - 1)) {
                        GetDlgItem(IDC_MOVE_UP)->EnableWindow(TRUE);
                        GetDlgItem(IDC_MOVE_DOWN)->EnableWindow(FALSE);
                    }
                    else {
                        GetDlgItem(IDC_MOVE_UP)->EnableWindow(TRUE);
                        GetDlgItem(IDC_MOVE_DOWN)->EnableWindow(TRUE);
                    }
                }
                else {
                    GetDlgItem(IDC_MOVE_UP)->EnableWindow(FALSE);
                    GetDlgItem(IDC_MOVE_DOWN)->EnableWindow(FALSE);
                }
            }
            else {
                GetDlgItem(IDC_AID_REMOVE)->EnableWindow(FALSE);
                GetDlgItem(IDC_MOVE_UP)->EnableWindow(FALSE);
                GetDlgItem(IDC_MOVE_DOWN)->EnableWindow(FALSE);
            }
        }
        else if (GetFocus() == pQuestBox) {
            GetDlgItem(IDC_AID_ADD)->EnableWindow(TRUE);
            GetDlgItem(IDC_AID_REMOVE)->EnableWindow(FALSE);
            GetDlgItem(IDC_MOVE_UP)->EnableWindow(FALSE);
            GetDlgItem(IDC_MOVE_DOWN)->EnableWindow(FALSE);
        }
        else {
            GetDlgItem(IDC_AID_ADD)->EnableWindow(FALSE);
            GetDlgItem(IDC_AID_REMOVE)->EnableWindow(FALSE);
            GetDlgItem(IDC_MOVE_UP)->EnableWindow(FALSE);
            GetDlgItem(IDC_MOVE_DOWN)->EnableWindow(FALSE);
        }
    }
    else {
        pAreaBox->EnableWindow(FALSE);
        GetDlgItem(IDC_STANDARD)->EnableWindow(FALSE);
        GetDlgItem(IDC_CUSTOM)->EnableWindow(FALSE);
        GetDlgItem(IDC_LOWEST_LEVEL)->EnableWindow(FALSE);
        GetDlgItem(IDC_EDIT_CUSTOM)->EnableWindow(FALSE);
        if (GetFocus() == pQuestBox) {
            GetDlgItem(IDC_AID_ADD)->EnableWindow(TRUE);
            GetDlgItem(IDC_AID_REMOVE)->EnableWindow(FALSE);
            GetDlgItem(IDC_MOVE_UP)->EnableWindow(FALSE);
            GetDlgItem(IDC_MOVE_DOWN)->EnableWindow(FALSE);
        }
        else {
            GetDlgItem(IDC_AID_ADD)->EnableWindow(FALSE);
            GetDlgItem(IDC_AID_REMOVE)->EnableWindow(FALSE);
            GetDlgItem(IDC_MOVE_UP)->EnableWindow(FALSE);
            GetDlgItem(IDC_MOVE_DOWN)->EnableWindow(FALSE);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                          CAreaDlg::UpdateLowest
//
/////////////////////////////////////////////////////////////////////////////

void CAreaDlg::UpdateLowest()
{
    CListBox* pAreaBox  = (CListBox*) GetDlgItem(IDC_AREAID_LIST);
    ASSERT_VALID(pAreaBox);
    CComboBox* pLowestLevel = (CComboBox*) GetDlgItem(IDC_LOWEST_LEVEL);
    ASSERT_VALID(pLowestLevel);
    pLowestLevel->ResetContent();
    if (pAreaBox->GetCount() > 0) {
        pLowestLevel->AddString(_T("Total"));
        for (int i = 0 ; i < pAreaBox->GetCount() ; i++) {
            const CDictItem* pIDItem = (const CDictItem*)pAreaBox->GetItemDataPtr(i);
            pLowestLevel->AddString(pIDItem->GetLabel());
        }
        m_iLowestLevel = pLowestLevel->GetCount() - 1;
        pLowestLevel->SetCurSel(m_iLowestLevel);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                      CAreaDlg::OnCbnSelchangeQuestRec
//
/////////////////////////////////////////////////////////////////////////////

void CAreaDlg::OnCbnSelchangeQuestRec()
{
    CComboBox* pRecBox = (CComboBox*) GetDlgItem(IDC_QUEST_REC);
    ASSERT_VALID(pRecBox);
    CListBox* pQuestBox = (CListBox*) GetDlgItem(IDC_QUESTID_LIST);
    ASSERT_VALID(pRecBox);
    pQuestBox->ResetContent();

    // Build Id Items
    for( const DictLevel& dict_level : m_pCurrDict->GetLevels() ) {
        const CDictRecord* pItemIds  = dict_level.GetIdItemsRec();
        for (int iCount = 0 ; iCount < pItemIds->GetNumItems(); iCount++)  {
            const CDictItem* pIDItem = pItemIds->GetItem(iCount);
            if (pIDItem->GetContentType() == ContentType::Numeric && pIDItem->GetDecimal() == 0) {
                int iLastPos = pQuestBox->AddString(pIDItem->GetLabel());
                pQuestBox->SetItemDataPtr(iLastPos, (void*)pIDItem);
            }
        }
    }

    // Build Record Items
    if (pRecBox->GetCurSel() > 0) {
        const CDictRecord* pRecord = (const CDictRecord*)pRecBox->GetItemDataPtr(pRecBox->GetCurSel());
        for (int iCount = 0 ; iCount < pRecord->GetNumItems(); iCount++)  {
            const CDictItem* pRecItem = pRecord->GetItem(iCount);
            if (pRecItem->GetOccurs() == 1 && pRecItem->GetContentType() == ContentType::Numeric && pRecItem->GetDecimal() == 0) {
                int iLastPos = pQuestBox->AddString(pRecItem->GetLabel());
                pQuestBox->SetItemDataPtr(iLastPos, (void*)pRecItem);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                      CAreaDlg::OnBnClickedEditCustom
//
/////////////////////////////////////////////////////////////////////////////

void CAreaDlg::OnBnClickedEditCustom()
{
    CListBox* pAreaBox  = (CListBox*) GetDlgItem(IDC_AREAID_LIST);
    ASSERT_VALID(pAreaBox);

    m_aLevels.RemoveAll();
    for (int i = 0 ; i < pAreaBox->GetCount() ; i++) {
        const CDictItem* pIDItem = (const CDictItem*)pAreaBox->GetItemDataPtr(i);
        m_aLevels.Add(pIDItem->GetName());
    }
    CConSpecDlg dlg;
    dlg.m_pCurrDict = m_pCurrDict;
    dlg.m_paLevels = &m_aLevels;
    dlg.m_paConSpecs = &m_aConSpecs;
    dlg.m_bIsModified = m_bIsModified;
    if (dlg.DoModal() == IDOK) {
        m_bIsModified = dlg.m_bIsModified;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                      CAreaDlg::GenerateStandard
//
/////////////////////////////////////////////////////////////////////////////

void CAreaDlg::GenerateStandard(void)
{
    CListBox* pAreaBox  = (CListBox*) GetDlgItem(IDC_AREAID_LIST);
    ASSERT_VALID(pAreaBox);

    if(pAreaBox->GetCount() ==0)
        return;

    RemoveAllConSpecs();
    CConSpec* pConSpec = new CConSpec(pAreaBox->GetCount());
    pConSpec->SetAreaLevel(_T("TOTAL"));
    m_aConSpecs.Add(pConSpec);
    int iLowestLevel = m_iLowestLevel;
    pAreaBox->GetCount() < m_iLowestLevel ? iLowestLevel = pAreaBox->GetCount() : iLowestLevel = iLowestLevel;
    for (int i = 0 ; i < iLowestLevel ; i++ ) {
        pConSpec = new CConSpec(pAreaBox->GetCount());
        const CDictItem* pIDItem = (const CDictItem*)pAreaBox->GetItemDataPtr(i);
        pConSpec->SetAreaLevel(pIDItem->GetName());
        for (int j = 0 ; j <= i ; j++) {
            CONITEM item = pConSpec->GetAction(j);
            item.level = j;
            pConSpec->SetAction(j, item);
        }
        m_aConSpecs.Add(pConSpec);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                      CAreaDlg::RemoveAllConSpecs
//
/////////////////////////////////////////////////////////////////////////////

void CAreaDlg::RemoveAllConSpecs(void)
{
    CConSpec* pConSpec;
    while (m_aConSpecs.GetSize() > 0) {
        pConSpec = m_aConSpecs[0];
        m_aConSpecs.RemoveAt(0);
        delete pConSpec;
     }
}

void CAreaDlg::OnLbnDblclkQuestidList() // GHM 20130705 a request from the intermediate workshop
{
    OnBnClickedAidAdd();
}


void CAreaDlg::OnLbnDblclkAreaidList() // GHM 20130705 a request from the intermediate workshop
{
    OnBnClickedAidRemove();
}

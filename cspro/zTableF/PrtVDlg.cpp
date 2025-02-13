// PrtVDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "PrtVDlg.h"


// CPrtViewGotoPageDlg dialog

IMPLEMENT_DYNAMIC(CPrtViewGotoPageDlg, CDialog)
CPrtViewGotoPageDlg::CPrtViewGotoPageDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CPrtViewGotoPageDlg::IDD, pParent)
    , m_iPage(0)
{
}

CPrtViewGotoPageDlg::~CPrtViewGotoPageDlg()
{
}

void CPrtViewGotoPageDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Text(pDX, IDC_PAGE, m_iPage);
    CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPrtViewGotoPageDlg, CDialog)
END_MESSAGE_MAP()


// helper function for adding table names from tab set
// to a combo box
void AddTableNamesToCombo(CTabSet* pTabSet, CComboBox* pBox)
{
    for (int iTbl = 0; iTbl < pTabSet->GetNumTables(); ++iTbl) {
        CTable* pTable = pTabSet->GetTable(iTbl);
        pBox->AddString(pTable->GetTitleText());
    }
}

IMPLEMENT_DYNAMIC(CPrtViewGotoTblDlg, CDialog)
CPrtViewGotoTblDlg::CPrtViewGotoTblDlg(CTabSet* pTabSet, CWnd* pParent /*=NULL*/)
    : CDialog(CPrtViewGotoTblDlg::IDD, pParent)
    , m_iTbl(0)
    , m_pTabSet(pTabSet)
{
}

CPrtViewGotoTblDlg::~CPrtViewGotoTblDlg()
{
}

void CPrtViewGotoTblDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_CBIndex(pDX, IDC_TBL, m_iTbl);
    CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPrtViewGotoTblDlg, CDialog)
END_MESSAGE_MAP()


// CPrtViewGotoTblDlg message handlers

BOOL CPrtViewGotoTblDlg::OnInitDialog()
{
    AddTableNamesToCombo(m_pTabSet, (CComboBox*) GetDlgItem(IDC_TBL));

    return CDialog::OnInitDialog();
}

IMPLEMENT_DYNAMIC(CPrtViewGotoAreaDlg, CDialog)
CPrtViewGotoAreaDlg::CPrtViewGotoAreaDlg(CTabSet* pTabSet, CMapStringToString& areaLookup, CWnd* pParent /*=NULL*/)
    : CDialog(CPrtViewGotoAreaDlg::IDD, pParent)
    , m_iTbl(0)
    , m_pTabSet(pTabSet)
    , m_areaLookup(areaLookup)
{
}

CPrtViewGotoAreaDlg::~CPrtViewGotoAreaDlg()
{
}

void CPrtViewGotoAreaDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_CBIndex(pDX, IDC_TBL, m_iTbl);
    DDX_CBIndex(pDX, IDC_COMBO_AREA, m_iArea);
    CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPrtViewGotoAreaDlg, CDialog)
    ON_CBN_SELCHANGE(IDC_TBL, OnCbnSelchangeTbl)
END_MESSAGE_MAP()


// CPrtViewGotoAreaDlg message handlers
BOOL CPrtViewGotoAreaDlg::OnInitDialog()
{
    AddAreaNamesToCombo(m_iTbl);
    AddTableNamesToCombo(m_pTabSet, (CComboBox*) GetDlgItem(IDC_TBL));

    return CDialog::OnInitDialog();
}

void CPrtViewGotoAreaDlg::AddAreaNamesToCombo(int iTbl)
{
    ASSERT(m_pTabSet);
    ASSERT(iTbl >= 0 && iTbl < m_pTabSet->GetNumTables());

    CComboBox* pCombo = (CComboBox*) GetDlgItem(IDC_COMBO_AREA);
    pCombo->ResetContent();

    CTable* pTable = m_pTabSet->GetTable(iTbl);
    const int iNumTabData = pTable->GetTabDataArray().GetSize();
    for (int iArea = 0; iArea < iNumTabData; ++iArea) {
        CString sAreaLabel = GetAreaLabel(iTbl, iArea, true); // get label with indentation
        pCombo->AddString(sAreaLabel);
     }
}

void CPrtViewGotoAreaDlg::OnCbnSelchangeTbl()
{
    CComboBox* pTblCombo = (CComboBox*) GetDlgItem(IDC_TBL);
    ASSERT_VALID(pTblCombo);
    const int iTbl = pTblCombo->GetCurSel();

    CComboBox* pAreaCombo = (CComboBox*) GetDlgItem(IDC_COMBO_AREA);
    CString sCurrSel;
    pAreaCombo->GetWindowText(sCurrSel);
    AddAreaNamesToCombo(iTbl);
    int iCurSel = pAreaCombo->FindStringExact(-1, sCurrSel);
    if (iCurSel == NONE) {
        iCurSel = (pAreaCombo->GetCount() > 0 ? 0 : -1);
    }
    pAreaCombo->SetCurSel(iCurSel);
}

CString CPrtViewGotoAreaDlg::GetArea() const
{
    return GetAreaLabel(m_iTbl, m_iArea, false);
}

void CPrtViewGotoAreaDlg::SetArea(LPCTSTR sArea)
{
    CTable* pTable = m_pTabSet->GetTable(m_iTbl);
    const int iNumTabData = pTable->GetTabDataArray().GetSize();
    for (int iArea = 0; iArea < iNumTabData; ++iArea) {
        CString sAreaLabel = GetAreaLabel(m_iTbl, iArea, false);
        if (sAreaLabel.Compare(sArea) == 0) {
            m_iArea = iArea;
            return;
        }
    }
}

CString CPrtViewGotoAreaDlg::GetAreaLabel(int iTbl, int iArea, bool bIndent) const
{
    CTable* pTable = m_pTabSet->GetTable(iTbl);
    CTabData* pTabData = pTable->GetTabDataArray()[iArea];

    // compute indentation
    CString sIndent;
    if (bIndent) {
        CIMSAString sBreakKey = pTabData->GetBreakKey();
        do {
            CIMSAString sToken = sBreakKey.GetToken(_T(";"));
            if (!SO::IsBlank(sToken)) {
                sIndent += _T(" ");
            }
        } while (!sBreakKey.IsEmpty());
    }

    if (pTabData->GetAreaLabel().IsEmpty()) {
        // if label is empty, may not have been computed yet.
        // Lookup based on key.
        CIMSAString sBreakKey = pTabData->GetBreakKey();
        sBreakKey.Remove(';');
        sBreakKey.Replace(_T("-"),_T(" "));
        sBreakKey.MakeUpper();
        CIMSAString sAreaLabel;
        if (m_areaLookup.Lookup(sBreakKey,sAreaLabel)) {
            return sIndent + sAreaLabel;
        }
        else {
            return sIndent + sBreakKey; //no area label, use code
        }
    }
    return sIndent + pTabData->GetAreaLabel();
}


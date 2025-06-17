// ConDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "ConDlg.h"


// CConSpecDlg dialog

IMPLEMENT_DYNAMIC(CConSpecDlg, CDialog)
CConSpecDlg::CConSpecDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CConSpecDlg::IDD, pParent)
{
}

CConSpecDlg::~CConSpecDlg()
{
}

void CConSpecDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

/////////////////////////////////////////////////////////////////////////////
//
//                      CAreaDlg::OnInitDialog
//
/////////////////////////////////////////////////////////////////////////////

BOOL CConSpecDlg::OnInitDialog()  {

    // Execute base class
    BOOL bRetVal = CDialog::OnInitDialog();
    if (!bRetVal) {
        return bRetVal;
    }
    m_ConSpecGrid.m_bClosing = false;
    // Create Grid
    CWnd* pWnd = GetDlgItem(IDC_CONSPEC_GRID);
    pWnd->GetClientRect(&m_Rect);

    // Load Grid
    m_ConSpecGrid.m_pCurrDict = m_pCurrDict;
    m_ConSpecGrid.m_paLevels = m_paLevels;
    m_ConSpecGrid.m_paConSpecs = m_paConSpecs;
    m_ConSpecGrid.m_bIsModified = m_bIsModified;

    //Attach the grid to the control
    pWnd->SetFocus();
    m_ConSpecGrid.AttachGrid(this,IDC_CONSPEC_GRID);
//  SetGridData();

    //Show the grid
    m_ConSpecGrid.SetFocus();
    m_ConSpecGrid.ShowWindow(SW_SHOW);
    m_ConSpecGrid.GotoCell(0,0);
    m_ConSpecGrid.StartEdit();
    return bRetVal;
}

BEGIN_MESSAGE_MAP(CConSpecDlg, CDialog)
    ON_BN_CLICKED(IDC_DELETE, OnBnClickedDelete)
    ON_BN_CLICKED(IDC_INSERT, OnBnClickedInsert)
    ON_BN_CLICKED(IDC_ADD, OnBnClickedAdd)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
END_MESSAGE_MAP()


// CConSpecDlg message handlers

/////////////////////////////////////////////////////////////////////////////
//
//                      CAreaDlg::OnBnClickedDelete
//
/////////////////////////////////////////////////////////////////////////////

void CConSpecDlg::OnBnClickedDelete()
{
    int col = m_ConSpecGrid.GetCurrentCol();
    int row = m_ConSpecGrid.GetCurrentRow();
    m_ConSpecGrid.DeleteRow(row);
    m_ConSpecGrid.SetFocus();
//  if (m_ConSpecGrid.GetNumberRows() > 0) {
//      m_ConSpecGrid.GotoCell(col,row);
//  }
    m_ConSpecGrid.RedrawAll();
}

/////////////////////////////////////////////////////////////////////////////
//
//                      CAreaDlg::OnBnClickedInsert
//
/////////////////////////////////////////////////////////////////////////////

void CConSpecDlg::OnBnClickedInsert()
{
    int col = m_ConSpecGrid.GetCurrentCol();
    int row = m_ConSpecGrid.GetCurrentRow();
    if (row < 0) {
        row = 0;
    }
    m_ConSpecGrid.InsertRow(row);
    m_ConSpecGrid.StartEdit(0,row,0);
    m_ConSpecGrid.RedrawAll();
}

/////////////////////////////////////////////////////////////////////////////
//
//                      CAreaDlg::OnBnClickedAdd
//
/////////////////////////////////////////////////////////////////////////////

void CConSpecDlg::OnBnClickedAdd()
{
    int col = m_ConSpecGrid.GetCurrentCol();
    int row = m_ConSpecGrid.GetNumberRows();
    m_ConSpecGrid.InsertRow(row);
    m_ConSpecGrid.StartEdit(0,row,0);
    m_ConSpecGrid.RedrawAll();
}

/////////////////////////////////////////////////////////////////////////////
//
//                      CAreaDlg::OnBnClickedOk
//
/////////////////////////////////////////////////////////////////////////////

void CConSpecDlg::OnBnClickedOk()
{
    OnOK();
    // Clear out old conspecs
    CConSpec* pConSpec;
    while (m_paConSpecs->GetSize() > 0) {
        pConSpec = m_paConSpecs->GetAt(0);
        m_paConSpecs->RemoveAt(0);
        delete pConSpec;
    }
    // Generate new conspecs
    for (int r = 0 ; r < m_ConSpecGrid.GetNumberRows() ; r++) {
        CUGCell cell;
        m_ConSpecGrid.GetCell(0, r, &cell);
        CIMSAString sName;
        cell.GetText(&sName);
        if (!sName.IsEmpty()) {
            pConSpec = new CConSpec(m_paLevels->GetSize());
            pConSpec->SetAreaLevel(sName);
            for (int c = 0 ; c < m_paLevels->GetSize() ; c++) {
                m_ConSpecGrid.GetCell(c + 1, r, &cell);
                CIMSAString sValue;
                cell.GetText(&sValue);
                CONITEM cItem;
                cItem.level = c;
                m_ConSpecGrid.ParseConSpec(sValue, cItem);
                pConSpec->SetAction(c, cItem);
            }
            m_paConSpecs->Add(pConSpec);
        }
    }
}

void CConSpecDlg::OnBnClickedCancel()
{
    m_ConSpecGrid.m_bClosing = true;
    OnCancel();
}

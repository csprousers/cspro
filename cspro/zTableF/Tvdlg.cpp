//***************************************************************************
//  File name: TVDlg.cpp
//
//  Description:
//       Implementation of the TVDlg class
//
//  History:    Date       Author     Comment
//              -----------------------------
//              1996         csc      created
//              10 dec 96    csc      save/print help context in CSelectionDialog
//
//***************************************************************************

#include "StdAfx.h"
#include "Tvdlg.h"
#include "TabDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPickTablesDlg dialog

CPickTablesDlg::CPickTablesDlg(int iIDTitle, CWnd* pParent /*=NULL*/)
    : CDialog(CPickTablesDlg::IDD, pParent), m_iIDTitle(iIDTitle)
{
    //{{AFX_DATA_INIT(CPickTablesDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CPickTablesDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPickTablesDlg)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPickTablesDlg, CDialog)
    //{{AFX_MSG_MAP(CPickTablesDlg)
    ON_BN_CLICKED(IDC_CHECKALL, OnCheckall)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CPickTablesDlg::OnInitDialog()  {

    BOOL bRet = CDialog::OnInitDialog();

    ASSERT(m_iIDTitle > 0);
    CIMSAString csTitle;
    csTitle.LoadString(m_iIDTitle);
    ASSERT(!csTitle.IsEmpty());
    SetWindowText(csTitle);

//    m_checklistbox.SubclassDlgItem(IDC_SAVETBLS, this);
    CListBox* pBox = (CListBox*)GetDlgItem(IDC_SAVETBLS);

    // fill up the list
    ASSERT(m_aiCheck.GetSize() <= m_acsList.GetSize());
    int i;
    for (i = 0 ; i < m_acsList.GetSize() ; i++)  {
        pBox->AddString(m_acsList[i]);
    }
//    for (i = 0 ; i < m_aiCheck.GetSize() ; i++)  {
//        m_checklistbox.SetCheck(i, m_aiCheck[i]);
//    }

   // int iTable = m_pDoc->GetCurTblNum();
    for (i = 0 ; i < m_acsList.GetSize() ; i++)  {
        if (GetCheck(i)) {
            pBox->SetSel(i, TRUE);
        }
    }


    return bRet;
}

void CPickTablesDlg::SetCheck(int iIndex, int iCheck)  {
    int iOldSize = m_aiCheck.GetSize();
    if (iOldSize < iIndex+1)  {
        m_aiCheck.SetSize(iIndex+1);
        for (int i = iOldSize ; i < m_aiCheck.GetSize() ; i++)  {
            m_aiCheck[i] = FALSE;
        }
    }
    m_aiCheck[iIndex] = iCheck;
}

int CPickTablesDlg::GetCheck(int iIndex)  {
    ASSERT(iIndex < m_aiCheck.GetSize());
    return m_aiCheck[iIndex];
}

void CPickTablesDlg::OnOK()  {
    // do a manual UpdateData, move checked tables into our array
    CListBox* pBox = (CListBox*)GetDlgItem(IDC_SAVETBLS);

    int iSelected = 0;
    m_aiCheck.RemoveAll();
    for (int i = 0 ; i < pBox->GetCount() ; i++)  {
        BOOL b = pBox->GetSel(i);
        if (b) {
            iSelected++;
        }
        m_aiCheck.Add(pBox->GetSel(i));
    }
    ASSERT(m_aiCheck.GetSize() == m_acsList.GetSize());
    ASSERT(iSelected);
//    if (!iSelected) {
//        AfxMessageBox("No tables selected");
//        return;
//    }

    CDialog::OnOK();
}

void CPickTablesDlg::OnCheckall()
{
    CListBox* pBox = (CListBox*)GetDlgItem(IDC_SAVETBLS);

    int iCount = pBox->GetCount();
    for (int i = 0 ; i < iCount ; i++)  {
        pBox->SetSel(i, TRUE);
    }
}

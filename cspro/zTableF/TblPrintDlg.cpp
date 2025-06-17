//***************************************************************************
//  File name: TblPrtDlg.cpp
//
//  Description:
//       Custom print dialog for printing tables.  Adds "current page" and "current table"
// radio buttons to standard dialog.  Removes "selection" radio button.
//
//
//***************************************************************************

// TblPrtDg.cpp : implementation file
//

#include "StdAfx.h"
#include "TblPrtDg.h"
#include "Tvdlg.h"

/////////////////////////////////////////////////////////////////////////////////
//                      PrintHookProc
// Hook procedure used to catch when user clicks on std print dlg controls.
// Specifically when user clicks on "All" or "Range" for pages so we can
// uncheck the additional radio buttons we added.
/////////////////////////////////////////////////////////////////////////////////
UINT APIENTRY PrintHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam,
                            LPARAM lParam)
{
    switch(uiMsg)
    {
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case rad1:
                case rad3:
                    CheckDlgButton(hdlg, IDC_CURRENT_PAGE, BST_UNCHECKED);
                    CheckDlgButton(hdlg, IDC_CURRENT_TABLE, BST_UNCHECKED);
                    CheckDlgButton(hdlg, IDC_TABLES, BST_UNCHECKED);
                    break;
            }
            break;
    }

    return FALSE; // pass message through to base CPrintDialog for processing
}

// CTablePrintDlg

IMPLEMENT_DYNAMIC(CTablePrintDlg, CPrintDialog)


/////////////////////////////////////////////////////////////////////////////////
//                      CTablePrintDlg::CTablePrintDlg
//
/////////////////////////////////////////////////////////////////////////////////
CTablePrintDlg::CTablePrintDlg(BOOL bPrintSetupOnly, CTabSet* pTabSet, DWORD dwFlags, CWnd* pParentWnd) :
    m_pTabSet(pTabSet),
    CPrintDialog(bPrintSetupOnly, dwFlags, pParentWnd)
{
    m_pd.hInstance = AfxFindResourceHandle(MAKEINTRESOURCE(IDD), RT_DIALOG);
    m_pd.lpPrintTemplateName = MAKEINTRESOURCE(IDD);
    m_pd.lpfnPrintHook = PrintHookProc;
    m_pd.Flags |= PD_ENABLEPRINTTEMPLATE | PD_ENABLEPRINTHOOK;
}

/////////////////////////////////////////////////////////////////////////////////
//                      CTablePrintDlg::~CTablePrintDlg
//
/////////////////////////////////////////////////////////////////////////////////
CTablePrintDlg::~CTablePrintDlg()
{
}

/////////////////////////////////////////////////////////////////////////////////
//                      CTablePrintDlg::OnOK
// Set enum value based on what page radio buttons are checked when dialog
// is dismissed.
/////////////////////////////////////////////////////////////////////////////////
void CTablePrintDlg::OnOK()
{
    if (IsDlgButtonChecked(IDC_CURRENT_PAGE)) {
        m_pageButtState = PG_CURR_PAGE;
    }
    else if (IsDlgButtonChecked(IDC_CURRENT_TABLE)) {
        m_pageButtState = PG_CURR_TABLE;
    }
    else if (IsDlgButtonChecked(IDC_TABLES)) {
        m_pageButtState = PG_SELECTED_TABLES;
    }
    else {
        m_pageButtState = PG_RANGE; // default
    }

    CPrintDialog::OnOK();
}

BEGIN_MESSAGE_MAP(CTablePrintDlg, CPrintDialog)
    ON_BN_CLICKED(IDC_CURRENT_TABLE, OnBnClickedCurrentTable)
    ON_BN_CLICKED(IDC_CURRENT_PAGE, OnBnClickedCurrentPage)
    ON_BN_CLICKED(IDC_TABLES, OnBnClickedSelectedTables)
    ON_BN_CLICKED(IDC_SELECT_TABLES, OnBnClickedSelectTables)
END_MESSAGE_MAP()



// CTablePrintDlg message handlers

/////////////////////////////////////////////////////////////////////////////////
//                      CTablePrintDlg::OnBnClickedCurrentTable
//
/////////////////////////////////////////////////////////////////////////////////
void CTablePrintDlg::OnBnClickedCurrentTable()
{
    // Uncheck std print dlg radio buttons since one of the custom ones was hit.
    CheckDlgButton(rad1, BST_UNCHECKED);
    CheckDlgButton(rad3, BST_UNCHECKED);

    // update check state for our custom buttons
    CheckDlgButton(IDC_CURRENT_PAGE, BST_UNCHECKED);
    CheckDlgButton(IDC_CURRENT_TABLE, BST_CHECKED);
    CheckDlgButton(IDC_TABLES, BST_UNCHECKED);
}

/////////////////////////////////////////////////////////////////////////////////
//                      CTablePrintDlg::OnBnClickedCurrentPage
//
/////////////////////////////////////////////////////////////////////////////////
void CTablePrintDlg::OnBnClickedCurrentPage()
{
    // Uncheck std print dlg radio buttons since one of the custom ones was hit.
    CheckDlgButton(rad1, BST_UNCHECKED);
    CheckDlgButton(rad3, BST_UNCHECKED);

    // update check state for our custom buttons
    CheckDlgButton(IDC_CURRENT_PAGE, BST_CHECKED);
    CheckDlgButton(IDC_CURRENT_TABLE, BST_UNCHECKED);
    CheckDlgButton(IDC_TABLES, BST_UNCHECKED);
}

/////////////////////////////////////////////////////////////////////////////////
//                      CTablePrintDlg::OnBnClickedSelectesTables
//
/////////////////////////////////////////////////////////////////////////////////
void CTablePrintDlg::OnBnClickedSelectedTables()
{
    // Uncheck std print dlg radio buttons since one of the custom ones was hit.
    CheckDlgButton(rad1, BST_UNCHECKED);
    CheckDlgButton(rad3, BST_UNCHECKED);

    // update check state for our custom buttons
    CheckDlgButton(IDC_CURRENT_PAGE, BST_UNCHECKED);
    CheckDlgButton(IDC_CURRENT_TABLE, BST_UNCHECKED);
    CheckDlgButton(IDC_TABLES, BST_CHECKED);
}

/////////////////////////////////////////////////////////////////////////////////
//                      CTablePrintDlg::OnBnClickedSelectTables
//
/////////////////////////////////////////////////////////////////////////////////
void CTablePrintDlg::OnBnClickedSelectTables()
{
    CPickTablesDlg dlgMulti(IDS_PICKPRINT);
    int iTbls = m_pTabSet->GetNumTables();

    for (int i = 0 ; i < iTbls ; i++)  {
        dlgMulti.AddString(m_pTabSet->GetTable(i)->GetName());
        dlgMulti.SetCheck(i, FALSE);
    }

    for (int i = 0; i < m_aSelectedTables.GetCount(); ++i) {
        dlgMulti.SetCheck(m_aSelectedTables[i], TRUE);
    }

    if (dlgMulti.DoModal() == IDOK)  {
        // automatically set the radio button for selected tables on
        OnBnClickedSelectedTables();

        m_aSelectedTables.RemoveAll();
        for (int i = 0 ; i < iTbls ; i++)  {
            if (dlgMulti.GetCheck(i)) {
                m_aSelectedTables.Add(i);
            }
        }
    }
}

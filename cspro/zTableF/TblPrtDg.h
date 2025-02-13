#pragma once
//***************************************************************************
//  File name: TblPrtDlg.h
//
//  Description:
//       Custom print dialog for printing tables.  Adds "current page" and "current table"
// radio buttons to standard dialog.  Removes "selection" radio button.
//
//
//***************************************************************************

#include <zTableF/zTableF.h>

// CTablePrintDlg

class CLASS_DECL_ZTABLEF CTablePrintDlg : public CPrintDialog
{
    DECLARE_DYNAMIC(CTablePrintDlg)

public:
    CTablePrintDlg(BOOL bPrintSetupOnly,
            // TRUE for Print Setup, FALSE for Print Dialog
            CTabSet* pTabSet,
            DWORD dwFlags = PD_ALLPAGES | PD_USEDEVMODECOPIES
                | PD_HIDEPRINTTOFILE | PD_NOSELECTION,
            CWnd* pParentWnd = NULL);
    virtual ~CTablePrintDlg();
    virtual void OnOK();

// Dialog Data
    enum { IDD = IDD_TABLE_PRINT_DLG };

    // page option chosen
    // filled in OnOK.
    enum PageButtState {
        PG_CURR_PAGE, // print current page only,
        PG_CURR_TABLE, // print only pages from current table
        PG_RANGE, // print the page range specified in m_pd (user chose all or range)
        PG_SELECTED_TABLES // print all pages from tables in m_aSelectedTables
    };
    PageButtState m_pageButtState;

protected:
    DECLARE_MESSAGE_MAP()
    CTabSet* m_pTabSet;

public:
    afx_msg void OnBnClickedCurrentTable();
    afx_msg void OnBnClickedCurrentPage();
    afx_msg void OnBnClickedSelectedTables();
    afx_msg void OnBnClickedSelectTables();

    CArray<int, int> m_aSelectedTables;
};

#pragma once

/*----------------------------------------------------------------------
Copyright (C)2001 MJSoft. All Rights Reserved.
          This source may be used freely as long as it is not sold for
                    profit and this copyright information is not altered or removed.
                    Visit the web-site at www.mjsoft.co.uk
                    e-mail comments to info@mjsoft.co.uk
File:     SortListCtrl.h
Purpose:  Provides a sortable list control, it will sort text, numbers
          and dates, ascending or descending, and will even draw the
                    arrows just like windows explorer!
----------------------------------------------------------------------*/

#include <zUtilF/zUtilF.h>
#include <zUtilF/DropFilesListCtrl.h>
#include <zUtilF/SrtHdrCt.h>


class CLASS_DECL_ZUTILF CSortListCtrl : public DropFilesListCtrl
{
// Construction
public:
    CSortListCtrl();

// Attributes
public:

// Operations
public:
    BOOL SetHeadings( UINT uiStringID );
    BOOL SetHeadings( const CString& strHeadings );

    int AddItem( LPCTSTR pszText, ... );
    BOOL DeleteItem( int iItem );
    BOOL DeleteAllItems();
    void LoadColumnInfo();
    void SaveColumnInfo();
    BOOL SetItemText( int nItem, int nSubItem, LPCTSTR lpszText );
    void Sort( int iColumn, BOOL bAscending );
    BOOL SetItemData(int nItem, DWORD dwData);
    DWORD GetItemData(int nItem) const;

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSortListCtrl)
protected:
    void PreSubclassWindow() override;
    //}}AFX_VIRTUAL

// Implementation
public:
    int SInsertItem( int pos,LPCTSTR pszText, ...);
    virtual ~CSortListCtrl();

    // Generated message map functions
protected:
    static int CALLBACK CompareFunction( LPARAM lParam1, LPARAM lParam2, LPARAM lParamData );
    void FreeItemMemory( const int iItem );
    BOOL CSortListCtrl::SetTextArray( int iItem, LPTSTR* arrpsz );
    LPTSTR* CSortListCtrl::GetTextArray( int iItem ) const;

    CSortHeaderCtrl m_ctlHeader;

    int m_iNumColumns;
    int m_iSortColumn;
    BOOL m_bSortAscending;

    //{{AFX_MSG(CSortListCtrl)
    afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDestroy();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

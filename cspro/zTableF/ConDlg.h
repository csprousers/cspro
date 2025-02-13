#pragma once

#include <zTableF/ConGrid.h>

// CConSpecDlg dialog

class CConSpecDlg : public CDialog
{
    DECLARE_DYNAMIC(CConSpecDlg)

public:
// Members
    const CDataDict*m_pCurrDict;                    // Pointer to dictionary object
    CStringArray*   m_paLevels;                     // Pointer to Area Structure Names
    CArray <CConSpec*, CConSpec*>* m_paConSpecs;    // Consolidations
    bool            m_bIsModified;                  // Consolidate was modified

    CConSpecGrid    m_ConSpecGrid;
    CRect           m_Rect;

public:
    CConSpecDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CConSpecDlg();
    virtual BOOL OnInitDialog();

// Dialog Data
    enum { IDD = IDD_AREA_CONSPEC };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedDelete();
    afx_msg void OnBnClickedInsert();
    afx_msg void OnBnClickedAdd();
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedCancel();
};

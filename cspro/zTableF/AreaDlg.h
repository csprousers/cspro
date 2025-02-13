#pragma once

// CAreaDlg dialog

class CAreaDlg : public CDialog
{
    DECLARE_DYNAMIC(CAreaDlg)

public:
// Members
    CConsolidate*    m_pConsolidate;            // Pointer to consolidate object
    const CDataDict* m_pCurrDict;               // Pointer to dictionary object
    bool             m_bIsModified;             // Consolidate was modified
    bool             m_bStandard;               // Standard
    int              m_iLowestLevel;
    CStringArray     m_aLevels;                 // Area Structure Names
    CArray<CConSpec*, CConSpec*> m_aConSpecs;   // Consolidations

    // Construction
    CAreaDlg(CWnd* pParent = NULL);
    virtual ~CAreaDlg();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    void EnableDisable(void);
    void UpdateLowest(void);
    void GenerateStandard(void);

    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedAidAdd();
    afx_msg void OnBnClickedAidRemove();
    afx_msg void OnLbnSelchangeQuestidList();
    afx_msg void OnLbnSelchangeAreaidList();
    afx_msg void OnBnClickedOk();
    BOOL m_bShow_Zero_Areas;
    afx_msg void OnBnClickedMoveUp();
    afx_msg void OnBnClickedMoveDown();
    afx_msg void OnLbnSetfocusAreaidList();
    afx_msg void OnLbnSetfocusQuestidList();
    afx_msg void OnBnClickedStandard();
    afx_msg void OnBnClickedCustom();
    afx_msg void OnCbnSelchangeLowestLevel();
    afx_msg void OnCbnSelchangeQuestRec();
    afx_msg void OnBnClickedEditCustom();
protected:
    void RemoveAllConSpecs(void);
public:
    afx_msg void OnLbnDblclkQuestidList();
    afx_msg void OnLbnDblclkAreaidList();
};

#pragma once

#include <zEdit2O/LogicCtrl.h>

class CTabView;

// CTblTallyFmtDlg dialog

class CTblTallyFmtDlg : public CDialog
{
    DECLARE_DYNAMIC(CTblTallyFmtDlg)

public:
    CTblTallyFmtDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CTblTallyFmtDlg();

// Dialog Data
    enum { IDD = IDD_TALLY_TABLE };
    CTabView* m_pTabView;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL PreTranslateMessage(MSG* pMsg);

    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedUniverse();
    CComboBox m_cBreakLevel;
    CIMSAString m_sUnivString;
    CIMSAString m_sWeight;
    CIMSAString m_sValue;
    CComboBox m_cSubTables;
    CComboBox m_cUnitName;
    CIMSAString m_sSubTable;
    CIMSAString m_sTabLogic;
    CIMSAString m_sPostCalc;
    CStringArray m_arrPostCalc;//For saving
    CLogicCtrl m_editUniv;
    CLogicCtrl m_editTabLogic;
    CLogicCtrl m_editPostCalc;
    CIMSAString m_sCommonUnitName; //used for "Entire table" when subtables  > 1

    int  m_iCurSubTable;
    CConsolidate* m_pConsolidate;
    int  m_iBreakLevel;
    BOOL m_bUseCustomSpecVal;
    BOOL m_bUseSpecValNotAppl;
    BOOL m_bUseSpecValMissing;
    BOOL m_bUseSpecValRefused;
    BOOL m_bUseSpecValDefault;
    BOOL m_bUseSpecValUndefined;
    void MakeTabLogicArray(CIMSAString sTabLogic , CStringArray& arrTabLogic);
    CIMSAString MakeStringFromTabLogicArray(CStringArray& arrTabLogic);

    CStringArray        m_arrSubTables;
    CArray<CStringArray, CStringArray&> m_arrUnitNames; // one array for each subtable
                                                        // plus one for entire table

    CArray<CUnitSpec, CUnitSpec&>  m_aUnitSpec;

    afx_msg void OnBnClickedOk();
    virtual BOOL OnInitDialog();
    void UpdateDlgContent();
    bool CheckSyntaxAll();
    bool CheckSyntaxSubtable(int iSubtable);
    bool CheckSyntax(int iSubtable, XTABSTMENT_TYPE eStatementType);
    bool CheckUniverseSyntax(const CString& sUniverseStatement);
    void UpdateSpecialValuesCheckBoxes();
    void UpdateUnitsCombo();
    void SetLoopingVar(CUnitSpec* pUnitSpec);

    afx_msg void OnCbnSelchangeSubtables();
    afx_msg void OnCbnSelendokSubtables();
    afx_msg void OnCbnSelchangeBreaklevel();
    afx_msg void OnBnClickedUseCustSpecVal();
    afx_msg void OnBnClickedWghtApplyall();
    afx_msg void OnBnClickedUnivApplyall();
    afx_msg void OnBnClickedBtnTablogic();
    afx_msg void OnBnClickedBtnPostcalc();
    afx_msg void OnBnClickedApplyallUnit();
};

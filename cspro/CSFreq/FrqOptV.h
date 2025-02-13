#pragma once

#include <zEdit2O/LogicCtrl.h>


class CFrqOptionsView : public CFormView
{
    DECLARE_DYNCREATE(CFrqOptionsView)

protected:
    CFrqOptionsView();           // protected constructor used by dynamic creation

public:
    enum { IDD = IDD_FRQ_OPTIONS_FORMVIEW };

    CSFreqDoc* GetDocument() { return (CSFreqDoc*)m_pDocument; }

    void ToDoc();
    void FromDoc();

    bool CheckUniverseSyntax(const CString& sUniverseStatement);
    bool CheckWeightSyntax(CIMSAString sWeight);

    void RefreshLexer();

protected:
    DECLARE_MESSAGE_MAP()

    void OnInitialUpdate() override;
    void DoDataExchange(CDataExchange* pDX) override;

    afx_msg void OnDataChange();
    afx_msg void OnDataChange(UINT nID);
    afx_msg void OnBnClickedEditUniverse();

private:
    void UpdateControlVisibility();

private:
    CLogicCtrl m_cEditUniverse;

    int m_iUseVSet;
    int m_iNoStats;
    int m_percentilesIndex;
    int m_sortOrderAscending;
    int m_sortTypeIndex;
    OutputFormat m_outputFormat;
    CString m_sUniverse;
    CString m_sWeight;
};

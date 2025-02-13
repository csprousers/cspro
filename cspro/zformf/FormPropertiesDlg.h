#pragma once


/////////////////////////////////////////////////////////////////////////////
// CFormPropDlg dialog

class CFormPropDlg : public CDialog
{
public:
    CFormPropDlg(CDEGroup* pGroup, CFormScrollView* pParent);

    enum { IDD = IDD_FORMPROP };

protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;
    void OnOK() override;

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnColor();
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    afx_msg void OnApplyColor();
    afx_msg void OnResetColor();

    afx_msg void OnSetCapturePosCheck();
    afx_msg void OnSetCapturePosApply();
    afx_msg void OnSetCapturePosDisable();

private:
    bool ValidateSetCapturePos();

public:
    CString m_sFormLabel;
    CString m_sFormName;
    COLORREF m_frmcolor;
    BOOL m_bSetcaptureposChecked;
    int m_iSetcaptureposX;
    int m_iSetcaptureposY;

private:
    CDEGroup* m_pGroup;
    CFormScrollView* m_pMyParent;
};

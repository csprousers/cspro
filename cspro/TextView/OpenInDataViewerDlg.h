#pragma once

// COpenInDataViewerDlg dialog

class COpenInDataViewerDlg : public CDialog
{
    DECLARE_DYNAMIC(COpenInDataViewerDlg)

private:
    CButton* m_pDataViewerRadioButton;
    bool m_OpenInDataViewer;
    bool m_bRememberSetting;

public:
    COpenInDataViewerDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~COpenInDataViewerDlg() { }

    bool OpenInDataViewer() { return m_OpenInDataViewer; }
    bool RememberSetting() { return m_bRememberSetting; }

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DATA_VIEWER_QUERY };
#endif

protected:
    virtual BOOL OnInitDialog();
    virtual void OnOK();
};

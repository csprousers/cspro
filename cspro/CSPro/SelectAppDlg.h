#pragma once


class SelectAppDlg : public CDialog
{
public:
    SelectAppDlg(std::vector<CAplDoc*> application_docs, const TCHAR* dialog_title = nullptr, CWnd* pParent = nullptr);

    CAplDoc* GetSelectedApplicaton() const;

protected:
    enum { IDD = IDD_SELECT_APP };

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

private:
    std::vector<CAplDoc*> m_applicationDocs;
    CString m_dialogTitle;
    int m_selectedIndex;
};

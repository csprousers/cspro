#pragma once


class VSLabelRenamerDlg : public CDialog
{
public:
    VSLabelRenamerDlg(CWnd* pParent = nullptr);

    const CString& GetTemplate() const { return m_template; }

protected:
    void DoDataExchange(CDataExchange* pDX) override;

private:
    CString m_template;
};

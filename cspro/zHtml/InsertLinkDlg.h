#pragma once


class InsertLinkDlg : public CDialog
{
public:
	InsertLinkDlg(CWnd* pParent = nullptr);

protected:
    void DoDataExchange(CDataExchange* pDX) override;

    void OnOK() override;

public:
    std::wstring m_text;
    std::wstring m_url;
};

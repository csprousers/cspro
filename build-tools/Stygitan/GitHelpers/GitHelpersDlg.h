#pragma once


class GitHelpersDlg : public CDialog
{
public:
    GitHelpersDlg(CWnd* pParent = nullptr);
    ~GitHelpersDlg();

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
};

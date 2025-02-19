#pragma once


class WindowsOAuth2AuthorizerWaitDlg : public CDialog
{
public:
    WindowsOAuth2AuthorizerWaitDlg(std::string client_name, CWnd* pParent = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;

    void OnCancelClick();

private:
    std::string m_clientName;
};

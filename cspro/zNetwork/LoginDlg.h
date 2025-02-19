#pragma once

#include <zNetwork/UsernamePassword.h>


class LoginDlg : public CDialog
{
public:
    LoginDlg(bool show_invalid_error, CWnd* pParent = nullptr);

    UsernamePassword ReleaseUsernamePassword() { return std::move(m_usernamePassword); }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnOK() override;

    HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

private:
    bool m_showInvalidPasswordError;
    UsernamePassword m_usernamePassword;

    CStatic m_staticError;
};

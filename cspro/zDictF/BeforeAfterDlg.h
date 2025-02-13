#pragma once


class BeforeAfterDlg : public CDialog
{
public:
    BeforeAfterDlg(CWnd* pParent = nullptr);

    bool SelectedAfter() const { return m_after; }

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnAfter();
    afx_msg void OnBefore();

private:
    bool m_after;
};

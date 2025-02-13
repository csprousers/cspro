#pragma once


class OnKeyCharacterMapDlg : public CDialogEx
{
    DECLARE_DYNAMIC(OnKeyCharacterMapDlg)

public:
    OnKeyCharacterMapDlg(CWnd* pParent = nullptr);
    BOOL PreTranslateMessage(MSG* pMsg) override;
};

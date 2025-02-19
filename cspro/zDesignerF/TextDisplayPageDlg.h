#pragma once


class TextDisplayPageDlg : public CDialog
{
public:
    static unsigned int GetDialogTemplateId();

    TextDisplayPageDlg(std::wstring text, CWnd* pParent = nullptr);
    TextDisplayPageDlg(std::string_view text_sv, CWnd* pParent = nullptr);

protected:
    void DoDataExchange(CDataExchange* pDX) override;

private:
    std::wstring m_text;
};

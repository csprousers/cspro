#pragma once

#include <zHtml/zHtml.h>


class ZHTML_API InsertLinkDlg : public CDialog
{
public:
    InsertLinkDlg(std::string text, std::string url, CWnd* pParent = nullptr);

    const std::string& GetText() const { return m_text; }
    const std::string& GetUrl() const  { return m_url; }

protected:
    void DoDataExchange(CDataExchange* pDX) override;

    void OnOK() override;

public:
    std::string m_text;
    std::string m_url;
};

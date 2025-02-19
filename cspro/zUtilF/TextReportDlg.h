#pragma once

#include <zUtilF/zUtilF.h>


class CLASS_DECL_ZUTILF TextReportDlg : public CDialog
{
public:
    TextReportDlg(std::string heading, std::string content, CWnd* pParent = nullptr);

    void UseFixedWidthFont();

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnBnClickedCopyToClipboard();

private:
    std::string m_heading;
    std::string m_content;
    std::unique_ptr<CFont> m_fixedWidthFont;
};

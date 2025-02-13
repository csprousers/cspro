#pragma once

#include <zUtilF/zUtilF.h>


class CLASS_DECL_ZUTILF TextReportDlg : public CDialog
{
    DECLARE_DYNAMIC(TextReportDlg)

public:
    TextReportDlg(const CString& heading, const CString& content, CWnd* pParent = NULL);

    void UseFixedWidthFont();

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    afx_msg void OnBnClickedCopyToClipboard();

private:
    CString m_heading;
    CString m_content;
    std::unique_ptr<CFont> m_fixedWidthFont;
};

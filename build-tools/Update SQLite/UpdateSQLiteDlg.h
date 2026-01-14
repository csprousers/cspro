#pragma once

#include <zUtilO/ResizableDlg.h>


class UpdateSQLiteDlg : public ResizableDlg
{
public:
    UpdateSQLiteDlg(CWnd* pParent = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnSeeWebsiteClick(NMHDR* pNMHDR, LRESULT* pResult);

    void OnOK() override;

private:
    std::string m_seeDirectory;
    std::string m_seeEncryptionVariant;
};

#pragma once

#include <zUtilO/ResizableDlg.h>
#include <zUtilF/LoggingListBox.h>


class CSProUsersWebsiteBuilderDlg : public ResizableDlg
{
public:
    CSProUsersWebsiteBuilderDlg(CWnd* pParent = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

private:
    LoggingListBox m_loggingListBox;
};

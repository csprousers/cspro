#pragma once

#include "Inputs.h"
#include <zUtilO/ResizableDlg.h>
#include <zUtilO/SettingsDb.h>


class CSProUsersWebsiteBuilderDlg : public ResizableDlg
{
public:
    CSProUsersWebsiteBuilderDlg(CWnd* pParent = nullptr);
    ~CSProUsersWebsiteBuilderDlg();

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnCancel() override;

    void OnBuildTask(UINT nID);

    LRESULT OnBuildTaskComplete(WPARAM wParam, LPARAM lParam);
    LRESULT OnDisplayErrorMessage(WPARAM wParam, LPARAM lParam);

private:
    SettingsDb m_settingsDb;
    Inputs m_inputs;
    LoggingListBox m_loggingListBox;

    std::unique_ptr<std::thread> m_buildThread;
};

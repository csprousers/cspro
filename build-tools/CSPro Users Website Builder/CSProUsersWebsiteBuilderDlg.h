#pragma once

#include "Inputs.h"
#include <zUtilO/ResizableDlg.h>
#include <zUtilO/SettingsDb.h>
#include <zEditO/LogicCtrl.h>
#include <afxmenubutton.h>


class CSProUsersWebsiteBuilderDlg : public ResizableDlgEx
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

    void OnClearOutputs();

    LRESULT OnBuildTaskComplete(WPARAM wParam, LPARAM lParam);
    LRESULT OnDisplayErrorMessage(WPARAM wParam, LPARAM lParam);

private:
    SettingsDb m_settingsDb;
    Inputs m_inputs;

    CLogicCtrl m_clearOutputsExclusionsLogicCtrl;
    CMenu m_clearOutputsMenu;
    CMFCMenuButton m_clearOutputsButton;

    LoggingListBox m_loggingListBox;

    std::unique_ptr<std::thread> m_buildThread;
};

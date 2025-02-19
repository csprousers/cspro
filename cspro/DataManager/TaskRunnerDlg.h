#pragma once

#include <DataManager/Task.h>
#include <DataManager/TaskRunner.h>
#include <zUtilO/ResizableDlg.h>
#include <zUtilF/LoggingListBox.h>


class TaskRunnerDlg : public ResizableDlg, public TaskRunner
{
    class LoggingCaseConstructionReporter;

public:
    TaskRunnerDlg(std::unique_ptr<Task> task, CWnd* pParent = nullptr);

    void SetCloseDialogOnSuccess() { m_closeDialogOnSuccess = true; }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnDestroy();

    void OnOK() override;
    void OnCancel() override;

    LRESULT OnTaskEvent(WPARAM wParam, LPARAM lParam);

    // TaskRunner overrides
    std::shared_ptr<CaseConstructionReporter> GetCaseConstructionReporter(const CaseAccess& case_access) override;
    void SetTitle(const std::string& title) override;
    void LogText(SharableString text = SharableString()) override;
    void UpdateProgress(int percent) override;

private:
    void ToggleProgressBarMarquee();

    void OnTaskEvent_StartTask();
    void OnTaskEvent_SetTitle(SharableString text);
    void OnTaskEvent_Complete();

    void RunTask();

private:
    LoggingListBox m_loggingListBox;
    CProgressCtrl m_progressCtrl;
    bool m_usingMarquee;

    std::unique_ptr<Task> m_task;
    std::thread m_runThread;

    std::optional<Task::Result> m_taskResult;
    bool m_cancelFlag;
    bool m_closeDialogOnSuccess;
};

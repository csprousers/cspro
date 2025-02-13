#pragma once

#include <zUtilF/zUtilF.h>

#ifdef WIN_DESKTOP
// ----- 
// ----- Windows desktop implementation
// ----- 

#include <zUtilF/BatchMeterDlg.h>


class CLASS_DECL_ZUTILF ProcessSummaryDlg : public BatchMeterDlg
{
public:
    ProcessSummaryDlg(CWnd* pParent = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;

    LRESULT OnStartTask(WPARAM wParam, LPARAM lParam);


#else
// ----- 
// ----- portable implementation
// ----- 

#include <zUtilF/ProcessSummaryReporter.h>
#include <thread>


class CLASS_DECL_ZUTILF ProcessSummaryDlg : public ProcessSummaryReporter
{
public:
    ProcessSummaryDlg();

    void Initialize(const CString& title, std::shared_ptr<ProcessSummary> process_summary, bool* cancel_flag) override;

    void SetSource(const CString& source_text) override;

    void SetKey(const CString& case_key) override;

    void DoModal();

private:
    std::shared_ptr<ProcessSummary> m_processSummary;


#endif


// ----- 
// ----- shared implementation
// ----- 

public:
    ~ProcessSummaryDlg();

    bool IsCanceled() const { return m_canceled; }

    void SetTask(std::function<void()> task) { m_task = task; }

    void Initialize(const CString& title, std::shared_ptr<ProcessSummary> process_summary);

    void RethrowTaskExceptions();

private:
    void RunSharedDestructor();

    void RunTask();

private:
    std::unique_ptr<std::thread> m_workThread;
    bool m_canceled;
    std::function<void()> m_task;
    std::exception_ptr m_taskException;
};

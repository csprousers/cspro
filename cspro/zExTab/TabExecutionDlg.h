#pragma once

#include <zUtilF/ApplicationShutdownRunner.h>
#include <zUtilF/BatchMeterDlg.h>
#include <zEngineF/EngineUI.h>

class CRunAplBatch;
class ObjectTransporter;


class TabExecutionDlg : public BatchMeterDlg
{
public:
    TabExecutionDlg(const CNPifFile& pff, CRunAplBatch* run_apl_batch, const std::function<void()>& tab_function, CWnd* pParent = nullptr);
    ~TabExecutionDlg();

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;

    LRESULT OnGetObjectTransporter(WPARAM wParam, LPARAM lParam);

    LRESULT OnStartEngine(WPARAM wParam, LPARAM lParam);
    LRESULT OnGetProcessSummaryReporter(WPARAM wParam, LPARAM lParam);

    LRESULT OnEngineAbort(WPARAM wParam, LPARAM lParam);
    LRESULT OnEngineUI(WPARAM wParam, LPARAM lParam);
    LRESULT OnRunOnUIThread(WPARAM wParam, LPARAM lParam);
    LRESULT OnGetApplicationShutdownRunner(WPARAM wParam, LPARAM lParam);

private:
    const CNPifFile& m_pff;
    CRunAplBatch* m_runAplBatch;
    const std::function<void()>& m_tabFunction;

    std::unique_ptr<std::thread> m_engineThread;

    std::unique_ptr<ObjectTransporter> m_objectTransporter;
    EngineUIProcessor m_engineUIProcessor;
    ApplicationShutdownRunner m_applicationShutdownRunner;
};

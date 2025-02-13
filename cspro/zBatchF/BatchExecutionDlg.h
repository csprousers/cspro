#pragma once

#include <zBatchF/UWMCallback.h>
#include <zUtilF/ApplicationShutdownRunner.h>
#include <zUtilF/BatchMeterDlg.h>
#include <zEngineF/EngineUI.h>
#include <zBatchO/BatchDriver.h>
#include <zBatchO/Runaplb.h>

class ObjectTransporter;


class BatchExecutionDlg : public BatchMeterDlg
{
public:
    BatchExecutionDlg(const CNPifFile& pff, std::variant<CRunAplBatch*, BatchDriver*> batch_driver,
        std::map<unsigned, std::shared_ptr<UWMCallback>> uwm_callbacks, CWnd* pParent = nullptr);
    ~BatchExecutionDlg();

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;

    LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) override;

    LRESULT OnGetObjectTransporter(WPARAM wParam, LPARAM lParam);

    LRESULT OnStartEngine(WPARAM wParam, LPARAM lParam);
    LRESULT OnGetProcessSummaryReporter(WPARAM wParam, LPARAM lParam);

    LRESULT OnEngineAbort(WPARAM wParam, LPARAM lParam);
    LRESULT OnEngineUI(WPARAM wParam, LPARAM lParam);
    LRESULT OnRunOnUIThread(WPARAM wParam, LPARAM lParam);
    LRESULT OnGetApplicationShutdownRunner(WPARAM wParam, LPARAM lParam);

private:
    const CNPifFile& m_pff;
    std::variant<CRunAplBatch*, BatchDriver*> m_batchDriver;
    std::map<unsigned, std::shared_ptr<UWMCallback>> m_uwmCallbacks;

    std::unique_ptr<std::thread> m_engineThread;

    std::unique_ptr<ObjectTransporter> m_objectTransporter;
    EngineUIProcessor m_engineUIProcessor;
    ApplicationShutdownRunner m_applicationShutdownRunner;
};

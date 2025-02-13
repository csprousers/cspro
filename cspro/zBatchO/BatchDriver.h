#pragma once

#include <zBatchO/zBatchO.h>
#include <engine/Engdrv.h>

class EngineCase;


class ZBATCHO_API BatchDriver : protected CEngineDriver
{
private:
    BatchDriver(CNPifFile& pff);

public:
    ~BatchDriver();

    static std::unique_ptr<BatchDriver> Create(CNPifFile& pff);

    const CEngineArea* GetEngineArea() const { return m_pEngineArea; }
    EngineData& GetEngineData()              { return CEngineDriver::GetEngineData(); }

    void Run();

private:
    bool InitializeRun();
    void FinalizeRun();

    void ExecuteApplicationProc(ProcType proc_type);

    void RunBatchOnInputs();
    void RunBatchOnInput(const ConnectionString& input_connection_string, double repository_percent_fraction);

    void RunBatchOnCase();

    // overrides
protected:
    bool UseNewDriver() const override { return true; }

private:
    ProcessSummaryReporter* m_processSummaryReporter;
    EngineCase* m_engineCase;
};

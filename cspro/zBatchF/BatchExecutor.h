#pragma once

#include <zBatchF/zBatchF.h>
#include <zBatchO/Runaplb.h>
#include <zBatchF/UWMCallback.h>


class BatchExecutorCallback
{
public:
    virtual ~BatchExecutorCallback() { }

    virtual bool QueryForFilename(CString& pff_or_batch_filename) = 0;

    virtual bool QueryForFileAssociations(CNPifFile& pff, const EngineData& engine_data) = 0;
};


class ZBATCHF_API BatchExecutor
{
public:
    BatchExecutor(BatchExecutorCallback* batch_executor_callback = nullptr);

    void AddUWMCallback(unsigned message, std::shared_ptr<UWMCallback> uwm_callback);

    void Run(const CString& pff_or_batch_filename);

private:
    BatchExecutorCallback* m_batchExecutorCallback;
    std::map<unsigned, std::shared_ptr<UWMCallback>> m_uwmCallbacks;
};

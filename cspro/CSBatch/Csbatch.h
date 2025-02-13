#pragma once

#include <zBatchF/BatchExecutor.h>


class CSBatchApp : public CWinApp, public BatchExecutorCallback
{
public:
    CSBatchApp();

protected:
    DECLARE_MESSAGE_MAP()

    BOOL InitInstance() override;

    // implementing BatchExecutorCallback
    bool QueryForFilename(CString& pff_or_batch_filename) override;

    bool QueryForFileAssociations(CNPifFile& pff, const EngineData& engine_data) override;
};

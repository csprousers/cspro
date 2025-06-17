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
    bool QueryForFilePath(std::string& pff_or_batch_file_path) override;

    bool QueryForFileAssociations(CNPifFile& pff, const EngineData& engine_data) override;
};

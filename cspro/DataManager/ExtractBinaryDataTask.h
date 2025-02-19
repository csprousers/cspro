#pragma once

#include <DataManager/CaseTask.h>
#include <DataManager/ExtractBinaryDataSettings.h>


class ExtractBinaryDataTask : public CaseTask
{
public:
    ExtractBinaryDataTask(ExtractBinaryDataSettings settings);

    static void ValidateSettings(const ExtractBinaryDataSettings& settings);

    static std::string CreateValidFilename(const ExtractBinaryDataSettings& settings, const std::string& case_key, std::string filename);

protected:
    // Task and CaseTask overrides
    void Initialize() override;
    void ProcessCase(Case& data_case) override;
    void Finalize(Result result) override;

private:
    ExtractBinaryDataSettings m_settings;
    size_t m_casesWithBinaryData;
    size_t m_extractedFiles;
    size_t m_skippedFiles;
    std::map<std::string, std::string> m_writtenSignatures;
};

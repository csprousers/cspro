#pragma once

#include <zParadataO/zParadataO.h>
#include <zParadataO/Concatenator.h>

class PFF;
class ProcessSummary;
class ProcessSummaryDlg;
namespace Paradata { class GuiConcatenator; }


class ZPARADATAO_API Paradata::GuiConcatenator : public Paradata::Concatenator
{
public:
    static int64_t GetNumberEvents(const std::string& file_path);

    static bool Run(const PFF& pff);

private:
    GuiConcatenator(const PFF& pff);

    bool Run();

    void OnInputProcessedSuccess(const std::variant<std::string, sqlite3*>& file_path_or_database, int64_t events_processed) override;
    void OnInputProcessedError(const std::string& input_file_path, const char* error_message) override;

    void OnProgressUpdate(const std::string& operation_message, int operation_percent, const char* total_message, int total_percent) override;
    bool UserRequestsCancellation() override;

    struct ProcessedSuccess
    {
        std::string file_path;
        int64_t number_events;
    };

    struct ProcessedError
    {
        std::string file_path;
        std::string error_message;
    };

private:
    const PFF& m_pff;

    std::shared_ptr<ProcessSummary> m_processSummary;
    ProcessSummaryDlg* m_processSummaryDlg;

    std::vector<ProcessedSuccess> m_processedSuccesses;
    std::vector<ProcessedError> m_processedErrors;
};

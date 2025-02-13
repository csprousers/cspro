#pragma once

#include <zParadataO/zParadataO.h>
#include <zParadataO/Concatenator.h>

class ProcessSummary;
class ProcessSummaryDlg;


namespace Paradata
{
    // this class helps avoid linker errors
    class PffWrapper
    {
    public:
        virtual ~PffWrapper() { }
        virtual CString GetListingFName() const = 0;
        virtual std::vector<std::wstring> GetInputParadataFilenames() const = 0;
        virtual CString GetOutputParadataFilename() const = 0;
        virtual void ViewListing(bool errors_occurred) const = 0;
    };


    class ZPARADATAO_API GuiConcatenator : public Paradata::Concatenator
    {
    public:
        static int64_t GetNumberEvents(NullTerminatedString filename);

        static bool Run(const PffWrapper& pff);

    private:
        GuiConcatenator(const PffWrapper& pff);

        bool Run();

        void OnInputProcessedSuccess(const std::variant<std::wstring, sqlite3*>& filename_or_database, int64_t iEventsProcessed) override;
        void OnInputProcessedError(NullTerminatedString input_filename, const std::wstring& error_message) override;

        void OnProgressUpdate(const CString& csOperationMessage, int iOperationPercent, const CString& csTotalMessage, int iTotalPercent) override;
        bool UserRequestsCancellation() override;

        struct ProcessedSuccess
        {
            CString csFilename;
            int64_t iNumberEvents;
        };

        struct ProcessedError
        {
            CString csFilename;
            CString csErrorMessage;
        };

        const PffWrapper& m_pff;

        std::shared_ptr<ProcessSummary> m_processSummary;
        ProcessSummaryDlg* m_processSummaryDlg;

        std::vector<ProcessedSuccess> m_aProcessedSuccess;
        std::vector<ProcessedError> m_aProcessedError;
    };
}

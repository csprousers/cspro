#pragma once

#include <zExportO/ExportWriter.h>
#include <zDataO/DataRepository.h>


class CSProExportWriter : public ExportWriter
{
public:
    static constexpr bool MultipleLevelExportSupported = false;

    CSProExportWriter(std::shared_ptr<const CaseAccess> case_access, const ConnectionString& connection_string);
    ~CSProExportWriter();

    static ConnectionString GetDataConnectionString(const ConnectionString& connection_string);
    static std::string GetDictionaryFilePath(const ConnectionString& connection_string);

    void WriteCase(const Case& data_case) override;

    void Close() override;

private:
    void SetupSingleRecordCaseAccess(const std::string& record_name);
    void WriteSingleRecordCase(const Case& data_case);

private:
    std::shared_ptr<const CaseAccess> m_caseAccess;
    std::unique_ptr<DataRepository> m_dataRepository;
    std::string m_dictionaryFilePath;

    // for exporting only a single record
    struct SingleRecordCaseDetails
    {
        size_t source_case_record_number;
        CDataDict dictionary;
        std::unique_ptr<Case> data_case;
        CaseRecord* id_case_record;
        CaseRecord* single_record_case_record;
    };

    std::unique_ptr<SingleRecordCaseDetails> m_singleRecordCaseDetails;
};

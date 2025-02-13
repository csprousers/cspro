#pragma once

#include <zExportO/ExportWriterBase.h>
#include <zUtilO/ExpansiveList.h>
#include <external/librdata/rdata.h>


class RExportWriter : public ExportWriterBase
{
public:
    RExportWriter(std::shared_ptr<const CaseAccess> case_access, const ConnectionString& connection_string);
    ~RExportWriter();

    void Close() override;

protected:
    bool SupportsBinaryData() override { return false; }
    bool IsReservedName(const std::wstring& name, bool record_name) override;

    void StartRecord(const ExportRecordMapping& /*export_record_mapping*/) override { }
    void StartRow() override { }
    void EndRow() override { }

    void WriteCaseItem(const ExportItemMapping& export_item_mapping, const CaseItemIndex& index) override;

private:
    struct FactoredVector
    {
        std::unique_ptr<ExpansiveList<int>> expansive_list;
        std::vector<std::wstring> labels;
    };

private:
    void Open();

    void SetupExpansiveLists();

    void WriteExpansiveListsToR();

    void WriteExpansiveListToR(const ExportRecordMapping& export_record_mapping);

    template<typename T>
    std::unique_ptr<FactoredVector> FactorVector(const ExportItemMapping& export_item_mapping);

    void WriteExpansiveListValuesToR(ExpansiveList<double>& numeric_expansive_list);
    void WriteExpansiveListValuesToR(ExpansiveList<std::string>& string_expansive_list);
    void WriteExpansiveListValuesToR(FactoredVector& factored_vector);

private:
    FILE* m_file;
    rdata_writer_t* m_rWriter;

    // options from the connection string
    bool m_writeCodes;
    bool m_writeFactors;
    bool m_factorRanges;

    // tag values
    std::vector<std::shared_ptr<ExpansiveList<double>>> m_numericExpansiveLists;
    std::vector<std::shared_ptr<ExpansiveList<std::string>>> m_stringExpansiveLists;
};

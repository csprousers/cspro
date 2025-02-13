#pragma once

#include <zExportO/ExportWriterBase.h>
#include <zExportO/SasTransportWriter.h>

class EncodedTextWriter;


class SasExportWriter : public ExportWriterBase
{
public:
    SasExportWriter(std::shared_ptr<const CaseAccess> case_access, const ConnectionString& connection_string);
    ~SasExportWriter();

    static std::wstring GetSyntaxPath(const ConnectionString& connection_string);

    void Close() override;

protected:
    bool SupportsBinaryData() override { return false; }
    bool IsReservedName(const std::wstring& name, bool record_name) override;

    void StartRecord(const ExportRecordMapping& export_record_mapping) override;
    void StartRow() override;
    void EndRow() override;

    void WriteCaseItem(const ExportItemMapping& export_item_mapping, const CaseItemIndex& index) override;

private:
    void Open();

    void InitializeDataSets();

    std::string CreateSasName(const void* data_entity, const std::wstring& name, std::set<std::string> used_names[]);
    std::string CreateSasLabel(const void* data_entity, const std::wstring& label);

    static std::wstring EscapeSasLiteral(std::wstring text);

    void WriteSyntaxFile();
    void CreateFormatsAndWriteSyntax();
    void WriteDataSetSyntax(const SasTransportWriter::DataSet& data_set, const std::string& data_set_name);

private:
    // transport file variables
    std::unique_ptr<SasTransportWriter> m_sasTransportWriter;
    std::vector<std::shared_ptr<SasTransportWriter::DataSet>> m_dataSets;
    bool m_writeNextCaseItemsDirectly;

    // syntax file variables
    std::unique_ptr<EncodedTextWriter> m_syntaxFileWriter;
    std::set<std::string> m_usedDataSetNames[2]; // SAS/transport names
    std::map<const void*, std::string> m_renameMap;
    std::map<const void*, std::string> m_relabelMap;
    std::map<const void*, std::wstring> m_formatMap;

    // options from the connection string
    bool m_useSasMissingCodes;
};

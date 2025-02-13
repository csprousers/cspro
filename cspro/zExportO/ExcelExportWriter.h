#pragma once

#include <zExportO/ExportWriterBase.h>
#include <zExcelO/ExcelWriter.h>


class ExcelExportWriter : public ExportWriterBase
{
public:
    ExcelExportWriter(std::shared_ptr<const CaseAccess> case_access, const ConnectionString& connection_string);
    ~ExcelExportWriter();

    void Close() override;

protected:
    bool SupportsBinaryData() override { return false; }
    bool IsReservedName(const std::wstring& name, bool record_name) override;

    void StartRecord(const ExportRecordMapping& export_record_mapping) override;
    void StartRow() override;
    void EndRow() override;

    void WriteCaseItem(const ExportItemMapping& export_item_mapping, const CaseItemIndex& index) override;

private:
    void Open(const ConnectionString& connection_string);

    void SetupWorksheets();

private:
    std::unique_ptr<ExcelWriter> m_excelWriter;

    // some information used for managing the mappings (tag values)
    struct RecordMappingInfo
    {
        size_t sheet_index;
        uint32_t row;
    };

    struct ColumnMappingInfo
    {
        uint16_t column;
        std::shared_ptr<const ValueProcessor> value_processor_for_labels;
    };

    struct ItemMappingInfo
    {
        const uint32_t& row;
        std::vector<ColumnMappingInfo> column_mapping_infos;
    };

    std::vector<std::shared_ptr<RecordMappingInfo>> m_recordMappingInfos;
    std::vector<std::shared_ptr<ItemMappingInfo>> m_itemMappingInfos;

    RecordMappingInfo* m_recordMappingInfoBeingWritten;
};

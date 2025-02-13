#pragma once

#include <zExportO/SingleRecordExportWriterBase.h>

class DelimitedTextCreator;
class EncodedTextWriter;


class DelimitedTextExportWriter : public SingleRecordExportWriterBase
{
public:
    DelimitedTextExportWriter(DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access, const ConnectionString& connection_string);
    ~DelimitedTextExportWriter();

    void Close() override;

protected:
    bool SupportsBinaryData() override                                               { return false; }
    bool IsReservedName(const std::wstring& /*name*/, bool /*record_name*/) override { return false; }

private:
    void InitializeSingleExportRecordMapping();

    void WriteLine();

    void StartRecord(const ExportRecordMapping& export_record_mapping) override;
    void StartRow() override;
    void EndRow() override;

    void WriteCaseItem(const ExportItemMapping& export_item_mapping, const CaseItemIndex& index) override;

private:
    std::unique_ptr<EncodedTextWriter> m_encodedTextWriter;
    std::unique_ptr<DelimitedTextCreator> m_delimitedTextCreator;

    // options from the connection string
    bool m_commaDecimalMark;

    // some information used for managing the mappings (tag values)
    struct ColumnMappingInfo
    {
        std::shared_ptr<const ValueProcessor> value_processor_for_labels;
        bool use_label_for_header;
    };

    struct FixedWidthNumericWorkingSpace
    {
        size_t text_length;
        size_t number_decimals;
        bool process_decimal_mark;
    };

    struct ItemMappingInfo
    {
        std::vector<ColumnMappingInfo> column_mapping_infos;
        std::unique_ptr<FixedWidthNumericWorkingSpace> fixed_width_numeric_working_space;
    };

    std::vector<std::shared_ptr<ItemMappingInfo>> m_itemMappingInfos;
};

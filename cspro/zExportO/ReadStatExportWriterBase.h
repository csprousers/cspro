#pragma once

#include <zExportO/SingleRecordExportWriterBase.h>
#include <external/ReadStat/readstat.h>


class ReadStatExportWriterBase : public SingleRecordExportWriterBase
{
public:
    ReadStatExportWriterBase(DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access, const ConnectionString& connection_string);
    ~ReadStatExportWriterBase();

    void Close() override;

    void RecordRowCountPosition(size_t count_position, size_t count_size, char count_is_unsigned);

    void RecordEmitMapVariables(size_t map_position, size_t record_len, uint64_t map10, uint64_t map11, uint64_t map12, uint64_t map13);

protected:
    bool SupportsBinaryData() override { return false; }

    void Initialize();

private:
    void Open();

    void InitializeReadStatVariables();

    void UpdateRowCounts();

    void StartRecord(const ExportRecordMapping& export_record_mapping) override;
    void StartRow() override;
    void EndRow() override;

    void WriteCaseItem(const ExportItemMapping& export_item_mapping, const CaseItemIndex& index) override;

protected:
    // methods to be overriden by the SPSS/Stata writers
    virtual bool AddStringLabelSets() = 0;

    virtual std::wstring GetFixedWidthNumericFormat(const CDictItem& dict_item) = 0;
    virtual std::optional<std::wstring> GetFixedWidthStringFormat(unsigned width) = 0;

    virtual readstat_error_t StartReadStatWriter(readstat_writer_t *writer, void *user_ctx, long row_count) = 0;

private:
    FILE* m_file;
    readstat_writer_t* m_readStatWriter;

    // row count variables
    struct RowCountPosition
    {
        size_t count_position;
        size_t count_size;
        char count_is_unsigned;
    };

    struct EmitMapVariables
    {
        size_t position;
        size_t record_len;
        uint64_t map_values[4];
    };

    std::vector<RowCountPosition> m_rowCountPositions;
    std::optional<EmitMapVariables> m_emitMapVariables;
    uint64_t m_rowCount;
};

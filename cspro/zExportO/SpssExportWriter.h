#pragma once

#include <zExportO/ReadStatExportWriterBase.h>


class SpssExportWriter : public ReadStatExportWriterBase
{
public:
    SpssExportWriter(std::shared_ptr<const CaseAccess> case_access, const ConnectionString& connection_string);

private:
    bool IsReservedName(const std::wstring& name, bool record_name) override;

    bool AddStringLabelSets() override;

    std::wstring GetFixedWidthNumericFormat(const CDictItem& dict_item) override;
    std::optional<std::wstring> GetFixedWidthStringFormat(unsigned width) override;

    readstat_error_t StartReadStatWriter(readstat_writer_t* writer, void* user_ctx, long row_count) override;
};

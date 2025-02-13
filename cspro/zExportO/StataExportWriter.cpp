#include "stdafx.h"
#include "StataExportWriter.h"


StataExportWriter::StataExportWriter(std::shared_ptr<const CaseAccess> case_access, const ConnectionString& connection_string)
    :   ReadStatExportWriterBase(DataRepositoryType::Stata, std::move(case_access), connection_string)
{
    Initialize();
}


bool StataExportWriter::IsReservedName(const std::wstring& /*name*/, bool /*record_name*/)
{
    // Stata reserved names are all lowercase (https://www.stata.com/manuals/u11.pdf),
    // so they will not clash with CSPro names
    return false;
}


bool StataExportWriter::AddStringLabelSets()
{
    return false;
}


std::wstring StataExportWriter::GetFixedWidthNumericFormat(const CDictItem& dict_item)
{
    return FormatTextCS2WS(_T("%%%d.%df"), static_cast<int>(dict_item.GetCompleteLen()),
                                           static_cast<int>(dict_item.GetDecimal()));
}


std::optional<std::wstring> StataExportWriter::GetFixedWidthStringFormat(unsigned /*width*/)
{
    return std::nullopt;
}


readstat_error_t StataExportWriter::StartReadStatWriter(readstat_writer_t* writer, void* user_ctx, const long row_count)
{
    return readstat_begin_writing_dta(writer, user_ctx, row_count);
}

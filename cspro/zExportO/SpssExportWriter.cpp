#include "stdafx.h"
#include "SpssExportWriter.h"

extern "C"
{
#include <external/ReadStat/spss/readstat_spss.h>
}


SpssExportWriter::SpssExportWriter(std::shared_ptr<const CaseAccess> case_access, const ConnectionString& connection_string)
    :   ReadStatExportWriterBase(DataRepositoryType::SPSS, std::move(case_access), connection_string)
{
    Initialize();
}


bool SpssExportWriter::IsReservedName(const std::string& name, bool /*record_name*/)
{
    return ( sav_validate_name_unreserved(name.c_str()) == READSTAT_ERROR_NAME_IS_RESERVED_WORD );
}


bool SpssExportWriter::AddStringLabelSets()
{
    return true;
}


std::string SpssExportWriter::GetFixedWidthNumericFormat(const CDictItem& dict_item)
{
    return FormatText("F%d.%d", static_cast<int>(dict_item.GetCompleteLen()),
                                static_cast<int>(dict_item.GetDecimal()));
}


std::optional<std::string> SpssExportWriter::GetFixedWidthStringFormat(const unsigned width)
{
    return FormatText("A%d", static_cast<int>(width));
}


readstat_error_t SpssExportWriter::StartReadStatWriter(readstat_writer_t* const writer, void* const user_ctx, const long row_count)
{
    return readstat_begin_writing_sav(writer, user_ctx, row_count);
}

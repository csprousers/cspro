#pragma once

#include <zCaseO/NumericCaseItem.h>
#include <zCaseO/StringCaseItem.h>
#include <zJson/Json.h>


namespace CaseItemJsonWriter
{
    void WriteCaseItemCode(JsonWriter& json_writer, const NumericCaseItem& numeric_case_item, const double& value);
    void WriteCaseItemCode(JsonWriter& json_writer, const NumericCaseItem& numeric_case_item, const CaseItemIndex& index);

    void WriteCaseItemCode(JsonWriter& json_writer, const StringCaseItem& string_case_item, wstring_view value_sv);
    void WriteCaseItemCode(JsonWriter& json_writer, const StringCaseItem& string_case_item, const CaseItemIndex& index);
}



// --------------------------------------------------------------------------
// NumericCaseItem writing
// --------------------------------------------------------------------------

inline void CaseItemJsonWriter::WriteCaseItemCode(JsonWriter& json_writer, const NumericCaseItem& numeric_case_item, const double& value)
{
    if( numeric_case_item.GetDictionaryItem().GetDecimal() > 0 || IsSpecial(value) )
    {
        json_writer.WriteEngineValue(value);
    }

    else
    {
        json_writer.Write(static_cast<int64_t>(value));
    }
}


inline void CaseItemJsonWriter::WriteCaseItemCode(JsonWriter& json_writer, const NumericCaseItem& numeric_case_item, const CaseItemIndex& index)
{
    ASSERT(!numeric_case_item.IsBlank(index));
    WriteCaseItemCode(json_writer, numeric_case_item, numeric_case_item.GetValueForOutput(index));
}



// --------------------------------------------------------------------------
// StringCaseItem writing
// --------------------------------------------------------------------------

inline void CaseItemJsonWriter::WriteCaseItemCode(JsonWriter& json_writer, const StringCaseItem& string_case_item, wstring_view value_sv)
{
    if( string_case_item.IsTypeFixed() )
        value_sv = SO::TrimRightSpace(value_sv);

    json_writer.Write(value_sv);
}


inline void CaseItemJsonWriter::WriteCaseItemCode(JsonWriter& json_writer, const StringCaseItem& string_case_item, const CaseItemIndex& index)
{
    ASSERT(!string_case_item.IsBlank(index));
    WriteCaseItemCode(json_writer, string_case_item, string_case_item.GetValue(index));
}

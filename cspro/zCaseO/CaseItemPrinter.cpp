#include "stdafx.h"
#include "CaseItemPrinter.h"
#include "BinaryCaseItem.h"
#include "NumericCaseItem.h"
#include "StringCaseItem.h"
#include <zToolsO/NumberConverter.h>
#include <zDictO/ValueProcessor.h>

/*
The formatting rules:

    Format::Label
        1) if a label exists for the code, use it
        2) otherwise, format as if using Format::Code

    Format::Code
        1) if string, right-trim the code
        2) if numeric:
            a) if notappl, output a blank
            b) if otherwise special, use its assigned code if one exists; otherwise use the special value's text
            c) otherwise, output the value, ensuring that -0. or 0. appear before values in the range (-10, 10) that have decimal components
                c1) if fixed-width numeric, output with the number of defined decimals
                c2) otherwise, output with only the number of decimals necessary (but with at least one decimal for decimal values)

    Format::LabelCode / Format::CodeLabel
        1) if a label exists and is different from the code, output as "<label>: <code>" or "<code>: <label>"
        2) otherwise, output the code

    Format::CaseTree
        1) format as if using Format::LabelCode except:
            a) when formatting codes, do not handle any numerics as fixed-width
            b) when formatting labels, do not use labels when the first value pair contains a range
*/


CaseItemPrinter::CaseItemPrinter(const Format format)
    :   m_format(format)
{
}


template<typename T>
std::string CaseItemPrinter::GetLabel(const CaseItem& case_item, const T& value) const
{
    // get the value processor
    const ValueProcessor* value_processor;
    const auto& value_processor_lookup = m_valueProcessors.find(&case_item);

    if( value_processor_lookup != m_valueProcessors.end() )
    {
        value_processor = value_processor_lookup->second.get();
    }

    else
    {
        // create the value processor for this item
        // CR_TODO this is not thread safe
        const CDictItem& dict_item = case_item.GetDictItem();
        value_processor = m_valueProcessors.emplace(&case_item, ValueProcessor::CreateValueProcessor(dict_item, dict_item.GetFirstValueSetOrNull())).first->second.get();
    }

    // when using checkboxes, process each component separately
    if constexpr(std::is_same_v<T, std::string>)
    {
        if( case_item.GetDictItem().GetCaptureInfo().GetCaptureType() == CaptureType::CheckBox )
            return UTF8_TODO::GetUtf8(CheckBoxCaptureInfo::GetResponseLabel(UTF8_TODO::GetCString(value), *value_processor));
    }

    // otherwise search for the label
    const DictValue* dict_value = value_processor->GetDictValue(value);

    if( dict_value != nullptr )
    {
        // for the case tree, only use labels for discrete values
        if( ( m_format != Format::CaseTree ) ||
            ( dict_value->HasValuePairs() && dict_value->GetValuePair(0).GetTo().IsEmpty() ) )
        {
            return UTF8_TODO::GetUtf8(dict_value->GetLabel());
        }
    }

    return std::string();
}


std::string CaseItemPrinter::GetText(const CaseItem& case_item, const CaseItemIndex& index) const
{
    std::string label;
    std::string code;

    switch( case_item.GetDataType() )
    {
        case DataType::Numeric:
            GetText(assert_cast<const NumericCaseItem&>(case_item), index, label, code);
            break;

        case DataType::String:
            GetText(assert_cast<const StringCaseItem&>(case_item), index, label, code);
            break;

        case DataType::Binary:
            GetText(assert_cast<const BinaryCaseItem&>(case_item), index, label, code);
            break;

        default:
            ASSERT(false);
    }

    // join the label and the code, avoiding identical labels and codes
    if( !label.empty() && !code.empty() && code != label )
    {
        SO::MakeTrimRight(label);

        if( m_format == Format::CodeLabel )
            std::swap(code, label);

        label.append(": ");
        label.append(code);
    }

    return label.empty() ? code :
                           label;
}


void CaseItemPrinter::GetText(const NumericCaseItem& numeric_case_item, const CaseItemIndex& index, std::string& label, std::string& code) const
{
    // GetValue, not GetValueForOutput, is used for the label because the non-serializable value is needed
    if( UseLabel() )
        label = GetLabel(numeric_case_item, numeric_case_item.GetValue(index));

    if( UseCode() || label.empty() )
        code = FormatNumber(numeric_case_item, numeric_case_item.GetValueForOutput(index));
}


void CaseItemPrinter::GetText(const StringCaseItem& string_case_item, const CaseItemIndex& index, std::string& label, std::string& code) const
{
    code = string_case_item.GetValue(index);

    if( UseLabel() )
        label = GetLabel(string_case_item, code);

    if( UseCode() || label.empty() )
    {
        // right-trim spaces from the text
        SO::MakeTrimRightSpace(code);
    }

    else
    {
        code.clear();
    }
}


void CaseItemPrinter::GetText(const BinaryCaseItem& binary_case_item, const CaseItemIndex& index, std::string& /*label*/, std::string& code) const
{
    const BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(index);

    if( !binary_data_accessor.IsDefined() )
        return;

    const BinaryDataMetadata& binary_data_metadata = binary_data_accessor.GetBinaryDataMetadata();

    code = binary_data_metadata.GetEvaluatedLabel();

    // if the evaluated label is not blank, add the filename (if different)
    if( !code.empty() )
    {
        const std::optional<std::string> filename = binary_data_metadata.GetFilename();

        if( filename.has_value() && code != *filename )
        {
            code.append("\n\n");
            code.append(*filename);
        }
    }

    // if the evaluated label is blank, try to use the MIME type
    else
    {
        std::optional<std::string> mime_type = binary_data_metadata.GetMimeType();

        if( mime_type.has_value() )
        {
            code = std::move(*mime_type);
        }

        // otherwise add whatever metadata exists
        else
        {
            for( const auto& [attribute, value] : binary_data_metadata.GetProperties() )
            {
                SO::AppendWithSeparator(code, attribute, "\n");

                if( !value.empty() )
                {
                    code.append(": ");
                    code.append(value);
                }
            }
        }
    }

    if( code.empty() )
        code = "Binary Data";
}


std::string CaseItemPrinter::FormatNumber(const CDictItem& dict_item, const double value, const bool case_tree_format)
{
    ASSERT(IsNumeric(dict_item));

    if( !IsSpecial(value) )
    {
        std::string value_text = NumberConverter::DoubleToText(value, dict_item.GetCompleteLen(), dict_item.GetDecimal(), false, true);

        // left-trim any spaces
        SO::MakeTrimLeft(value_text);

        if( dict_item.GetDecimal() > 0 )
        {
            // for the case tree, remove any excess decimal zeros
            if( case_tree_format )
            {
                SO::MakeTrimRight(value_text, '0');

                // make sure that there is at least one value after the decimal mark
                if( value_text.back() == '.' )
                    value_text.push_back('0');
            }

            // don't allow strings to start with a decimal mark
            if( value_text.front() == '.' )
            {
                ASSERT(false);
                value_text.insert(0, 1, '0');
            }

            else if( SO::StartsWith(value_text, "-.") )
            {
                ASSERT(false);
                value_text.insert(1, 1, '0');
            }
        }

        return value_text;
    }

    // return blanks for notappl but other special values will use their string values
    else if( value == NOTAPPL )
    {
        return std::string();
    }

    else
    {
        return SpecialValues::ValueToString(value);
    }
}


std::string CaseItemPrinter::FormatNumber(const CaseItem& case_item, const double value) const
{
    ASSERT(case_item.IsFixedWidth());
    return FormatNumber(case_item.GetDictItem(), value, ( m_format == Format::CaseTree ));
}

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


CaseItemPrinter::CaseItemPrinter(Format format)
    :   m_format(format)
{
}


template<typename T>
std::wstring CaseItemPrinter::GetLabel(const CaseItem& case_item, const T& value) const
{
    // get the value processor
    std::shared_ptr<const ValueProcessor> value_processor;
    const auto& value_processor_lookup = m_valueProcessors.find(&case_item);

    if( value_processor_lookup != m_valueProcessors.end() )
    {
        value_processor = value_processor_lookup->second;
    }

    else
    {
        // create the value processor for this item
        // CR_TODO this is not thread safe
        const CDictItem& dict_item = case_item.GetDictionaryItem();
        value_processor = ValueProcessor::CreateValueProcessor(dict_item, dict_item.GetFirstValueSetOrNull());
        m_valueProcessors.emplace(&case_item, value_processor);
    }

    // when using checkboxes, process each component separately
    if constexpr(std::is_same_v<T, CString>)
    {
        if( case_item.GetDictionaryItem().GetCaptureInfo().GetCaptureType() == CaptureType::CheckBox )
            return CS2WS(CheckBoxCaptureInfo::GetResponseLabel(value, *value_processor));
    }

    // otherwise search for the label
    const DictValue* dict_value = value_processor->GetDictValue(value);

    if( dict_value != nullptr )
    {
        // for the case tree, only use labels for discrete values
        if( ( m_format != Format::CaseTree ) ||
            ( dict_value->HasValuePairs() && dict_value->GetValuePair(0).GetTo().IsEmpty() ) )
        {
            return CS2WS(dict_value->GetLabel());
        }
    }

    return std::wstring();
}


std::wstring CaseItemPrinter::GetText(const CaseItem& case_item, const CaseItemIndex& index) const
{
    std::wstring label;
    std::wstring code;
    const bool use_label = ( m_format != Format::Code );
    bool use_code = ( m_format != Format::Label );

    // format numerics
    if( case_item.IsTypeNumeric() )
    {
        const NumericCaseItem& numeric_case_item = assert_cast<const NumericCaseItem&>(case_item);

        // GetValue, not GetValueForOutput, is used for the label because the non-serializable value is needed
        if( use_label )
        {
            label = GetLabel(numeric_case_item, numeric_case_item.GetValue(index));
            use_code |= label.empty();
        }

        if( use_code )
            code = FormatNumber(numeric_case_item, numeric_case_item.GetValueForOutput(index));
    }


    // format strings
    else if( case_item.IsTypeString() )
    {
        code = CS2WS(assert_cast<const StringCaseItem&>(case_item).GetValue(index));

        if( use_label )
        {
            label = GetLabel(case_item, WS2CS(code));
            use_code |= label.empty();
        }

        if( use_code )
        {
            // right-trim spaces from the text
            SO::MakeTrimRightSpace(code);
        }

        else
        {
            code.clear();
        }
    }


    // format binary data
    else if( case_item.IsTypeBinary() )
    {
        const BinaryCaseItem& binary_case_item = assert_cast<const BinaryCaseItem&>(case_item);
        const BinaryDataMetadata* binary_data_metadata = binary_case_item.GetBinaryDataMetadata_noexcept(index);

        if( binary_data_metadata != nullptr )
        {
            code = binary_data_metadata->GetEvaluatedLabel();

            // if the evaluated label is not blank, add the filename (if different)
            if( !code.empty() )
            {
                const std::optional<std::wstring> filename = binary_data_metadata->GetFilename();

                if( filename.has_value() && code != *filename )
                {
                    code.append(_T("\n\n"));
                    code.append(*filename);
                }
            }

            // if the evaluated label is blank, try to use the MIME type
            else
            {
                const std::optional<std::wstring> mime_type = binary_data_metadata->GetMimeType();

                if( mime_type.has_value() )
                {
                    code = WS2CS(*mime_type);
                }

                // otherwise add whatever metadata exists
                else
                {
                    for( const auto& [attribute, value] : binary_data_metadata->GetProperties() )
                    {
                        SO::Append(code, code.empty() ? _T("") : _T("\n"),
                                         attribute.c_str(),
                                         value.empty() ? _T("") : _T(": "),
                                         value.c_str());
                    }
                }
            }

            if( code.empty() )
                code = _T("Binary Data");
        }
    }


    else
    {
        ASSERT(false);
    }


    // join the label and the code, avoiding identical labels and codes
    if( !label.empty() && !code.empty() && code != label )
    {
        SO::MakeTrimRight(label);

        if( m_format == Format::CodeLabel )
            std::swap(code, label);

        label.append(_T(": "));
        label.append(code);
    }

    return label.empty() ? code : label;
}


std::wstring CaseItemPrinter::FormatNumber(const CaseItem& case_item, double value) const
{
    if( !IsSpecial(value) )
    {
        ASSERT(case_item.IsTypeFixed());
        const CDictItem& dict_item = case_item.GetDictionaryItem();
        const size_t numeric_length = dict_item.GetCompleteLen();

        std::wstring value_text(numeric_length, '\0');
        NumberConverter::DoubleToText(value, value_text.data(), numeric_length, dict_item.GetDecimal(), false, true);

        // left-trim any spaces
        SO::MakeTrimLeft(value_text);

        if( dict_item.GetDecimal() > 0 )
        {
            // for the case tree, remove any excess decimal zeros
            if( m_format == Format::CaseTree )
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

            else if( SO::StartsWith(value_text, _T("-.")) )
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
        return std::wstring();
    }

    else
    {
        return SpecialValues::ValueToString(value);
    }
}

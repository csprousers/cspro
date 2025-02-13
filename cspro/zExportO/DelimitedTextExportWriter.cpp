#include "stdafx.h"
#include "DelimitedTextExportWriter.h"
#include "ExportPropertiesValuesProcessor.h"
#include "EncodedTextWriter.h"
#include <zToolsO/DelimitedTextCreator.h>
#include <zToolsO/NumberConverter.h>


DelimitedTextExportWriter::DelimitedTextExportWriter(const DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access,
                                                     const ConnectionString& connection_string)
    :   SingleRecordExportWriterBase(type, std::move(case_access), connection_string)
{
    m_commaDecimalMark = ( m_connectionString.HasProperty(CSProperty::decimalMark, CSValue::comma) ||
                           m_connectionString.HasProperty(_T("decimal-mark"), CSValue::comma) ); // pre-8.0

    CreateExportRecordMappings();
    InitializeSingleExportRecordMapping();

    const DelimitedTextCreator::Type delimited_text_creator_type =
          ( type == DataRepositoryType::CommaDelimited )     ?   DelimitedTextCreator::Type::CSV :
          ( type == DataRepositoryType::SemicolonDelimited ) ?   DelimitedTextCreator::Type::Semicolon :
        /*( type == DataRepositoryType::TabDelimited )       ? */DelimitedTextCreator::Type::Tab;

    // for supported formats, write newlines as \r\n because that is how EncodedTextWriter writes lines
    const DelimitedTextCreator::NewlineType newline_type =
        ( type == DataRepositoryType::SemicolonDelimited ) ? DelimitedTextCreator::NewlineType::Remove :
                                                             DelimitedTextCreator::NewlineType::WriteAsCRLF;

    m_delimitedTextCreator = std::make_unique<DelimitedTextCreator>(delimited_text_creator_type, newline_type);

    // open the file
    m_encodedTextWriter = std::make_unique<EncodedTextWriter>(type, *m_caseAccess, m_connectionString.GetFilename(), m_connectionString);

    // write the column headings
    if( !m_connectionString.HasProperty(CSProperty::header, CSValue::suppress) &&
        !m_connectionString.HasProperty(CSProperty::header, _T("none")) ) // pre-8.0
    {
        for( const ExportItemMapping& export_item_mapping : m_singleExportRecordMapping->item_mappings )
        {
            const ItemMappingInfo* item_mapping_info = static_cast<const ItemMappingInfo*>(export_item_mapping.tag);

            for( const ColumnMappingInfo& column_mapping_info : item_mapping_info->column_mapping_infos )
            {
                m_delimitedTextCreator->AddText(column_mapping_info.use_label_for_header ? CS2WS(export_item_mapping.case_item->GetDictionaryItem().GetLabel()) :
                                                                                           export_item_mapping.formatted_item_name);
            }
        }

        WriteLine();
    }
}


DelimitedTextExportWriter::~DelimitedTextExportWriter()
{
    Close();
}


void DelimitedTextExportWriter::InitializeSingleExportRecordMapping()
{
    ExportPropertiesValuesProcessor export_properties_values_processor(m_connectionString);

    for( ExportItemMapping& export_item_mapping : m_singleExportRecordMapping->item_mappings )
    {
        ItemMappingInfo* item_mapping_info = m_itemMappingInfos.emplace_back(std::make_shared<ItemMappingInfo>()).get();
        export_item_mapping.tag = item_mapping_info;

        // add columns for codes and/or labels
        const CDictItem& dict_item = export_item_mapping.case_item->GetDictionaryItem();

        export_properties_values_processor.Process(dict_item,
            [&](std::shared_ptr<const ValueProcessor> value_processor, const bool use_label_for_header)
            {
                item_mapping_info->column_mapping_infos.emplace_back(ColumnMappingInfo { std::move(value_processor), use_label_for_header });
            });

        // create the working space for fixed-width numeric values
        if( export_item_mapping.case_item->IsTypeNumeric() && export_item_mapping.case_item->IsTypeFixed() )
        {
            item_mapping_info->fixed_width_numeric_working_space = std::make_unique<FixedWidthNumericWorkingSpace>(
                FixedWidthNumericWorkingSpace
                {
                    dict_item.GetCompleteLen(),
                    dict_item.GetDecimal(),
                    ( m_commaDecimalMark && dict_item.GetDecimal() > 0 )
                });
        }
    }
}


void DelimitedTextExportWriter::Close()
{
    m_encodedTextWriter.reset();
}


void DelimitedTextExportWriter::WriteLine()
{
    m_encodedTextWriter->WriteLine(wstring_view(m_delimitedTextCreator->GetTextBuffer(), m_delimitedTextCreator->GetTextLength()));
    m_delimitedTextCreator->ResetText();
}


void DelimitedTextExportWriter::StartRecord(const ExportRecordMapping& /*export_record_mapping*/)
{
}


void DelimitedTextExportWriter::StartRow()
{
}


void DelimitedTextExportWriter::EndRow()
{
    WriteLine();
}


void DelimitedTextExportWriter::WriteCaseItem(const ExportItemMapping& export_item_mapping, const CaseItemIndex& index)
{
    const ItemMappingInfo* item_mapping_info = static_cast<const ItemMappingInfo*>(export_item_mapping.tag);
    
    for( const ColumnMappingInfo& column_mapping_info : item_mapping_info->column_mapping_infos )
    {
        auto write_label_if_valid = [&](const auto& value)
        {
            if( column_mapping_info.value_processor_for_labels != nullptr )
            {
                const DictValue* dict_value = column_mapping_info.value_processor_for_labels->GetDictValue(value);

                if( dict_value != nullptr && !dict_value->GetLabel().IsEmpty() )
                {
                    m_delimitedTextCreator->AddText(dict_value->GetLabel());
                    return true;
                }
            }

            return false;
        };


        // blank values
        auto write_blank_value = [&]()
        {
            m_delimitedTextCreator->AddText(SO::EmptyString);
        };

        if( export_item_mapping.case_item->IsBlank(index) )
        {
            write_blank_value();
        }


        // numeric values
        else if( export_item_mapping.case_item->IsTypeNumeric() )
        {
            const NumericCaseItem& numeric_case_item = assert_cast<const NumericCaseItem&>(*export_item_mapping.case_item);
            double value = numeric_case_item.GetValue(index);

            if( !write_label_if_valid(value) )
            {
                ModifyValueForOutput(numeric_case_item, value);

                std::wstring value_text;
                bool process_decimal_mark;

                // use the text for the special value
                if( IsSpecial(value) )
                {
                    // if notappl, print a blank value and get out
                    if( value == NOTAPPL )
                    {
                        write_blank_value();
                        return;
                    }

                    value_text = SpecialValues::ValueToString(value);
                    process_decimal_mark = false;
                }

                // for fixed-width numerics, format the value as specified in the dictionary
                else if( item_mapping_info->fixed_width_numeric_working_space != nullptr )
                {
                    const FixedWidthNumericWorkingSpace& fwnws = *item_mapping_info->fixed_width_numeric_working_space;

                    value_text.resize(fwnws.text_length);
                    NumberConverter::DoubleToText(value, value_text.data(), fwnws.text_length, fwnws.number_decimals, false, true);
                    SO::MakeTrim(value_text);

                    process_decimal_mark = fwnws.process_decimal_mark;
                }

                // for other numerics, use the generalized DoubleToString function for the conversion
                else
                {
                    value_text = DoubleToString(value);
                    process_decimal_mark = m_commaDecimalMark;
                }

                if( process_decimal_mark )
                    SO::Replace(value_text, '.', ',');

                m_delimitedTextCreator->AddText(value_text);
            }
        }


        // string values
        else if( export_item_mapping.case_item->IsTypeString() )
        {
            const StringCaseItem& string_case_item = assert_cast<const StringCaseItem&>(*export_item_mapping.case_item);
            std::wstring text = CS2WS(string_case_item.GetValue(index));

            if( !write_label_if_valid(text) )
            {
                ModifyValueForOutput(text);
                m_delimitedTextCreator->AddText(text);
            }
        }


        else
        {
            throw ProgrammingErrorException();
        }
    }
}

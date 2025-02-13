#include "stdafx.h"
#include "ExcelExportWriter.h"
#include "ExportPropertiesValuesProcessor.h"


ExcelExportWriter::ExcelExportWriter(std::shared_ptr<const CaseAccess> case_access, const ConnectionString& connection_string)
    :   ExportWriterBase(DataRepositoryType::Excel, std::move(case_access), connection_string),
        m_recordMappingInfoBeingWritten(nullptr)
{
    CreateExportRecordMappings();

    Open(connection_string);

    SetupWorksheets();
}


ExcelExportWriter::~ExcelExportWriter()
{
    Close();
}


void ExcelExportWriter::Open(const ConnectionString& connection_string)
{
    m_excelWriter = std::make_unique<ExcelWriter>();
    m_excelWriter->CreateWorkbook(connection_string.GetFilename());
}


void ExcelExportWriter::Close()
{
    if( m_excelWriter != nullptr )
    {
        m_excelWriter->Close();
        m_excelWriter.reset();
    }
}


bool ExcelExportWriter::IsReservedName(const std::wstring& name, const bool record_name)
{
    return ( record_name && SO::EqualsNoCase(name, _T("History")) );
}


void ExcelExportWriter::SetupWorksheets()
{
    ExportPropertiesValuesProcessor export_properties_values_processor(m_connectionString);

    lxw_format* header_format = m_excelWriter->GetFormat(ExcelWriter::Format::Bold);

    const bool write_header = ( !m_connectionString.HasProperty(CSProperty::header, CSValue::suppress) &&
                                !m_connectionString.HasProperty(CSProperty::header, _T("none")) ); // pre-8.0

    for( std::vector<ExportRecordMapping>& export_record_mapping_for_level : m_exportRecordMappingByLevel )
    {
        for( ExportRecordMapping& export_record_mapping : export_record_mapping_for_level )
        {
            std::wstring worksheet_name = export_record_mapping.formatted_record_name;

            // if the sheet name is not valid, set it to empty (which will then use the default sheet name)
            if( !m_excelWriter->ValidateWorksheetName(worksheet_name) )
                worksheet_name.clear();

            // create the worksheet
            const size_t sheet_index = m_excelWriter->AddAndSetCurrentWorksheet(worksheet_name);

            RecordMappingInfo* record_mapping_info = m_recordMappingInfos.emplace_back(std::make_shared<RecordMappingInfo>(
                RecordMappingInfo
                {
                    sheet_index,
                    0
                })).get();

            export_record_mapping.tag = record_mapping_info;

            // setup the columns and write the header
            uint16_t column = 0;

            for( ExportItemMapping& export_item_mapping : export_record_mapping.item_mappings )
            {
                ItemMappingInfo* item_mapping_info = m_itemMappingInfos.emplace_back(std::make_shared<ItemMappingInfo>(
                    ItemMappingInfo
                    {
                        record_mapping_info->row,
                        { }
                    })).get();

                export_item_mapping.tag = item_mapping_info;

                // add columns for codes and/or labels
                const CDictItem& dict_item = export_item_mapping.case_item->GetDictionaryItem();

                export_properties_values_processor.Process(dict_item,
                    [&](std::shared_ptr<const ValueProcessor> value_processor, const bool use_label_for_header)
                    {
                        item_mapping_info->column_mapping_infos.emplace_back(ColumnMappingInfo { column, std::move(value_processor) });

                        if( write_header )
                        {
                            m_excelWriter->Write(item_mapping_info->row, column,
                                                 use_label_for_header ? CS2WS(dict_item.GetLabel()) : export_item_mapping.formatted_item_name,
                                                 header_format);
                        }

                        ++column;
                    });
            }

            if( write_header )
                ++record_mapping_info->row;
        }
    }
}


void ExcelExportWriter::StartRecord(const ExportRecordMapping& export_record_mapping)
{
    m_recordMappingInfoBeingWritten = static_cast<RecordMappingInfo*>(export_record_mapping.tag);
    m_excelWriter->SetCurrentWorksheet(m_recordMappingInfoBeingWritten->sheet_index);
}


void ExcelExportWriter::StartRow()
{
}


void ExcelExportWriter::EndRow()
{
    ASSERT(m_recordMappingInfoBeingWritten != nullptr);
    ++m_recordMappingInfoBeingWritten->row;
}


void ExcelExportWriter::WriteCaseItem(const ExportItemMapping& export_item_mapping, const CaseItemIndex& index)
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
                    m_excelWriter->Write(item_mapping_info->row, column_mapping_info.column, dict_value->GetLabel());
                    return true;
                }
            }

            return false;
        };


        // blank values
        if( export_item_mapping.case_item->IsBlank(index) )
        {
            // write nothing
        }


        // numeric values
        else if( export_item_mapping.case_item->IsTypeNumeric() )
        {
            const NumericCaseItem& numeric_case_item = assert_cast<const NumericCaseItem&>(*export_item_mapping.case_item);
            double value = numeric_case_item.GetValue(index);

            if( !write_label_if_valid(value) )
            {
                ModifyValueForOutput(numeric_case_item, value);

                // use the text for the special value
                if( IsSpecial(value) )
                {
                    if( value != NOTAPPL )
                        m_excelWriter->Write(item_mapping_info->row, column_mapping_info.column, SpecialValues::ValueToString(value));
                }

                // otherwise write the double
                else
                {
                    m_excelWriter->Write(item_mapping_info->row, column_mapping_info.column, value);
                }
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
                m_excelWriter->Write(item_mapping_info->row, column_mapping_info.column, text);
            }
        }


        else
        {
            throw ProgrammingErrorException();
        }
    }
}

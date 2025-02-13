#include "stdafx.h"
#include "ReadStatExportWriterBase.h"
#include <zDictO/ValueSetResponse.h>


namespace
{
    unsigned GetStringWidth(const CaseItem& case_item)
    {
        constexpr unsigned MaxStringSize = 255;

        ASSERT(case_item.IsTypeString());

        if( case_item.IsTypeFixed() )
        {
            // a character can be represented by as many as four UTF-8 characters
            return std::min(case_item.GetDictionaryItem().GetLen() * 4, MaxStringSize);
        }

        else
        {
            return MaxStringSize;
        }
    }

    // routines for working with the ReadStat writer
    ssize_t write_bytes(const void* bytes, const size_t len, void* ctx)
    {
        FILE* file = static_cast<FILE*>(ctx);
        return fwrite(bytes, 1, len, file);
    }

    void row_count_position_recorder(const size_t count_position, const size_t count_size, const char count_is_unsigned, void* object_holding_writer)
    {
        static_cast<ReadStatExportWriterBase*>(object_holding_writer)->RecordRowCountPosition(count_position, count_size, count_is_unsigned);
    }

    void emit_map_position_recorder(const size_t map_position, const size_t record_len, const uint64_t map10, const uint64_t map11,
                                    const uint64_t map12, const uint64_t map13, void* object_holding_writer)
    {
        static_cast<ReadStatExportWriterBase*>(object_holding_writer)->RecordEmitMapVariables(map_position, record_len, map10, map11, map12, map13);
    }
}


ReadStatExportWriterBase::ReadStatExportWriterBase(const DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access,
                                                   const ConnectionString& connection_string)
    :   SingleRecordExportWriterBase(type, std::move(case_access), connection_string),
        m_file(nullptr),
        m_readStatWriter(nullptr),
        m_rowCount(0)
{
}


void ReadStatExportWriterBase::Initialize()
{
    CreateExportRecordMappings();

    Open();

    InitializeReadStatVariables();
}


ReadStatExportWriterBase::~ReadStatExportWriterBase()
{
    Close();
}


void ReadStatExportWriterBase::Open()
{
    SetupEnvironmentToCreateFile(m_connectionString.GetFilename());

    m_file = PortableFunctions::FileOpen(m_connectionString.GetFilename(), _T("wb"));

    if( m_file == nullptr )
    {
        throw CSProException(_T("Could not create the %s data file: %s"),
                             ToString(m_type), m_connectionString.GetFilename().c_str());
    }

    m_readStatWriter = readstat_writer_init();
    readstat_set_data_writer(m_readStatWriter, &write_bytes);

    m_readStatWriter->object_holding_writer = this;
    m_readStatWriter->row_count_position_recorder = &row_count_position_recorder;
    m_readStatWriter->dta_emit_map_position_recorder = &emit_map_position_recorder;

    // use the dictionary name as the file label
    readstat_writer_set_file_label(m_readStatWriter, UTF8Convert::WideToUTF8(m_caseAccess->GetDataDict().GetName()).c_str());
}


void ReadStatExportWriterBase::Close()
{
    if( m_readStatWriter != nullptr )
    {
        // update row_count because readstat_end_writing checks if it is valid
        m_readStatWriter->row_count = static_cast<int>(m_rowCount);

        readstat_end_writing(m_readStatWriter);
        readstat_writer_free(m_readStatWriter);

        m_readStatWriter = nullptr;
    }

    if( m_file != nullptr )
    {
        UpdateRowCounts();

        fclose(m_file);
        m_file = nullptr;
    }
}


void ReadStatExportWriterBase::RecordRowCountPosition(const size_t count_position, const size_t count_size, const char count_is_unsigned)
{
    m_rowCountPositions.emplace_back(RowCountPosition { count_position, count_size, count_is_unsigned });
}


void ReadStatExportWriterBase::RecordEmitMapVariables(const size_t map_position, const size_t record_len, const uint64_t map10, const uint64_t map11,
                                                      const uint64_t map12, const uint64_t map13)
{
    ASSERT(!m_emitMapVariables.has_value());

    m_emitMapVariables = EmitMapVariables
    {
        map_position,
        record_len,
        { map10, map11, map12, map13 }
    };
}



void ReadStatExportWriterBase::UpdateRowCounts()
{
    ASSERT(( m_type == DataRepositoryType::SPSS && m_rowCountPositions.size() == 2 ) ||
           ( m_type == DataRepositoryType::Stata && m_rowCountPositions.size() == 1 ));

    for( const RowCountPosition& row_count_position : m_rowCountPositions )
    {
        auto write_row_count = [&](auto row_count)
        {
            PortableFunctions::fseeki64(m_file, row_count_position.count_position, SEEK_SET);
            fwrite(&row_count, sizeof(row_count), 1, m_file);
        };

        if( row_count_position.count_size == sizeof(int) && row_count_position.count_is_unsigned == 0 )
        {
            write_row_count(static_cast<int>(m_rowCount));
        }

        else if( row_count_position.count_size == sizeof(m_rowCount) && row_count_position.count_is_unsigned == 1 )
        {
            write_row_count(m_rowCount);
        }

        else
        {
            ASSERT(false);
        }
    }

    ASSERT(( m_type == DataRepositoryType::SPSS && !m_emitMapVariables.has_value() ) ||
           ( m_type == DataRepositoryType::Stata && m_emitMapVariables.has_value() ));

    if( m_emitMapVariables.has_value() )
    {
        // adjust the values by the size of all of the records
        const uint64_t length_all_records = m_emitMapVariables->record_len * m_rowCount;

        for( size_t i = 0; i < _countof(m_emitMapVariables->map_values); ++i )
            m_emitMapVariables->map_values[i] += length_all_records;

        PortableFunctions::fseeki64(m_file, m_emitMapVariables->position, SEEK_SET);
        fwrite(m_emitMapVariables->map_values, sizeof(m_emitMapVariables->map_values), 1, m_file);
    }
}


void ReadStatExportWriterBase::InitializeReadStatVariables()
{
    // preprocess each variable's primary value set to create a label set and to get the missing values
    std::vector<readstat_label_set_t*> label_sets;
    std::vector<std::vector<double>> missing_values;

    for( ExportItemMapping& export_item_mapping : m_singleExportRecordMapping->item_mappings )
    {
        readstat_label_set_t*& label_set = label_sets.emplace_back();
        std::vector<double>& missing_values_set = missing_values.emplace_back();

        const CDictItem& dict_item = export_item_mapping.case_item->GetDictionaryItem();

        if( !dict_item.HasValueSets() )
            continue;

        const DictValueSet& dict_value_set = dict_item.GetValueSet(0);
        const std::string label_set_name = UTF8Convert::WideToUTF8(CreateUniqueName(dict_value_set.GetName()));

        std::shared_ptr<const ValueProcessor> value_processor = ValueProcessor::CreateValueProcessor(dict_item, &dict_value_set);

        if( export_item_mapping.case_item->IsTypeNumeric() )
        {
            const NumericCaseItem& numeric_case_item = assert_cast<const NumericCaseItem&>(*export_item_mapping.case_item);

            for( const auto& response : value_processor->GetResponses() )
            {
                // only add discrete values
                if( !response->IsDiscrete() )
                    continue;

                double value_to_add = response->GetMinimumValue();
                bool add_to_missing_values_set = false;

                // process special values, only adding missing/refused
                if( IsSpecial(value_to_add) )
                {
                    if( m_suppressMappedSpecialValues || ( value_to_add != MISSING && value_to_add != REFUSED ) )
                        continue;

                    add_to_missing_values_set = true;
                }

                ModifyValueForOutput(numeric_case_item, value_to_add);

                if( add_to_missing_values_set )
                    missing_values_set.emplace_back(value_to_add);

                if( label_set == nullptr )
                    label_set = readstat_add_label_set(m_readStatWriter, READSTAT_TYPE_DOUBLE, label_set_name.c_str());

                readstat_label_double_value(label_set, value_to_add, UTF8Convert::WideToUTF8(response->GetLabel()).c_str());
            }
        }

        else if( AddStringLabelSets() )
        {
            ASSERT(export_item_mapping.case_item->IsTypeString());

            label_set = readstat_add_label_set(m_readStatWriter, READSTAT_TYPE_STRING, label_set_name.c_str());

            for( const auto& response : value_processor->GetResponses() )
            {
                std::string value_to_add = UTF8Convert::WideToUTF8(response->GetCode());

                value_to_add.resize(GetStringWidth(*export_item_mapping.case_item), ' ');

                readstat_label_string_value(label_set,
                                            value_to_add.c_str(),
                                            UTF8Convert::WideToUTF8(response->GetLabel()).c_str());
            }
        }
    }

    ASSERT(m_singleExportRecordMapping->item_mappings.size() == label_sets.size() &&
           m_singleExportRecordMapping->item_mappings.size() == missing_values.size());


    // create each variable
    for( size_t i = 0; i < m_singleExportRecordMapping->item_mappings.size(); ++i )
    {
        ExportItemMapping& export_item_mapping = m_singleExportRecordMapping->item_mappings[i];
        readstat_label_set_t* label_set = label_sets[i];
        const std::vector<double> missing_values_set = missing_values[i];

        // setup the data type, width, decimals, and format
        const CDictItem& dict_item = export_item_mapping.case_item->GetDictionaryItem();

        readstat_type_t data_type;
        unsigned width = 0;
        unsigned decimals = 0;
        std::optional<std::wstring> format;

        if( export_item_mapping.case_item->IsTypeNumeric() )
        {
            data_type = READSTAT_TYPE_DOUBLE;

            if( export_item_mapping.case_item->IsTypeFixed() )
            {
                width = dict_item.GetLen();
                decimals = dict_item.GetDecimal();
                format = GetFixedWidthNumericFormat(dict_item);
            }
        }

        else
        {
            ASSERT(export_item_mapping.case_item->IsTypeString());

            data_type = READSTAT_TYPE_STRING;
            width = GetStringWidth(*export_item_mapping.case_item);
            format = GetFixedWidthStringFormat(width);
        }
        
        readstat_variable_t* variable = readstat_add_variable(m_readStatWriter,
                                                              UTF8Convert::WideToUTF8(export_item_mapping.formatted_item_name).c_str(),
                                                              data_type, width);

        readstat_variable_set_label(variable, UTF8Convert::WideToUTF8(dict_item.GetLabel()).c_str());

        readstat_variable_set_display_width(variable, static_cast<int>(width));
        variable->decimals = static_cast<int>(decimals);

        if( format.has_value() )
            readstat_variable_set_format(variable, UTF8Convert::WideToUTF8(*format).c_str());

        // add the label set and any missing values
        if( label_set != nullptr )
            readstat_variable_set_label_set(variable, label_set);

        for( double missing_value : missing_values_set )
            readstat_variable_add_missing_double_value(variable, missing_value);

        export_item_mapping.tag = variable;
    }

    // use -1 for the number of rows, but this will be updated when closing the file
    StartReadStatWriter(m_readStatWriter, m_file, -1);
}


void ReadStatExportWriterBase::StartRecord(const ExportRecordMapping& /*export_record_mapping*/)
{
}


void ReadStatExportWriterBase::StartRow()
{
    readstat_begin_row(m_readStatWriter);
}


void ReadStatExportWriterBase::EndRow()
{
    readstat_end_row(m_readStatWriter);
    ++m_rowCount;
}


void ReadStatExportWriterBase::WriteCaseItem(const ExportItemMapping& export_item_mapping, const CaseItemIndex& index)
{
    const readstat_variable_t* variable = static_cast<const readstat_variable_t*>(export_item_mapping.tag);

    // blank values
    auto write_blank_value = [&]()
    {
        readstat_insert_missing_value(m_readStatWriter, variable);
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
        ModifyValueForOutput(numeric_case_item, value);

        if( value == NOTAPPL )
        {
            write_blank_value();
        }

        else
        {
            readstat_insert_double_value(m_readStatWriter, variable, value);
        }
    }


    // string values
    else if( export_item_mapping.case_item->IsTypeString() )
    {
        ASSERT(export_item_mapping.case_item->IsTypeString());
        ASSERT(variable->display_width == static_cast<int>(GetStringWidth(*export_item_mapping.case_item)));

        const StringCaseItem& string_case_item = assert_cast<const StringCaseItem&>(*export_item_mapping.case_item);

        std::string text_utf8 = UTF8Convert::WideToUTF8(string_case_item.GetValue(index));
        text_utf8.resize(variable->display_width, ' ');

        readstat_insert_string_value(m_readStatWriter, variable, text_utf8.c_str());
    }
}

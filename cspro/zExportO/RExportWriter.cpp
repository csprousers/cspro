#include "stdafx.h"
#include "RExportWriter.h"


RExportWriter::RExportWriter(std::shared_ptr<const CaseAccess> case_access, const ConnectionString& connection_string)
    :   ExportWriterBase(DataRepositoryType::R, std::move(case_access), connection_string),
        m_rWriter(nullptr)
{
    m_writeCodes = m_connectionString.HasPropertyOrDefault(CSProperty::writeCodes, CSValue::true_, true);
    m_writeFactors = m_connectionString.HasProperty(CSProperty::writeFactors, CSValue::true_, true);

    // handle pre-8.0 values
    const std::string* const vector_property_pre80 = connection_string.GetProperty("vector");

    if( vector_property_pre80 != nullptr )
    {
        if( SO::EqualsNoCase(*vector_property_pre80, CSValue::codes) )
        {
            m_writeCodes = true;
            m_writeFactors = false;
        }

        else if( SO::EqualsNoCase(*vector_property_pre80, "factors") )
        {
            m_writeCodes = false;
            m_writeFactors = true;
        }

        else if( SO::EqualsNoCase(*vector_property_pre80, "codes-and-factors") )
        {
            m_writeCodes = true;
            m_writeFactors = true;
        }
    }

    if( !m_writeCodes && !m_writeFactors )
    {
        throw CSProException("You cannot suppress writing both codes and factors when writing a file of type '%s'.",
                             ToString(connection_string.GetType()));
    }

    m_factorRanges = ( m_writeFactors && ( m_connectionString.HasProperty(CSProperty::factorRanges, CSValue::true_, true) ||
                                           m_connectionString.HasProperty("factor", "all") ) ); // pre-8.0

    CreateExportRecordMappings();

    Open();

    SetupExpansiveLists();
}


void RExportWriter::Open()
{
    SetupEnvironmentToCreateFile(m_connectionString.GetFilePath());

    m_file.SetDeleteFileOnError(true)
          .OpenForWritingCreate(m_connectionString.GetFilePath());
}


void RExportWriter::Close()
{
    ASSERT(m_file.IsOpen() && m_rWriter == nullptr);

    WriteExpansiveListsToR();

    m_file.Close();
}


bool RExportWriter::IsReservedName(const std::string& name, bool /*record_name*/)
{
    // R reserved words from https://stat.ethz.ch/R-manual/R-devel/library/base/html/Reserved.html
    // names that are not valid in CSPro are commented out
    constexpr const char* ReservedWords[] =
    {
     // "if",
     // "else",
        "repeat",
     // "while",
     // "function",
     // "for",
     // "in",
     // "next",
     // "break",
     // "TRUE",
     // "FALSE",
        "NULL",
        "Inf",
        "NaN",
        "NA",
     // "NA_integer_",
     // "NA_real_",
     // "NA_complex_",
     // "NA_character_",
    };

    for( const char* const reserved_word : ReservedWords )
    {
        if( SO::EqualsNoCase(name, reserved_word) )
            return true;
    }

    return false;
}


void RExportWriter::SetupExpansiveLists()
{
    for( std::vector<ExportRecordMapping>& export_record_mapping_for_level : m_exportRecordMappingByLevel )
    {
        for( ExportRecordMapping& export_record_mapping : export_record_mapping_for_level )
        {
            for( ExportItemMapping& export_item_mapping : export_record_mapping.item_mappings )
            {
                auto set_tag = [&](auto& lists, auto&& list)
                {
                    export_item_mapping.tag = list.get();
                    lists.emplace_back(std::move(list));
                };

                if( IsNumeric(export_item_mapping.case_item->GetDataType()) )
                {
                    set_tag(m_numericExpansiveLists, std::make_unique<ExpansiveList<double>>(ExportExpansiveListVectorSize));
                }

                else if( IsString(export_item_mapping.case_item->GetDataType()) )
                {
                    set_tag(m_stringExpansiveLists, std::make_unique<ExpansiveList<std::string>>(ExportExpansiveListVectorSize));
                }

                else
                {
                    throw ProgrammingErrorException();
                }
            }
        }
    }
}


void RExportWriter::WriteCaseItem(const ExportItemMapping& export_item_mapping, const CaseItemIndex& index)
{
    // numeric values
    if( IsNumeric(export_item_mapping.case_item->GetDataType()) )
    {
        const NumericCaseItem& numeric_case_item = assert_cast<const NumericCaseItem&>(*export_item_mapping.case_item);
        double value = numeric_case_item.GetValue(index);
        ModifyValueForOutput(numeric_case_item, value);

        ExpansiveList<double>* numeric_expansive_list = static_cast<ExpansiveList<double>*>(export_item_mapping.tag);
        numeric_expansive_list->AddValue(value);
    }

    // string values
    else
    {
        ASSERT(IsString(export_item_mapping.case_item->GetDataType()));

        const StringCaseItem& string_case_item = assert_cast<const StringCaseItem&>(*export_item_mapping.case_item);
        std::string value = string_case_item.GetValue(index);
        ModifyValueForOutput(value);

        ExpansiveList<std::string>* string_expansive_list = static_cast<ExpansiveList<std::string>*>(export_item_mapping.tag);
        string_expansive_list->AddValue(value);
    }
}



// routines for working with the R writer
namespace
{
    ssize_t r_write_data(const void* const bytes, const size_t len, void* const ctx)
    {
        FileIO::File* file = static_cast<FileIO::File*>(ctx);
        file->Write(bytes, len);
        return len;
    }

    inline double r_notappl()
    {
        static std::optional<double> double_value;

        if( !double_value.has_value() )
        {
            // routine from the arithmetic.c file in the R source code
            typedef union
            {
                double value;
                unsigned int word[2];
            } ieee_double;

#ifdef WORDS_BIGENDIAN
            static CONST int hw = 0;
            static CONST int lw = 1;
#else  /* !WORDS_BIGENDIAN */
            static CONST int hw = 1;
            static CONST int lw = 0;
#endif /* WORDS_BIGENDIAN */

            ieee_double y;
            y.value = NAN;
            y.word[lw] = 1954;

            double_value = y.value;
        }

        return *double_value;
    }
}


void RExportWriter::WriteExpansiveListsToR()
{
    // once all of the data has been loaded into the expansive lists, it can be written to R tables
    m_rWriter = rdata_writer_init(&r_write_data, RDATA_WORKSPACE);

    rdata_begin_file(m_rWriter, &m_file);

    for( std::vector<ExportRecordMapping>& export_record_mapping_for_level : m_exportRecordMappingByLevel )
    {
        for( ExportRecordMapping& export_record_mapping : export_record_mapping_for_level )
            WriteExpansiveListToR(export_record_mapping);
    }

    rdata_end_file(m_rWriter);

    rdata_writer_free(m_rWriter);
}


void RExportWriter::WriteExpansiveListToR(const ExportRecordMapping& export_record_mapping)
{
    // determine what should be factored and setup the columns for the table
    struct TableColumn
    {
        using RowData = std::variant<ExpansiveList<double>*, ExpansiveList<std::string>*, std::shared_ptr<FactoredVector>>;
        rdata_column_t* column;
        RowData row_data;
    };

    std::vector<TableColumn> table_columns;
    std::optional<size_t> number_rows;

    for( const ExportItemMapping& export_item_mapping : export_record_mapping.item_mappings )
    {
        rdata_type_t data_type;
        TableColumn::RowData row_data;

        auto set_number_rows_and_row_data = [&](const auto& expansive_list)
        {
            ASSERT(!number_rows.has_value() || *number_rows == expansive_list->GetSize());

            if( !number_rows.has_value() )
                number_rows = expansive_list->GetSize();

            row_data = expansive_list;
        };

        if( IsNumeric(export_item_mapping.case_item->GetDataType()) )
        {
            data_type = RDATA_TYPE_REAL;
            set_number_rows_and_row_data(static_cast<ExpansiveList<double>*>(export_item_mapping.tag));
        }

        else
        {
            ASSERT(IsString(export_item_mapping.case_item->GetDataType()));

            data_type = RDATA_TYPE_STRING;
            set_number_rows_and_row_data(static_cast<ExpansiveList<std::string>*>(export_item_mapping.tag));
        }


        // factor the vector if necessary
        std::shared_ptr<FactoredVector> factored_vector;

        if( m_writeFactors )
        {
            if( IsNumeric(export_item_mapping.case_item->GetDataType()) )
            {
                factored_vector = FactorVector<double>(export_item_mapping);
            }

            else
            {
                factored_vector = FactorVector<std::string>(export_item_mapping);
            }
        }

        // a routine for adding a column
        std::string column_name = export_item_mapping.formatted_item_name;

        auto add_column = [&](const rdata_type_t column_data_type)
        {
            rdata_column_t* column = rdata_add_column(m_rWriter, column_name.c_str(), column_data_type);

            // use the item label as the column label
            rdata_column_set_label(column, UTF8_TODO::GetUtf8(export_item_mapping.case_item->GetDictItem().GetLabel()).c_str());

            return column;
        };

        // add the codes column
        const bool add_codes_column = ( m_writeCodes || factored_vector == nullptr );

        if( add_codes_column )
        {
            table_columns.emplace_back(TableColumn { add_column(data_type), row_data });
        }

        // add the factors column
        if( factored_vector != nullptr )
        {
            // append .f to the name if both codes and factors are output
            if( add_codes_column )
                column_name.append(".f");

            rdata_column_t* column = add_column(RDATA_TYPE_INT32);
            table_columns.emplace_back(TableColumn { column, factored_vector });

            // add the labels to the column
            for( const std::string& label : factored_vector->labels )
                rdata_column_add_factor(column, label.c_str());
        }
    }

    ASSERT(number_rows.has_value());


    // start the table
    rdata_begin_table(m_rWriter, export_record_mapping.formatted_record_name.c_str());


    // write out each column
    for( const TableColumn& table_column : table_columns )
    {
        rdata_begin_column(m_rWriter, table_column.column, static_cast<int32_t>(*number_rows));

        // write out the values
        std::visit([&](auto& row_data) { WriteExpansiveListValuesToR(*row_data); }, table_column.row_data);

        rdata_end_column(m_rWriter, table_column.column);
    }


    // end the table, using the record label as the data label
    rdata_end_table(m_rWriter, static_cast<int32_t>(*number_rows),
                    UTF8_TODO::GetUtf8(export_record_mapping.case_record_metadata->GetDictRecord().GetLabel()).c_str());

    // reset the columns for the next table
    rdata_writer_reset_columns(m_rWriter);
}


template<typename T>
std::unique_ptr<RExportWriter::FactoredVector> RExportWriter::FactorVector(const ExportItemMapping& export_item_mapping)
{
    const CDictItem& dict_item = export_item_mapping.case_item->GetDictItem();

    // if there is no value set, there is nothing to factor
    if( !dict_item.HasValueSets() )
        return nullptr;

    const DictValueSet& dict_value_set = dict_item.GetValueSet(0);

    // prepared the factored vector struct and value processors
    auto factored_vector = std::make_unique<FactoredVector>();
    factored_vector->expansive_list = std::make_unique<ExpansiveList<int>>(ExportExpansiveListVectorSize);

    std::shared_ptr<const ValueProcessor> value_processor = ValueProcessor::CreateValueProcessor(dict_item, &dict_value_set);
    const NumericValueProcessor* numeric_value_processor;

    if constexpr(std::is_same_v<T, double>)
        numeric_value_processor = assert_cast<const NumericValueProcessor*>(value_processor.get());

    std::map<T, int> one_based_code_expansive_list_map;
    bool at_least_one_valid_label_found = false;

    ExpansiveList<T>* values_expansive_list = static_cast<ExpansiveList<T>*>(export_item_mapping.tag);
    T value;

    while( values_expansive_list->GetValue(value) )
    {
        int one_based_code;

        // use the code if it has already been added
        const auto& one_based_code_lookup = one_based_code_expansive_list_map.find(value);

        if( one_based_code_lookup != one_based_code_expansive_list_map.cend() )
        {
            one_based_code = one_based_code_lookup->second;
        }

        // if not, look up the label using the value processor
        else
        {
            std::string label;

            auto use_dict_value_if_valid = [&](const DictValue* dict_value)
            {
                if( ( dict_value != nullptr ) &&
                    ( m_factorRanges || ( dict_value->GetNumValuePairs() == 1 && dict_value->GetValuePair(0).GetTo().IsEmpty() ) ) )
                {
                    label = UTF8_TODO::GetUtf8(dict_value->GetLabel());
                    at_least_one_valid_label_found = true;
                    return true;
                }

                else
                {
                    return false;
                }
            };

            if constexpr(std::is_same_v<T, double>)
            {
                const double engine_value = numeric_value_processor->ConvertNumberToEngineFormat(value);

                // if the label doesn't exist, format the number with DoubleToString
                if( !use_dict_value_if_valid(numeric_value_processor->GetDictValue(engine_value)) )
                    label = DoubleToString(value);
            }

            else
            {
                // if the label doesn't exist, use the value
                if( !use_dict_value_if_valid(value_processor->GetDictValue(UTF8_TODO::GetWide(value))) )
                    label = value;
            }

            factored_vector->labels.emplace_back(std::move(label));
            one_based_code = int32_cast(factored_vector->labels.size());

            // add the value / one-based-code to the lookup
            one_based_code_expansive_list_map[value] = one_based_code;
        }

        // add the one-based-code of this entry
        factored_vector->expansive_list->AddValue(one_based_code);
    }

    // restart the iterator (in case vector codes are being output)
    values_expansive_list->RestartIterator();

    // if no valid labels were found, don't factor the vector
    if( !at_least_one_valid_label_found )
        factored_vector.reset();

    return factored_vector;
}


void RExportWriter::WriteExpansiveListValuesToR(ExpansiveList<double>& numeric_expansive_list)
{
    double value;

    while( numeric_expansive_list.GetValue(value) )
    {
        // preprocess special values
        if( IsSpecial(value) )
        {
            if( value == NOTAPPL )
            {
                value = r_notappl();
            }

            else if( value == DEFAULT )
            {
                value = NAN;
            }
        }

        rdata_append_real_value(m_rWriter, value);
    }
}


void RExportWriter::WriteExpansiveListValuesToR(ExpansiveList<std::string>& string_expansive_list)
{
    std::string value;

    while( string_expansive_list.GetValue(value) )
        rdata_append_string_value(m_rWriter, value.c_str());
}


void RExportWriter::WriteExpansiveListValuesToR(FactoredVector& factored_vector)
{
    int index;

    while( factored_vector.expansive_list->GetValue(index) )
        rdata_append_int32_value(m_rWriter, index);
}
